#include "battleRemoteIcharus.h"

// Constants: knobs and buttons.
#define PIN_BUTTON_1        4  // YELLOW
#define PIN_BUTTON_2		5  // BLUE
#define PIN_BUTTON_3		6  // RED
#define PIN_BUTTON_4		7  // GREEN
#define PIN_KNOB_1			A3 // BLUE
#define PIN_KNOB_2			A2 // RED
#define COMMAND_BUTTON_1    '!'
#define COMMAND_BUTTON_2    '%'
#define COMMAND_BUTTON_3    '#'
#define COMMAND_BUTTON_4    '$'

/*
*/
battleRemoteIcharus::battleRemoteIcharus( 
  const char *buildStamp,
  int pinSda, int pinScl,
  int pinBlueRecv, int pinBlueSend, int pinBlueEnable,
  int pinJoystickX, int pinJoystickY,
  int pinButtonLight) : 
  battleRemoteStandard(
	  buildStamp, 
	  pinSda, pinScl,
	  pinBlueRecv, pinBlueSend, pinBlueEnable,
	  pinJoystickX, pinJoystickY,
	  pinButtonLight),
  _knob1Value(0),
  _knob2Value(0) {
}


const char * battleRemoteIcharus::getName() {
  return "battle-remote Icharus";
}


void battleRemoteIcharus::setup() {
  battleRemoteStandard::setup();
	
  // Init: knobs and buttons.
  pinMode(PIN_BUTTON_1, INPUT);
  pinMode(PIN_BUTTON_2, INPUT);
  pinMode(PIN_BUTTON_3, INPUT);
  pinMode(PIN_BUTTON_4, INPUT);
  pinMode(PIN_KNOB_1, INPUT);
  pinMode(PIN_KNOB_2, INPUT);
  
  Serial.println(F("\tmore knobs and buttons"));
}
	

void battleRemoteIcharus::loop() {
  battleRemoteStandard::loop();

  // Loop: buttons and knobs.
  processButton(PIN_BUTTON_1, COMMAND_BUTTON_1);
  processButton(PIN_BUTTON_2, COMMAND_BUTTON_2);
  processButton(PIN_BUTTON_3, COMMAND_BUTTON_3);
  processButton(PIN_BUTTON_4, COMMAND_BUTTON_4);
  
  processKnob(PIN_KNOB_1, _knob1Value);
  processKnob(PIN_KNOB_2, _knob2Value);
}
