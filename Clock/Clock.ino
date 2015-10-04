/*
 * Simple 7-seg display counter program created with arduino IDE. I
 * plan to rewrite some or all of this in C and then hopefully assembly.
 *
 * Materials:
 * Arduino Uno
 *
 * 7 Segment Display:
 * NFD-3641AH-11
 * http://www.dipmicro.com/?datasheet=NFD-3641.pdf
 *
 * Shift Register:
 * SN74HC595N
 * https://www.sparkfun.com/datasheets/IC/SN74HC595.pdf
 *
 * Version 1.0
 *
 * NOTE: CATHODE DISPLAYS LIKE MINE ARE ON WHEN WRITTEN LOW, ANODES
 * ARE ON WHEN HIGH.
 */

// Shift Register pins
const int shiftSerClk = 2;  // Clock for each time data is written to ser
const int shiftRegClk = 3;  // Clock for each time serial data is finished
const int shiftSerData = 4; // Pin to write serial data to
// 7 Segment Display pins
const int disp1 = 11;       // Left-most dipslay, minute high digit.
const int disp2 = 10;       // Middle display, minute low and decimal point.
const int disp3 = 9;        // Middle dispkay, seconds high.
const int disp4 = 8;        // Right-most display, seconds low.

// Allows simple addressing of displays by variable.
const int dispArray[4] = {disp1, disp2, disp3, disp4};

// Used to convert form
const byte digits[10] = {
  B00111111,// 0
  B00000110,// 1
  B01011011,// 2
  B01001111,// 3
  B01100110,// 4
  B01101101,// 5
  B01111101,// 6
  B00000111,// 7
  B01111111,// 8
  B01101111,// 9
};

const int secInMin  = 60; // Seconds convert to a minute.
const int minOVF = 100;   // Minutes overflow to 0.
const int millisInSecond = 1000; // I really hate magic numbers...

// Milliseconds to wait between refreshing display. This may be a
// risk of seizure if the delay is from 10 to 100 milliseconds.
const int refreshDelay = 0;
// Millis to turn of each segment for. Increase this if the display
// is to bright, set to 0 if you can't see it.
const int dimmerDelay = 0;

const byte dispError[4] = {B01111001, B01010000, B0000000, B00000000};

// Store the values from the millis() function for accurate timing.
const long startTime = millis();
long lastRefresh;
long lastSecond;

// Current time
int minutes;
int seconds;

// The byte to output for each display.
byte dispBytes[4];

// The currently on digit for refreshing the display.
int curDigit;

void setup() {
  // Set output pins
  pinMode(shiftSerClk, OUTPUT); // Shift Register
  pinMode(shiftSerData, OUTPUT);
  pinMode(shiftRegClk, OUTPUT);
  pinMode(disp1, OUTPUT); // Display
  pinMode(disp2, OUTPUT);
  pinMode(disp3, OUTPUT);
  pinMode(disp4, OUTPUT);

  // Initialize minutes and seconds.
  seconds = 0;
  minutes = 0;

  // Intialize the byte array to output all zeros.
  for (int i = 0; i < 4; i++)
    dispBytes[i] = digits[0];
  // Set decimal point on second digit.
  bitSet(dispBytes[1], 7);

  lastSecond = startTime;
  lastRefresh = startTime;
}

void loop() {
  // Test if the display is ready to be refreshed.
  if (millis() >= lastRefresh + refreshDelay) {
    lastRefresh = millis();
    refreshDisplay();
  }
}

// Write the next segment of the display occasionally.
void refreshDisplay() {
  // Initialize shift bits for writing
  digitalWrite(shiftRegClk, LOW);

  // Clear Current digit and decrement digit counter.
  digitalWrite(dispArray[curDigit], HIGH);
  delay(dimmerDelay);
  curDigit--;

  // There are only 4 digits in the display.
  if (curDigit < 0)
    curDigit = 3;

  // Write data to shift register
  shiftOut(shiftSerData, shiftSerClk,
           MSBFIRST, dispBytes[curDigit]);

  // Refresh registers and turn display on.
  digitalWrite(shiftRegClk, HIGH);
  digitalWrite(dispArray[curDigit], LOW);

  // Test if a second has passed.
  if (millis() >= lastSecond + millisInSecond) {
    lastSecond += millisInSecond;
    nextSecond();
  }
}

// Keep track of the current time every second.
void nextSecond() {
  // After 1 second a second has gone by.
  seconds++;

  // Increment minutes every 60 seconds.
  if (seconds >= secInMin) {
    seconds = 0;
    minutes++;
  }

  // If the minutes get to 100 the program will crash.
  if (minutes >= minOVF) {
    minutes = 0;
  }

  // Store the new values.
  convTime();
}

// Converts minutes and seconds shift register format. Crashes if the time is not valid.
void convTime() {
  // Seperate minutes and seconds into 4 digits for display. Div 10 and
  // mod 10 make the high digit the tens digit and the low digit the ones.
  int minSecDigits[4] = {minutes / 10, minutes % 10, seconds / 10, seconds % 10};
  
  // Convert each digit to its 7 segment value. DispBytes is the data to be sent to the 
  // shift register, digits is the array with the data for each digit at that digit's
  // location in the array, and minSecDigits is the array we just made that converted the 
  // seconds and minutes into individual digits.
  for (int i = 0; i < 4; i++)
    dispBytes[i] = digits[minSecDigits[i]];

  // Set DP for second digit.
  bitSet(dispBytes[1], 7);
}

// If a problem is detected loop until device loses power.
void writeError() {
  // Store the error values
  for (int i = 0; i < 4; i++)
    dispBytes[i] = dispError[i];
  // Reset the current digit.
  curDigit = 0;

  // Turn of the 3rd and 4th digits.
  digitalWrite(disp3, HIGH);
  digitalWrite(disp4, HIGH);

  // Prevent any other code from executing on an error.
  noInterrupts();
  // This code is very similar to the other output code.
  while (true) {
    // Initialize shift bits for writing.
    digitalWrite(shiftRegClk, LOW);

    // Clear Current digit and increment digit counter.
    digitalWrite(dispArray[curDigit], HIGH);
    delay(dimmerDelay);
    curDigit++;
    // There are only 2 digits to display.
    if (curDigit > 1)
      curDigit = 0;

    // Write data to shift register
    shiftOut(shiftSerData, shiftSerClk,
             MSBFIRST, dispBytes[curDigit]);

    // Refresh registers and turn display on.
    digitalWrite(shiftRegClk, HIGH);
    digitalWrite(dispArray[curDigit], LOW);
  }
}

