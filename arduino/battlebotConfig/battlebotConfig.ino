// Import external libraries
#include <EEPROM.h>

/**
 * Entrypoint: called once when the program first starts, just to initialize all the sub-components.
 */
void setup() {  
  
  // Init the serial line.
  Serial.begin(9600);
  Serial.setTimeout(-1);

  // Print application welcome text.
  Serial.println(F("\n\n"));
  Serial.println(F("------------------------------------------------"));
  Serial.println();
  Serial.println(F("[Setting the BatteBot configuration]\n"));

  // Prompt for bluetooth name.
  Serial.print(F("Enter the new bluetooth Name: "));
  String blueName = Serial.readStringUntil('\n');
  Serial.println(blueName);

  // Prompt for bluetooth password.
  Serial.print(F("Enter the new bluetooth Password: "));
  String bluePass = Serial.readStringUntil('\n');
  Serial.println(bluePass);
  Serial.println();

  // Actually write out the config.
  if (!configExport(blueName, bluePass)) {
    Serial.println(F("Sorry, configuration failed."));
    return;
  }

  // Print successful result.
  Serial.print(F("\n\nBattleBot Configuration successful for robot \""));
  Serial.print(blueName);
  Serial.println(F("\"!\nYou may proceed to running the BattleBot code now.\n"));
  Serial.println(F("------------------------------------------------"));
}


void loop() {
  // This application does nothing except for what it does in setup()..
}


/**
 *  CONFIG
 */


boolean configExport(String blueName, String bluePass) {

  int memoryOffset = 0;
  
  // Write the magic number, and make sure it matches.
  configWriteString(memoryOffset, "BTLBT");
  
  // Write the current Version of the config.
  configWriteString(memoryOffset, "1");

  // Bluetooth Name & passwd.
  configWriteString(memoryOffset, blueName.c_str());
  configWriteString(memoryOffset, bluePass.c_str());
  
  Serial.print(F("configExport: successful export of "));
  Serial.print(memoryOffset);
  Serial.println(" bytes");
  return true;
}


/*
 */
boolean configWriteString(int& memoryOffset, char *output) {

  int outputLen = strlen(output);
  if (memoryOffset + outputLen + 2 >= EEPROM.length()) {
     Serial.println(F("configWriteString: blew eeprom buffer"));
     return false;
  }

  // Loop through the string to be outputted.
  for (int i = 0 ; i < outputLen ; i++) {
    char c = output[i];
    
    // Make sure this is a cool character.
    if (!isAscii(c)) {
      Serial.print(F("configWriteString: illegal char: "));
      Serial.println((int) c);
      return false;
    }
    
    // Write out to eeprom, and increment the current memory offset.
    EEPROM[memoryOffset++] = c;
  }

  // Cap off with a nullbyte.
  EEPROM[memoryOffset++] = 0;

  // If we got here, we never hit the null byte.
  Serial.print(F("configWriteString: wrote: "));
  Serial.println(output);
  return true;
}
