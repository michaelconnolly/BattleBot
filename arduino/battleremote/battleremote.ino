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

// Global Variables: the OLED Display, connected via I2C interface
Adafruit_ssd1306syp display(PIN_I2C_SDA, PIN_I2C_SCL);

// Global Variables: Bluetooth communication module
SoftwareSerial bluetooth(PIN_BLUETOOTH_RECV, PIN_BLUETOOTH_SEND);
BluetoothState bluetoothState = BLUETOOTH_DISCONNECTED;
int bluetoothEnabled = 0;
char bluetoothReadbuffer[BLUETOOTH_READ_BUFFER + 1];
char bluetoothAddress[15];


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
  Serial.println(F("Bluetooth setup complete..."));
  
  // Init the OLED display.
  delay(1000);
  display.initialize();
  Serial.println(F("OLED setup complete..."));

  // Init the rest of our internal state.
  Serial.println(F("setup end"));
}


/**
 * Main Loop: called over and over again as the robot runs, 
 */
void loop() {
  
  int now = millis();
  //Serial.print("loop: ");
  //Serial.println(now);

  // Bluetooth testing. 
  /*if (!bluetoothEnabled && ((now - startTime) > 5000)) {
    swithToBluetoothNormalMode();
    bluetoothEnabled = 1;
  } else if (bluetoothEnabled == 1 && ((now - startTime) > 10000)) {
    swithToBluetoothOffMode();
    bluetoothEnabled = 2;
  } else if (bluetoothEnabled == 2 && ((now - startTime) > 15000)) {
    swithToBluetoothCommandMode();
    bluetoothEnabled = 3;
  } else if (bluetoothEnabled == 3 && ((now - startTime) > 30000)) {
    swithToBluetoothOffMode();
    bluetoothEnabled = 4;
  } */


  // After a bit, probe the address.
  if (!bluetoothEnabled && ((now - startTime) > 8000)) { 
    switchToBluetoothCommandMode();
    writeToBluetoothCommand("AT+ADDR?");
  
   // Future probing: set to master mode.
    /*AT+VERSION?
    AT+RMAAD
    AT+NAME=[new roboto name]
    AT+ROLE=1 (To set it as master)
    AT+CMODE=0
    AT+BIND=xxxx,xx,xxxxxx
    AT+PSWD=[new passwd]
    AT+UART=38400,0,0 */

    const char *response = readFromBluetoothCommand();
    if (strlen(response) == 20) {
      // +ADDR:98d3:b1:fd60df
      strncpy(bluetoothAddress, response + 6, 14);
    } else {
      strcpy(bluetoothAddress, "invalid");
    }

    switchToBluetoothNormalMode();
    bluetoothEnabled = 1;
  }
    
  // Update the LED screen with our current state.
  bool connected = (bluetoothState == BLUETOOTH_CONNECTED);
  int upSecs = (millis() - startTime) / 1000;
  displayStatus(
    connected ? F("CONNECTED") : F("DISCONNECTED"),
    "runtime: " + String(upSecs) + ", bstate=" + String(bluetoothEnabled),
    "stick: " + String(analogRead(PIN_JOYSTICK_X)) + "/" + String(analogRead(PIN_JOYSTICK_Y)),
    "blue: " + String(bluetoothAddress));
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
  
  display.setCursor(0,20);
  display.println(line2);
    
  display.setCursor(0,30);
  display.println(line3);

  display.setCursor(0,40);
  display.println(line4);

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
