// Constants: knobs and buttons.
#define PIN_BUTTON_YELLOW 2
#define PIN_BUTTON_BLUE   4
#define PIN_LED_YELLOW    12
#define PIN_LED_BLUE  11


void setup() {

  Serial.begin(9600);
  Serial.println("Starting setup.");
  
  // Init: knobs and buttons.
  pinMode(PIN_BUTTON_YELLOW, INPUT);
  pinMode(PIN_BUTTON_BLUE, INPUT);

  // LED's
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);

  Serial.println("Ending setup.");
}


void loop() {
 
  // Loop: buttons and knobs.
  processButton(PIN_BUTTON_YELLOW, PIN_LED_YELLOW);
  processButton(PIN_BUTTON_BLUE, PIN_LED_BLUE);
}


void processButton(int buttonId, int ledId) {

  int buttonState = digitalRead(buttonId);

  if (buttonState == HIGH) {
    
    Serial.print("buttonId: ");
    Serial.print(buttonId);
    Serial.print(", buttonState: ");
    Serial.print(buttonState);
    Serial.print(", ledId: ");
    Serial.println(ledId);

    digitalWrite(ledId, HIGH);
  }
  else {
    digitalWrite(ledId, LOW);
  }
}
