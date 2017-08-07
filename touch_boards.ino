
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

