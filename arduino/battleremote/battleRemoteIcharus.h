#include "battleRemoteStandard.h"


/**/
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
