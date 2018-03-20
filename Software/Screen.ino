
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9340.h"

#define BLACK 0x0000 //0x tells Arduino that this a hexadecimal value 
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0
#define WHITE    0xFFFF

// These are the pins used for the UNO
// for Due/Mega/Leonardo use the hardware SPI pins (which are different)
#define _sclk 13
#define _miso 12
#define _mosi 11
#define _cs 10
#define _dc 8
#define _rst 9

// Using software SPI is really not suggested, its incredibly slow
//Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _mosi, _sclk, _rst, _miso);
// Use hardware SPI
Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _rst);

int screenHeight = 240;
int screenWidth = 320;

int freq[] = {
  131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247,
  262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494,
  523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988
};

String notes[] = {
  "C3", "CS3", "D3", "DS3", "E3", "F3", "FS3", "G3", "GS3", "A3", "AS3", "B3",
  "C4", "CS4", "D4", "DS4", "E4", "F4", "FS4", "G4", "GS4", "A4", "AS4", "B4",
  "C5", "CS5", "D5", "DS5", "E5", "F5", "FS5", "G5", "GS5", "A5", "AS5", "B5",
};

//clipping indicator variables
boolean clipping = 0;

//data storage variables
byte newData = 0;
byte prevData = 0;
unsigned int time = 0;//keeps time and sends vales to store in timer[] occasionally
int timer[10];//sstorage for timing of events
int slope[10];//storage for slope of events
unsigned int totalTimer;//used to calculate period
unsigned int period;//storage for period of wave
byte index = 0;//current storage index
float frequency;//storage for frequency calculations
int maxSlope = 0;//used to calculate max slope as trigger point
int newSlope;//storage for incoming slope data

//variables for decided whether you have a match
byte noMatch = 0;//counts how many non-matches you've received to reset variables if it's been too long
byte slopeTol = 3;//slope tolerance- adjust this if you need
int timerTol = 10;//timer tolerance- adjust this if you need

//variables for amp detection
unsigned int ampTimer = 0;
byte maxAmp = 0;
byte checkMaxAmp;
byte ampThreshold = 30;//raise if you have a very noisy signal

void setup() {

  tft.begin();
  tft.fillScreen(BLACK);
  tft.setRotation(1);

  Serial.begin(9600);

  pinMode(13, OUTPUT); //led indicator pin
  pinMode(12, OUTPUT); //output pin

  cli();//diable interrupts

  //set up continuous sampling of analog pin 0 at 38.5kHz

  //clear ADCSRA and ADCSRB registers
  ADCSRA = 0;
  ADCSRB = 0;

  ADMUX |= (1 << REFS0); //set reference voltage
  ADMUX |= (1 << ADLAR); //left align the ADC value- so we can read highest 8 bits from ADCH register only

  ADCSRA |= (1 << ADPS2) | (1 << ADPS0); //set ADC clock with 32 prescaler- 16mHz/32=500kHz
  ADCSRA |= (1 << ADATE); //enabble auto trigger
  ADCSRA |= (1 << ADIE); //enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN); //enable ADC
  ADCSRA |= (1 << ADSC); //start ADC measurements

  sei();//enable interrupts
}

ISR(ADC_vect) {//when new ADC value ready

  PORTB &= B11101111;//set pin 12 low
  prevData = newData;//store previous value
  newData = ADCH * 2;//get value from A0
  //Serial.print(prevData);
  //Serial.print(", " );
  //Serial.println(newData);
  if (prevData < 127 && newData >= 127) { //if increasing and crossing midpoint

    newSlope = newData - prevData;//calculate slope
    if (abs(newSlope - maxSlope) < slopeTol) { //if slopes are ==
      //record new data and reset time
      slope[index] = newSlope;
      timer[index] = time;
      time = 0;
      if (index == 0) { //new max slope just reset
        PORTB |= B00010000;//set pin 12 high
        noMatch = 0;
        index++;//increment index
      }
      else if (abs(timer[0] - timer[index]) < timerTol && abs(slope[0] - newSlope) < slopeTol) { //if timer duration and slopes match
        //sum timer values
        totalTimer = 0;
        for (byte i = 0; i < index; i++) {
          totalTimer += timer[i];
        }
        period = totalTimer;//set period
        //reset new zero index values to compare with
        timer[0] = timer[index];
        slope[0] = slope[index];
        index = 1;//set index to 1
        PORTB |= B00010000;//set pin 12 high
        noMatch = 0;
      }
      else { //crossing midpoint but not match
        index++;//increment index
        if (index > 9) {
          reset();
        }
      }
    }
    else if (newSlope > maxSlope) { //if new slope is much larger than max slope
      maxSlope = newSlope;
      time = 0;//reset clock
      noMatch = 0;
      index = 0;//reset index
    }
    else { //slope not steep enough
      noMatch++;//increment no match counter
      if (noMatch > 9) {
        reset();
      }
    }
  }

  if (newData == 0 || newData == 1023) { //if clipping
    PORTB |= B00100000;//set pin 13 high- turn on clipping indicator led
    clipping = 1;//currently clipping
  }

  time++;//increment timer at rate of 38.5kHz

  ampTimer++;//increment amplitude timer
  if (abs(127 - ADCH) > maxAmp) {
    maxAmp = abs(127 - ADCH);
  }
  if (ampTimer == 1000) {
    ampTimer = 0;
    checkMaxAmp = maxAmp;
    maxAmp = 0;
  }

}

void reset() { //clea out some variables
  index = 0;//reset index
  noMatch = 0;//reset match couner
  maxSlope = 0;//reset slope
}


void checkClipping() { //manage clipping indicator LED
  if (clipping) { //if currently clipping
    PORTB &= B11011111;//turn off clipping indicator led
    clipping = 0;
  }
}


void loop() {
  /*
    checkClipping();

    if (checkMaxAmp > ampThreshold) {
      frequency = 38462 / float(period); //calculate frequency timer rate/period

      //print results
      Serial.print(frequency);
      Serial.print(" hz");

      whichNote(frequency);
    }

    delay(100);//delete this if you want

    //do other stuff here
  */


  tft.setCursor(70, 0);
  tft.setTextColor(GREEN, BLACK);
  tft.setTextSize(16);
  tft.print("A4");
  
  //tft.drawRect(95, 184, 15, 22, WHITE);   //flat 3
  tft.fillRect(95, 184, 15, 22, RED);   

  //tft.drawRect(115, 176, 15, 38, WHITE);   //flat 2
  tft.fillRect(115, 176, 15, 38, RED);
  
  //tft.drawRect(135, 168, 15, 54, WHITE);   //flat 1
  tft.fillRect(135, 168, 15, 54, RED);

  //tft.drawRect(155, 160, 15, 70, WHITE);   //in tune
  tft.fillRect(155, 160, 15, 70, GREEN);

  //tft.drawRect(175, 168, 15, 54, WHITE);   //sharp 1
  tft.fillRect(175, 168, 15, 54, YELLOW);

  //tft.drawRect(195, 176, 15, 38, WHITE);   //sharp 2
  tft.fillRect(195, 176, 15, 38, YELLOW);

 // tft.drawRect(215, 184, 15, 22, WHITE);   //sharp 3
  tft.fillRect(215, 184, 15, 22, YELLOW);




}

void whichNote (float inNote) {
  int d = 999;
  int v = 999;
  for (int i = 0; i < 36; i++) {
    int n = freq[i];
    int newD = abs((int)inNote - n);
    if (newD < d) {
      d = newD;
      v = i;
    }
  }
  Serial.print(" Note:");
  Serial.print(notes[v]);
  Serial.print(" Dif.:");
  Serial.println(d);
}



