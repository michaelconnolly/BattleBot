#include "battleRemoteStandard.h"

// Local variables.
SoftwareSerial* _bluetooth;

// Constants: joystick.
#define PIN_JOYSTICK_X     A6
#define PIN_JOYSTICK_Y     A7
int joystickX;
int joystickY;
char lastCommand = 0;

// Constants: knobs and buttons.
#define PIN_BUTTON_1        4  // YELLOW
#define COMMAND_BUTTON_1    '!'


battleRemoteStandard::battleRemoteStandard(SoftwareSerial* bluetooth) {

	_bluetooth = bluetooth;
}


SoftwareSerial* battleRemoteStandard::getBluetooth() {
	
	return _bluetooth;
}


battleRemoteStandard::~battleRemoteStandard() {

}


void battleRemoteStandard::setup() {

	  Serial.print(F("setup: "));
    Serial.println(this->getName());

    // Init: knobs and buttons.
    pinMode(PIN_BUTTON_1, INPUT);
    Serial.println(F("\tknobs and buttons"));

    // Init Joystick.
    pinMode(PIN_JOYSTICK_X, INPUT);
    pinMode(PIN_JOYSTICK_Y, INPUT);
    Serial.println(F("\tjoystick"));
}

	
void battleRemoteStandard::loop() {

    // Loop: buttons and knobs.
    processButton(PIN_BUTTON_1, COMMAND_BUTTON_1);

    // Loop: joystick.
    processJoystick(PIN_JOYSTICK_X, PIN_JOYSTICK_Y);
}


String battleRemoteStandard::getName() {
	
	return F("battle-remote Standard");
}


String battleRemoteStandard::getJoystickInfo() {

  //return F("stick: " + String(joystickX) + "/" + String(joystickY));
  return "stick: " + String(joystickX) + "/" + String(joystickY);
}


void battleRemoteStandard::processJoystick(int pinX, int pinY) {
  
  // Basic joystick control. TODO: we can get way more precise with this joystick!
  joystickX = analogRead(pinX);
  joystickY = analogRead(pinY);
  
  if (joystickY > 600) {
    Serial.println(F("Sending forward"));
    _bluetooth->write('F');
    lastCommand = 'F';
  } else if (joystickY < 400) {
    Serial.println(F("Sending reverse"));
    _bluetooth->write('B');
    lastCommand = 'B';
  } else if (joystickX > 600) {
    Serial.println(F("Sending right"));
    _bluetooth->write('R');
    lastCommand = 'R';
  } else if (joystickX < 400) {
    Serial.println(F("Sending left"));
    _bluetooth->write('L');
    lastCommand = 'L';
  } else if (lastCommand != 'S') {
    Serial.println(F("Sending stop"));
    _bluetooth->write('S');
    lastCommand = 'S';
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
