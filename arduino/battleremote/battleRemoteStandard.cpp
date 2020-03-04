#include "battleRemoteStandard.h"


// Local variables.
SoftwareSerial* _bluetooth;


// Constants: knobs and buttons.
#define PIN_BUTTON_1        4  // YELLOW
#define COMMAND_BUTTON_1    '!'


battleRemoteStandard::battleRemoteStandard(SoftwareSerial* bluetooth) {

	_bluetooth = bluetooth;

	Serial.print(F("Initializing "));
    Serial.println(this->getName());
}


SoftwareSerial* battleRemoteStandard::getBluetooth() {
	
	return _bluetooth;
}


battleRemoteStandard::~battleRemoteStandard() {

}


void battleRemoteStandard::setup() {

	Serial.print(F("Setup: "));
    Serial.println(this->getName());

    // Init: knobs and buttons.
    pinMode(PIN_BUTTON_1, INPUT);
}

	
void battleRemoteStandard::loop() {

    // Loop: buttons and knobs.
    processButton(PIN_BUTTON_1, COMMAND_BUTTON_1);
}


String battleRemoteStandard::getName() {
	
	return "battle-remote Standard";
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
