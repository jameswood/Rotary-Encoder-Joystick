#include <Arduino.h>
#include <Encoder.h>
#include <Joystick.h>
#include <Bounce2.h>

#define MAXLAG 5
#define ENCODERS 5
#define SWITCHES 20
#define KNOBPOSITIONS 5
#define BUTTONS 12
#define BOUNCEINTERVAL 25
#define PRESSDURATION 50
#define RELEASEDURATION 25
#define SWITCHBUTTONS 2 * SWITCHES
#define ENCODERBUTTONS 2 * ENCODERS
//button order: buttons, switches, encoders
#define SWITCHVBUTTONOFFSET BUTTONS
#define ENCODERVBUTTONOFFSET BUTTONS + SWITCHBUTTONS

const int encoderPins[][2] = {{28,29},{30,31},{33,32},{35,34},{37,36}}; //data, clock
const int buttonPins[] =     {2,3,4,5,6,7,8,38,40,42,44,46};
const int switchPins[] =     {1,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27};
const int analogSwitchPin =  A0;

const long speedLimit = 10;
const long holdTime = 100;

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD, BUTTONS + SWITCHBUTTONS + ENCODERBUTTONS, 1, false, false, false, false, false, false, false, false, false, false, false);
Bounce * switches = new Bounce[SWITCHES];

enum buttonStates {ready, requested, active, hold};

struct vbuttonStatus {
  buttonStates state = ready;
  unsigned long activeTime = 0;
};

struct encoderStatus : vbuttonStatus {
  long position = 0;
  long reportedPosition = 0;
};

encoderStatus encoderState[ENCODERS];
vbuttonStatus switchButtonState[SWITCHBUTTONS];
vbuttonStatus encoderButtonState[SWITCHBUTTONS];

Encoder encoder[ENCODERS] = {
  Encoder(encoderPins[0][1], encoderPins[0][0]),
  Encoder(encoderPins[1][1], encoderPins[1][0]),
  Encoder(encoderPins[2][1], encoderPins[2][0]),
  Encoder(encoderPins[3][1], encoderPins[3][0]),
  Encoder(encoderPins[4][1], encoderPins[4][0])
};

void setup() {
  for(int i = 0; i<BUTTONS; i++) pinMode(buttonPins[i], INPUT_PULLUP);
  for(int i = 0; i<SWITCHES; i++){
    switches[i].attach( switchPins[i], INPUT_PULLUP );
    switches[i].interval(BOUNCEINTERVAL);
  }
  pinMode(analogSwitchPin, INPUT);
  Joystick.begin();
};

void updateKnob(){
  int8_t knobPosition = analogRead(analogSwitchPin) / 205;
  knobPosition--;
  if (knobPosition > -1) knobPosition = knobPosition * 90;
  Joystick.setHatSwitch(0, knobPosition);
};

void readSwitch(int switchNumber){
    switches[switchNumber].update();
    if (switches[switchNumber].rose()) {
      vbuttonState[switchNumber].state = requested;            // start clock for hold
    } else if (switches[switchNumber].fell()){
      vbuttonState[switchNumber + SWITCHES].state = requested;
    }
  }
  uint32_t buttonStateNow = 0;
  for (int i=0; i < BUTTONS * 2; i++) {     // if button was activated within holdtime, set it as 1
    if (millis() - pressedTime[i] < holdTime) bitWrite(buttonStateNow, i, 1);
  }
};

void updateSwitch(int switchNumber){
  const uint8_t downButton = switchNumber + SWITCHVBUTTONOFFSET;
  const uint8_t upButton = downButton + 1;
};

void updateEncoder(int encoderNumber){
  const uint8_t downButton = encoderNumber + ENCODERVBUTTONOFFSET;
  const uint8_t upButton = downButton + 1;
  if((vbuttonState[upButton].state == ready) && (encoderState[encoderNumber].position > encoderState[encoderNumber].reportedPosition)) {
    vbuttonState[upButton].state = requested;
    if (encoderState[encoderNumber].position - encoderState[encoderNumber].reportedPosition > MAXLAG){
      encoderState[encoderNumber].reportedPosition = encoderState[encoderNumber].position - MAXLAG;
    } else {
      encoderState[encoderNumber].reportedPosition++;
    }
  }
  if((vbuttonState[downButton].state == ready) && (encoderState[encoderNumber].reportedPosition > encoderState[encoderNumber].position)) {
    vbuttonState[downButton].state = requested;
    if (encoderState[encoderNumber].reportedPosition - encoderState[encoderNumber].position > MAXLAG){
      encoderState[encoderNumber].reportedPosition = encoderState[encoderNumber].position + MAXLAG
    } else {
      encoderState[encoderNumber].reportedPosition--;
    }
  }
};
};

void performPresses(int vButtonNumber){
  if(vbuttonState[vButtonNumber].state == requested) pushVButton(vButtonNumber);
  if((vbuttonState[vButtonNumber].state == active) && (millis() - vbuttonState[vButtonNumber].activeTime > PRESSDURATION)) releaseVButton(vButtonNumber);
  if((vbuttonState[vButtonNumber].state == hold) && (millis() - vbuttonState[vButtonNumber].activeTime > RELEASEDURATION)) vbuttonState[vButtonNumber].state = ready;
};

void pushVButton(int vButtonNumber){
  vbuttonState[vButtonNumber].pressTime = millis();
  vbuttonState[vButtonNumber].releaseTime = millis() + PRESSDURATION;
  vbuttonState[vButtonNumber].nextAvailablePress = millis() + PRESSDURATION + RELEASEDURATION;
  vbuttonState[vButtonNumber].state = active;
  Joystick.pressButton(vButtonNumber);
};

void releaseVButton(int vButtonNumber){
  vbuttonState[vButtonNumber].state = hold;
  vbuttonState[vButtonNumber].activeTime = millis();
  Joystick.releaseButton(vButtonNumber);
};

void loop() {
  //button order: buttons, switches, encoders
  static uint8_t switchNumber = 0;
  static uint8_t buttonNumber = 0;
  static uint8_t encoderNumber = 0;
  static uint8_t switchButtonNumber = 0;
  static uint8_t encoderButtonNumber = 0;
  updateKnob();
  digitalRead(buttonPins[buttonNumber]) ? Joystick.releaseButton(buttonNumber) : Joystick.pressButton(buttonNumber);

  readSwitch(switchNumber);
  updateSwitch(switchNumber);

  encoderState[encoderNumber].position = encoder[encoderNumber].read() / 4;
  updateEncoder(encoderNumber);

  buttonNumber == BUTTONS-1 ? buttonNumber = 0 : buttonNumber++;
  switchNumber == SWITCHES-1 ? switchNumber = 0 : switchNumber++;
  encoderNumber == ENCODERS-1 ? encoderNumber = 0 : encoderNumber++;
  switchButtonNumber == SWITCHBUTTONS-1 ? switchButtonNumber = 0 : switchButtonNumber++;
  encoderButtonNumber == ENCODERBUTTONS-1 ? encoderButtonNumber = 0 : encoderButtonNumber++;
  Joystick.sendState();
};