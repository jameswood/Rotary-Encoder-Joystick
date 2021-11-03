#include <Arduino.h>
#include <Encoder.h>
#include <Joystick.h>
#include <Bounce2.h>

#define MAXLAG 5
#define ENCS 5
#define SWTS 15 // max 16 (32 overall)
#define KNBPOSS 5
#define BUTS 12
#define PRSDUR 40
#define RLSDUR 20
#define BNCINT PRSDUR
#define SWTBUTS 2 * SWTS
#define ENCBUTS 2 * ENCS

const int encPins[ENCS][2]={{45,44},{46,47},{48,49},{50,51},{52,53}}; //data, clock (swap #1 because it sucks)
const int swtPins[SWTS] =  {17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
const int butPins[BUTS] =  {33,32,34,35,36,37,38,39,40,41,42,43};
const int knbPin = A0;

Joystick_ joyBut(0x03, JOYSTICK_TYPE_GAMEPAD, BUTS + KNBPOSS, 0, false, false, false, false, false, false, false, false, false, false, false);
Joystick_ joySwt(0x04, JOYSTICK_TYPE_GAMEPAD, SWTBUTS, 0, false, false, false, false, false, false, false, false, false, false, false);
Joystick_ joyEnc(0x05, JOYSTICK_TYPE_GAMEPAD, ENCBUTS, 0, false, false, false, false, false, false, false, false, false, false, false);

Bounce * switches = new Bounce[SWTS];

struct vbuttonStatus {
  unsigned long relTime = 0;
  unsigned long freeTime = 0;
  void update(){
    relTime = millis() + PRSDUR;
    freeTime = relTime + RLSDUR;
  };
};

struct knobStatus : vbuttonStatus {
  uint8_t selected = 0;
};

struct encoderStatus {
  long pos = 0;
  long repPos = 0;
};

encoderStatus encStat[ENCS];
vbuttonStatus swtButStat[SWTBUTS];
vbuttonStatus encButStat[SWTBUTS];
knobStatus knbStat;

Encoder encoder[ENCS] = {
  Encoder(encPins[0][0], encPins[0][1]),
  Encoder(encPins[1][0], encPins[1][1]),
  Encoder(encPins[2][0], encPins[2][1]),
  Encoder(encPins[3][0], encPins[3][1]),
  Encoder(encPins[4][0], encPins[4][1])
};

void setup() {
  for(int i = 0; i<BUTS; i++) pinMode(butPins[i], INPUT_PULLUP);
  for(int i = 0; i<SWTS; i++){
    switches[i].attach( swtPins[i], INPUT_PULLUP );
    switches[i].interval(BNCINT);
  };
  pinMode(knbPin, INPUT);
  joyBut.begin();
  joySwt.begin();
  joyEnc.begin();
};

void doKnb(){
  const int8_t knbPos = analogRead(knbPin) / 205;
  if ( knbPos != knbStat.selected ) {
    joyBut.releaseButton(knbStat.selected + BUTS);
    knbStat.update();
    joyBut.pressButton(knbPos + BUTS);
    knbStat.selected = knbPos;
  }
};

void doSwt(const int swtNum){
  switches[swtNum].update();
  if (switches[swtNum].changed()){
    const uint8_t upBut = swtNum * 2;
    const uint8_t dnBut = upBut + 1;
    if (switches[swtNum].rose()) {
      swtButStat[upBut].update();
      joySwt.pressButton(upBut);
    } else {
      swtButStat[dnBut].update();
      joySwt.pressButton(dnBut);
    }
  }
};

void doEnc(const int encNum){
  const uint8_t dnBut = encNum * 2;
  const uint8_t upBut = dnBut + 1;
  encStat[encNum].pos = encoder[encNum].read() / 4;
  if (encStat[encNum].pos != encStat[encNum].repPos) {
    if (encStat[encNum].pos > encStat[encNum].repPos) {
      if (encButStat[upBut].freeTime <= millis()) {
        encButStat[upBut].update();
        joyEnc.pressButton(upBut);
        encStat[encNum].repPos++;
      }
      if (encStat[encNum].pos - encStat[encNum].repPos > MAXLAG) {
        encStat[encNum].repPos += MAXLAG;
      }
    } else {
      if (encButStat[dnBut].freeTime <= millis()) {
        encButStat[dnBut].update();
        joyEnc.pressButton(dnBut);
        encStat[encNum].repPos--;
      }
      if (encStat[encNum].repPos - encStat[encNum].pos > MAXLAG) {
        encStat[encNum].repPos -= MAXLAG;
      }
    }
  }
};

void loop(){
  static uint8_t knbPos = 0;
  static uint8_t swtNum = 0;
  static uint8_t butNum = 0;
  static uint8_t encNum = 0;
  static uint8_t swtButNum = 0;
  static uint8_t encButNum = 0;

  if (knbStat.freeTime <= millis()) doKnb();
  digitalRead(butPins[butNum]) ? joyBut.releaseButton(butNum) : joyBut.pressButton(butNum);

  doSwt(swtNum);
  doEnc(encNum);

  if (swtButStat[swtButNum].relTime <= millis()) joySwt.releaseButton(swtButNum);
  if (encButStat[encButNum].relTime <= millis()) joyEnc.releaseButton(encButNum);
  if (knbStat.relTime <= millis()) joyBut.releaseButton(knbPos + BUTS);
  
  butNum == BUTS-1 ? butNum = 0 : butNum++;
  swtNum == SWTS-1 ? swtNum = 0 : swtNum++;
  encNum == ENCS-1 ? encNum = 0 : encNum++;
  knbPos == KNBPOSS-1 ? knbPos = 0 : knbPos++;
  swtButNum == SWTBUTS-1 ? swtButNum = 0 : swtButNum++;
  encButNum == ENCBUTS-1 ? encButNum = 0 : encButNum++;

  joyBut.sendState();
  joySwt.sendState();
  joyEnc.sendState();
};