#include <SoftwareSerial.h>
#include <SPI.h>


class battleRemoteStandard {

	public:

		battleRemoteStandard(SoftwareSerial* bluetooth);
		~battleRemoteStandard();
	
		String getName();
    String getJoystickInfo();
		void setup();
		void loop();

	protected:

		SoftwareSerial* getBluetooth();
		void processButton(int buttonId, char commandToSend);
		void processKnob(int knobId, int &knobValueOld);
    void processJoystick(int pinX, int pinY);
};
