#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>  // For wifi connection

#include <DNSServer.h>        // Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h> // Local WebServer used to serve the configuration portal
#include <WiFiManager.h>      // https://github.com/tzapu/WiFiManager WiFi Configuration Manager

//default values, if there are different values stored in config.json, these are overwritten.
char mqtt_server[40] = "192.168.1.16";  // This is the local IP of phil's mac at home
char mqtt_port[6] = "1883";  // this is the default mqtt port

WiFiManager wifiManager;
WiFiClient espClient;

bool usingMQTT = true;

enum DebugChannel {
  VERBOSE,
  INFO,
  WARNING,
  ERROR,
  NUMBER_OF_DEBUG_CHANNELS
};
const char* debug_channels[NUMBER_OF_DEBUG_CHANNELS];

enum OutputChannel {
  DEVICE_STATUS,
  EVENT,
  NUMBER_OF_OUTPUT_CHANNELS
};
const char* output_channels[NUMBER_OF_OUTPUT_CHANNELS];

enum CommandChannel {
  BROADCAST,
  CONFIG,
  PRIVATE,
  NUMBER_OF_COMMAND_CHANNELS
};
const char* command_channels[NUMBER_OF_COMMAND_CHANNELS];

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
                                  "WOLFSBAN[E]",     // 3
                                  "WOLFSBA[N]E",     // 4
                                  "WOLFSB[A]NE",     // 5
                                  "WOLFS[B]ANE",     // 6
                                  "WOLF[S]BANE",     // 7
                                  "WOL[F]SBANE",     // 8
                                  "WO[L]FSBANE",     // 9
                                  "W[O]LFSBANE",     // 10
                                  "[W]OLFSBANE"     // 11
                                },
                                {
                                  "[H]OLLOW",  // 12
                                  "H[O]LLOW",  // 13
                                  "HO[L]LOW",  // 14
                                  "HOL[L]OW",  // 15
                                  "HOLL[O]W",  // 16
                                  "HOLLO[W]",   // 17
                                  "ERROR: Unused pin 6, 'Hollow board'", // 18
                                  "ERROR: Unused pin 7, 'Hollow board'", // 19  
                                  "ERROR: Unused pin 8, 'Hollow board'", // 20
                                  "ERROR: Unused pin 9, 'Hollow board'", // 21
                                  "ERROR: Unused pin 10, 'Hollow board'", // 22  
                                  "ERROR: Unused pin 11, 'Hollow board'" // 23
                                }
                              };

enum signName {WOLFSBANE, HOLLOW};




// tracking the progress through the combination
int currentComboIndex = 0;


const int redLEDPin = 0; // Pin 0 is a red LED
const int blueLEDPin = 2; // Pin 12 is a blue LED
const int resetPin = 12; // Pin 12 is a factory reset button for clearing wifi/mqtt data
const int relayPin = 14; // Pin 14 is hardwired as the input for the relay featherwing

// Status of the mag-lock
const bool LOCKED = true;
const bool UNLOCKED = false;
bool isLocked = LOCKED;

void setup_connectivity() {
  setup_fileSystem();

  digitalWrite(redLEDPin, HIGH);
  setup_wifi();
  digitalWrite(redLEDPin, LOW);
  
  digitalWrite(blueLEDPin, HIGH);
  setup_MQTT();
  digitalWrite(blueLEDPin, LOW);
  
  sendStartupAlert();
}

void processLetter ( int pressedLetterIndex ) {
  int target = -1;
  int start_letter = -1;
  int numDigits = -1;

  if( isLocked ) {
    target = lockCombination[ currentComboIndex ];
    start_letter = lockCombination[ 0 ];
    numDigits = sizeof(lockCombination)/sizeof(int);
  }
  else {
    target = resetCombination[ currentComboIndex ];
    start_letter = lockCombination[ 0 ];
    numDigits = sizeof(resetCombination)/sizeof(int);
  }
  
  if( pressedLetterIndex == target ) {
    printDebug( "Combination advanced!", VERBOSE );
    currentComboIndex += 1;
  }
  else if( pressedLetterIndex == start_letter ) {
    printDebug( "Start letter pressed mid-combo!  Combo reset to second letter.", WARNING );
    currentComboIndex = 1;
  }
  else {
    printDebug( "Incorrect entry!  Combo reset.", VERBOSE );
    currentComboIndex = 0;
  }
  
  if( currentComboIndex == numDigits ) {
    printDebug( "Combination completed, toggling magnet.", INFO);
    currentComboIndex = 0;
    toggleMagnet( );
  }
}



void setMagnet( bool status ) {
  isLocked = status;
  writeLockStateToMagnet();
}

void toggleMagnet( ) {
  isLocked = !isLocked;
  writeLockStateToMagnet();
}

void writeLockStateToMagnet( ) {
  if( isLocked ) {
    digitalWrite(relayPin, HIGH);
    printDebug( "Locking magnet", INFO );
  }
  else {
    digitalWrite(relayPin, LOW);
    printDebug( "Unlocking magnet", INFO );
  }
}




void factoryReset() {
  String message = "Performing factory reset of configuration parameters. Connect to Mailbox Wifi Configuration access point to reconfigure.";
  Serial.println(message);
  
  if( usingMQTT ) {
    printDebug(message, INFO);
  }
  
  wifiManager.resetSettings();
  SPIFFS.format();

  // For production we want a factory reset to restart the system
  //ESP.restart();
}

