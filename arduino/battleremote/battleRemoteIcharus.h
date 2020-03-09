#include <SoftwareSerial.h>
#include "battleRemoteStandard.h"


class battleRemoteIcharus : public battleRemoteStandard {

	public:

		battleRemoteIcharus(SoftwareSerial* bluetooth);
		~battleRemoteIcharus();
  
		String getName();
		void setup();
		void loop();
};
