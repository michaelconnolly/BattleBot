// Import external libraries
#include <Adafruit_ssd1306syp.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

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
#define BLUETOOTH_MAX_NAME_LEN    20
#define BLUETOOTH_MAX_ADDR_LEN    20
#define BLUETOOTH_MAX_PASS_LEN    16

// The various states of our bluetooth connection.
enum BluetoothState {
  BLUETOOTH_DISCONNECTED,
  BLUETOOTH_CONNECTED,
  BLUETOOTH_ABANDONDED
};

typedef struct {
  char bluetoothName[BLUETOOTH_MAX_NAME_LEN];
  char bluetoothAddr[BLUETOOTH_MAX_ADDR_LEN];
  char bluetoothPass[BLUETOOTH_MAX_PASS_LEN];
} BattleBotRemoteConfig;


// Global Variables: Run state
BattleBotRemoteConfig botRemoteConfig;
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
unsigned long bluetoothPulseLastSendTime = 0;
unsigned long bluetoothPulseLastRecvTime = 0;

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
  Serial.println(F("setup: begin..."));

  // Read our config/saved-state from the local disk.
  configImport();

  // Init LED pin, and initially set it to on.
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);
  Serial.println(F("setup: LED complete..."));

  // Init Joystick.
  pinMode(PIN_JOYSTICK_X, INPUT);
  pinMode(PIN_JOYSTICK_Y, INPUT);
  Serial.println(F("setup: Joystick complete..."));

  // Init the Bluetooth Module.
  pinMode(PIN_BLUETOOTH_POWER, OUTPUT);
  pinMode(PIN_BLUETOOTH_ENABLE, OUTPUT);
  digitalWrite(PIN_BLUETOOTH_POWER, LOW);
  digitalWrite(PIN_BLUETOOTH_ENABLE, LOW);
  bluetoothEnabled = 0;
  bluetoothReadbuffer[0] = 0;
  strcpy(bluetoothAddress, "unknown");
  bluetoothPulseLastSendTime = 0;
  bluetoothPulseLastRecvTime = 0;
  bluetooth.begin(38400);
  Serial.println(F("setup: Bluetooth complete..."));
  
  // Init the OLED display.
  delay(1000);
  display.initialize();
  Serial.println(F("setup: OLED complete..."));

  // Init the rest of our internal state.
  lastCommand = 0;
  Serial.println(F("setup: end"));
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

  if (bluetoothEnabled) {
    
    // Send the keep-alive pulse every once in a while.
    if (now > (bluetoothPulseLastSendTime + 5000)) {
      //Serial.println(F("Sending pulse"));
      bluetooth.write('@');
      bluetoothPulseLastSendTime = now;
    }

    // Process incoming commands from the robot.
    int numCmds;
    for (numCmds = 0; (numCmds < 100) && (bluetooth.available() > 0) ; numCmds++) {
      char cmd = bluetooth.read();
      switch (cmd) {
        case '@':
          Serial.println(F("Received pulse"));
          bluetoothPulseLastRecvTime = now;
          break;

        default:
          Serial.print(F("Recevied illegal command of: "));
          Serial.println(cmd);
          break;
      }
    }

    switch (bluetoothState) {
      case BLUETOOTH_DISCONNECTED:
      case BLUETOOTH_ABANDONDED:
        if (numCmds > 0) {
          Serial.println(F("bluetooth is now connected!"));
          bluetoothState = BLUETOOTH_CONNECTED;
        } 
        break;

       case BLUETOOTH_CONNECTED:
         if ((now - bluetoothPulseLastRecvTime) > 10000) {
          Serial.println(F("bluetooth is now disconnected!"));
          bluetoothState = BLUETOOTH_DISCONNECTED;
         }
         break;
    }
  }

  // Basic joystick control. TODO: we can get way more precise with this joystick!
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
    
  // Update the LED screen with our current state.
  bool connected = (bluetoothState == BLUETOOTH_CONNECTED);
  int upSecs = (millis() - startTime) / 1000;
  displayStatus(
    connected ? F("CONNECTED") : F("DISCONNECTED"),
    "runtime: " + String(upSecs) + ", bstate=" + String(bluetoothEnabled),
    "stick: " + String(joystickX) + "/" + String(joystickY),
    "dude: " + String("yo"));
}


/**
 * 
 */
void bluetoothWriteMasterConfiguration() { 
  char commandBuffer[30];
  commandBuffer[0] = 0;
  const char *response;

  // Get the chip in command mode.
  switchToBluetoothCommandMode();

  // Reset the chip.
  writeToBluetoothCommand("AT+RMAAD");
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to reset"));
  }

  // Probe the version; this should always work. 
  writeToBluetoothCommand("AT+VERSION?");
  response = readFromBluetoothCommand();
  Serial.print(F("Bluetooth Version: "));
  Serial.println(response);
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to probe version"));
  }

  // Set the baud for the two bluetooth chips.
  writeToBluetoothCommand("AT+UART=38400,0,0");
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set speed"));
  }

  // Set the local name. 
  strcpy(commandBuffer, "AT+NAME=");
  strcat(commandBuffer, botRemoteConfig.bluetoothName);
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
  strcat(commandBuffer, botRemoteConfig.bluetoothAddr);
  writeToBluetoothCommand(commandBuffer);
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to bind remote address"));
  }

  // Set the remote slave passwd. 
  strcpy(commandBuffer, "AT+PSWD=");
  strcat(commandBuffer, botRemoteConfig.bluetoothPass);
  writeToBluetoothCommand(commandBuffer);
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to bind remote pass"));
  }

   // Get the local bluetooth address- errors out.
  /*writeToBluetoothCommand("AT+ADDR?");
  response = readFromBluetoothCommand();
  if (strlen(response) == 20) {
    // +ADDR:98d3:b1:fd60df
    strncpy(bluetoothAddress, response + 6, 14);
  } else {
    strcpy(bluetoothAddress, "invalid");
  }
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to completely get address"));
  } */

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
boolean switchToBluetoothCommandMode() {
  Serial.println(F("bluetooth command mode starting..."));

  // power it up with the enable pin on.
  digitalWrite(PIN_BLUETOOTH_ENABLE, HIGH);
  delay(500);
  digitalWrite(PIN_BLUETOOTH_POWER, HIGH);
  delay(750);
  
  Serial.println(F("bluetooth command mode started..."));

  for (int i = 0 ; i < 6; i++) {
    // Send the simple ack command to berify the command channel is up.
    writeToBluetoothCommand("AT");
    const char *response = readFromBluetoothCommand();
    if (!strcmp(response, "OK")) {
       Serial.print(F("bluetooth command mode established on try "));
       Serial.println(i);
       return true;
    } 
    delay(100);
  }

  while (bluetooth.available()) {
    const char *response = readFromBluetoothCommand();
    Serial.print(F("bluetooth clearing buffer: "));
    Serial.println(response);
  }

  Serial.println(F("bluetooth command mode FAILED to start"));
  return false;
}

/**
 * Turn the power off the bluetooth module.
 */
void switchToBluetoothOffMode() {
  // power it down
  digitalWrite(PIN_BLUETOOTH_POWER, LOW);
  delay(100);
}



/**
 *  CONFIG
 */


#define READ_BUFFER_SIZE 20
 

void configReset() {
  Serial.println(F("configReset: factory reseting the config..."));
  memset(&botRemoteConfig, sizeof(BattleBotRemoteConfig), 0);
  strcpy(botRemoteConfig.bluetoothName, "remote-newb");
  strcpy(botRemoteConfig.bluetoothAddr, "");
  strcpy(botRemoteConfig.bluetoothPass, "666");
}

void configImport() {

  // Init the global variable tht holds the config, restting back to the defaults.
  configReset();

  int memoryOffset = 0;
  char readBuffer[READ_BUFFER_SIZE];
  
  // Read the magic number, and make sure it matches.
  if (!configReadString(memoryOffset, readBuffer, READ_BUFFER_SIZE) || strcmp(readBuffer, "BTLRM")) {
    Serial.println(F("configImport: could not read magic, sticking with default"));
    return;
  }

  // Read the version number.
  if (!configReadString(memoryOffset, readBuffer, READ_BUFFER_SIZE)) {
    Serial.println(F("configImport: could not read version, sticking with default"));
    return;
  } 
  if (strcmp(readBuffer, "1")) {
    Serial.print(F("configImport: could not understand config version of "));
    Serial.print(readBuffer);
    Serial.println(F(", sticking with default"));
    return;
  }

  // Read the rest of the data.
  boolean success = configReadString(memoryOffset, botRemoteConfig.bluetoothName, BLUETOOTH_MAX_NAME_LEN) &&
    configReadString(memoryOffset, botRemoteConfig.bluetoothAddr, BLUETOOTH_MAX_ADDR_LEN) &&
    configReadString(memoryOffset, botRemoteConfig.bluetoothPass, BLUETOOTH_MAX_PASS_LEN);
  if (!success) {
    Serial.println(F("configImport: could not read bluetoothRemoteConfig"));
    configReset();
    return;
  }

  Serial.println(F("configImport: successful import"));
}


/*
 * Read one null-terminated string.
 */
boolean configReadString(int& memoryOffset, char *output, int outputMaxLen) {
  
  for (int i = 0 ; i < outputMaxLen ; i++) {
    if (memoryOffset >= EEPROM.length()) {
       Serial.println(F("configReadBytes: blew eeprom buffer"));
       return false;
    }

    // Read from the EProm, and increment the current memory offset.
    char c = EEPROM[memoryOffset++];

    // Is this the null byte? If so, the output is done.
    if (c == 0) {
      output[i] = 0;
      return true;
    }

    // Make sure this is a cool character.
    if (!isAscii(c)) {
      Serial.print(F("configReadBytes: illegal non-ascii char: "));
      Serial.println((int) c);
      return false;
    }

    // Copy it over.
    output[i] = c;
  }

  // If we got here, we never hit the null byte.
  Serial.println(F("configReadString: blew out the read buffer"));
  return false;
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

  //display.setCursor(0,36);
  //display.println(line4);

  // Print the local bluetooth name.
  int beginBlueName = 36;
  display.setCursor(0, beginBlueName);
  display.print(F("name:"));
  display.setCursor(34, beginBlueName);
  display.println(botRemoteConfig.bluetoothName);

  // Print the remote bluetooth address.
  int beginBlue = 46;
  display.setCursor(0, beginBlue);
  display.print("addr:");
  display.setCursor(34, beginBlue);
  display.println(botRemoteConfig.bluetoothAddr);
  
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
