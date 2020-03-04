#include "custom_remote.h"


// Constants: knobs and buttons.
#define PIN_BUTTON_YELLOW 4
#define PIN_BUTTON_BLUE   5
#define PIN_BUTTON_RED    6
#define PIN_BUTTON_GREEN  7
#define PIN_KNOB_BLUE  A3
#define PIN_KNOB_RED  A2
int knobBlueValue = 0;
int knobRedValue = 0;


custom_remote::custom_remote(SoftwareSerial* bluetooth) : base_remote(bluetooth) {

	Serial.println(F("Inside custom_remote::custom_remote"));
}


custom_remote::~custom_remote() {

	Serial.println(F("Inside custom_remote::~custom_remote"));
}


String custom_remote::getName() {
	
	return "mc-custom-bot";
}


void custom_remote::setup() {

	Serial.println(F("Inside custom_remote::setup"));
	
	base_remote::setup();

	// Init: knobs and buttons.
    pinMode(PIN_BUTTON_YELLOW, INPUT);
    pinMode(PIN_BUTTON_BLUE, INPUT);
    pinMode(PIN_BUTTON_RED, INPUT);
    pinMode(PIN_BUTTON_GREEN, INPUT);
    pinMode(PIN_KNOB_BLUE, INPUT);
    pinMode(PIN_KNOB_RED, INPUT);
}
	

void custom_remote::loop() {

	//Serial.println(F("Inside custom_remote::loop"));
	
	base_remote::loop();

	// Loop: buttons and knobs.
    processButton(PIN_BUTTON_YELLOW, '!');
    processButton(PIN_BUTTON_BLUE, '@');
    processButton(PIN_BUTTON_RED, '#');
    processButton(PIN_BUTTON_GREEN, '$');
    processKnob(PIN_KNOB_BLUE, knobBlueValue);
    processKnob(PIN_KNOB_RED, knobRedValue);
}


void custom_remote::processButton(int buttonId, char commandToSend) {

  int buttonState = digitalRead(buttonId);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH) {
    Serial.print(F("command to send: "));
    Serial.println(commandToSend);
    
    //getBluetooth().write(commandToSend);
  } 
}


void custom_remote::processKnob(int knobId, int &knobValueOld) {

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
