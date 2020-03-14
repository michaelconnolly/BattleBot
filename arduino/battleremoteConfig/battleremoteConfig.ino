// Import external libraries
#include <SeaRobConfiguration.h>

// Globals: the config
SeaRobConfigRemote remoteConfig;


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
  Serial.println(F("[Setting the BatteBot REMOTE configuration]\n"));

  // Reset the config.
  remoteConfig.configReset();
  
  // Prompt for our config data.
  promptForString(
      "Enter the new bluetooth Name for the Remote: ", 
      remoteConfig.bluetoothName, BLUETOOTH_MAX_NAME_LEN);
  promptForString(
      "Enter the bluetooth address (for example: 98d3,b1,fd60df): ", 
      remoteConfig.bluetoothAddr, BLUETOOTH_MAX_ADDR_LEN);
  promptForString(
      "Enter the password for the device at that address: ", 
      remoteConfig.bluetoothPass, BLUETOOTH_MAX_PASS_LEN);
  Serial.println();

  // Actually write out the config.
  if (!remoteConfig.configExport()) {
    Serial.println(F("Sorry, configuration failed."));
    return;
  }

  // Print successful result.
  Serial.print(F("\n\nBattleBot REMOTE Configuration successful for remote \""));
  Serial.print(remoteConfig.bluetoothName);
  Serial.println(F("\"!\nYou may proceed to running the BattleBot code now.\n"));
  Serial.println(F("------------------------------------------------"));
}


void loop() {
  // This application does nothing except for what it does in setup()..
}


/* 
 *  Reads one string from the serial input line.
 */
void promptForString(const char *prompt, char *readBuffer, int maxLen) {
  Serial.print(prompt);
  int numRead = Serial.readBytesUntil('\n', readBuffer, maxLen);
  readBuffer[numRead] = 0;
  Serial.print(readBuffer);
  Serial.println();
}
