#ifndef __battleremote_customtemplate_h__
#define __battleremote_customtemplate_h__

#include "battleRemoteStandard.h"


/*
	Template Class; duplicate this to extend the Standard Remote.
*/
class battleRemoteCustomTemplate : public battleRemoteStandard {
  public:
	battleRemoteCustomTemplate( 
	    const char *buildStamp,
        int pinSda, int pinScl,
        int pinBlueRecv, int pinBlueSend, int pinBlueEnable,
        int pinJoystickX, int pinJoystickY,
        int pinButtonLight);

	virtual const char * getName();
	virtual void setup();
	virtual void loop();
};

#endif // __battleremote_customtemplate_h__
