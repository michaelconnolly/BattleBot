#include "battleRemoteCustomTemplate.h"

// Constants: add here.

/* */
battleRemoteCustomTemplate::battleRemoteCustomTemplate( const char *buildStamp,
     int pinSda, int pinScl,
      int pinBlueRecv, int pinBlueSend, int pinBlueEnable,
      int pinJoystickX, int pinJoystickY,
      int pinButtonLight) : 
  battleRemoteStandard(buildStamp,
      pinSda,  pinScl,
       pinBlueRecv,  pinBlueSend,  pinBlueEnable,
       pinJoystickX,  pinJoystickY,
       pinButtonLight) {
}


const char *battleRemoteCustomTemplate::getName() {
  return "battle-remote custom";
}


void battleRemoteCustomTemplate::setup() {
  battleRemoteStandard::setup();

  // Init: add here.
}
	

void battleRemoteCustomTemplate::loop() {
  battleRemoteStandard::loop();

  // Loop: add here.
}
