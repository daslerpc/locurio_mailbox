#include <PubSubClient.h> // MQTT client

PubSubClient mqttClient(espClient);


byte macAddr[6];

/************************/
/*  Published MQTT Channels  */
/************************/

// Debug channels are raw debug logging text (not JSON encoded)

void setupChannels() {
  debug_channels[VERBOSE] = ("debug/verbose/" + macAddress()).c_str();
  debug_channels[INFO] = ("debug/info/" + macAddress()).c_str();
  debug_channels[WARNING] = ("debug/warning/" + macAddress()).c_str();
  debug_channels[ERROR] = ("debug/error/" + macAddress()).c_str();

  output_channels[DEVICE_STATUS] = "device/status";
  output_channels[EVENT] = "locurio/output/woods/mailbox";

  command_channels[BROADCAST] = ""; // unused, meant for commands to all devices
  command_channels[CONFIG] = ("device/config/" + macAddress()).c_str();
  command_channels[PRIVATE] = "locurio/command/woods/mailbox";

  for( int i=0; i< NUMBER_OF_COMMAND_CHANNELS; i++)
    mqttClient.subscribe(command_channels[i]);
}

void setup_MQTT() {
  if ( mqtt_server == "" ) {
    Serial.println("MQTT Server not configured");
    usingMQTT = false;
  }

  if ( usingMQTT ) {
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

      // Subscribe to channels and publish an announcement...
      sendStartupAlert();
      setupChannels();

    } else {
      String errorMessage = "failed, rc=";
      errorMessage = errorMessage + mqttClient.state();
      errorMessage = errorMessage + " try again in 5 seconds";

      printDebug( errorMessage, ERROR );

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

