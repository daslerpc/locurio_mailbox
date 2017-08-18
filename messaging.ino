#include <PubSubClient.h> // MQTT client
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

extern PubSubClient mqttClient;

void sendStartupAlert() {
  DynamicJsonBuffer jsonBuffer;
  
  JsonObject& root = jsonBuffer.createObject();
  root["source"] = "mailbox";
  root["macAddress"] = macAddress();
  root["alive"] = true;

  String output; 
  root.printTo(output);
  mqttClient.publish(output_channels[DEVICE_STATUS], output.c_str() ); 
}

void sendEventReport(String message) {
  DynamicJsonBuffer jsonBuffer;
  
  JsonObject& root = jsonBuffer.createObject();
  root["source"] = macAddress();
  root["millis"] = millis();
  root["message"] = message;

  String output; 
  root.printTo(output);
  
  mqttClient.publish(output_channels[DEVICE_STATUS], output.c_str() ); 
}

void sendFullStatus() {
 DynamicJsonBuffer jsonBuffer;
  
  JsonObject& root = jsonBuffer.createObject();
  root["macAddress"] = macAddress();
  root["locked"] = isLocked;
  root["touch_sensitivity"] = "default";
  root["unlock_code"] = "default";
  root["lock_code"] = "default";
  root["next_letter"] = "not implemented";

  
  String output; 
  root.printTo(output);
  
  mqttClient.publish(debug_channels[INFO], output.c_str() ); 
}


void printDebug( String text, DebugChannel chan ) {
  if( usingMQTT ){
    mqttClient.publish(debug_channels[chan], text.c_str());  
  }
  else
    Serial.println(text);
}

void processMessage(char* topic, byte* payload, unsigned int length) {
  String output = "";
  String message = "";
  
  output = output + "Message arrived [" + String(topic) + "] ";

  for (int i = 0; i < length; i++) {
    message = message + (char)payload[i];
  }

  output = output + message;
  
  printDebug(output, INFO);

  if (message == "lock_toggle")
    toggleMagnet();
}


