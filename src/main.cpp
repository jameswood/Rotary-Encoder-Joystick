#include <Arduino.h>
#include <Encoder.h>
#include <Joystick.h>
#include <Bounce2.h>

#define MAXLAG 5
#define ENCODERS 5
#define SWITCHES 20
#define KNOBPOSITIONS 5
#define BUTTONS 12
#define PRESSDURATION 50
#define RELEASEDURATION 25
#define BOUNCEINTERVAL PRESSDURATION
#define SWITCHBUTTONS 2 * SWITCHES
#define ENCODERBUTTONS 2 * ENCODERS
//button order: buttons, switches, encoders
#define SWITCHBUTTONOFFSET BUTTONS
#define ENCODERBUTTONOFFSET BUTTONS + SWITCHBUTTONS

const int encoderPins[][2] = {{28,29},{30,31},{33,32},{35,34},{37,36}}; //data, clock
const int buttonPins[] =     {2,3,4,5,6,7,8,38,40,42,44,46};
const int switchPins[] =     {1,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27};
const int analogSwitchPin =  A0;

const long speedLimit = 10;
const long holdTime = 100;

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD, BUTTONS + SWITCHBUTTONS + ENCODERBUTTONS, 1, false, false, false, false, false, false, false, false, false, false, false);
Bounce * switches = new Bounce[SWITCHES];

// enum buttonStates {ready, requested, active, hold};

struct vbuttonStatus {
  // buttonStates state = ready;
  // unsigned long pressTime = 0;
  unsigned long releaseTime = 0;
  unsigned long freeTime = 0;
};

struct encoderStatus {
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
  };
  pinMode(analogSwitchPin, INPUT);
  Joystick.begin();
};

void updateKnob(){
  int8_t knobPosition = analogRead(analogSwitchPin) / 205;
  knobPosition--;
  if (knobPosition > -1) knobPosition = knobPosition * 90;
  Joystick.setHatSwitch(0, knobPosition);
};

void doSwitches(int switchNumber){
  switches[switchNumber].update();
  if (switches[switchNumber].changed()){
    const uint8_t upButton = switchNumber + SWITCHBUTTONOFFSET;
    const uint8_t downButton = upButton + SWITCHES;
    if (switches[switchNumber].rose()) {
      switchButtonState[upButton].releaseTime = millis() + PRESSDURATION;
      switchButtonState[downButton].releaseTime = millis();
      Joystick.pressButton(upButton);
    } else {
      switchButtonState[downButton].releaseTime = millis() + PRESSDURATION;
      switchButtonState[upButton].releaseTime = millis();
      Joystick.pressButton(downButton);
    }
  }
};

void interpretEncoder(int encoderNumber){
  const uint8_t downButton = encoderNumber + ENCODERBUTTONOFFSET;
  const uint8_t upButton = downButton + 1;
  encoderState[encoderNumber].position = encoder[encoderNumber].read() / 4;
  if (encoderState[encoderNumber].position != encoderState[encoderNumber].reportedPosition){
    if((encoderState[encoderNumber].position > encoderState[encoderNumber].reportedPosition) && (encoderButtonState[upButton].freeTime <= millis())) {
      encoderButtonState[upButton].releaseTime = millis() + PRESSDURATION;
      encoderButtonState[upButton].freeTime = encoderButtonState[upButton].releaseTime + RELEASEDURATION;
      encoderButtonState[downButton].releaseTime = millis();
      encoderButtonState[downButton].freeTime = millis() + RELEASEDURATION;
      Joystick.pressButton(upButton);
      encoderState[encoderNumber].reportedPosition ++;
    } else if((encoderState[encoderNumber].position < encoderState[encoderNumber].reportedPosition) && (encoderButtonState[downButton].freeTime <= millis())) {
      encoderButtonState[downButton].releaseTime = millis() + PRESSDURATION;
      encoderButtonState[downButton].freeTime = encoderButtonState[downButton].releaseTime + RELEASEDURATION;
      encoderButtonState[upButton].releaseTime = millis();
      encoderButtonState[upButton].freeTime = millis() + RELEASEDURATION;
      Joystick.pressButton(downButton);
      encoderState[encoderNumber].reportedPosition --;
    }
  }
};

void loop(){
  //button order: buttons, switches, encoders
  static uint8_t switchNumber = 0;
  static uint8_t buttonNumber = 0;
  static uint8_t encoderNumber = 0;
  static uint8_t switchButtonNumber = 0;
  static uint8_t encoderButtonNumber = 0;
  updateKnob();
  digitalRead(buttonPins[buttonNumber]) ? Joystick.releaseButton(buttonNumber) : Joystick.pressButton(buttonNumber);

  doSwitches(switchNumber);
  interpretEncoder(encoderNumber);

  if (switchButtonState[switchButtonNumber].releaseTime <= millis()) Joystick.releaseButton(switchButtonNumber + SWITCHBUTTONOFFSET);
  if (encoderButtonState[encoderButtonNumber].releaseTime <= millis()) Joystick.releaseButton(encoderButtonNumber + ENCODERBUTTONOFFSET);

  buttonNumber == BUTTONS-1 ? buttonNumber = 0 : buttonNumber++;
  switchNumber == SWITCHES-1 ? switchNumber = 0 : switchNumber++;
  encoderNumber == ENCODERS-1 ? encoderNumber = 0 : encoderNumber++;
  switchButtonNumber == SWITCHBUTTONS-1 ? switchButtonNumber = 0 : switchButtonNumber++;
  encoderButtonNumber == ENCODERBUTTONS-1 ? encoderButtonNumber = 0 : encoderButtonNumber++;
  Joystick.sendState();
};