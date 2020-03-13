#ifndef __battleremote_standard_h__
#define __battleremote_standard_h__

#include "Arduino.h"
#include <SeaRobConfiguration.h>
#include <SeaRobBluetooth.h>
#include <SeaRobDisplay.h>

/**
 * Basic Remote Control for a BattleBot. 
 */
class battleRemoteStandard {
  public:
    battleRemoteStandard(
	  const char *buildStamp,
	  int pinSda, int pinScl,
	  int pinBlueRecv, int pinBlueSend, int pinBlueEnable,
	  int pinJoystickX, int pinJoystickY,
	  int pinButtonLight);

    virtual const char * getName();
  
	virtual void setup();
	virtual void loop();
	virtual void updateDisplay();

  protected:
	void processButton(int buttonId, char commandToSend);
	void processKnob(int knobId, int &knobValueOld);
	void processJoystick();
	void processLightButton();
  
  private:
    unsigned long   _startTime; 
    unsigned char * _buildStamp;
    
    int 			_pinJoystickX;
    int 			_pinJoystickY;
    int 			_valueJoystickX;
    int 			_valueJoystickY;
    
    int 			_pinButtonLight;
    int 			_lastButtonLightState;
    boolean 	_lightState;
    
    char 			_lastCommand;
    
    // OLED Display
    SeaRobDisplay 	_display;
    
    // Persistent Configuration
    SeaRobConfigRemote _remoteConfig;

    // Bluetooth communication module
    SeaRobBluetoothMaster _bluetooth;
}; 

#endif // __battleremote_standard_h__
