#include <SoftwareSerial.h>
#include <Wire.h>
#include <SPI.h>


class base_remote {

	public:

		base_remote(SoftwareSerial* bluetooth);
		~base_remote();
	
		String getName();
		void setup();
		void loop();

	protected:

		SoftwareSerial* getBluetooth();
};
