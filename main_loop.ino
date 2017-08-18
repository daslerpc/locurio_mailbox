#include <PubSubClient.h> // MQTT client
extern PubSubClient mqttClient;

void setup() {  
  pinMode(relayPin, OUTPUT);           // set relay pin to output

  Serial.begin(9600);

  setup_touchBoards();
  setMagnet( LOCKED );
}

void loop() {

  processTouchBoard(WOLFSBANE);
  processTouchBoard(HOLLOW);

  // put a delay so touch board output isn't overwhelming
  // Not 100% sure this is necessary
  delay(100);
  
}
