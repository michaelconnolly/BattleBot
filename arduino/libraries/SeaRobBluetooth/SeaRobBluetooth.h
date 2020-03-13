#ifndef __searob_bluetooth_h__
#define __searob_bluetooth_h__

// Import external libraries
#include "Arduino.h"
#include "SoftwareSerial.h"
#include "SeaRobConfiguration.h"

// Constants
#define BLUETOOTH_READ_BUFFER 30
#define BLUETOOTH_CONFIG_DELAY 5000
#define BLUETOOTH_PULSE_DELAY 5000
#define BLUETOOTH_PULSE_TIMEOUT 10000

// The various states of our bluetooth connection.
enum BluetoothState {
  BLUETOOTH_DISCONNECTED,
  BLUETOOTH_CONNECTED,
  BLUETOOTH_ABANDONDED
};


/**
 * Base class; dont use directly. Use SeaRobBluetoothMaster or SeaRobBluetoothClient instead.
 */
class SeaRobBluetooth {
  public:
	SeaRobBluetooth(int pinRecv, int pinSend, int pinEnable);

  public:
	void 		loop();
	boolean 	ready();
	char		read();
	void		write(char data);
	boolean 	isConnected();
	boolean 	isEnabled();
	const char * getAddress() { return _localAddress; }
	
  protected:	
    virtual void    writeConfiguration() = 0;
    
	const char* 	readFromBluetoothCommand();
	void 			writeToBluetoothCommand(const char * data);

	boolean 		switchToBluetoothCommandMode();
	void 			switchToBluetoothNormalMode();
	
  protected:
    char 			_localAddress[15];
    
  private:
	int 			_pinRecv;
	int 			_pinSend;
	int 			_pinEnable;
	unsigned long	_startTime;
	SoftwareSerial 	_serial;
	BluetoothState 	_bluetoothState;
	boolean			_configEnabled;
	unsigned long 	_pulseLastSendTime;
	unsigned long 	_pulseLastRecvTime;
	char 			_readbuffer[BLUETOOTH_READ_BUFFER + 1];
};


/**
 *
 */
class SeaRobBluetoothMaster : public SeaRobBluetooth {
  public:
	SeaRobBluetoothMaster(int pinRecv, int pinSend, int pinEnable);
	
  public:
  	 void setup(SeaRobConfigRemote *botConfig);
  	 
  protected:
     virtual void    writeConfiguration();
	
  private:
  	 SeaRobConfigRemote *	_botRemoteConfig;
};


/**
 *
 */
class SeaRobBluetoothSlave : public SeaRobBluetooth {
  public:
	SeaRobBluetoothSlave(int pinRecv, int pinSend, int pinEnable);
	
  public:
  	 void setup(SeaRobConfigRobot *botConfig);
  	 
  protected:
     virtual void    writeConfiguration();
	
  private:
  	 SeaRobConfigRobot *	_botRobotConfig;
};

  
#endif // __searob_bluetooth_h__
