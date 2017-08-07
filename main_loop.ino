
void setup() {  
  pinMode(relayPin, OUTPUT);           // set relay pin to output
  pinMode(resetPin, INPUT_PULLUP);     // set reset pin to input, using the internal pullup resistor
  
  Serial.begin(9600);

  // THIS IS FOR TESTING ONLY
  // Wipe communications credentials
  //factoryReset(); 
  
  setup_connectivity();
  
  setup_touchBoards();
  setMagnet( LOCKED );
}

void loop() {
  
  if( usingMQTT ) {
    if (!mqttClient.connected()) {
      connectMQTT();
    }
    mqttClient.loop();
  }
  
  processTouchBoard(WOLFSBANE);
  processTouchBoard(HOLLOW);
  
  // put a delay so touch board output isn't overwhelming
  // Not 100% sure this is necessary
  delay(100);
  
}
