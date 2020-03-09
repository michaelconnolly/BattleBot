// Import external libraries
#include <SeaRobConfiguration.h>

// Globals: the config
SeaRobConfigRobot botConfig;

/**
 * Entrypoint: called once when the program first starts, just to initialize all the sub-components.
 */
void setup() {  
  
  // Init the serial line.
  Serial.begin(9600);
  Serial.setTimeout(-1);

  // Reset the config.
  botConfig.configReset();
  
  // Print application welcome text.
  Serial.println(F("\n\n"));
  Serial.println(F("------------------------------------------------"));
  Serial.println();
  Serial.println(F("[Setting the BatteBot ROBOT configuration]\n"));

  // Prompt for our config data.
  promptForString(
      "Enter the new bluetooth Name: ", 
      botConfig.bluetoothName, BLUETOOTH_MAX_NAME_LEN);
  promptForString(
      "Enter the new bluetooth Password: ", 
      botConfig.bluetoothPass, BLUETOOTH_MAX_PASS_LEN);
  Serial.println();

  // Actually write out the config.
  if (!botConfig.configExport()) {
    Serial.println(F("Sorry, configuration failed."));
    return;
  }

  // Print successful result.
  Serial.print(F("\n\nBattleBot ROBOT Configuration successful for robot \""));
  Serial.print(botConfig.bluetoothName);
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
