// Import external libraries
#include <Adafruit_ssd1306syp.h>
#include "SoftwareSerial.h"

// Constants: I/O Pins
#define PIN_BLUETOOTH_RECV 2
#define PIN_BLUETOOTH_SEND 3
#define PIN_BLUETOOTH_POWER 12
#define PIN_BLUETOOTH_ENABLE 13
#define PIN_LED            10
#define PIN_I2C_SDA        A4
#define PIN_I2C_SCL        A5
#define PIN_JOYSTICK_X     A6
#define PIN_JOYSTICK_Y     A7

// Constants: knobs and buttons.
#define PIN_BUTTON_YELLOW 4
#define PIN_BUTTON_BLUE   5
#define PIN_BUTTON_RED    6
#define PIN_BUTTON_GREEN  7

// More Constants
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define BLUETOOTH_READ_BUFFER 30

// The various states of our bluetooth connection.
enum BluetoothState {
  BLUETOOTH_DISCONNECTED,
  BLUETOOTH_CONNECTED,
  BLUETOOTH_ABANDONDED
};

// Global Variables: Run state
unsigned long startTime = 0; 
const char buildTimestamp[] =  __DATE__ " " __TIME__;

// Global Variables: Pulse state.
unsigned long pulseLastTime = 0;

// Global Variables: the OLED Display, connected via I2C interface
Adafruit_ssd1306syp display(PIN_I2C_SDA, PIN_I2C_SCL);

// Global Variables: Bluetooth communication module
SoftwareSerial bluetooth(PIN_BLUETOOTH_RECV, PIN_BLUETOOTH_SEND);
BluetoothState bluetoothState = BLUETOOTH_DISCONNECTED;
int bluetoothEnabled = 0;
char bluetoothReadbuffer[BLUETOOTH_READ_BUFFER + 1];
char bluetoothAddress[15];

// Driving state.
char lastCommand = 0;

/**
 * Entrypoint: called once when the program first starts, just to initialize all the sub-components.
 */
void setup() {  
  
  // Record what time we started.
  startTime = millis();
  
  // Init the serial line; important for debug messages back to the Arduino Serial Monitor, so you can plug
  // the robot into the USB port, and get real time debug messages.
  // Make sure you set the baudrate at 9600 in Serial Monitor as well.
  Serial.begin(9600);
  Serial.println(F("setup start..."));

  // Init LED pin, and initially set it to on.
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);
  Serial.println(F("LED setup complete..."));

  // Init Joystick.
  pinMode(PIN_JOYSTICK_X, INPUT);
  pinMode(PIN_JOYSTICK_Y, INPUT);
  Serial.println(F("Joystick setup complete..."));

  // Init the Bluetooth Module.
  pinMode(PIN_BLUETOOTH_POWER, OUTPUT);
  pinMode(PIN_BLUETOOTH_ENABLE, OUTPUT);
  digitalWrite(PIN_BLUETOOTH_POWER, LOW);
  digitalWrite(PIN_BLUETOOTH_ENABLE, LOW);
  bluetoothEnabled = 0;
  bluetoothReadbuffer[0] = 0;
  strcpy(bluetoothAddress, "unknown");
  bluetooth.begin(38400);
  switchToBluetoothNormalMode();
  Serial.println(F("Bluetooth setup complete..."));

  // Init: knobs and buttons.
  pinMode(PIN_BUTTON_YELLOW, INPUT);
  pinMode(PIN_BUTTON_BLUE, INPUT);
  pinMode(PIN_BUTTON_RED, INPUT);
  pinMode(PIN_BUTTON_GREEN, INPUT);
  
  // Init the OLED display.
  delay(1000);
  display.initialize();
  Serial.println(F("OLED setup complete..."));

  // Init the rest of our internal state.
  pulseLastTime = 0;
  lastCommand = 0;
  Serial.println(F("setup end"));
}


/**
 * Main Loop: called over and over again as the robot runs, 
 */
void loop() {
  
  unsigned long now = millis();

  // After a bit, configure the bluetooth chip.
  if (!bluetoothEnabled && ((now - startTime) > 8000)) { 
    bluetoothWriteMasterConfiguration();
    // Mark the flag so we dont probe this again.
    bluetoothEnabled = 1;
  }

  // Send the keep-alive pulse.
  if (now > (pulseLastTime + 5000)) {
    //Serial.println(F("Sending pulse"));
    bluetooth.write('@');
    pulseLastTime = now;
  }

  // Basic joystick control.
  int joystickX = analogRead(PIN_JOYSTICK_X);
  int joystickY = analogRead(PIN_JOYSTICK_Y);
  if (joystickY > 600) {
    Serial.println(F("Sending forward"));
    bluetooth.write('F');
    lastCommand = 'F';
  } else if (joystickY < 400) {
    Serial.println(F("Sending reverse"));
    bluetooth.write('B');
    lastCommand = 'B';
  } else if (joystickX > 600) {
    Serial.println(F("Sending right"));
    bluetooth.write('R');
    lastCommand = 'R';
  } else if (joystickX < 400) {
    Serial.println(F("Sending left"));
    bluetooth.write('L');
    lastCommand = 'L';
  } else if (lastCommand != 'S') {
    Serial.println(F("Sending stop"));
    bluetooth.write('S');
    lastCommand = 'S';
  }

  // Loop: buttons and knobs.
  processButton(PIN_BUTTON_YELLOW, '!');
  processButton(PIN_BUTTON_BLUE, '@');
  processButton(PIN_BUTTON_RED, '#');
  processButton(PIN_BUTTON_GREEN, '$');
    
  // Update the LED screen with our current state.
  bool connected = (bluetoothState == BLUETOOTH_CONNECTED);
  int upSecs = (millis() - startTime) / 1000;
  displayStatus(
    connected ? F("CONNECTED") : F("DISCONNECTED"),
    "runtime: " + String(upSecs) + ", bstate=" + String(bluetoothEnabled),
    "stick: " + String(joystickX) + "/" + String(joystickY),
    "dude: " + String("yo"));
}

void processButton(int buttonId, char commandToSend) {

  int buttonState = digitalRead(buttonId);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH) {
    // turn LED on:
    //digitalWrite(ledPin, HIGH);
    Serial.print("command to send: ");
    Serial.println(commandToSend);
    bluetooth.write(commandToSend);
  }
//  } else {
//    // turn LED off:
//    //digitalWrite(ledPin, LOW);
//  }
//  
}


/**
 * 
 */
void bluetoothWriteMasterConfiguration() { 
  char commandBuffer[30];
  commandBuffer[0] = 0;
  const char *response;

  //  TODO: Read data from EEPROM
    const char * selfName = "battlebotremote";
  //  const char * remoteAddress = "98d3,b1,fd60df";
  //  const char * remotePass = "666";
 // const char * selfName = "mcbotremote";
  const char * remoteAddress = "98d3,71,fd435c";
  const char * remotePass = "666";


  // Get the chip in command mode.
  switchToBluetoothCommandMode();

  // Probe the version; this should always work. 
  writeToBluetoothCommand("AT+VERSION?");
  response = readFromBluetoothCommand();
  Serial.print(F("Bluetooth Version: "));
  Serial.println(response);
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set name"));
  }

  // Reset the chip.
  writeToBluetoothCommand("AT+RMAAD");
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to reset"));
  }

  // Set the baud for the two bluetooth chips.
  writeToBluetoothCommand("AT+UART=38400,0,0");
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set speed"));
  }

  // Set the advertised name. 
  strcpy(commandBuffer, "AT+NAME=");
  strcat(commandBuffer, selfName);
  writeToBluetoothCommand(commandBuffer);
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set name"));
  }

  // Set master/slave mode to master, so we can actively connect to a remote peer. 
  writeToBluetoothCommand("AT+ROLE=1");
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set role"));
  }

  // Set conenction mode to 0, so it binds to a specific address. 
  writeToBluetoothCommand("AT+CMODE=0");
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set cmode"));
  }

  // Set the remote slave address. 
  strcpy(commandBuffer, "AT+BIND=");
  strcat(commandBuffer, remoteAddress);
  writeToBluetoothCommand(commandBuffer);
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to bind remote address"));
  }

  // Set the remote slave passwd. 
  strcpy(commandBuffer, "AT+PSWD=");
  strcat(commandBuffer, remotePass);
  writeToBluetoothCommand(commandBuffer);
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to bind remote pass"));
  }

   // Get the local bluetooth address.
  writeToBluetoothCommand("AT+ADDR?");
  response = readFromBluetoothCommand();
  if (strlen(response) == 20) {
    // +ADDR:98d3:b1:fd60df
    strncpy(bluetoothAddress, response + 6, 14);
  } else {
    strcpy(bluetoothAddress, "invalid");
  }
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set pass"));
  }

  // Back to normal operation mode.
  switchToBluetoothNormalMode();
}

/*
 * Write one line out to the bluetooth in command mode.
 */
void writeToBluetoothCommand(const char * data) {
  Serial.print(F("bluetooth sent: "));
  Serial.println(data);
  bluetooth.println(data);
}

/**
 * Read one line of text from the bluetooth during command mode; all responses are separeated by a CR/LF.
 */
const char* readFromBluetoothCommand() {

  // We will give a limited amount of time for the bluetooth to respond.
  int startTime = millis();
  int endTime = startTime + 10000;
  
  // Continue reading characters until we get to the end.
  int curr = 0;
  bool readComplete = false;
  while (!readComplete && (curr < BLUETOOTH_READ_BUFFER) && (millis() < endTime)) {
    if (bluetooth.available()) {
      char c = bluetooth.read();
      //Serial.println("\nbluetooth read: " + String(c));
      bluetoothReadbuffer[curr++] = c;
      if (c == 10)
        readComplete = true;
    }
  }

  if (!readComplete) {
    // If we did not read a full complete line, return the error signal.
    bluetoothReadbuffer[0] = 0;
  } else {
    // cap off the CR/LF from the end of the response.
    curr -= 2;
    bluetoothReadbuffer[curr++] = 0; 
  }
  
  Serial.print(F("bluetooth read: "));
  Serial.println(bluetoothReadbuffer);
  return bluetoothReadbuffer;
}

/**
 * Cycle the power on the bluetooth module, starting it back up in normal mode.
 */
void switchToBluetoothNormalMode() {
  Serial.println(F("bluetooth normal mode starting..."));
  
  // power it down
  digitalWrite(PIN_BLUETOOTH_POWER, LOW);
  delay(500);

  // power it up with the enable pin off.
  digitalWrite(PIN_BLUETOOTH_ENABLE, LOW);
  delay(500);
  digitalWrite(PIN_BLUETOOTH_POWER, HIGH);
  delay(750);

  Serial.println(F("bluetooth normal mode started..."));
}

/**
 * Cycle the power on the bluetooth module, starting it back up in command mode.
 */
void switchToBluetoothCommandMode() {
  Serial.println(F("bluetooth command mode starting..."));
  
  // power it down
  digitalWrite(PIN_BLUETOOTH_POWER, LOW);
  delay(500);

  // power it up with the enable pin on.
  digitalWrite(PIN_BLUETOOTH_ENABLE, HIGH);
  delay(500);
  digitalWrite(PIN_BLUETOOTH_POWER, HIGH);
  delay(750);
  
  Serial.println(F("bluetooth command mode started..."));
}

/**
 * Turn the power off the bluetooth module.
 */
void switchToBluetoothOffMode() {
  // power it down
  digitalWrite(PIN_BLUETOOTH_POWER, LOW);
  delay(100);
}


/*** DISPLAY ***/


/**
 * Main output for status while in main sequence. This is called once per loop.
 */
void displayStatus(String line1, String line2, String line3, String line4) {  
  display.clear();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0,2);
  display.println(line1);

  // Fun little animation to prove that we are not locked up.
  const int circleRadius = 5;
  const int circleOffset = 0;
  const int maxRight = (SCREEN_WIDTH - 1) - circleRadius;
  const int maxLeft = 82;
  int numFrames = maxRight - maxLeft;
  int numFramesDouble = numFrames * 2;
  int timePerFrame = 3000 / numFramesDouble;
  int currentFrame = (millis() / timePerFrame) % numFramesDouble;
  int circleCenterX = (currentFrame < numFrames) ? (maxRight - currentFrame) : (maxLeft + (currentFrame - numFrames));
  display.drawCircle(circleCenterX, circleRadius + circleOffset, circleRadius, WHITE);
  
  display.setCursor(0,16);
  display.println(line2);
    
  display.setCursor(0,26);
  display.println(line3);

  display.setCursor(0,36);
  display.println(line4);

  // Print the local bluetooth address.
  int beginBlue = 46;
  display.setCursor(0, beginBlue);
  display.print("blue:");
  display.setCursor(34, beginBlue);
  display.println(bluetoothAddress);
  
  // Print the timestamp of when we built this code.
  const int stampHeight = 10;
  int beginStamp = 55;
  display.drawLine(0, beginStamp, SCREEN_WIDTH - 1, beginStamp, WHITE); // top line
  display.drawLine(0, beginStamp, 0, beginStamp + stampHeight, WHITE); // left line
  display.drawLine(SCREEN_WIDTH - 1, beginStamp, SCREEN_WIDTH - 1, beginStamp + stampHeight, WHITE); // right line
  display.setCursor(4, beginStamp + 2);
  display.println(buildTimestamp);

  display.update();
}
