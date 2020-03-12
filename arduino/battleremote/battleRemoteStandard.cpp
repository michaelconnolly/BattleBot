#include "battleRemoteStandard.h"


/*
*/


battleRemoteStandard::battleRemoteStandard(
  const char * buildStamp,
  int pinSda, int pinScl,
  int pinBlueRecv, int pinBlueSend, int pinBlueEnable,
  int pinJoystickX, int pinJoystickY,
  int pinButtonLight) :
  _startTime(millis()),
  _buildStamp(buildStamp),
  _display(pinSda, pinScl),
  _bluetooth(pinBlueRecv, pinBlueSend, pinBlueEnable),
  _pinJoystickX(pinJoystickX),
  _pinJoystickY(pinJoystickY),
  _valueJoystickX(512),
  _valueJoystickY(512),
  _pinButtonLight(pinButtonLight),
  _lastButtonLightState(HIGH),
  _lightState(false),
  _lastCommand(0) {
}


const char *battleRemoteStandard::getName() {
  return "battle-remote Standard";
}


void battleRemoteStandard::setup() {
  Serial.print(F("setup: "));
  Serial.println(getName());
  
  // Read our config/saved-state from the local disk. This will either be successful or reset to the default config.
  _remoteConfig.configImport();

  // Init headlight button.
  pinMode(_pinButtonLight, INPUT_PULLUP);
  _lastButtonLightState = HIGH;
  _lightState = false;
  Serial.println(F("\tbutton light"));

  // Init Joystick.
  pinMode(_pinJoystickX, INPUT);
  pinMode(_pinJoystickY, INPUT);
  Serial.println(F("\tjoystick"));
  
  // Init Bluetooth.
  _bluetooth.setup(&_remoteConfig);
  Serial.println(F("\tbluetooth"));
  
  // Init the OLED display.
  _display.setup(_buildStamp);
  Serial.println(F("\tdisplay"));
}

	
void battleRemoteStandard::loop() {

  // Advance the bluetooth state machine.
  _bluetooth.loop();
  if (_bluetooth.isEnabled() && !_display.isBluetoothSet()) {
    _display.setupBluetoothName(
    	_remoteConfig.bluetoothName, _remoteConfig.bluetoothAddr);
  }
  
   // Process incoming network commands from the robot.
  for (int numCmds = 0; (numCmds < 100) && _bluetooth.ready() ; numCmds++) {
    char cmd = _bluetooth.read();
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
  
  // Process our input controls.
  processLightButton();
  processJoystick();
}

void battleRemoteStandard::processLightButton() {

  int buttonState = digitalRead(_pinButtonLight);
  //Serial.print(F("Button state: "));
  //Serial.println(button1State);
  if (buttonState != _lastButtonLightState) {
    _lastButtonLightState = buttonState;
    Serial.println(F("Button toggle"));
    if (_lastButtonLightState == LOW) {
     _lightState = !_lightState;
      Serial.print(F("Headlight state now: "));
      Serial.println(_lightState);
      _bluetooth.write(_lightState ? 'W' : 'w');
    }
  }
}

void battleRemoteStandard::processJoystick() {
  // Basic joystick control. TODO: we can get way more precise with this joystick!
  _valueJoystickX = analogRead(_pinJoystickX);
  _valueJoystickY = analogRead(_pinJoystickY);
  
  if (_valueJoystickY > 600) {
    Serial.println(F("Sending forward"));
    _bluetooth.write('F');
    _lastCommand = 'F';
  } else if (_valueJoystickY < 400) {
    Serial.println(F("Sending reverse"));
    _bluetooth.write('B');
    _lastCommand = 'B';
  } else if (_valueJoystickX > 600) {
    Serial.println(F("Sending right"));
    _bluetooth.write('R');
    _lastCommand = 'R';
  } else if (_valueJoystickX < 400) {
    Serial.println(F("Sending left"));
    _bluetooth.write('L');
    _lastCommand = 'L';
  } else if (_lastCommand != 'S') {
    Serial.println(F("Sending stop"));
    _bluetooth.write('S');
    _lastCommand = 'S';
  }
}


void battleRemoteStandard::processButton(int buttonId, char commandToSend) {

  int buttonState = digitalRead(buttonId);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH) {
    Serial.print(F("command to send: "));
    Serial.println(commandToSend);
    
    //getBluetooth().write(commandToSend);
  } 
}


void battleRemoteStandard::processKnob(int knobId, int &knobValueOld) {

  int knobValue = analogRead(knobId);

  // TODO: a knob should make something going left and right.
  // One model is that the span of the knob's rotation covers the same span
  // that the thing is that we are moving.
  // Another model is that anything left of center means keep going to the left,
  // and a particular speed.  Moving the knob into it's center position means
  // to stop moving.

  // If the value is different from the last value we knew about, 
  // we should probably tell the robot that the value changed.
  if (knobValue != knobValueOld) {
    
    Serial.print(F("knobId: "));
    Serial.print(knobId);
    Serial.print(F(", knobValue: "));
    Serial.println(knobValue);

    knobValueOld = knobValue;
  }
}

void battleRemoteStandard::updateDisplay() {
  // Format the Uptime.
  int upSecs = (millis() - _startTime) / 1000;
  char line1Buffer[25];
  snprintf(line1Buffer, 25, "rtime: %2d  b: %d", upSecs, _bluetooth.isEnabled());

  // Format the joystick.
  char line2Buffer[25];
  snprintf(line2Buffer, 25, "stick: %d/%d", _valueJoystickX, _valueJoystickY);

  // Send to the display.
  _display.displayConnectedStandard(_bluetooth.isConnected(), line1Buffer, line2Buffer);
}
