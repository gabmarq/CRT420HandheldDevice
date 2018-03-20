#include <Bounce2.h>

int Cbtn = 2;
int Rbtn = 3;
int Lbtn = 4;
int Hbtn = 5;   //pin 6 and 3 don't work...


Bounce debouncerC = Bounce ();
Bounce debouncerR = Bounce ();
Bounce debouncerL = Bounce ();
Bounce debouncerH = Bounce ();


void setup() {
  Serial.begin(9600);
  
  pinMode(Cbtn, INPUT);
  pinMode(Rbtn, INPUT);
  pinMode(Lbtn, INPUT);
  pinMode(Hbtn, INPUT);

  
  debouncerC.attach(Cbtn);
  debouncerR.attach(Rbtn);
  debouncerL.attach(Lbtn);
  debouncerH.attach(Hbtn);
  
}

void loop() {

//tests buttons 
  if (digitalRead(Cbtn) == HIGH)
  {
    Serial.println("C");
  }
  if (digitalRead(Rbtn) == HIGH)
  {
    Serial.println("R");
  }
  if (digitalRead(Lbtn) == HIGH)
  {
    Serial.println("L");
  }
  if (digitalRead(Hbtn) == HIGH)
  {
    Serial.println("H");
  }

  delay(100);
  
}


