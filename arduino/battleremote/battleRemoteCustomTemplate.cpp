#include "battleRemoteCustomTemplate.h"


// Constants and Globals: add here.


battleRemoteCustomTemplate::battleRemoteCustomTemplate(SoftwareSerial* bluetooth) : battleRemoteStandard(bluetooth) {

}


battleRemoteCustomTemplate::~battleRemoteCustomTemplate() {

}


String battleRemoteCustomTemplate::getName() {
	
	return "battle-remote custom";
}


void battleRemoteCustomTemplate::setup() {

  battleRemoteCustomTemplate::setup();

   Serial.print(F("Setup: "));
   Serial.println(this->getName());
	
	// Init: add here.
}
	

void battleRemoteCustomTemplate::loop() {
	
	battleRemoteStandard::loop();

	// Loop: add here.
}
