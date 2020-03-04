#include <SoftwareSerial.h>
#include "battleRemoteStandard.h"


class battleRemoteCustomTemplate : public battleRemoteStandard {

	public:

		battleRemoteCustomTemplate(SoftwareSerial* bluetooth);
		~battleRemoteCustomTemplate();
  
		String getName();
		void setup();
		void loop();
};
