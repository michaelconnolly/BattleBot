#ifndef __searob_configuration_h__
#define __searob_configuration_h__

// Import external libraries
#include "Arduino.h"

// Constants
#define BLUETOOTH_MAX_NAME_LEN    20
#define BLUETOOTH_MAX_ADDR_LEN    20
#define BLUETOOTH_MAX_PASS_LEN    16


/**
 * Base class for all configuration objects serializing to EEPROM.
 *
 * Do not create one of these directly: 
 *		use SeaRobConfigRemote or SeaRobConfigRobot
 */
class SeaRobConfiguration {
  public:
	// Implemented in the derived classes.
	virtual boolean configExport() = 0;
	virtual boolean configImport() = 0;
	virtual void configReset() = 0;
  
  protected:
	/**
	 *  CONFIG: Write out one null terminated string to EEPROM, incrementing memoryOffset.
	 */
	boolean configWriteString(
	  int& memoryOffset, 
	  const char *output);

	/*
	 * CONFIG: Read one null-terminated string, incrementing memoryOffset.
	 */
	boolean configReadString(
	  int& memoryOffset, 
	  char *output, 
	  int outputMaxLen);
};
  

/**
 * Configuration object for the remote control.
 */
class SeaRobConfigRemote : public SeaRobConfiguration {
  public:
  						SeaRobConfigRemote();
  	virtual boolean 	configExport();
  	virtual boolean 	configImport();
    virtual void 		configReset();

  public:
	char bluetoothName[BLUETOOTH_MAX_NAME_LEN];
	char bluetoothAddr[BLUETOOTH_MAX_ADDR_LEN];
	char bluetoothPass[BLUETOOTH_MAX_PASS_LEN];
};


/**
 * Configuration object for the robot.
 */
class SeaRobConfigRobot : public SeaRobConfiguration {
  public:
  						SeaRobConfigRobot();
  	virtual boolean 	configExport();
  	virtual boolean 	configImport();
  	virtual void 		configReset();
  	
  public:
	char bluetoothName[BLUETOOTH_MAX_NAME_LEN];
	char bluetoothPass[BLUETOOTH_MAX_PASS_LEN];
};
  
#endif // __searob_configuration_h__
