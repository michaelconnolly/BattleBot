// Import external libraries
#include "SeaRobBluetooth.h"


/**
 *
 */
SeaRobBluetooth::SeaRobBluetooth(int pinRecv, int pinSend, int pinEnable) :
	_pinRecv(pinRecv),
	_pinSend(pinSend),
	_pinEnable(pinEnable),
	_startTime(0),
	_serial(pinRecv, pinSend),
	_bluetoothState(BLUETOOTH_DISCONNECTED),
	_configEnabled(false),
	_pulseLastSendTime(0),
	_pulseLastRecvTime(0) {
	
  // Init the enable pin.
  pinMode(_pinEnable, OUTPUT);
  digitalWrite(_pinEnable, LOW);
  
  // SerialSoftware will set the pinmode, we just need to set the serial protocol speed.
  _serial.begin(38400);
  
  // Null cap the input string.
  _readbuffer[0] = 0;
  strcpy(_localAddress, "unknown");
  
  Serial.println(F("setup: Bluetooth complete..."));
}


/**
 * Call this at every arduino loop.
 */
void SeaRobBluetooth::loop() {

  unsigned long now = millis();

  // After a bit, configure the bluetooth chip.
  if (!_configEnabled && (!_startTime || ((now - _startTime) > BLUETOOTH_CONFIG_DELAY))) { 
    writeConfiguration();
    
    // Mark the flag so we dont probe this again.
    _configEnabled = true;
    _pulseLastSendTime = now;
  }
  
  if (_configEnabled) {
    // Send the keep-alive pulse every once in a while.
    if (now > (_pulseLastSendTime + BLUETOOTH_PULSE_DELAY)) {
      //Serial.println(F("Sending pulse"));
      _serial.write('@');
      _pulseLastSendTime = now;
    }
  
	// Track the connection state. If we were previously disconnected but there is data to read,
	// then we are now connected. If we were connected but are not receiving pulses, we are now 
	// disconnected.
	switch (_bluetoothState) {
	  case BLUETOOTH_DISCONNECTED:
	  case BLUETOOTH_ABANDONDED:
		if (ready()) {
		  Serial.println(F("bluetooth is now connected!"));
		  _bluetoothState = BLUETOOTH_CONNECTED;
		} 
		break;

	  case BLUETOOTH_CONNECTED:
		if ((now - _pulseLastRecvTime) > BLUETOOTH_PULSE_TIMEOUT) {
		  Serial.println(F("bluetooth is now disconnected!"));
		  _bluetoothState = BLUETOOTH_DISCONNECTED;
		}
	  break;
	}
  }
}


/**
 *
 */
boolean SeaRobBluetooth::ready() {
  return _configEnabled && (_serial.available() > 0);
}


/**
 *
 */
char SeaRobBluetooth::read() {
  char cmd = _serial.read();
  if (cmd == '@') {
	  Serial.println(F("Received pulse"));
	  _pulseLastRecvTime = millis();
  }
  return cmd;
}


/**
 *
 */
void SeaRobBluetooth::write(char data) {
  if (_configEnabled) {
    _serial.write(data);
  }
}


/**
 *
 */
boolean SeaRobBluetooth::isConnected() {
  return (_bluetoothState == BLUETOOTH_CONNECTED);
}


/**
 *
 */
boolean SeaRobBluetooth::isEnabled() {
  return _configEnabled;
}


/**
 * Cycle the power on the bluetooth module, starting it back up in normal mode.
 */
void SeaRobBluetooth::switchToBluetoothNormalMode() {
  Serial.println(F("bluetooth normal mode starting..."));

  // power it up with the enable pin off.
  digitalWrite(_pinEnable, LOW);
  delay(1250);

  Serial.println(F("bluetooth normal mode started..."));
}


/**
 * Cycle the power on the bluetooth module, starting it back up in command mode.
 */
boolean SeaRobBluetooth::switchToBluetoothCommandMode() {
  Serial.println(F("bluetooth command mode starting..."));

  // power it up with the enable pin on.
  digitalWrite(_pinEnable, HIGH);
  delay(1250);
  
  Serial.println(F("bluetooth command mode started..."));

  for (int i = 0 ; i < 6; i++) {
    // Send the simple ack command to berify the command channel is up.
    writeToBluetoothCommand("AT");
    const char *response = readFromBluetoothCommand();
    if (!strcmp(response, "OK")) {
       Serial.print(F("bluetooth command mode established on try "));
       Serial.println(i);
       return true;
    } 
    delay(100);
  }

  while (_serial.available()) {
    const char *response = readFromBluetoothCommand();
    Serial.print(F("bluetooth clearing buffer: "));
    Serial.println(response);
  }

  Serial.println(F("bluetooth command mode FAILED to start"));
  return false;
}


/**
 * Write one line out to the bluetooth in command mode.
 */
void SeaRobBluetooth::writeToBluetoothCommand(const char * data) {
  Serial.print(F("bluetooth send: "));
  Serial.println(data);
  
  _serial.println(data);
}


/**
 * Read one line of text from the bluetooth during command mode; all responses are separeated by a CR/LF.
 */
const char* SeaRobBluetooth::readFromBluetoothCommand() {

  // We will give a limited amount of time for the bluetooth to respond.
  int startTime = millis();
  int endTime = startTime + 10000;
  
  // Continue reading characters until we get to the end.
  int curr = 0;
  bool readComplete = false;
  while (!readComplete && (curr < BLUETOOTH_READ_BUFFER) && (millis() < endTime)) {
    if (_serial.available()) {
      char c = _serial.read();
      //Serial.println("\nbluetooth read: " + String(c));
      _readbuffer[curr++] = c;
      if (c == 10)
        readComplete = true;
    }
  }

  if (!readComplete) {
    // If we did not read a full complete line, return the error signal.
    _readbuffer[0] = 0;
  } else {
    // cap off the CR/LF from the end of the response.
    curr -= 2;
    _readbuffer[curr++] = 0; 
  }
  
  Serial.print(F("bluetooth read: "));
  Serial.println(_readbuffer);
  return _readbuffer;
}


/*** SeaRobBluetoothMaster ***/


SeaRobBluetoothMaster::SeaRobBluetoothMaster(int pinRecv, int pinSend, int pinEnable) :
	SeaRobBluetooth(pinRecv, pinSend, pinEnable),
	_botRemoteConfig(NULL) {
}

void SeaRobBluetoothMaster::setup(SeaRobConfigRemote *botRemoteConfig) {
  _botRemoteConfig = botRemoteConfig;
}


/**
 * 
 */
void SeaRobBluetoothMaster::writeConfiguration() { 
  char commandBuffer[30];
  commandBuffer[0] = 0;
  const char *response;

  // Get the chip in command mode.
  switchToBluetoothCommandMode();

  // Reset the chip.
  writeToBluetoothCommand("AT+RMAAD");
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to reset"));
  }

  // Probe the version; this should always work. 
  writeToBluetoothCommand("AT+VERSION?");
  response = readFromBluetoothCommand();
  Serial.print(F("Bluetooth Version: "));
  Serial.println(response);
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to probe version"));
  }

  // Set the baud for the two bluetooth chips.
  writeToBluetoothCommand("AT+UART=38400,0,0");
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set speed"));
  }

  // Set the local name. 
  strcpy(commandBuffer, "AT+NAME=");
  strcat(commandBuffer, _botRemoteConfig->bluetoothName);
  writeToBluetoothCommand(commandBuffer);
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set name"));
  }

  // Set master/slave mode to master, so we can actively connect to a remote peer. 
  writeToBluetoothCommand("AT+ROLE=1");
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set role"));
  }

  // Set conenction mode to 0, so it binds to a specific address. 
  writeToBluetoothCommand("AT+CMODE=0");
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set cmode"));
  }

  // Set the remote slave address. 
  strcpy(commandBuffer, "AT+BIND=");
  strcat(commandBuffer, _botRemoteConfig->bluetoothAddr);
  writeToBluetoothCommand(commandBuffer);
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to bind remote address"));
  }

  // Set the remote slave passwd. 
  strcpy(commandBuffer, "AT+PSWD=");
  strcat(commandBuffer, _botRemoteConfig->bluetoothPass);
  writeToBluetoothCommand(commandBuffer);
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to bind remote pass"));
  }

  // Get the local bluetooth address- errors out.
  writeToBluetoothCommand("AT+ADDR?");
  response = readFromBluetoothCommand();
  if (strlen(response) == 20) {
    // +ADDR:98d3:b1:fd60df
    strncpy(_localAddress, response + 6, 14);
  } else {
    strcpy(_localAddress, "invalid");
  }
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to completely get address"));
  }

  // Back to normal operation mode.
  switchToBluetoothNormalMode();
}


/*** SeaRobBluetoothSlave ***/


SeaRobBluetoothSlave::SeaRobBluetoothSlave(int pinRecv, int pinSend, int pinEnable) :
	SeaRobBluetooth(pinRecv, pinSend, pinEnable),
	_botRobotConfig(NULL) {
}

void SeaRobBluetoothSlave::setup(SeaRobConfigRobot *botRobotConfig) {
  _botRobotConfig = botRobotConfig;
}



void SeaRobBluetoothSlave::writeConfiguration() { 
  char commandBuffer[30];
  commandBuffer[0] = 0;
  const char *response;

  // Get the chip in command mode.
  switchToBluetoothCommandMode();
  
  // Reset the chip.
  writeToBluetoothCommand("AT+RMAAD");
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to reset"));
  }

  // Probe the version; this should always work. 
  writeToBluetoothCommand("AT+VERSION?");
  response = readFromBluetoothCommand();
  Serial.print(F("Bluetooth Version: "));
  Serial.println(response);
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to probe version"));
  }

  // Set the baud for the two bluetooth chips.
  writeToBluetoothCommand("AT+UART=38400,0,0");
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set speed"));
  }

  // Set the advertised name.
  //assert(strlen(botConfig.bluetoothName) > 0);
  strcpy(commandBuffer, "AT+NAME=");
  strcat(commandBuffer, _botRobotConfig->bluetoothName);
  writeToBluetoothCommand(commandBuffer);
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set name"));
  }

  // Set master/slave mode to slave, so we will passively wait for a slave to connect to us. 
  writeToBluetoothCommand("AT+ROLE=0");
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set role"));
  }

  // Set the slave passwd. 
  //assert(strlen(botConfig.bluetoothPass) > 0);
  strcpy(commandBuffer, "AT+PSWD=");
  strcat(commandBuffer, _botRobotConfig->bluetoothPass);
  writeToBluetoothCommand(commandBuffer);
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set pass"));
  }

   // Get the local bluetooth address.
  writeToBluetoothCommand("AT+ADDR?");
  response = readFromBluetoothCommand();
  if (strlen(response) == 20) {
    // +ADDR:98d3:b1:fd60df
    strncpy(_localAddress, response + 6, 14);
  } else {
    strcpy(_localAddress, "invalid");
    Serial.print(F("ERROR: got illegal bluetooth address: "));
    Serial.println(response);
  }
  response = readFromBluetoothCommand();
  if (strcmp(response, "OK")) {
     Serial.println(F("Failed to set pass"));
  }

  // Back to normal operation mode.
  switchToBluetoothNormalMode();
}

