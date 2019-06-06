#include <FastLED.h>
#include <ResponsiveAnalogRead.h>
#include <AceButton.h>
using namespace ace_button;

#include "board.h"
#include "config.h"
#define POWER_SWITCH_ID 0
#define HORN_BUTTON_ID  1
#define COLOR_BUTTON_ID 2

#define NUM_LEDS_FRONT_WHEEL 96
#define NUM_LEDS_BACK_WHEEL 120
#define NUM_LEDS_SIDE_LIGHTS 200 // FIXME
#define NUM_LEDS_HEADLIGHT 40
#define NUM_LEDS_SPEEDO 22 // NB: there are technically 24 of these but we want a "dead zone"  at the bottom

#define CHASE_LENGTH_FRONT 7
#define MAX_ENERGY 255
#define DECAY_RATE 0.94*256 // on every step, an LED outside of the chase retains 94% energy from previous step

CRGB headlightColors[] = {CRGB::Red, CRGB::Green, CRGB::Blue};

ResponsiveAnalogRead analog(A0, true);
AceButton powerSwitch(SW_0_PIN, HIGH, POWER_SWITCH_ID);
AceButton colorButton(SW_1_PIN, HIGH, COLOR_BUTTON_ID);
AceButton hornButton(SW_2_PIN, HIGH, HORN_BUTTON_ID);

CRGB frontWheel[NUM_LEDS_FRONT_WHEEL];
CRGB backWheel[NUM_LEDS_BACK_WHEEL];
CRGB sideLights[NUM_LEDS_SIDE_LIGHTS];
CRGB headlight[NUM_LEDS_HEADLIGHT];
CRGB speedo[NUM_LEDS_SPEEDO];

int maxAnalogRead = 600;
int minAnalogRead = 0;
int currentHeadlightColor = 0;
short chaseCenter = 0;

bool globalOn = false;

void setAll(CRGB c, CRGB* strip, unsigned int numLeds) {
  for (unsigned int i = 0; i < numLeds; ++i) {
    strip[i] = c;
  }
}

void showStrips() {
  FastLED.show();
}

void handleButtons(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  switch (eventType) {
    case AceButton::kEventPressed:
      switch (button->getId()) {
        case POWER_SWITCH_ID:
          globalOn = true;
          Serial.println("startup");
          break;
        case COLOR_BUTTON_ID:
          setAll(headlightColors[++currentHeadlightColor % 3], headlight, NUM_LEDS_HEADLIGHT);
          Serial.println("color");
          break;
        case HORN_BUTTON_ID:
          Serial.println("horn");
          break;
      }
      break;
    case AceButton::kEventReleased:
      switch (button->getId()) {
        case POWER_SWITCH_ID:
          globalOn = false;
          allLEDsOff();
          break;
        case COLOR_BUTTON_ID: // fall through on purpose
        case HORN_BUTTON_ID:
          // nothing to do on release for either of these
          break;
      }
      break;
  }
}

void allLEDsOff() {
  setAll(CRGB::Black, frontWheel, NUM_LEDS_FRONT_WHEEL);
  setAll(CRGB::Black, backWheel, NUM_LEDS_BACK_WHEEL);
  //setAll(CRGB::Black, sideLights, NUM_LEDS_SIDE_LIGHTS);
  setAll(headlightColors[currentHeadlightColor], headlight, NUM_LEDS_HEADLIGHT);
  setAll(CRGB::Black, speedo, NUM_LEDS_SPEEDO);
  showStrips();
}
  
void setup() {
  pinMode(SW_0_PIN, INPUT_PULLUP);
  pinMode(SW_1_PIN, INPUT_PULLUP);
  pinMode(SW_2_PIN, INPUT_PULLUP);
  pinMode(SW_3_PIN, INPUT_PULLUP);

  powerSwitch.setEventHandler(handleButtons); // actually sets a handler for the ButtonConfig shared by all buttons by default
  colorButton.setEventHandler(handleButtons);
  hornButton.setEventHandler(handleButtons);
  
  delay(10);
  analog.update();
  minAnalogRead = analog.getValue();
  
  FastLED.addLeds<SK9822, STRIP_0_DATA, STRIP_0_CLOCK, BGR>(frontWheel, NUM_LEDS_FRONT_WHEEL);
  // NB: attach other side of front to power and GND for STRIP_1, then attach data and clock to STRIP_0
  FastLED.addLeds<SK9822, STRIP_2_DATA, STRIP_2_CLOCK, BGR>(backWheel, NUM_LEDS_BACK_WHEEL);
  // NB: attach other side of rear to power and GND for STRIP_3, then attach data and clock to STRIP_2
  //FastLED.addLeds<SK9822, STRIP_4_DATA, STRIP_4_CLOCK, BGR>(sideLights, NUM_LEDS_SIDE_LIGHTS);
  // NB: attach other side of sidelights to power and GND for STRIP_5, then attach data and clock to STRIP_4
  FastLED.addLeds<WS2812B, STRIP_9_DATA, GRB>(headlight, NUM_LEDS_HEADLIGHT);
  FastLED.addLeds<WS2812B, STRIP_8_DATA, GRB>(speedo, NUM_LEDS_SPEEDO);
  
  allLEDsOff();

  Serial.begin(9600);
}

float distanceFromCenterOfChase(unsigned int pos, unsigned int center, unsigned int NUM_LEDS) {
  float distance = abs((float)pos - center);
  if (distance >= NUM_LEDS/2)
    distance = NUM_LEDS - distance;
   
  return distance;
}

bool isWithinChase(unsigned int pos, unsigned int center, unsigned int NUM_LEDS, unsigned short CHASE_LENGTH) {
  return distanceFromCenterOfChase(pos, center, NUM_LEDS) <= CHASE_LENGTH/2;
}

void loop() {
  static unsigned long prevTime = millis();
  static int analogVal = 0;
  
  powerSwitch.check();

  if (!globalOn) {
    delay(25);
    prevTime = millis();
    return;
  }
  
  colorButton.check();
  hornButton.check();
  
  analog.update();
  analogVal = analog.getValue();
  if (analogVal > maxAnalogRead)
    maxAnalogRead = analogVal;
  unsigned short rpm = constrain(map(analogVal, minAnalogRead, maxAnalogRead-5, 15, 300), 15, 300);

  unsigned short mappedVal = constrain(map(analogVal, minAnalogRead, maxAnalogRead, 0, 100), 0, 100);
  //Serial.println(mappedVal);

  unsigned short speedoVal = constrain(map(analogVal, minAnalogRead, maxAnalogRead-5, 0, NUM_LEDS_SPEEDO), 0, NUM_LEDS_SPEEDO);

  unsigned long curTime = millis();
  float elapsed = curTime - prevTime;

  float rotateBy = (elapsed/60000)*rpm;
  short newChaseCenter = chaseCenter + NUM_LEDS_FRONT_WHEEL*rotateBy;

  if (newChaseCenter == chaseCenter)
    return;

  for (unsigned int i = 0; i < NUM_LEDS_SPEEDO; ++i) {
    if (i < speedoVal)
      //speedo[i] = CHSV(i*4, 255, 128);
      speedo[i] = CRGB(constrain(i*12, 0, 255), constrain(255-i*12, 0, 255), 0);
    else
      speedo[i] = CRGB::Black;
  }

  chaseCenter = newChaseCenter;
  prevTime = curTime;
  if (chaseCenter >= NUM_LEDS_FRONT_WHEEL)
    chaseCenter = 0;
  
  for (unsigned int i = 0; i < NUM_LEDS_FRONT_WHEEL; ++i) {
    unsigned int real_i = i%NUM_LEDS_FRONT_WHEEL;
    if (isWithinChase(i, chaseCenter, NUM_LEDS_FRONT_WHEEL, CHASE_LENGTH_FRONT)) { // add energy based on distance from center of chase
      float distance = distanceFromCenterOfChase(i, chaseCenter, NUM_LEDS_FRONT_WHEEL);
      short energy = MAX_ENERGY - min(MAX_ENERGY, distance * 10);
      frontWheel[real_i] += CRGB(0, energy, energy);
      backWheel[real_i] += CRGB(0, energy, energy);
    } else { // decay
      //frontWheel[i%NUM_LEDS_FRONT_WHEEL] %= DECAY_RATE;
      frontWheel[i%NUM_LEDS_FRONT_WHEEL].nscale8(DECAY_RATE);
      backWheel[i%NUM_LEDS_FRONT_WHEEL].nscale8(DECAY_RATE);
    }
  }
    
  showStrips();
}
