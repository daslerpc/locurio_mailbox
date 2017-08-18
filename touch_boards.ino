#include <Wire.h>             // needed for I2C communication
#include "Adafruit_MPR121.h"  // 12-touch capacitive boards


// The 2 capacitive touch boards on the i2c line.
Adafruit_MPR121 touchBoard[] = { Adafruit_MPR121(), Adafruit_MPR121() };

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched[] = {0,0};
uint16_t currtouched[] = {0,0};

void setup_touchBoards() {
  Serial.println("Finding Adafruit MPR121 Capacitive Touch boards"); 
  
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!touchBoard[WOLFSBANE].begin(0x5A)) {
    Serial.println("WOLFSBANE touch board not found, check wiring and reset.");
    while (1);
  }

  if (!touchBoard[HOLLOW].begin(0x5B)) {
    Serial.println("HOLLOW touch board not found, check wiring and reset.");
    while (1);
  }
  
  Serial.println("Touch boards found!");
}

void processTouchBoard( signName checkWord ) {
   // Get the currently touched pads
  currtouched[checkWord] = touchBoard[checkWord].touched();
  
  for (uint8_t i=0; i<12; i++) {
    // if it *is* touched and *wasnt* touched before, alert!
    if ((currtouched[checkWord] & _BV(i)) && !(lasttouched[checkWord] & _BV(i)) ) {
      Serial.println( letterPinMapping[checkWord][i] + " touched." );
    }
    
    // if it *was* touched and now *isnt*, alert!
    if (!(currtouched[checkWord] & _BV(i)) && (lasttouched[checkWord] & _BV(i)) ) {
      Serial.println( letterPinMapping[checkWord][i] + " released." );
      processLetter ( checkWord*12 + i );
    }
  }

  // reset our state
  lasttouched[checkWord] = currtouched[checkWord];
}

