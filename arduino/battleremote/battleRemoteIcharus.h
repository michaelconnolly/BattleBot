#ifndef __battleremote_icharus_h__
#define __battleremote_icharus_h__

#include "battleRemoteStandard.h"

/*
 * Extended Remote with some knobs and more buttons.
 */
class battleRemoteIcharus : public battleRemoteStandard {
  public:
	  battleRemoteIcharus(
	    const char *buikdStamp,
	    int pinSda, int pinScl,
		  int pinBlueRecv, int pinBlueSend, int pinBlueEnable,
		  int pinJoystickX, int pinJoystickY,
		  int pinButtonLight);

	  virtual const char * getName();
	  virtual void setup();
	  virtual void loop();
	
  private:
	  int _knob1Value;
    int _knob2Value;
}; 

#endif // __battleremote_icharus_h__
