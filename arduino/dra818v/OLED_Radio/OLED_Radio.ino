/**************************************************************************
Copyright 2019 Ruwan Abeykoon

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
 **************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <SimpleRotary.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)

#define FREQ_DISPLAY_DIGITS 7
#define MHZ_DEVISOR   1000000
#define EEPROM_ADDR_LAST_FREQ_POSITION  0x00

// Modes, which changes the screen layout and what displayes in the screen
#define MODE_MAIN_SCREEN  0
#define MODE_EDIT_RX_FREQ 1
#define MODE_EDIT_TX_FREQ 2
#define MODE_SET_VOLUME   3
#define MODE_SET_SQUELCH  4
#define MAX_MODE          4
#define SUB_MODE_NULL                 0
#define SUB_MODE_SELECT_FREQ_NUMERAL  1
#define SUB_MODE_FREQ_NUMERAL_UP_DOWN 2


//Encoder Pins
#define ENCODER_0_PINA  2
#define ENCODER_0_PINB  3
#define ENCODER_0_PUSH  4

//Radio DRA818 related constants
#define MAX_VOLUME  8


//Macros
#define ITOA(n) 48+n

long FREQ_STEP_MULTIPLIER =1;

long rxFreq = 144000000;
long txFreq = 144600000;
int volume = 3;
byte squelch = 5;
int currentMode = MODE_MAIN_SCREEN;
int currentSubMode = SUB_MODE_NULL;
int freqSetPosition = 2;
int frequencyStep = 100000;
volatile bool displayChanged  = false;

SimpleRotary rotary(ENCODER_0_PINA, ENCODER_0_PINB, ENCODER_0_PUSH);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {

  FREQ_STEP_MULTIPLIER = powint(10, 9 - FREQ_DISPLAY_DIGITS);

  Serial.begin(9600);
  EEPROM.get(EEPROM_ADDR_LAST_FREQ_POSITION, freqSetPosition);
  if(freqSetPosition < 0 || freqSetPosition >FREQ_DISPLAY_DIGITS) {
    freqSetPosition = 2;
    saveCurrentFreqEditPosition();
  }

  frequencyStep = calculateFrequencyStep();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  drawRxTxFreqScreen();
}

void loop() {
  readRoteryEncoder();
  if(displayChanged) {
    drawRxTxFreqScreen();
    displayChanged = false;
  }
}

void readRoteryEncoder() {
  byte i;

  // 0 = not turning, 1 = CW, 2 = CCW
  i = rotary.rotate();

  if ( i == 1 ) {
    handleRoteryClockwise();
  }

  if ( i == 2 ) {
    handleRoteryCounterClockwise();
  }

  
  // 0 = not pushed, 1 = pushed, 2 = long pushed (after 1000 ms)
  i = rotary.pushType(1000);
  if ( i == 1 ) {
    int nextMode = (currentMode +1) % MAX_MODE;
    currentMode = nextMode ; 
    displayChanged = true;  
  } else if (i == 2) {
    handleRoteryLongPush();
  }
}


/**
 * Handles the rotery encoder clockwise move.
 */
void handleRoteryClockwise() {
  switch (currentMode) {
    case MODE_SET_VOLUME :
      volume ++;
      if(volume > MAX_VOLUME) {volume = MAX_VOLUME;}
      displayChanged = true; 
      break;
    case MODE_EDIT_RX_FREQ:
      rxFreq = rxFreq +10000;
      displayChanged = true; 
      break;  
    case MODE_EDIT_TX_FREQ:
      txFreq = txFreq +10000;
      displayChanged = true; 
      break;  
  }
}

/**
 * Handles the rotery encoder counter-clockwise move.
 */
void handleRoteryCounterClockwise() {
    switch (currentMode) {
    case MODE_SET_VOLUME :
      volume --;
      if(volume < 0) {volume = 0;}
      displayChanged = true; 
      break;
    case MODE_EDIT_RX_FREQ:
      if(currentSubMode == SUB_MODE_SELECT_FREQ_NUMERAL) {
        int newPosition = freqSetPosition --;
        if(newPosition <= 0) {
          newPosition = 0;
        }
        currentSubMode = newPosition;
        frequencyStep = calculateFrequencyStep();
      } else {
        rxFreq = rxFreq - frequencyStep;      
      }
      displayChanged = true; 
      break;  
    case MODE_EDIT_TX_FREQ:
      if(currentSubMode == SUB_MODE_SELECT_FREQ_NUMERAL) {
        int newPosition = freqSetPosition --;
        if(newPosition <= 0) {
          newPosition = 0;
        }
        currentSubMode = newPosition;
        frequencyStep = calculateFrequencyStep();
      } else {
        txFreq = txFreq - frequencyStep;
      }
      displayChanged = true; 
      break;  
  }
}

/**
 * Handles the long push of the rotery encoder.
 * Enter the menu.
 * It will cause numeral values to be selected in the frequency type if rx or tx is selected.
 */
void handleRoteryLongPush() {
   switch (currentMode) {
    case MODE_EDIT_RX_FREQ:
    case MODE_EDIT_TX_FREQ:
      if(currentSubMode = SUB_MODE_SELECT_FREQ_NUMERAL) {
        currentSubMode = SUB_MODE_FREQ_NUMERAL_UP_DOWN;
      } else {
        currentSubMode =  SUB_MODE_SELECT_FREQ_NUMERAL;
      }
      break;  
  }
}

/**
 * Draws the main screen, which contains two frequency lines
 * First line is Rx frequency
 * Second line is Tx Frequency
 * 
 */
void drawRxTxFreqScreen(void) {
  display.clearDisplay();
  bool editing = currentMode == MODE_EDIT_RX_FREQ;
  drawFreqLine(20, 4, F("Rx"), rxFreq, editing);

  editing = currentMode == MODE_EDIT_TX_FREQ;
  drawFreqLine(20, 24, F("Tx"), txFreq, editing);
  drawSignalStrength(1, 44, 5);
  drawVolume(1, 58, 5);
  drawSquelch(70, 58, 5);

  display.display(); 
}

/**
 * Draw the line with frequency.
 * Can be used to display Tx and Rx frequencies.
 * 
 * x X coordinate to draw the line
 * y Y coordinate to draw the line
 * txRx String to display before the requency eithe Tx, or Rx
 * freq The frequency in KHz
 * editing A flag indicating that we are changing the frequency
 */
void drawFreqLine(int x, int y, const __FlashStringHelper *txRx, long freq, bool editing) {

  display.drawChar(2, y, 'R', WHITE, BLACK, 1);
  if(editing) {
    display.drawChar(8, y, '_', WHITE, BLACK, 1); 
  }
  drawFreq(x, y, freq, editing);
}

void drawFreq(int x, int y, long freq,  bool editing) {

  long remainder = freq / FREQ_STEP_MULTIPLIER;
  for(int i= (FREQ_DISPLAY_DIGITS -1) ; i >= 0; i--) {
    long divisor = powint(10, i);
    int currentNum = remainder/divisor;
    remainder = remainder - (currentNum * divisor);
    if ( editing &&  freqSetPosition == i) {
      display.drawChar(x+ ((FREQ_DISPLAY_DIGITS - i)*12), y, ITOA(currentNum), BLACK, WHITE, 2);
    } else {
      display.drawChar(x+ ((FREQ_DISPLAY_DIGITS - i)*12), y, ITOA(currentNum), WHITE, BLACK, 2);
    } 
  }
}

/**
 * Draw a bar with volume indicator
 * x, y are screen coordinates
 * h is the height of the volume indicator bar
 */
void drawVolume(int x, int y, int h) {
  int volumeToScale = (volume * 50) / 8;
  display.drawRect(x, y, 50, h, WHITE);
  display.fillRect(x, y, volumeToScale, h, WHITE);
  if(currentMode == MODE_SET_VOLUME) {
    display.fillRect(x + volumeToScale, y-1, 2, h+3, WHITE);
  }
}

/**
 * Draw a bar with Squelch value set
 * x, y are screen coordinates
 * h is the height of the volume indicator bar
 */
void drawSquelch(int x, int y, int h) {
  display.drawRect(x, y, 50, h, WHITE);
  display.fillRect(x, y, 25, h, WHITE);
}


/**
 * Draw a bar with S meter
 * x, y are screen coordinates
 * h is the height of the volume indicator bar
 */
void drawSignalStrength(int x, int y, int h) {
  display.drawRect(x, y, 127, h, WHITE);
  display.fillRect(x, y, 10, h, WHITE);
}

/**
 * Retruns the value x to the power y
 */
long powint(int x, int y) {
 long val = x;
 for(int z=0; z<=y; z++) {
   if(z == 0) {
      val=1;
   } else {
      val=val*x;
   }
 }
 return val;
}

long calculateFrequencyStep() {
  return powint(10, freqSetPosition) *100;
}

void saveCurrentFreqEditPosition() {
  EEPROM.update(EEPROM_ADDR_LAST_FREQ_POSITION, freqSetPosition);
}
