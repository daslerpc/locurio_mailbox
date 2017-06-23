#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <Wire.h>             // needed for I2C communication
#include "Adafruit_MPR121.h"  // 12-touch capacitive boards

#include <ESP8266WiFi.h>  // For wifi connection
#include <PubSubClient.h> // MQTT client

#include <DNSServer.h>        // Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h> // Local WebServer used to serve the configuration portal
#include <WiFiManager.h>      // https://github.com/tzapu/WiFiManager WiFi Configuration Manager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson


// Use the WiFi credentials provided below?  If not, use Wifi Manager for configuration
//bool hardwiredWiFi = false;

//const char* ssid     = "Livebox-8B4C";
//const char* password = "4AFDAF77557C5A5";
//const char* mqtt_server = "192.168.1.11";  // This is the local IP of my Mac

//default values, if there are different values in config.json, they are overwritten.
char mqtt_server[40] = "192.168.1.11";  // This is the local IP of phil's mac at home
char mqtt_port[6] = "1883";  // this is the default mqtt port

//flag for saving data
bool shouldSaveConfig = false;

WiFiManager wifiManager;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

const char* statusTopic = "Mailbox/status";
const char* debugTopic = "Mailbox/debug";
const char* cmdTopic = "Mailbox/toggle_cmd";

// Current solution - LAWWHF
// W, O, L, F, S, B, A, N, E, H, O, L, L, O, W
// 2        5        1        4        0     3  

const int lockCombination[] = {15, 5, 11, 17, 12, 8};

// Relock code - ENABLE
// W, O, L, F, S, B, A, N,   E,  H, O, L, L, O, W
//       4        3  2  1   0/5                   

const int resetCombination[] = {3, 4, 5, 6, 9, 3};

String letterPinMapping[2][12] = {
                                {
                                  "ERROR: Unused pin 0, 'Wolfsbane' board", // 0
                                  "ERROR: Unused pin 1, 'Wolfsbane' board", // 1
                                  "ERROR: Unused pin 2, 'Wolfsbane' board", // 2
                                  "WOLFSBAN(E)",     // 3
                                  "WOLFSBA(N)E",     // 4
                                  "WOLFSB(A)NE",     // 5
                                  "WOLFS(B)ANE",     // 6
                                  "WOLF(S)BANE",     // 7
                                  "WOL(F)SBANE",     // 8
                                  "WO(L)FSBANE",     // 9
                                  "W(O)LFSBANE",     // 10
                                  "(W)OLFSBANE",     // 11
                                },
                                {
                                  "(H)OLLOW",  // 12
                                  "H(O)LLOW",  // 13
                                  "HO(L)LOW",  // 14
                                  "HOL(L)OW",  // 15
                                  "HOLL(O)W",  // 16
                                  "HOLLO(W)"   // 17
                                  "ERROR: Unused pin 6, 'Hollow board'", // 18
                                  "ERROR: Unused pin 7, 'Hollow board'", // 19  
                                  "ERROR: Unused pin 8, 'Hollow board'", // 20
                                  "ERROR: Unused pin 9, 'Hollow board'", // 21
                                  "ERROR: Unused pin 10, 'Hollow board'", // 22  
                                  "ERROR: Unused pin 11, 'Hollow board'", // 23
                                }
                              };


// The 2 capacitive touch boards on the i2c line.
Adafruit_MPR121 touchBoard[] = { Adafruit_MPR121(), Adafruit_MPR121() };
enum signName {WOLFSBANE, HOLLOW};

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched[] = {0,0};
uint16_t currtouched[] = {0,0};


// tracking the progress through the combination
int currentComboIndex = 0;

// Pin 14 is hardwired as the input for the relay featherwing
const int relayPin = 14;

// Status of the mag-lock
const bool LOCKED = true;
const bool UNLOCKED = false;
bool isLocked = LOCKED;

void setup() {  
  pinMode(relayPin, OUTPUT);           // set relay pin to output

  Serial.begin(9600);
  
  setup_fileSystem();
  setup_wifi();
  
  mqttClient.setServer(mqtt_server, atoi(mqtt_port));
  mqttClient.setCallback(processMessage);
  connectMQTT();
  
 // checkTouchBoards();
  setMagnet( LOCKED );
}

void loop() {
  /*
  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();
  
  processTouchBoard(WOLFSBANE);
  processTouchBoard(HOLLOW);
  
  // put a delay so it isn't overwhelming
  delay(100);
  */
}

void setup_fileSystem() {
  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
}

void setup_wifi() {
/*
  if (hardwiredWiFi) {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.print("Connecting to " + String(ssid));
  
    WiFi.begin(ssid, password);
  
    int cnt = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print( "." );
    }
  
    Serial.println();
    Serial.println("WiFi connected");
    Serial.println("IP address: " + String(WiFi.localIP()));
  } else {*/
    // Auto config for Wifi
      
    // The extra parameters to be configured (can be either global or just in the setup)
    // After connecting, parameter.getValue() will get you the configured value
    // id/name placeholder/prompt default length
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);

    //set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    //add all your parameters here
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);

    wifiManager.autoConnect("Mailbox Wifi Configuration");

    //read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());

    
    //save the custom parameters to FS
    if (shouldSaveConfig) {
      Serial.println("saving config");
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();
      json["mqtt_server"] = mqtt_server;
      json["mqtt_port"] = mqtt_port;
  
      File configFile = SPIFFS.open("/config.json", "w");
      if (!configFile) {
        Serial.println("failed to open config file for writing");
      }
  
      json.printTo(Serial);
      json.printTo(configFile);
      configFile.close();
      //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  //}
  
}

void checkTouchBoards() {
  printDebug("Finding Adafruit MPR121 Capacitive Touch boards"); 
  
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!touchBoard[WOLFSBANE].begin(0x5A)) {
    printDebug("Wolfsbane touch board not found, check wiring and reset.");
    while (1);
  }

  if (!touchBoard[HOLLOW].begin(0x5B)) {
    printDebug("Hollow touch board not found, check wiring and reset.");
    while (1);
  }
  
  printDebug("Touch boards found!");
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

void processTouchBoard( signName checkWord ) {
   // Get the currently touched pads
  currtouched[checkWord] = touchBoard[checkWord].touched();
  
  for (uint8_t i=0; i<12; i++) {
    // if it *is* touched and *wasnt* touched before, alert!
    if ((currtouched[checkWord] & _BV(i)) && !(lasttouched[checkWord] & _BV(i)) ) {
      printStatus( letterPinMapping[checkWord][i] + " touched." );
    }
    
    // if it *was* touched and now *isnt*, alert!
    if (!(currtouched[checkWord] & _BV(i)) && (lasttouched[checkWord] & _BV(i)) ) {
      printStatus( letterPinMapping[checkWord][i] + " released." );
      processLetter ( checkWord*12 + i );
    }
  }

  // reset our state
  lasttouched[checkWord] = currtouched[checkWord];
}

void processLetter ( int pressedLetterIndex ) {

  int target = -1;
  int numDigits = -1;

  if( isLocked ) {
    target = lockCombination[ currentComboIndex ];
    numDigits = sizeof(lockCombination)/sizeof(int);
  }
  else {
    target = resetCombination[ currentComboIndex ];
    numDigits = sizeof(resetCombination)/sizeof(int);
  }
  
  if( pressedLetterIndex == target ) {
    printStatus( "Combination advanced!" );
    currentComboIndex += 1;
  }
  else {
    printStatus( "Incorrect entry!  Combo reset." );
    currentComboIndex = 0;
  }
  
  if( currentComboIndex == numDigits ) {
    printStatus( "Combination completed, toggling magnet.");
    currentComboIndex = 0;
    toggleMagnet( );
  }
}

void processMessage(char* topic, byte* payload, unsigned int length) {
  String output = "";
  String message = "";
  
  output = output + "Message arrived [" + String(topic) + "] ";

  for (int i = 0; i < length; i++) {
    message = message + (char)payload[i];
  }

  output = output + message;
  
  printDebug(output);

  if (message == "lock_toggle")
    toggleMagnet();

}

void setMagnet( bool status ) {
  isLocked = status;
  writeToMagnet();
}

void toggleMagnet( ) {
  isLocked = !isLocked;
  writeToMagnet();
}

void writeToMagnet( ) {
  if( isLocked )
    digitalWrite(relayPin, HIGH);
  else
    digitalWrite(relayPin, LOW);
}

void printStatus( String text ) {
  char charBuf[50];
  text.toCharArray(charBuf, 50);
  mqttClient.publish(statusTopic, charBuf );  
}

void printDebug( String text ) {
  char charBuf[50];
  text.toCharArray(charBuf, 50);
  mqttClient.publish(debugTopic, charBuf );
}

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void factoryReset() {
  Serial.println("Resetting configuration to default.");
  printDebug("Resetting configuration to default.");
  printStatus("Resetting configuration to default.");
  
  wifiManager.resetSettings();
  SPIFFS.format();
}

