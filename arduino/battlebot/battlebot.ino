// Import external libraries
#include <SeaRobConfiguration.h>
#include <SeaRobBluetooth.h>
#include <SeaRobDisplay.h>
#include <Servo.h>

// Constants: I/O Pins
#define PIN_BLUETOOTH_RECV    2
#define PIN_BLUETOOTH_SEND    3
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

// Global Variables: Feature Enablement.
//  To temporarily disable various subsystems, set these appropriately.
boolean useMotors = true;
boolean useLed = true;
boolean useDisplay = true;
boolean useBluetooth = true;
boolean useServo = false;

// Global Variables: Run state
unsigned long startTime = 0; 
const char buildTimestamp[] =  __DATE__ " " __TIME__;
boolean dead = false;
boolean autoShutOff = false;
int velocity = 100;  
char command = 'S';
char prevCommand = 'S';
unsigned long timeLastCommand = 0;  //Stores the time when the last command was received from the phone

// Global Variables: Persistent Configuration
SeaRobConfigRobot botConfig;

// Global Variables: the OLED Display, connected via I2C interface
SeaRobDisplay display(PIN_I2C_SDA, PIN_I2C_SCL);

// Global Variables: Bluetooth communication module
SeaRobBluetoothSlave bluetooth(PIN_BLUETOOTH_RECV, PIN_BLUETOOTH_SEND, PIN_BLUETOOTH_ENABLE);

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
  botConfig.configImport();

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
    bluetooth.setup(&botConfig);
    Serial.println(F("setup: Bluetooth complete..."));
  }
  
  // Init the OLED display
  if (useDisplay) {
    display.setup(buildTimestamp);
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
    bluetooth.loop();

    // If we are fully initialized, process the bluetooth command queue, which is all the 
    // commands from our remote control.
    if (bluetooth.isEnabled()) {
      if (!display.isBluetoothSet()) {
        display.setupBluetoothName(botConfig.bluetoothName, bluetooth.getAddress());
      }
      
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
    // Format the Uptime.
    int upSecs = (now - startTime) / 1000;
    char line1Buffer[25];
    snprintf(line1Buffer, 25, "rtime: %2d  b: %d", upSecs, bluetooth.isEnabled());

    // Format cmd line.
    char line2buffer[10];
    strcpy(line2buffer, "cmd: ");
    line2buffer[5] = command;
    line2buffer[6] = '/';
    line2buffer[7] = prevCommand;
    line2buffer[8] = 0;

    // Send to the display.
    display.displayConnectedStandard(
      bluetooth.isConnected(),
      line1Buffer,
      line2buffer);
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


/**
 * 
 */
void bluetoothProcess() {

  // If no commands, then nothing to do.
  if (!bluetooth.ready()) {
    return;
  }

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
