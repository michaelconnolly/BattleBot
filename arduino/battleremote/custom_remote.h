#include <SoftwareSerial.h>
#include "base_remote.h"


class custom_remote : public base_remote {

	public:

		custom_remote(SoftwareSerial* bluetooth);
		~custom_remote();
  
		String getName();
		void setup();
		void loop();
		int test(int val);

	private:

		void processButton(int buttonId, char commandToSend);
		void processKnob(int knobId, int &knobValueOld);
};
