#include "base_remote.h"


SoftwareSerial* _bluetooth;


base_remote::base_remote(SoftwareSerial* bluetooth) {

	_bluetooth = bluetooth;

	Serial.println(F("Inside base_remote::base_remote"));
}


SoftwareSerial* base_remote::getBluetooth() {
	
	return _bluetooth;
}


base_remote::~base_remote() {

	Serial.println(F("Inside base_remote::~base_remote"));
}


void base_remote::setup() {

	Serial.println(F("Inside base_remote::setup"));	
}

	
void base_remote::loop() {

	//Serial.println(F("Inside base_remote::loop"));
}


String base_remote::getName() {
	
	return "standard battle-remote";
}
