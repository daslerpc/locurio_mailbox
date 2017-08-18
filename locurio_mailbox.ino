
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

const int relayPin = 14; // Pin 14 is hardwired as the input for the relay featherwing

// Status of the mag-lock
const bool LOCKED = true;
const bool UNLOCKED = false;
bool isLocked = LOCKED;

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
    start_letter = resetCombination[ 0 ];
    numDigits = sizeof(resetCombination)/sizeof(int);
  }
  
  if( pressedLetterIndex == target ) {
    Serial.println( "Combination advanced!" );
    currentComboIndex += 1;
  }
  else if( pressedLetterIndex == start_letter ) {
    Serial.println( "Start-letter pressed mid-combo!" );
    currentComboIndex = 1;
  }
  else {
    Serial.println( "Incorrect entry!  Combo reset." );
    currentComboIndex = 0;
  }

  Serial.println();
  
  if( currentComboIndex == numDigits ) {
    Serial.println( "Combination completed, toggling magnet.");
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
    Serial.println( "Locking magnet\n" );
  }
  else {
    digitalWrite(relayPin, LOW);
    Serial.println( "Unlocking magnet\n" );
  }
}




