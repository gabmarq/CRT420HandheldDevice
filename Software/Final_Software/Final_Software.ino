#include <SoftwareSerial.h>

#define ARDUINO_RX 6//should connect to TX of the Serial MP3 Player module
#define ARDUINO_TX 7//connect to RX of the module
SoftwareSerial mySerial(ARDUINO_RX, ARDUINO_TX);//init the serial protocol, tell to myserial wich pins are TX and RX

static int8_t Send_buf[8] = {0} ;//The MP3 player undestands orders in a 8 int string
//0X7E FF 06 command 00 00 00 EF;(if command =01 next song order)

#define CMD_PLAY_W_INDEX 0X03 //DATA IS REQUIRED (number of song)
#define VOLUME_UP_ONE 0X04
#define VOLUME_DOWN_ONE 0X05
#define CMD_PLAY_WITHVOLUME 0X22 //data is needed  0x7E 06 22 00 xx yy EF;(xx volume)(yy number of song)
#define CMD_SEL_DEV 0X09 //SELECT STORAGE DEVICE, DATA IS REQUIRED
#define DEV_TF 0X02 //HELLO,IM THE DATA REQUIRED

#define SLEEP_MODE_START 0X0A
#define SLEEP_MODE_WAKEUP 0X0B
#define CMD_RESET 0X0C//CHIP RESET
#define CMD_PLAY 0X0D //RESUME PLAYBACK
#define CMD_PAUSE 0X0E //PLAYBACK IS PAUSED
#define STOP_PLAY 0X16
#define SET_DAC 0X17//data is needed 00 start DAC OUTPUT;01 DAC no output



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

//BUTTON THINGS
#include <Bounce2.h>

int Cbtn = 4;
int Rbtn = 5;
int Lbtn = 3;
int Hbtn = 2;   //pin 6 and 3 don't work...


Bounce debouncerC = Bounce ();
Bounce debouncerR = Bounce ();
Bounce debouncerL = Bounce ();
Bounce debouncerH = Bounce ();

enum State {
  INT_PRACT,
  SONG,
  ABOUT,
  HELP,
  INT_CHOICE,
  TUNEA,
  TUNEAC
};

State currentState;

//TONE PRINTER THINGS

int freq[] = {
  131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247,
  262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494,
  523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988
};

String notes[] = {
  "C 3", "C#3", "D 3", "D#3", "E 3", "F 3", "F#3", "G 3", "G#3", "A 3", "A#3", "B 3",
  "C 4", "C#4", "D 4", "D#4", "E 4", "F 4", "F#4", "G 4", "G#4", "A 4", "A#4", "B 4",
  "C 5", "C#5", "D 5", "D#5", "E 5", "F 5", "F#5", "G 5", "G#5", "A 5", "A#5", "B 5",
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

  Serial.begin(9600);//Start our Serial coms for serial monitor in our pc
  mySerial.begin(9600);//Start our Serial coms for THE MP3
  delay(500);//Wait chip initialization is complete
  sendCommand(CMD_SEL_DEV, DEV_TF);//select the TF card
  delay(200);//wait for 200ms

  tft.begin();
  tft.setRotation(-1);
  tft.fillScreen(BLACK);
  splash();
  tft.fillScreen(BLACK);

  pinMode(13, OUTPUT); //led indicator pin
  pinMode(12, OUTPUT); //output pin

  pinMode(Cbtn, INPUT);
  pinMode(Rbtn, INPUT);
  pinMode(Lbtn, INPUT);
  pinMode(Hbtn, INPUT);

  debouncerC.attach(Cbtn);
  debouncerR.attach(Rbtn);
  debouncerL.attach(Lbtn);
  debouncerH.attach(Hbtn);

  currentState = INT_PRACT;

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
  checkClipping();

  if (checkMaxAmp > ampThreshold) {
    frequency = 38462 / float(period); //calculate frequency timer rate/period
  }

  //display states
  if (currentState == INT_PRACT) {
    movecursor(GREEN, WHITE, WHITE, WHITE);
  } else if (currentState == SONG) {
    movecursor(WHITE, GREEN, WHITE, WHITE);
  } else if (currentState == ABOUT) {
    movecursor(WHITE, WHITE, GREEN, WHITE);
  } else if (currentState == HELP) {
    movecursor(WHITE, WHITE, WHITE, GREEN);
  } else if (currentState == INT_CHOICE) {
    int_pract();
  } else if (currentState == TUNEA) {
    displayNote(frequency);
    playA();
  } else if (currentState == TUNEAC) {
    displayNote(frequency);
    playAC();
  }

  //read inputs

  debouncerC.update();
  debouncerR.update();
  debouncerL.update();
  debouncerH.update();


  int pressed0 = debouncerC.fell();
  int pressed1 = debouncerR.fell();
  int pressed2 = debouncerL.fell();
  int pressed3 = debouncerH.fell();

  // update state
  if (currentState == INT_PRACT) {
    if (pressed1) {
      currentState = SONG ;
    } else if (pressed0) {
      tft.fillScreen(BLACK);
      currentState = INT_CHOICE ;
    } else if (pressed2) {
      currentState = HELP ;
    }
  }

  else if (currentState == SONG) {
    if (pressed1) {
      currentState = ABOUT ;
    } else if (pressed2) {
      currentState = INT_PRACT ;
    }
  }

  else if (currentState == ABOUT) {
    if (pressed1) {
      currentState = HELP ;
    } else if (pressed2) {
      currentState = SONG ;
    }
  }

  else if (currentState == HELP) {
    if (pressed1) {
      currentState = INT_PRACT ;
    } else if (pressed2) {
      currentState = ABOUT ;
    }
  }
  else if (currentState == INT_CHOICE) {
    if (pressed0) {
      tft.fillScreen(BLACK);
      currentState = TUNEA ;
    }
    if (pressed2) {
      tft.fillScreen(BLACK);
      currentState = INT_PRACT ;
    }
  } else if (currentState == TUNEA) {
    if (pressed2) {
      tft.fillScreen(BLACK);
      currentState = INT_CHOICE ;
    }
    if (pressed3) {
      currentState = TUNEAC ;
    }
  } else if (currentState == TUNEAC) {
    if (pressed3) {
      currentState = TUNEA;
    }
  }
}






