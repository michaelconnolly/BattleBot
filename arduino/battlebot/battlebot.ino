// Import external libraries
#include <Adafruit_ssd1306syp.h>
#include <SoftwareSerial.h>
#include <Servo.h>
#include <EEPROM.h>

// Constants: I/O Pins
#define PIN_BLUETOOTH_RECV    2
#define PIN_BLUETOOTH_SEND    3
#define PIN_BLUETOOTH_POWER   12
#define PIN_BLUETOOTH_ENABLE  13
#define PIN_MOTOR_A_ENABLE    5
#define PIN_MOTOR_B_ENABLE    6
#define PIN_MOTOR_A_INPUT1    7
#define PIN_MOTOR_A_INPUT2    8
#define PIN_MOTOR_B_INPUT1    11
#define PIN_MOTOR_B_INPUT2    4
#define PIN_LED               10
#define PIN_I2C_SDA           A4
#define PIN_I2C_SCL           A5
#define PIN_SERVO             9
 
// More Constants
#define AUTO_SHUTOFF_TIME 30000
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Constants: Bluetooth
#define BLUETOOTH_DETECTION_TIME  8000
#define BLUETOOTH_PULSE_TIME      5000
#define BLUETOOTH_READ_BUFFER     30
#define BLUETOOTH_MAX_NAME_LEN    20
#define BLUETOOTH_MAX_PASS_LEN    16

// The various states of our bluetooth connection.
enum BluetoothState {
  BLUETOOTH_DISCONNECTED,
  BLUETOOTH_CONNECTED,
  BLUETOOTH_ABANDONDED
};

typedef struct {
  char bluetoothName[BLUETOOTH_MAX_NAME_LEN];
  char bluetoothPass[BLUETOOTH_MAX_PASS_LEN];
} BattleBotConfig;


void displayStatus(String line1, String line2, String line3, String line4);

// Global Variables: Feature Enablement.
//  To temporarily disable various subsystems, set these appropriately.
boolean useMotors = true;
boolean useLed = true;
boolean useDisplay = true;
boolean useBluetooth = true;
boolean useServo = false;

// Global Variables: Run state
BattleBotConfig botConfig;
boolean dead = false;
boolean autoShutOff = false;
unsigned long startTime = 0; 
const char buildTimestamp[] =  __DATE__ " " __TIME__;

// Global Variables: the OLED Display, connected via I2C interface
Adafruit_ssd1306syp display(PIN_I2C_SDA, PIN_I2C_SCL);

// Global Variables: motor control
int velocity = 100;  

// Global Variables: Bluetooth command Queue
SoftwareSerial bluetooth(PIN_BLUETOOTH_RECV, PIN_BLUETOOTH_SEND);
BluetoothState bluetoothState = BLUETOOTH_DISCONNECTED;
char bluetoothReadbuffer[BLUETOOTH_READ_BUFFER + 1];
int bluetoothAddressState = 0;
char bluetoothAddress[15];
unsigned long bluetoothPulseLastTime = 0;

char command = 'S';
char prevCommand = 'S';
unsigned long timeLastCommand = 0;  //Stores the time when the last command was received from the phone

// Global Variables: Servo Weapon
Servo servoMotor; 
float servoPos = 0.0;

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

  // Init motor control pins.
  if (useMotors) {
    pinMode(PIN_MOTOR_A_INPUT1, OUTPUT);
    pinMode(PIN_MOTOR_A_INPUT2, OUTPUT);
    pinMode(PIN_MOTOR_B_INPUT1, OUTPUT);
    pinMode(PIN_MOTOR_B_INPUT2, OUTPUT);
    pinMode(PIN_MOTOR_A_ENABLE, OUTPUT);
    pinMode(PIN_MOTOR_B_ENABLE, OUTPUT);
    Serial.println(F("setup: Motor complete..."));
  }

  // Init LED pin, and initially set it to off.
  if (useLed) {
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);
    Serial.println(F("setup: LED complete..."));
  }

  // Init the serial pipe to the bluetooth receiver
  if (useBluetooth) {
    pinMode(PIN_BLUETOOTH_POWER, OUTPUT);
    pinMode(PIN_BLUETOOTH_ENABLE, OUTPUT);
    digitalWrite(PIN_BLUETOOTH_POWER, HIGH);
    digitalWrite(PIN_BLUETOOTH_ENABLE, LOW);
    bluetoothAddressState = 0;
    strcpy(bluetoothAddress, "unknown");
    bluetooth.begin(38400);
    Serial.println(F("setup: Bluetooth complete..."));
  }
  
  // Init the OLED display
  if (useDisplay) {
    delay(1000);
    display.initialize();
    Serial.println(F("setup: OLED complete..."));
  }

  // Init the servo.
  if (useServo) {
    servoMotor.attach(PIN_SERVO); 
    servoPos = 0.0;
    servoMotor.write(0.0);
    Serial.println(F("setup: Servo complete..."));
  }

  // Init the rest of our internal state.
  dead = false;
  bluetoothPulseLastTime = 0;
  Serial.println(F("setup: end"));
}


/**
 * Main Loop: called over and over again as the robot runs, 
 */
void loop() {

  // Get the current time.
  unsigned long now = millis();
  
  // If we have marked as dead, be dead and do nothing.
  if (dead) { 
    //displayMessage("DEAD", "Deactivated due to auto-shutoff");
    return;
  }

  // Engage auto-shutoff if it has been enabled.
  if (autoShutOff && (millis() > (startTime + AUTO_SHUTOFF_TIME))) {
    dead = true;
    motorStop();
    //displayMessage("Auto-Stop", "Stopped due to shutdown timer");
    delay(3000);
  }
  
  // Bluetooth subsystem
  if (useBluetooth) {
    
     // Send the keep-alive pulse.
    if (now > (bluetoothPulseLastTime + BLUETOOTH_PULSE_TIME)) {
      //Serial.println(F("Sending pulse"));
      bluetooth.write('@');
      bluetoothPulseLastTime = now;
    }

    // Have we determined our bluetooth address yet?
    if (!bluetoothAddressState && ((now - startTime) > BLUETOOTH_DETECTION_TIME)) { 
      // Write the default slave configuration.
      bluetoothWriteSlaveConfiguration();
  
      // Mark the flag so we dont probe this again.
      bluetoothAddressState = 1;
      Serial.print(F("local bluetooth address is: "));
      Serial.println(bluetoothAddress);
    }

     // If we are fully initialized, process the bluetooth command queue, which is all the 
     //   commands from our remote control.
    if (bluetoothAddressState) {
      bluetoothProcess();
    }
  }

  if (useServo) {
      /*for (servoPos = 0; servoPos <= 180; servoPos += 1) { // goes from 0 degrees to 180 degrees
      // in steps of 1 degree
      servoMotor.write(servoPos);              // tell servo to go to position in variable 'pos'
      delay(15);                       // waits 15ms for the servo to reach the position
    }
    for (servoPos = 180; servoPos >= 0; servoPos -= 1) { // goes from 180 degrees to 0 degrees
      servoMotor.write(servoPos);              // tell servo to go to position in variable 'pos'
      delay(15);                       // waits 15ms for the servo to reach the position
    } */
  }
 
  // Update the OLED screen with our current state.
  if (useDisplay) {
    bool connected = (bluetoothState == BLUETOOTH_CONNECTED);
    int upSecs = (millis() - startTime) / 1000;
    char cmdbuf[10];
    strcpy(cmdbuf, "cmd: ");
    cmdbuf[5] = command;
    cmdbuf[6] = '/';
    cmdbuf[7] = prevCommand;
    cmdbuf[8] = 0;
    displayStatus(
      connected ? F("CONNECTED") : F("DISCONNECTED"),
      "runtime: " + String(upSecs), 
      // TODO: printing the command sometimes has a lf, so need to clean first.
      cmdbuf,
      "objective: " + String("KILL"));
  }
}


/** MOTOR CONTROL **/

#define TURN_SCALER 0.40

void motorForward() {
  digitalWrite(PIN_MOTOR_A_INPUT1, HIGH);
  digitalWrite(PIN_MOTOR_A_INPUT2, LOW);
  analogWrite(PIN_MOTOR_A_ENABLE, velocity);
  
  digitalWrite(PIN_MOTOR_B_INPUT1, HIGH);
  digitalWrite(PIN_MOTOR_B_INPUT2, LOW);
  analogWrite(PIN_MOTOR_B_ENABLE, velocity); 
}

void motorReverse() {
  digitalWrite(PIN_MOTOR_A_INPUT1, LOW);
  digitalWrite(PIN_MOTOR_A_INPUT2, HIGH);
  analogWrite(PIN_MOTOR_A_ENABLE, velocity);
    
  digitalWrite(PIN_MOTOR_B_INPUT1, LOW);
  digitalWrite(PIN_MOTOR_B_INPUT2, HIGH);
  analogWrite(PIN_MOTOR_B_ENABLE, velocity); 
}

void motorTurnLeft() {
  digitalWrite(PIN_MOTOR_A_INPUT1, HIGH);
  digitalWrite(PIN_MOTOR_A_INPUT2, LOW);
  analogWrite(PIN_MOTOR_A_ENABLE, velocity);
  
  digitalWrite(PIN_MOTOR_B_INPUT1, LOW);
  digitalWrite(PIN_MOTOR_B_INPUT2, HIGH);
  analogWrite(PIN_MOTOR_B_ENABLE, velocity); 
}

void motorTurnLeftForward() {
  digitalWrite(PIN_MOTOR_A_INPUT1, HIGH);
  digitalWrite(PIN_MOTOR_A_INPUT2, LOW);
  analogWrite(PIN_MOTOR_A_ENABLE, velocity);
  
  digitalWrite(PIN_MOTOR_B_INPUT1, HIGH);
  digitalWrite(PIN_MOTOR_B_INPUT2, LOW);
  analogWrite(PIN_MOTOR_B_ENABLE, velocity * TURN_SCALER); 
}

void motorTurnLeftReverse() {
  digitalWrite(PIN_MOTOR_A_INPUT1, LOW);
  digitalWrite(PIN_MOTOR_A_INPUT2, HIGH);
  analogWrite(PIN_MOTOR_A_ENABLE, velocity);
    
  digitalWrite(PIN_MOTOR_B_INPUT1, LOW);
  digitalWrite(PIN_MOTOR_B_INPUT2, HIGH);
  analogWrite(PIN_MOTOR_B_ENABLE, velocity * TURN_SCALER); 
}

void motorTurnRight() {
  digitalWrite(PIN_MOTOR_A_INPUT1, LOW);
  digitalWrite(PIN_MOTOR_A_INPUT2, HIGH);
  analogWrite(PIN_MOTOR_A_ENABLE, velocity);
  
  digitalWrite(PIN_MOTOR_B_INPUT1, HIGH);
  digitalWrite(PIN_MOTOR_B_INPUT2, LOW);
  analogWrite(PIN_MOTOR_B_ENABLE, velocity); 
}

void motorTurnRightForward() {
  digitalWrite(PIN_MOTOR_A_INPUT1, HIGH);
  digitalWrite(PIN_MOTOR_A_INPUT2, LOW);
  analogWrite(PIN_MOTOR_A_ENABLE, velocity * TURN_SCALER);
  
  digitalWrite(PIN_MOTOR_B_INPUT1, HIGH);
  digitalWrite(PIN_MOTOR_B_INPUT2, LOW);
  analogWrite(PIN_MOTOR_B_ENABLE, velocity); 
}

void motorTurnRightReverse() {
  digitalWrite(PIN_MOTOR_A_INPUT1, LOW);
  digitalWrite(PIN_MOTOR_A_INPUT2, HIGH);
  analogWrite(PIN_MOTOR_A_ENABLE, velocity * TURN_SCALER);
    
  digitalWrite(PIN_MOTOR_B_INPUT1, LOW);
  digitalWrite(PIN_MOTOR_B_INPUT2, HIGH);
  analogWrite(PIN_MOTOR_B_ENABLE, velocity); 
}

void motorStop() {
  analogWrite(PIN_MOTOR_A_ENABLE, 0); 
  analogWrite(PIN_MOTOR_B_ENABLE, 0); 
}


/**** BLUETOOTH ****/

void bluetoothWriteSlaveConfiguration() { 
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

  // Set the advertised name.
  //assert(strlen(botConfig.bluetoothName) > 0);
  strcpy(commandBuffer, "AT+NAME=");
  strcat(commandBuffer, botConfig.bluetoothName);
  writeToBluetoothCommand(commandBuffer);
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set name"));
  }

  // Set master/slave mode to slave, so we will passively wait for a slave to connect to us. 
  writeToBluetoothCommand("AT+ROLE=0");
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set role"));
  }

  // Set the slave passwd. 
  //assert(strlen(botConfig.bluetoothPass) > 0);
  strcpy(commandBuffer, "AT+PSWD=");
  strcat(commandBuffer, botConfig.bluetoothPass);
  writeToBluetoothCommand(commandBuffer);
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set pass"));
  }

   // Get the local bluetooth address.
  writeToBluetoothCommand("AT+ADDR?");
  response = readFromBluetoothCommand();
  if (strlen(response) == 20) {
    // +ADDR:98d3:b1:fd60df
    strncpy(bluetoothAddress, response + 6, 14);
  } else {
    strcpy(bluetoothAddress, "invalid");
    Serial.print(F("ERROR: got illegal bluetooth address: "));
    Serial.println(response);
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
const char * readFromBluetoothCommand() {

  // We will give a limited amount of time for the bluetooth to respond.
  int startTime = millis();
  int endTime = startTime + 10000;
  
  // Continue reading characters until we get to the end.
  int curr = 0;
  bool readComplete = false;
  while (!readComplete && (curr < BLUETOOTH_READ_BUFFER) && (millis() < endTime)) {
    if (bluetooth.available()) {
      char c = bluetooth.read();
      int code = (int) c;
      //Serial.println("\nbluetooth read: " + String(code));
      bluetoothReadbuffer[curr++] = c;
      if (c == 10)
        readComplete = true;
    }
  }

  // If we did not read a full complete line, return the error signal.
  if (!readComplete) {
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
 * 
 */
void bluetoothProcess() {

  // If no commands, then nothing to do.
  if (bluetooth.available() <= 0) {
    return;
  }

  // A little state management. Since we have data ready to read, that means we are connected.
  bluetoothState = BLUETOOTH_CONNECTED;

  // Record when the last command was processed.
  timeLastCommand = millis();  

  // Read the actual command from the bluetooth buffer.
  prevCommand = command;
  command = bluetooth.read();
  if (command != 'S') {
    //Serial.println("cmd2new: " + String(command) + "/" + String(prevCommand));
  }
  
  // Process the known commands. Note that we change pin mode only if new command is different from previous.  
  if (command != prevCommand) {
    //Serial.println("cmd2: " + String(command));
    switch (command){
    case 'F': 
      motorForward();
      break;
    case 'B': 
      motorReverse();
      break;
    case 'L': 
      motorTurnLeft();
      break;
    case 'R':
      motorTurnRight();
      break;
    case 'S': 
      motorStop();
      break;
    case 'I':
      motorTurnRightForward();
      break;
    case 'J':
     motorTurnRightReverse();
      break;       
    case 'G':
      motorTurnLeftForward();
      break;
    case 'H':
      motorTurnLeftReverse();
      break;
    case 'W':  
      //Front Lights ON
      digitalWrite(PIN_LED, HIGH);
      break;
    case 'w':  
      //Front Lights OFF
      digitalWrite(PIN_LED, LOW);
      break;
    case 'U':  //Back ON
      //digitalWrite(pinbackLights, HIGH);
      //servoMotor.write(180.0);  
      break;
    case 'u':  //Back OFF
      //digitalWrite(pinbackLights, LOW);
      //servoMotor.write(0.0);  
      break;
    case 'D':  //Everything OFF
      digitalWrite(PIN_LED, LOW);
      //digitalWrite(pinbackLights, LOW);
      motorStop();
      break;        
    default:  //Get velocity
      if(command=='q'){
        velocity = 255;  //Full velocity
        //yellowCar.SetSpeed_4W(velocity);
      }
      else{
        //Chars '0' - '9' have an integer equivalence of 48 - 57, accordingly.
        if((command >= 48) && (command <= 57)){
          //Subtracting 48 changes the range from 48-57 to 0-9.
          //Multiplying by 25 changes the range from 0-9 to 0-225.
          velocity = (command - 48)*25;      
          //yellowCar.SetSpeed_4W(velocity);
        }
      }
    }
  }
}


/**
 *  CONFIG
 */


 #define READ_BUFFER_SIZE 20
 

void configReset() {
  Serial.println(F("configReset: factory reseting the config..."));
  memset(&botConfig, sizeof(BattleBotConfig), 0);
  strcpy(botConfig.bluetoothName, "battlebot-newb");
  strcpy(botConfig.bluetoothPass, "666");
}

void configImport() {

  // Init the global variable tht holds the config, restting back to the defaults.
  configReset();

  int memoryOffset = 0;
  char readBuffer[READ_BUFFER_SIZE];
  
  // Read the magic number, and make sure it matches.
  if (!configReadString(memoryOffset, readBuffer, READ_BUFFER_SIZE) || strcmp(readBuffer, "BTLBT")) {
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
  boolean success = configReadString(memoryOffset, botConfig.bluetoothName, BLUETOOTH_MAX_NAME_LEN) &&
    configReadString(memoryOffset, botConfig.bluetoothPass, BLUETOOTH_MAX_PASS_LEN);
  if (!success) {
    Serial.println(F("configImport: could not read bluetoothConfig"));
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

 
/*** DISPLAY FUNCTIONS ***/

/**
 * Main output for status while in main sequence. This is called once per loop.
 * TODO: can we get rid of strings? they are memory hogs.
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
  display.println(botConfig.bluetoothName);

  // Print the local bluetooth address.
  int beginBlueAddr = 46;
  display.setCursor(0, beginBlueAddr);
  display.print(F("addr:"));
  display.setCursor(34, beginBlueAddr);
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
