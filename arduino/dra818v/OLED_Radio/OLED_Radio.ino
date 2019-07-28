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
#include <SoftwareSerial.h>
#include <DRA818.h>



#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)

#define FREQ_DISPLAY_DIGITS 7
#define MHZ_DEVISOR   1000000
#define EEPROM_ADDR_RX_FREQ_POSITION  0x00
#define EEPROM_ADDR_TX_FREQ_POSITION  0x04
#define EEPROM_ADDR_VOLUME_POSITION  0x10
#define EEPROM_ADDR_SQUELCH_POSITION  0x12
#define EEPROM_ADDR_RX_CTSS_POSITION  0x20
#define EEPROM_ADDR_TX_CTSS_POSITION  0x21
#define EEPROM_ADDR_LAST_FREQ_POSITION  0x30

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
#define SOFTWARE_SERIAL_TX   5
#define SOFTWARE_SERIAL_RX   6

//Radio DRA818 related constants
#define MAX_VOLUME  8
#define MAX_SQUELCH 8
#define TUNER_RX_F_CHANGED    1
#define TUNER_TX_F_CHANGED    2
#define TUNER_VOLUME_CHANGED  4
#define TUNER_SQUELCH_CHANGED 8

#define MHz   1000000;


//Macros
#define ITOA(n) 48+n

long FREQ_STEP_MULTIPLIER =1;

long rxFreq = 144000000;
long txFreq = 144600000;
int volume = 3;
int squelch = 1;
int currentMode = MODE_MAIN_SCREEN;
int currentSubMode = SUB_MODE_NULL;
int freqSetPosition = 2;
long frequencyStep = 100000;
volatile bool displayChanged  = false;
volatile int tunerChangedFlags = 0x00;

SimpleRotary rotary(ENCODER_0_PINA, ENCODER_0_PINB, ENCODER_0_PUSH);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
SoftwareSerial *dra_serial;
DRA818 *dra;

void setup() {

  FREQ_STEP_MULTIPLIER = powint(10, 9 - FREQ_DISPLAY_DIGITS);

  Serial.begin(9600);
  loadEepromData();
  bootstrapData();
    
  frequencyStep = calculateFrequencyStep();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  drawRxTxFreqScreen();
  setupDra818();
}

void loop() {
  readRoteryEncoder();
  if(displayChanged) {
    drawRxTxFreqScreen();
    displayChanged = false;
  }
  if(tunerChangedFlags != 0) {
    handleTunerChange();
    saveEepromData();
  }
}

/**
 * Sets up the DRA 818 module
 */
void setupDra818() {
  dra_serial = new SoftwareSerial(SOFTWARE_SERIAL_TX, SOFTWARE_SERIAL_RX);
 
  // all in one configuration
  float draFreqTx = txFreq / MHz;
  float draFreqRx = rxFreq / MHz;
  
//  // object-style configuration
  dra = new DRA818(dra_serial, DRA818_VHF);
//  dra->handshake();
  dra->group(DRA818_12K5, draFreqTx, draFreqRx, 0, squelch, 0);
  dra->volume(volume);
  dra->filters(true, true, true);
}

/**
 * Handles the vaue change for the RDA 818 Tuner
 */
void handleTunerChange() {
  if(tunerChangedFlags & TUNER_VOLUME_CHANGED != 0) {
    dra->volume(volume);
  }
  if(tunerChangedFlags & TUNER_SQUELCH_CHANGED != 0 ||
    tunerChangedFlags & TUNER_RX_F_CHANGED != 0 ||
    tunerChangedFlags & TUNER_TX_F_CHANGED != 0) {
    float draFreqTx = txFreq / MHz;
    float draFreqRx = rxFreq / MHz;
    dra->group(DRA818_12K5, draFreqTx, draFreqRx, 0, squelch, 0);
  }

  tunerChangedFlags = 0x00;
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
    int nextMode = currentMode +1;
    //Roll the mode menu to main screen.
    if(nextMode > MAX_MODE) {
      nextMode = MODE_MAIN_SCREEN;
    }
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
      {
        int newVolume = volume +1;
        if(newVolume > MAX_VOLUME) {newVolume = MAX_VOLUME;}
        volume = newVolume;
        displayChanged = true; 
        tunerChangedFlags = tunerChangedFlags | TUNER_VOLUME_CHANGED;
      }
      break;
    case MODE_SET_SQUELCH :
      {
        int newSqeuelch = squelch +1;
        if(newSqeuelch > MAX_SQUELCH) {newSqeuelch = MAX_SQUELCH;}
        squelch = newSqeuelch;
        tunerChangedFlags = tunerChangedFlags | TUNER_SQUELCH_CHANGED;
        displayChanged = true; 
      }
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
      {
        int newVolume = volume -1;
        if(newVolume < 0) {newVolume = 0;}
        volume = newVolume;
        tunerChangedFlags = tunerChangedFlags | TUNER_VOLUME_CHANGED;
        displayChanged = true; 
      }
      break;
    case MODE_SET_SQUELCH :
      {
        int newSquelch = squelch -1;
        if(newSquelch < 0) {newSquelch = 0;}
        squelch = newSquelch;
        tunerChangedFlags = tunerChangedFlags | TUNER_SQUELCH_CHANGED;
        displayChanged = true;
      }   
      break;
    case MODE_EDIT_RX_FREQ:
      {
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
      }
      break;  
    case MODE_EDIT_TX_FREQ:
      {
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
      }  
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
  int sqelchToScale = (squelch * 50) / 8;
  display.drawRect(x, y, 50, h, WHITE);
  display.fillRect(x, y, sqelchToScale, h, WHITE);
  if(currentMode == MODE_SET_SQUELCH) {
    display.fillRect(x + sqelchToScale, y-1, 2, h+3, WHITE);
  }
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

void loadEepromData() {
  EEPROM.get(EEPROM_ADDR_RX_FREQ_POSITION, rxFreq);
  EEPROM.get(EEPROM_ADDR_TX_FREQ_POSITION, txFreq);
  EEPROM.get(EEPROM_ADDR_VOLUME_POSITION, volume);
  EEPROM.get(EEPROM_ADDR_SQUELCH_POSITION, squelch);
  
  EEPROM.get(EEPROM_ADDR_LAST_FREQ_POSITION, freqSetPosition);
}

void saveEepromData() {
  EEPROM.put(EEPROM_ADDR_RX_FREQ_POSITION, rxFreq);
  EEPROM.put(EEPROM_ADDR_TX_FREQ_POSITION, txFreq);
  EEPROM.put(EEPROM_ADDR_VOLUME_POSITION, volume);
  EEPROM.put(EEPROM_ADDR_SQUELCH_POSITION, squelch);
    
  EEPROM.put(EEPROM_ADDR_LAST_FREQ_POSITION, freqSetPosition);
}

void bootstrapData() {
  if(rxFreq < 144000000 || rxFreq > 146000000) {rxFreq = 144000000; }
  if(txFreq < 144000000 || txFreq > 146000000) {txFreq = 144000000; }
  if(volume < 0 || volume > 7) {volume = 1; }
  if(squelch < 0 || squelch > 7) {squelch = 2; }
  if(freqSetPosition < 0 || freqSetPosition > FREQ_DISPLAY_DIGITS) {freqSetPosition = 2; }
}
