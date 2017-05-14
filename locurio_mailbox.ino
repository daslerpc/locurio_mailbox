#include <ESP8266WiFi.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"

const char* ssid     = "Livebox-8B4C";
const char* password = "4AFDAF77557C5A5";

const char* host = "wifitest.adafruit.com";

// Letter order
// W1, O1, L1, F, S, B, A, N, E, H, O2, L2, L3, O3, W2
//  0,  1,  2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14

// Puzzle unlock order
// L3, A, W1, W2, H, F

// Current mapping (with a single 12-touch capacitive sensor) and 10-connector ribbon cable
// W1, O1, L1, F, S, B, A, N, E, H, O2, L2, L3, O3, W2
//  2          5        1        4           0       3  

const int lockCombination[] = {0, 1, 2, 3, 4};
const int resetCombination[] = {0, 1, 2};

String letterPinMapping[] = {"W in Wolfsbane",     // 0
                             "O in Wolfsbane",     // 1 
                             "L in Wolfsbane",     // 2
                             "F in Wolfsbane",     // 3
                             "S in Wolfsbane",     // 4
                             "B in Wolfsbane",     // 5
                             "A in Wolfsbane",     // 6
                             "N in Wolfsbane",     // 7
                             "E in Wolfsbane",     // 8
                             "H in Hollow",        // 9
                             "first O in Hollow",  // 10
                             "first L in Hollow",  // 11
                             "second L in Hollow", // 12
                             "second O in Hollow", // 13
                             "W in Hollow"         // 14
                             };

// tracking the progress through the combination
int currentComboIndex = 0;

// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 cap = Adafruit_MPR121();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

// Pin 14 is hardwired as the input for the relay featherwing
int relayPin = 14;

// Status of the mag-lock
const bool LOCKED = true;
const bool UNLOCKED = false;
bool isLocked = LOCKED;

void setup() {  
  Serial.begin(9600);
  
  pinMode(relayPin, OUTPUT);           // set relay pin to output

  while (!Serial) { // wait for serial to initialize
    delay(10);
  }

  printDebug("Finding Adafruit MPR121 Capacitive Touch sensor"); 
  
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    printDebug("MPR121 not found, check wiring and reset.");
    while (1);
  }
  
  printDebug("MPR121 found!");

  setMagnet( LOCKED );
}

void loop() {
  // Get the currently touched pads
  currtouched = cap.touched();
  
  for (uint8_t i=0; i<12; i++) {
    // if it *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) {
      printStatus( "The " + letterPinMapping[i] + " was touched." );
    }
    // if it *was* touched and now *isnt*, alert!
    if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) ) {
      printStatus( "The " + letterPinMapping[i] + " was realeased." );
      checkLetterPress( i );
    }
  }

  // reset our state
  lasttouched = currtouched;
  
  // put a delay so it isn't overwhelming
  delay(100);
}

void checkLetterPress ( int pressedLetterIndex ) {

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
  Serial.print( text + "\n" );
}

void printDebug( String text ) {
  Serial.print( text + "\n" );
}

