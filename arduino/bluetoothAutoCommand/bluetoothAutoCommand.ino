 
#include <SoftwareSerial.h>

/*
 * Constants
 */
#define PIN_BLUETOOTH_RECV 2
#define PIN_BLUETOOTH_SEND 3
#define PIN_BLUETOOTH_POWER 5
#define PIN_BLUETOOTH_ENABLE 4

#define BLUETOOTH_BAUD_RATE 38400

/*
 * Global Variables
 */
SoftwareSerial  bluetoothSerial(PIN_BLUETOOTH_RECV, PIN_BLUETOOTH_SEND); // RX | TX
const char      buildTimestamp[] =  __DATE__ " " __TIME__;

void setup() 
{
  Serial.begin(9600);
  Serial.print("Buildtime: ");   
  Serial.println(buildTimestamp);
  Serial.println("");
  
  // Init the 2 bluetooth control pins; POWER goes to its VIn, so 
  // flipping that high and low turns it on and off. The enable pin is used 
  // to put the HC-05 in control (at) mode; If enable is high when you turn
  // it on, it starts in command mode.
  pinMode(PIN_BLUETOOTH_POWER, OUTPUT);
  pinMode(PIN_BLUETOOTH_ENABLE, OUTPUT);
  digitalWrite(PIN_BLUETOOTH_POWER, LOW);
  digitalWrite(PIN_BLUETOOTH_ENABLE, LOW);
  
  //swithToBluetoothNormalMode();
  swithToBluetoothCommandMode();
  
  bluetoothSerial.begin(BLUETOOTH_BAUD_RATE);  
  Serial.print("Communications started at "); 
  Serial.println(BLUETOOTH_BAUD_RATE);
  Serial.println("");
}
 
void loop()
{
  // Read all responses from the Bluetooth module and send to the Arduino Serial Monitor
  if (bluetoothSerial.available())
  {
    char c = bluetoothSerial.read();
    Serial.write("received: ");
    Serial.write(c);
    Serial.write(" (");
    Serial.print(String(int(c)));
    Serial.write(")\r\n");
  }

  // Read from the Serial Monitor and send to the Bluetooth module
  if (Serial.available())
  {
    char c = Serial.read();
    int charcode = c;  
    Serial.write("serial-recv: ");
    Serial.println(String(charcode));
    
    bluetoothSerial.println("at");
    delay(100);
  }
}

/**
 * Cycle the power on the bluetooth module, starting it back up in normal mode.
 */
void swithToBluetoothNormalMode() {
  // power it down
  digitalWrite(PIN_BLUETOOTH_POWER, LOW);
  delay(100);

  // power it up with the enable pin off.
  digitalWrite(PIN_BLUETOOTH_ENABLE, LOW);
  delay(200);
  digitalWrite(PIN_BLUETOOTH_POWER, HIGH);
  delay(500);
}


/**
 * Cycle the power on the bluetooth module, starting it back up in command mode.
 */
void swithToBluetoothCommandMode() {
  // power it down
  digitalWrite(PIN_BLUETOOTH_POWER, LOW);
  delay(100);

  // power it up with the enable pin on.
  digitalWrite(PIN_BLUETOOTH_ENABLE, HIGH);
  delay(200);
  digitalWrite(PIN_BLUETOOTH_POWER, HIGH);
  delay(500);
}
