// Import external libraries
#include <SeaRobConfiguration.h>
#include <SeaRobBluetooth.h>
#include <SeaRobDisplay.h>

// Constants: I/O Pins
#define PIN_BLUETOOTH_RECV    2
#define PIN_BLUETOOTH_SEND    3
#define PIN_BLUETOOTH_ENABLE  13
#define PIN_BUTTON_1          8
#define PIN_LED               10
#define PIN_I2C_SDA           A4
#define PIN_I2C_SCL           A5
#define PIN_JOYSTICK_X        A6
#define PIN_JOYSTICK_Y        A7

// Global Variables: Run state
unsigned long           startTime = 0; 
const char              buildTimestamp[] =  __DATE__ " " __TIME__;

// Global Variables: Persistent Configuration
SeaRobConfigRemote remoteConfig;

// Global Variables: the OLED Display, connected via I2C interface
SeaRobDisplay display(PIN_I2C_SDA, PIN_I2C_SCL);

// Global Variables: Bluetooth communication module
SeaRobBluetoothMaster bluetooth(PIN_BLUETOOTH_RECV, PIN_BLUETOOTH_SEND, PIN_BLUETOOTH_ENABLE);

// Remote state
char lastCommand = 0;
int lastButton1State = LOW;
boolean headlightState = false;

// Driving state.
//char lastCommand = 0;

// Remote class that implements all business logic.
battleRemoteIcharus* remote = new battleRemoteIcharus(&bluetooth);


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

  // Read our config/saved-state from the local disk. This will either be successful or reset to the default config.
  remoteConfig.configImport();

  // Init LED pin, and initially set it to on.
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);
  Serial.println(F("setup: LED complete..."));

  // Init Joystick.
  pinMode(PIN_JOYSTICK_X, INPUT);
  pinMode(PIN_JOYSTICK_Y, INPUT);
  Serial.println(F("setup: Joystick complete..."));

  // Init buttons.
  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  lastButton1State = HIGH;
  headlightState = false;
  Serial.println(F("setup: Buttons complete..."));

  // Init Bluetooth.
  bluetooth.setup(&remoteConfig);
  Serial.println(F("setup: Bluetooth complete..."));

  // Custom Remote logic.
  remote->setup();
  
  // Init the OLED display.
  display.setup(buildTimestamp);
  Serial.println(F("setup: OLED complete..."));

  // Init the rest of our internal state.
  //lastCommand = 0;
  Serial.println(F("setup: end"));
}


/**
 * Main Loop: called over and over again as the robot runs, 
 */
void loop() {
  
  unsigned long now = millis();

  // Advance the bluetooth state machine.
  bluetooth.loop();
  if (bluetooth.isEnabled() && !display.isBluetoothSet()) {
    display.setupBluetoothName(remoteConfig.bluetoothName, remoteConfig.bluetoothAddr);
  }

  // Process incoming commands from the robot.
  for (int numCmds = 0; (numCmds < 100) && bluetooth.ready() ; numCmds++) {
    char cmd = bluetooth.read();
    switch (cmd) {
      case '@':
        Serial.println(F("Received pulse"));
        break;
  
      default:
        Serial.print(F("Received illegal command of: "));
        Serial.println(cmd);
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
  
  // Update the LED screen with our current state.
  bool connected = (bluetoothState == BLUETOOTH_CONNECTED);
  int upSecs = (millis() - startTime) / 1000;
  String joystickInfo = remote->getJoystickInfo();
  
  displayStatus(
    connected ? F("CONNECTED") : F("DISCONNECTED"),
    "runtime: " + String(upSecs) + ", bstate=" + String(bluetoothEnabled),
    joystickInfo,
    "dude: " + String("yo"));


  // Loop: custom remote class.
  remote->loop();

  // Toggle the lights.
  int button1State = digitalRead(PIN_BUTTON_1);
  //Serial.print(F("Button state: "));
  //Serial.println(button1State);
  if (button1State != lastButton1State) {
    lastButton1State = button1State;
    Serial.println(F("Button toggle"));
    if (lastButton1State == LOW) {
      headlightState = !headlightState;
      Serial.print(F("Headlight state now: "));
      Serial.println(headlightState);
      bluetooth.write(headlightState ? 'W' : 'w');
    }
  }
   
  // Update the LED screen with our current state.
  updateDisplay(now, joystickX, joystickY);
}


void updateDisplay(unsigned long now, int joystickX, int joystickY) {
  // Format the Uptime.
  int upSecs = (now - startTime) / 1000;
  char line1Buffer[25];
  snprintf(line1Buffer, 25, "rtime: %2d  b: %d", upSecs, bluetooth.isEnabled());

  // Format the joystick.
  char line2Buffer[25];
  snprintf(line2Buffer, 25, "stick: %d/%d", joystickX, joystickY);

  // Send to the display.
  display.displayConnectedStandard(bluetooth.isConnected(), line1Buffer, line2Buffer);
}
