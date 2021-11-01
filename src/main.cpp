#include <Arduino.h>
#include <Encoder.h>
#include <Joystick.h>

#define MAXLAG 5
#define NUMBEROFENCODERS 5
#define NUMBEROFBUTTONS 6
#define BUTTONPRESSTIME 50
#define BUTTONRELEASETIME 25

const int dataPins[]={28,31,33,35,37};
const int clockPins[]={29,30,32,34,36};
const int encButtonPins[]={38,40,42,44,46};
const int buttonPins[]={2,3,4,5,6,7};

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,
  JOYSTICK_TYPE_GAMEPAD, (NUMBEROFENCODERS * 3) + NUMBEROFBUTTONS, 0,
  false, false, false, false, false, false,
  false, false, false, false, false);

enum buttonStates {ready, requested, active, hold};
buttonStates encButtonState[NUMBEROFENCODERS * 3];
unsigned long encButtonActiveTime[NUMBEROFENCODERS * 3];
long reportedPosition[NUMBEROFENCODERS];

// struct RGB {
//   byte r;
//   byte g;
//   byte b;
// };

// RGB variable = { 255 , 0 , 0 };

Encoder myEnc[NUMBEROFENCODERS] = {
  Encoder(clockPins[0], dataPins[0]),
  Encoder(clockPins[1], dataPins[1]),
  Encoder(clockPins[2], dataPins[2]),
  Encoder(clockPins[3], dataPins[3]),
  Encoder(clockPins[4], dataPins[4])
};

void setup() {
  for(int i = 0; i<NUMBEROFENCODERS; i++) pinMode(encButtonPins[i], INPUT_PULLUP);
  for(int i = 0; i<NUMBEROFBUTTONS; i++) pinMode(buttonPins[i], INPUT_PULLUP);
  Joystick.begin();
}

void pushEncButton(int buttonNumber){
  encButtonActiveTime[buttonNumber] = millis();
  encButtonState[buttonNumber] = active;
  Joystick.pressButton(buttonNumber);
}

void releaseEncButton(int buttonNumber){
  encButtonState[buttonNumber] = hold;
  encButtonActiveTime[buttonNumber] = millis();
  Joystick.releaseButton(buttonNumber);
}

void checkRotary(uint8_t encoderNumber){
  uint8_t dnBtn = (encoderNumber * 3) + 1;
  uint8_t upBtn = dnBtn + 1;
  long encoderPosition = myEnc[encoderNumber].read() / 4;
  if((encButtonState[upBtn] == ready) && (encoderPosition > reportedPosition[encoderNumber])) {
    encButtonState[upBtn] = requested;
    encoderPosition - reportedPosition[encoderNumber] > MAXLAG ? reportedPosition[encoderNumber] = encoderPosition - MAXLAG : reportedPosition[encoderNumber]++;
  }
  if((encButtonState[dnBtn] == ready) && (reportedPosition[encoderNumber] > encoderPosition)) {
    encButtonState[dnBtn] = requested;
    reportedPosition[encoderNumber] - encoderPosition > MAXLAG ? reportedPosition[encoderNumber] = encoderPosition + MAXLAG : reportedPosition[encoderNumber]--;
  }
}

void performPresses(int buttonNumber){
  if(encButtonState[buttonNumber] == requested) pushEncButton(buttonNumber);
  if((encButtonState[buttonNumber] == active) && (millis() - encButtonActiveTime[buttonNumber] > BUTTONPRESSTIME)) releaseEncButton(buttonNumber);
  if((encButtonState[buttonNumber] == hold) && (millis() - encButtonActiveTime[buttonNumber] > BUTTONRELEASETIME)) encButtonState[buttonNumber] = ready;
}

void loop() {
  for(int buttonNumber = 0; buttonNumber<NUMBEROFBUTTONS; buttonNumber++){
    uint8_t buttonOutputNumber = buttonNumber + (NUMBEROFENCODERS * 3);
    digitalRead(buttonPins[buttonNumber]) ? Joystick.releaseButton(buttonOutputNumber) : Joystick.pressButton(buttonOutputNumber);
  }

  for(int encoderNumber = 0; encoderNumber<NUMBEROFENCODERS; encoderNumber++){
    uint8_t encButtonOutputNumber = encoderNumber * 3;
    digitalRead(encButtonPins[encoderNumber]) ? Joystick.releaseButton(encButtonOutputNumber) : Joystick.pressButton(encButtonOutputNumber);
    checkRotary(encoderNumber);
    performPresses((encoderNumber * 3) + 1);
    performPresses((encoderNumber * 3) + 2);
    Joystick.sendState();
  }
}