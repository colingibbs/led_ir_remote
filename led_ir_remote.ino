#include <IRremote.h>
#include <IRremoteInt.h>

// these must be PWM pins
#define pinRED 10
#define pinGREEN 5
#define pinBLUE 6

//remote values
#define POWER   0XFF02FD
#define REP     4294967295
//row 1
#define RED     0XFF1AE5
#define GREEN   0XFF9A65
#define BLUE    0XFFA25D
#define WHITE   0XFF22DD
//row 2
#define RED2    0XFF2AD5
#define GREEN2  0XFFAA55
#define BLUE2   0XFF926D
#define PINK    0XFF12ED
//row 3
#define ORANGE  0XFF0AF5
//row 4
#define YELLOW  0XFF38C7
//row 5
#define MAGENTA 0XFF58A7

//fade buttons
#define FADE3   0XFF609F
#define FADE7   0XFFE01F

// variables set by ethernet used to control internal modes
long mode = 0;
long red_power = 0;
long green_power = 0;
long blue_power = 0;
long timeAtColor = 1000;
long timePerStep = 120000;

#define mOFF 0
#define mRGB 1
#define mFADE3 2
#define mFADE7 3

//IR variables
int RECV_PIN = 11;
IRrecv irrecv(RECV_PIN);
decode_results results;

// ==================== begin led controller ==================== //

int currentRGB[] = {255,0,0};
int initializeFade = 0;
unsigned long nextFadeEvent = 0;
unsigned long currentTime = 0;

//mFADE3 vars
int fadeState = 1;  //determines which stage of the fade we're currently in
//start with color set to 255,0,0
//1) increase green: 255,255,0
//2) decrease red: 0,255,0
//3) increase blue: 0,255,255
//4) decrease green: 0,0,255
//5) increase red: 255,0,255
//6) decrease blue: 255,0,0
//go to #1

//mFADE7 vars
int targetRed = 0;
int targetBlue = 0;
int targetGreen = 0;


void ledFader3(long timePerStep, long timeAtColor){

  currentTime = micros();
  
  if(initializeFade == 0){
    nextFadeEvent = currentTime + timePerStep;
    initializeFade = 1;
  }
  
  if(currentTime > nextFadeEvent){
    //set time for next update
    nextFadeEvent = currentTime + timePerStep;   
    
    switch(fadeState){
      case 1:
        currentRGB[1]++;
        if(currentRGB[1] == 255){
          //move on to the next transition
          fadeState = 2;
          
          //add extra time to stay on this color for a bit
          nextFadeEvent += timeAtColor;
        }
      break;
      case 2:
        currentRGB[0]--;
        if(currentRGB[0] == 1){
    	fadeState = 3;
        nextFadeEvent += timeAtColor;
        }
      break;
      case 3:
        currentRGB[2]++;
        if(currentRGB[2] == 255){
          fadeState = 4;
          nextFadeEvent += timeAtColor;
        }
      break;
      case 4:
        currentRGB[1]--;
        if(currentRGB[1] == 1){
          fadeState = 5;
          nextFadeEvent += timeAtColor;
        }
      break;
      case 5:
        currentRGB[0]++;
        if(currentRGB[0] == 255){
          fadeState = 6;
          nextFadeEvent += timeAtColor;
        }
      break;
      case 6:
        currentRGB[2]--;
        if(currentRGB[2] == 1){
          fadeState = 1;
          nextFadeEvent += timeAtColor;
        }
      break;
    }
    analogWrite(pinRED, currentRGB[0]);
    analogWrite(pinBLUE, currentRGB[1]);
    analogWrite(pinGREEN, currentRGB[2]);
  }
}

void ledFader7(long timePerStep, long timeAtColor){

  currentTime = micros();  
  
  if(initializeFade == 0){
    nextFadeEvent = currentTime + timePerStep;
    initializeFade = 1; 
    
    //pick power for the channels
    float r1 = random(0,100);
    float r2 = random(0,100);
    float sum = r1 + r2;
    //pick which channel to skip
    int r3 = random (0, 3); 
    
    switch(r3){
      case(0):
        targetRed = max(min((r1/sum) * 510, 255), 50);
        targetBlue = max(min((r2/sum) * 510, 255), 50);
        targetGreen = 0;
      break;
      case(1):
        targetRed = max(min((r1/sum) * 510, 255), 50);
        targetBlue = 0;
        targetGreen = max(min((r2/sum) * 510, 255), 50);
      break;
      case(2):
        targetRed = 0;
        targetBlue = max(min((r1/sum) * 510, 255), 50);
        targetGreen = max(min((r2/sum) * 510, 255), 50);
      break;
    }

    //Serial.print("Target red: ");
    //Serial.println(targetRed);
    //Serial.print("Target blue: ");
    //Serial.println(targetBlue);
    //Serial.print("Target green: ");
    //Serial.println(targetGreen);
  }
  
  if(targetRed == currentRGB[0] && targetBlue == currentRGB[1] && targetGreen == currentRGB[2]){
    //move on to the next transition
    initializeFade = 0;
  }  
  
  if(currentTime > nextFadeEvent){
    //set time for next update
    nextFadeEvent = currentTime + timePerStep;   
    int r4 = random (0, 3);
    
    if(targetRed < currentRGB[0] && r4 == 0){
      currentRGB[0]--;
    } else if(targetRed > currentRGB[0] && r4 == 0){
      currentRGB[0]++;
    }
    
    if(targetBlue < currentRGB[1] && r4 == 1){
      currentRGB[1]--;
    } else if(targetBlue > currentRGB[1] && r4 == 1){
      currentRGB[1]++;
    }
        
    if(targetGreen < currentRGB[2] && r4 == 2){
      currentRGB[2]--;
    } else if(targetGreen > currentRGB[2] && r4 == 2){
      currentRGB[2]++;
    }
    
    analogWrite(pinRED, currentRGB[0]);
    analogWrite(pinBLUE, currentRGB[1]);
    analogWrite(pinGREEN, currentRGB[2]);
  }
}

void ledcontrollerLoop() {
    
  if (mode == mOFF) {
    analogWrite(pinRED, 0);
    analogWrite(pinBLUE, 0);
    analogWrite(pinGREEN, 0);
    initializeFade = 0;
   
  } else if (mode == mRGB) {
    analogWrite(pinRED, red_power);
    analogWrite(pinBLUE, blue_power);
    analogWrite(pinGREEN, green_power);
    initializeFade = 0;

  } else if (mode == mFADE3) {
    ledFader3(timePerStep, timeAtColor);
  } else if (mode == mFADE7) {
    ledFader7(timePerStep, timeAtColor);
  }
}
// ==================== end led controller ==================== //

void setup() {

  Serial.begin(9600);
  randomSeed(analogRead(0));

  irrecv.enableIRIn(); // Start the IR receiver

  // setup output pins on pwm
  pinMode(pinRED, OUTPUT);
  pinMode(pinBLUE, OUTPUT);
  pinMode(pinGREEN, OUTPUT);
}

void loop() {
  
  ledcontrollerLoop();
  
  if (irrecv.decode(&results))
  {
   //Serial.println(results.value, HEX);
   irrecv.resume(); // Receive the next value
   
   switch(results.value){
     case POWER:
       mode = mOFF;
       break;
     case FADE3:
       mode = mFADE3;
       timePerStep = 120000;
       timeAtColor = 1000;
       break;
     case FADE7:
       mode = mFADE7;
       timePerStep = 80000;
       timeAtColor = 1000;
       break;
     case RED:
     case RED2:
       mode = mRGB;
       red_power = 255;
       blue_power = 0;
       green_power = 0;
       break;
     case GREEN:
     case GREEN2:
       mode = mRGB;
       red_power = 0;
       blue_power = 0;
       green_power = 255;
       break;
     case BLUE:
     case BLUE2:
       mode = mRGB;
       red_power = 0;
       blue_power = 255;
       green_power = 0;
       break;
     case WHITE:
       mode = mRGB;
       red_power = 255;
       blue_power = 255;
       green_power = 255;
       break;
     case PINK:
       mode = mRGB;
       red_power = 255;
       blue_power = 255;
       green_power = 100;
       break;
     case ORANGE:
       mode = mRGB;
       red_power = 255;
       blue_power = 0;
       green_power = 128;
       break;
     case YELLOW:
       mode = mRGB;
       red_power = 255;
       blue_power = 0;
       green_power = 255;
       break;
     case MAGENTA:
       mode = mRGB;
       red_power = 255;
       blue_power = 255;
       green_power = 0;
       break;

   } 
   
  }
}
