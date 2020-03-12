#include "battleRemoteStandard.h"


/*
*/
class battleRemoteCustomTemplate : public battleRemoteStandard {
  public:
	battleRemoteCustomTemplate( const char *buildStamp,
     int pinSda, int pinScl,
      int pinBlueRecv, int pinBlueSend, int pinBlueEnable,
      int pinJoystickX, int pinJoystickY,
      int pinButtonLight);

	virtual const char * getName();
	virtual void setup();
	virtual void loop();
};
