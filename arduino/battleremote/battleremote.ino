// Import external libraries
#include "battleRemoteStandard.h"

// Constants: I/O Pins
#define PIN_BLUETOOTH_RECV    2
#define PIN_BLUETOOTH_SEND    3
#define PIN_BLUETOOTH_ENABLE  13
#define PIN_BUTTON_LIGHT      8
#define PIN_I2C_SDA           A4
#define PIN_I2C_SCL           A5
#define PIN_JOYSTICK_X        A6
#define PIN_JOYSTICK_Y        A7

// Global Variables: Build Timestamp.
const char buildTimestamp[] =  __DATE__ " " __TIME__;

// Remote class that implements all business logic.
battleRemoteStandard remote(
    buildTimestamp,
    PIN_I2C_SDA, PIN_I2C_SCL,
  	PIN_BLUETOOTH_RECV, PIN_BLUETOOTH_SEND, PIN_BLUETOOTH_ENABLE,
  	PIN_JOYSTICK_X, PIN_JOYSTICK_Y,
  	PIN_BUTTON_LIGHT);

/**
 * Entrypoint: called once when the program first starts, just to initialize all the sub-components.
 */
void setup() {  
  // Init the serial line; important for debug messages back to the Arduino Serial Monitor, so you can plug
  // the robot into the USB port, and get real time debug messages.
  // Make sure you set the baudrate at 9600 in Serial Monitor as well.
  Serial.begin(9600);

  // Let the remote class handle the event.
  Serial.println(F("setup: begin..."));
  remote.setup();
  Serial.println(F("setup: end"));
}


/**
 * Main Loop: called over and over again as the robot runs.
 */
void loop() {
  // Let the remote class handle the event.
  remote.loop();

  // Update the LED screen with our current state.
  remote.updateDisplay();
}
