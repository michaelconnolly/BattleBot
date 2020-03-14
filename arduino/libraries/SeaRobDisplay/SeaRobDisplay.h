#ifndef __searob_display_h__
#define __searob_display_h__

// Import external libraries
#include "Arduino.h"
#include <Adafruit_ssd1306syp.h>

// Constants
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
  
class SeaRobDisplay {
  public:
          SeaRobDisplay(int pinSda, int pinScl);
  
    void  	setup(
    		  const char *timestamp);
    void  	setupBluetoothName(
      		  const char *bluetoothName,
      		  const char *bluetoothAddr);
    
    void 	displayConnectedStandard(
    		  boolean 		connected, 
    		  const char * 	line2, 
    		  const char * 	line3);
    		  
    void 	displayStandard(
    		  const char * 	line1, 
    		  const char * 	line2, 
    		  const char * 	line3);
    		  
    boolean isBluetoothSet() { return _bluetoothSet; }
    
  private:
	Adafruit_ssd1306syp _display;
	const char *		_timestamp;
	
	boolean				_bluetoothSet;
	const char *		_bluetoothName;
	const char *		_bluetoothAddr;
};

#endif // __searob_display_h__
