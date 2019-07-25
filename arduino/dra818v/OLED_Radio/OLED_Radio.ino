/**************************************************************************
 This is an example for our Monochrome OLEDs based on SSD1306 drivers

 Pick one up today in the adafruit shop!
 ------> http://www.adafruit.com/category/63_98

 This example is for a 128x32 pixel display using I2C to communicate
 3 pins are required to interface (two I2C and one reset).

 Adafruit invests time and resources providing this open
 source code, please support Adafruit and open-source
 hardware by purchasing products from Adafruit!

 Written by Limor Fried/Ladyada for Adafruit Industries,
 with contributions from the open source community.
 BSD license, check license.txt for more information
 All text above, and the splash screen below must be
 included in any redistribution.
 **************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES     10 // Number of snowflakes in the animation example
#define FREQ_DISPLAY_DIGITS 7
#define MHZ_DEVISOR   1000000
#define MODE_SET_FREQ_FLAG  0x01
#define MODE_SET_RX_FREQ_FLAG  0x02
#define MODE_SET_TX_FREQ_FLAG  0x04
#define MAX_MODE 0x08
#define EEPROM_ADDR_LAST_FREQ_POSITION  0x00


//Encoder
#define ENCODER_0_PINA  2
#define ENCODER_0_PINB  3
#define ENCODER_0_PUSH  4

long FREQ_STEP_MULTIPLIER =1;

long rxFreq = 144000000;
long txFreq = 144600000;
int currentMode = 3;
int freqSetPosition = 2;
volatile unsigned int encoder0Pos = 0;
volatile bool displayChanged  = false;

void setup() {

  FREQ_STEP_MULTIPLIER = powint(10, 9 - FREQ_DISPLAY_DIGITS);
  pinMode(ENCODER_0_PINA, INPUT);
  pinMode(ENCODER_0_PINB, INPUT);
  pinMode(ENCODER_0_PUSH, INPUT);

  // encoder pin on interrupt 0 (pin 2)
  attachInterrupt(0, doEncoderA, CHANGE);
  attachInterrupt(1, doEncoderB, CHANGE);
  attachInterrupt(2, doEncoderPush, CHANGE);
  
  Serial.begin(9600);
  EEPROM.get(EEPROM_ADDR_LAST_FREQ_POSITION, freqSetPosition);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  drawRxFreq();
}

void loop() {
  if(displayChanged) {
    drawRxFreq();
    displayChanged = false;
  }
}

void drawRxFreq(void) {
  display.clearDisplay();
  
  Serial.println(F("1"));
  bool editing = (currentMode && MODE_SET_FREQ_FLAG) > 0;
  drawFreq(20, 4, F("Rx"), rxFreq, editing);
//  display.println();

  drawFreq(20, 24, F("Tx"), txFreq, editing);

  display.drawLine(1, 50, 50, 50, WHITE);
  display.display(); 
}

void drawFreq(int x, int y, const __FlashStringHelper *txRx, long freq, bool editing) {
//  display.setTextSize(1);
//  display.setTextColor(WHITE);
//  display.setCursor(x, y);
//  display.print(txRx);
  display.drawChar(2, y, 'R', WHITE, BLACK, 1);
//  display.setTextSize(2); // Draw 2X-scale text

  printFreq(x, y, freq, editing, display);
}

void printFreq(int x, int y, long freq,  bool editing, Adafruit_SSD1306 disp) {

  long remainder = freq / FREQ_STEP_MULTIPLIER;
  for(int i= (FREQ_DISPLAY_DIGITS -1) ; i >= 0; i--) {
    long divisor = powint(10, i);
    int currentNum = remainder/divisor;
    remainder = remainder - (currentNum * divisor);
    if ( editing &  freqSetPosition == i) {
      display.drawChar(x+ ((FREQ_DISPLAY_DIGITS - i)*12), y, 48+currentNum, BLACK, WHITE, 2);
    } else {
      display.drawChar(x+ ((FREQ_DISPLAY_DIGITS - i)*12), y, 48+currentNum, WHITE, BLACK, 2);
    }
//    disp.print(currentNum);
//    disp.setTextColor(WHITE);
    if( i == 3) {
//      disp.print(F("."));
    } 
  }
}


long powint(int x, int y) {
 long val=x;
 for(int z=0;z<=y;z++) {
   if(z==0) {val=1;
   } else {
     val=val*x;
   }
 }
 return val;
}

void saveCurrentFreqEditPosition() {
  EEPROM.update(EEPROM_ADDR_LAST_FREQ_POSITION, freqSetPosition);
}

void doEncoderA() {
  // look for a low-to-high on channel A
  if (digitalRead(ENCODER_0_PINA) == HIGH) {

    // check channel B to see which way encoder is turning
    if (digitalRead(ENCODER_0_PINB) == LOW) {
      encoder0Pos = encoder0Pos + 1;         // CW
      rxFreq = rxFreq +10000;
      displayChanged = true; 
    }
    else {
      encoder0Pos = encoder0Pos - 1;         // CCW
       rxFreq = rxFreq -10000;
       displayChanged = true; 
    }
  }

  else   // must be a high-to-low edge on channel A
  {
    // check channel B to see which way encoder is turning
    if (digitalRead(ENCODER_0_PINB) == HIGH) {
      encoder0Pos = encoder0Pos + 1;          // CW
    }
    else {
      encoder0Pos = encoder0Pos - 1;          // CCW
    }
  }
  Serial.println (encoder0Pos, DEC);
  // use for debugging - remember to comment out
}

void doEncoderB() {
  // look for a low-to-high on channel B
  if (digitalRead(ENCODER_0_PINB) == HIGH) {

    // check channel A to see which way encoder is turning
    if (digitalRead(ENCODER_0_PINA) == HIGH) {
      encoder0Pos = encoder0Pos + 1;         // CW
    }
    else {
      encoder0Pos = encoder0Pos - 1;         // CCW
    }
  }

  // Look for a high-to-low on channel B

  else {
    // check channel B to see which way encoder is turning
    if (digitalRead(ENCODER_0_PINA) == LOW) {
      encoder0Pos = encoder0Pos + 1;          // CW
    }
    else {
      encoder0Pos = encoder0Pos - 1;          // CCW
    }
  }
}

void doEncoderPush() {
  if (digitalRead(ENCODER_0_PUSH) == HIGH) {

    // check channel A to see which way encoder is turning
    if (digitalRead(ENCODER_0_PUSH) == HIGH) {
      int nextMode = currentMode +1;
      if(nextMode > MAX_MODE) {
        nextMode = 0;
      }
      currentMode = nextMode ; 
      displayChanged = true;       
    }
  }
}
