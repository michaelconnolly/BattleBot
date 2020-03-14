// Import external libraries
#include "SeaRobConfiguration.h"
#include <EEPROM.h>

// Constants
#define READ_BUFFER_SIZE 20

/**
 * CONFIG: Write out one null terminated string to EEPROM, incrementing memoryOffset.
 */
boolean SeaRobConfiguration::configWriteString(int& memoryOffset, const char *output) {
  // Make sure this won't blow out the buffer.
  int outputLen = strlen(output);
  if (memoryOffset + outputLen + 2 >= EEPROM.length()) {
     Serial.println(F("writeString: blew eeprom buffer"));
     return false;
  }

  // Loop through the string to be outputted, one char at a time.
  for (int i = 0 ; i < outputLen ; i++) {
    char c = output[i];
    
    // Make sure this is a cool character.
    if (!isAscii(c)) {
      Serial.print(F("writeString: illegal non-ascii char: "));
      Serial.println((int) c);
      c = '_';
    }
    
    // Write out to eeprom, and increment the current memory offset.
    EEPROM[memoryOffset++] = c;
  }

  // Cap off with a nullbyte.
  EEPROM[memoryOffset++] = 0;
  //Serial.print(F("writeString: wrote: "));
  //Serial.println(output);
  return true;
}


/*
 * Read one null-terminated string from EEPROM.
 */
boolean SeaRobConfiguration::configReadString(int& memoryOffset, char *output, int outputMaxLen) {
  
  for (int i = 0 ; i < outputMaxLen ; i++) {
    if (memoryOffset >= EEPROM.length()) {
       Serial.println(F("readString: blew eeprom buffer"));
       return false;
    }

    // Read from the EProm, and increment the current memory offset.
    char c = EEPROM[memoryOffset++];

    // Is this the null byte? If so, the output is done.
    if (c == 0) {
      output[i] = 0;
      return true;
    }

    // Make sure this is a cool character.
    if (!isAscii(c)) {
      Serial.print(F("readString: illegal non-ascii char: "));
      Serial.println((int) c);
      c = '_';
    }

    // Copy it over.
    output[i] = c;
  }

  // If we got here, we never hit the null byte.
  Serial.println(F("readString: blew out the read buffer"));
  return false;
}


SeaRobConfigRemote::SeaRobConfigRemote() {
  configReset();
}


void SeaRobConfigRemote::configReset() {
  strcpy(bluetoothName, "remote-newb");
  strcpy(bluetoothAddr, "");
  strcpy(bluetoothPass, "666");
}


/**
 *  CONFIG: RobotExport 
 */
boolean SeaRobConfigRemote::configExport() {
  // Start at the beginning of the EEPROM memory space.
  int memoryOffset = 0;
  boolean success = true;
  
  // Write out the header: the magic number & Version.
  success &= configWriteString(memoryOffset, "BTLRM");
  success &= configWriteString(memoryOffset, "1");

  // Bluetooth Name & passwd.
  success &= configWriteString(memoryOffset, bluetoothName);
  success &= configWriteString(memoryOffset, bluetoothAddr);
  success &= configWriteString(memoryOffset, bluetoothPass);
  
  // If we got here, we are successful.
  Serial.print(F("configRemoteExport: "));
  Serial.print(success ? F("good") : F("bad"));
  Serial.print(F(" export of "));
  Serial.print(memoryOffset);
  Serial.println(" bytes");
  return success;
}


/**
 *  CONFIG: RemoteImport
 */
boolean SeaRobConfigRemote::configImport() {

  // Reset it.
  configReset();

  int memoryOffset = 0;
  char readBuffer[READ_BUFFER_SIZE];
  
  // Read the magic number, and make sure it matches.
  if (!configReadString(memoryOffset, readBuffer, READ_BUFFER_SIZE) || strcmp(readBuffer, "BTLRM")) {
    Serial.println(F("configRemoteImport: could not read magic, sticking with default"));
    return false;
  }

  // Read the version number.
  if (!configReadString(memoryOffset, readBuffer, READ_BUFFER_SIZE)) {
    Serial.println(F("configRemoteImport: could not read version, sticking with default"));
    return false;
  } 
  if (strcmp(readBuffer, "1")) {
    Serial.print(F("configRemoteImport: could not understand config version of "));
    Serial.print(readBuffer);
    Serial.println(F(", sticking with default"));
    return false;
  }

  // Read the rest of the data.
  boolean success = 
    configReadString(memoryOffset, bluetoothName, BLUETOOTH_MAX_NAME_LEN) &&
    configReadString(memoryOffset, bluetoothAddr, BLUETOOTH_MAX_ADDR_LEN) &&
    configReadString(memoryOffset, bluetoothPass, BLUETOOTH_MAX_PASS_LEN);
  if (!success) {
    Serial.println(F("configRemoteImport: could not read BattleBotRemoteConfig"));
    configReset();
    return false;
  }

  // If we got here, we are successful.
  Serial.print(F("configRemoteImport: successful import of "));
  Serial.print(memoryOffset);
  Serial.println(" bytes");
  return true;
}


SeaRobConfigRobot::SeaRobConfigRobot() {
  configReset();
}


void SeaRobConfigRobot::configReset() {
  strcpy(bluetoothName, "battlebot-newb");
  strcpy(bluetoothPass, "666");
}


/**
 * CONFIG: RobotExport
 */
boolean SeaRobConfigRobot::configExport() {
  // Start at the beginning of the EEPROM memory space.
  int memoryOffset = 0;
  boolean success = true;
  
  // Write out the header: the magic number & Version.
  success &= configWriteString(memoryOffset, "BTLBT");
  success &= configWriteString(memoryOffset, "1");

  // Bluetooth Name & passwd.
  success &= configWriteString(memoryOffset, bluetoothName);
  success &= configWriteString(memoryOffset, bluetoothPass);
  
    // If we got here, we are successful.
  Serial.print(F("configRobotExport: "));
  Serial.print(success ? F("good") : F("bad"));
  Serial.print(F(" export of "));
  Serial.print(memoryOffset);
  Serial.println(" bytes");
  return success;
}

/**
 * CONFIG: RobotImport
 */
boolean SeaRobConfigRobot::configImport() {

  // Init the global variable tht holds the config, restting back to the defaults.
  configReset();

  int memoryOffset = 0;
  char readBuffer[READ_BUFFER_SIZE];
  
  // Read the magic number, and make sure it matches.
  if (!configReadString(memoryOffset, readBuffer, READ_BUFFER_SIZE) || strcmp(readBuffer, "BTLBT")) {
    Serial.println(F("configRobotImport: could not read magic, sticking with default"));
    return;
  }

  // Read the version number.
  if (!configReadString(memoryOffset, readBuffer, READ_BUFFER_SIZE)) {
    Serial.println(F("configRobotImport: could not read version, sticking with default"));
    return;
  } 
  if (strcmp(readBuffer, "1")) {
    Serial.print(F("configRobotImport: could not understand config version of "));
    Serial.print(readBuffer);
    Serial.println(F(", sticking with default"));
    return;
  }

  // Read the rest of the data.
  boolean success = 
    configReadString(memoryOffset, bluetoothName, BLUETOOTH_MAX_NAME_LEN) &&
    configReadString(memoryOffset, bluetoothPass, BLUETOOTH_MAX_PASS_LEN);
  if (!success) {
    Serial.println(F("configRobotImport: could not read BattleBotRobotConfig"));
    configReset();
    return;
  }

   // If we got here, we are successful.
  Serial.print(F("configRobotImport: successful import of "));
  Serial.print(memoryOffset);
  Serial.println(" bytes");
  return true;
}
