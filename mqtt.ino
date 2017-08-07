
void setup_MQTT() {
  if( mqtt_server == "" ) {
    Serial.println("MQTT Server not configured");
    usingMQTT = false;
  }

  if( usingMQTT ) {  
    mqttClient.setServer(mqtt_server, atoi(mqtt_port));
    mqttClient.setCallback(processMessage);
    connectMQTT();
  }
}

void connectMQTT() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-Mailbox";
    
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      
      // Once connected, publish an announcement...
      mqttClient.publish(debugTopic, "Mailbox connected to MQTT Broker");
      
      // ... and resubscribe
      mqttClient.subscribe(cmdTopic);
      
    } else {
      String errorMessage = "failed, rc=";
      errorMessage = errorMessage + mqttClient.state();
      errorMessage = errorMessage + " try again in 5 seconds";
      
      printDebug( errorMessage );
      
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

