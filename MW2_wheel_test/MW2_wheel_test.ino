#include <FastLED.h>
#include <ResponsiveAnalogRead.h>
#include <AceButton.h>
using namespace ace_button;

#define NUM_LEDS_FRONT_WHEEL 120
#define NUM_LEDS_BACK_WHEEL 120
#define NUM_LEDS_HEADLIGHT 40
#define NUM_LEDS_SPEEDO 24

#define STRIP_0_DATA 31
#define STRIP_0_CLOCK 30
#define STRIP_1_DATA 33
#define STRIP_1_CLOCK 32
#define STRIP_2_DATA 35
#define STRIP_2_CLOCK 34
#define STRIP_3_DATA 37
#define STRIP_3_CLOCK 36
#define STRIP_4_DATA 39
#define STRIP_4_CLOCK 38
#define STRIP_5_DATA 49
#define STRIP_5_CLOCK 48
#define STRIP_6_DATA 47
#define STRIP_6_CLOCK 46
#define STRIP_7_DATA 45
#define STRIP_7_CLOCK 44
#define STRIP_8_DATA 43
#define STRIP_8_CLOCK 42
#define STRIP_9_DATA 41
#define STRIP_9_CLOCK 40

#define SW_0_PIN 11
#define SW_1_PIN 10
#define SW_2_PIN 9

#define CHASE_LENGTH 5
#define MAX_ENERGY 255
#define DECAY_RATE 0.75

ResponsiveAnalogRead analog(A0, true);
AceButton sw0(SW_0_PIN);

int currentHeadlightColor = 0;
int maxAnalogRead = 630;
int minAnalogRead = 0;

CRGB headlightColors[] = {CRGB::Red, CRGB::Green, CRGB::Blue};

CRGB frontWheel[NUM_LEDS_FRONT_WHEEL];
CRGB backWheel[NUM_LEDS_BACK_WHEEL];
CRGB headlight[NUM_LEDS_HEADLIGHT];
CRGB speedo[NUM_LEDS_SPEEDO];

short chaseCenter = 0;

void setAll(CRGB c, CRGB* strip, unsigned int numLeds) {
  for (unsigned int i = 0; i < numLeds; ++i) {
    strip[i] = c;
  }
}

void showStrips(unsigned int time) {
  FastLED.show();
  FastLED.delay(time);
}

void handleStartButton(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  Serial.println(eventType);
  switch (eventType) {
    case AceButton::kEventPressed:
      setAll(headlightColors[++currentHeadlightColor % 3], headlight, NUM_LEDS_HEADLIGHT);
      break;
  }
}
  
void setup() {
  delay(250);
  pinMode(SW_0_PIN, INPUT_PULLUP);
  pinMode(SW_1_PIN, INPUT_PULLUP);
  pinMode(SW_2_PIN, INPUT_PULLUP);
  Serial.begin(9600);

  delay(10);
  analog.update();
  minAnalogRead = analog.getValue();
  delay(10);
  
  FastLED.addLeds<SK9822, STRIP_0_DATA, STRIP_0_CLOCK, BGR>(frontWheel, NUM_LEDS_FRONT_WHEEL);
  FastLED.addLeds<SK9822, STRIP_1_DATA, STRIP_1_CLOCK, BGR>(backWheel, NUM_LEDS_BACK_WHEEL);
  FastLED.addLeds<WS2812B, STRIP_9_DATA, GRB>(headlight, NUM_LEDS_HEADLIGHT);
  FastLED.addLeds<WS2812B, STRIP_8_DATA, GRB>(speedo, NUM_LEDS_SPEEDO);
  
  setAll(CRGB::Black, frontWheel, NUM_LEDS_FRONT_WHEEL);
  setAll(CRGB::Black, backWheel, NUM_LEDS_BACK_WHEEL);
  setAll(headlightColors[currentHeadlightColor], headlight, NUM_LEDS_HEADLIGHT);
  setAll(CRGB::Black, speedo, NUM_LEDS_SPEEDO);
  //FastLED.setBrightness(5);
  showStrips(0);

  sw0.setEventHandler(handleStartButton);
}

float distanceFromCenterOfChase(unsigned int pos) {
  float distance = abs((float)pos - chaseCenter);
  if (distance >= NUM_LEDS_FRONT_WHEEL/2)
    distance = NUM_LEDS_FRONT_WHEEL - distance;
   
  return distance;
}

bool isWithinChase(unsigned int pos) {
  return distanceFromCenterOfChase(pos) <= CHASE_LENGTH/2;
}

int analogVal = 0;

void loop() {
//  int sw0 = !digitalRead(SW_0_PIN);
//  if (sw0) {
//    setAll(headlightColors[currentHeadlightColor++ % 3], headlight, NUM_LEDS_HEADLIGHT);
//    //setAll(CRGB::Black, frontWheel, NUM_LEDS_FRONT_WHEEL);
//  }
  sw0.check();

  analog.update();
  analogVal = analog.getValue();
  if (analogVal > maxAnalogRead)
    maxAnalogRead = analogVal;
  //Serial.println(val);
  analogVal = map(analogVal, minAnalogRead, maxAnalogRead-5, 0, NUM_LEDS_SPEEDO);
  analogVal = constrain(analogVal, 0, NUM_LEDS_SPEEDO);
  //Serial.println(val);

  for (unsigned int i = 0; i < NUM_LEDS_SPEEDO; ++i) {
    if (i < analogVal)
      //speedo[i] = CHSV(i*4, 255, 128);
      speedo[i] = CRGB(constrain(i*11, 0, 255), constrain(255-i*11, 0, 255), 0);
    else
      speedo[i] = CRGB::Black;
  }
  
  for (unsigned int i = 0; i < NUM_LEDS_FRONT_WHEEL; ++i) {
    if (isWithinChase(i)) { // add energy based on distance from center of chase
      float distance = distanceFromCenterOfChase(i);
      short energy = MAX_ENERGY - min(MAX_ENERGY, distance * 10);
      frontWheel[i%NUM_LEDS_FRONT_WHEEL] += CRGB(0, energy, energy);
      backWheel[i%NUM_LEDS_BACK_WHEEL] += CRGB(0, energy, energy);
//      frontWheel[i%NUM_LEDS_FRONT_WHEEL] += headlightColors[currentHeadlightColor] * energy;
    } else { // decay
      frontWheel[i%NUM_LEDS_FRONT_WHEEL] %= DECAY_RATE*256;
      backWheel[i%NUM_LEDS_BACK_WHEEL] %= DECAY_RATE*256;
    }
  }

  ++chaseCenter;
  if (chaseCenter >= NUM_LEDS_FRONT_WHEEL)
    chaseCenter = 0;

  showStrips(0);
}
