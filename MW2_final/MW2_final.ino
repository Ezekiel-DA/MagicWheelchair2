#include <FastLED.h>
#include <ResponsiveAnalogRead.h>
#include <AceButton.h>
using namespace ace_button;

#include "board.h"
#include "config.h"

ResponsiveAnalogRead analog(THROTTLE_PIN, true);
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
unsigned short currentHue = 128;

bool colorShifting = false;
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
          currentHeadlightColor = 0;
          setAll(headlightColors[currentHeadlightColor], headlight, NUM_LEDS_HEADLIGHT);
          Serial.println("startup");
          break;
        case COLOR_BUTTON_ID:
          setAll(headlightColors[++currentHeadlightColor % HEADLIGHT_COLORS_COUNT], headlight, NUM_LEDS_HEADLIGHT);
          colorShifting = true;
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
        case COLOR_BUTTON_ID:
          colorShifting = false;
          break;
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
  setAll(CRGB::Black, sideLights, NUM_LEDS_SIDE_LIGHTS);
  setAll(CRGB::Black, headlight, NUM_LEDS_HEADLIGHT);
  setAll(CRGB::Black, speedo, NUM_LEDS_SPEEDO);
  showStrips();
}

void updateSpeedo(unsigned short val) {
  static unsigned long previousUpdate = millis();
  unsigned long currentTime = millis();
  float elapsed = currentTime - previousUpdate;

  if (elapsed < 50) {
    return; 
  }
  previousUpdate = currentTime;
  
  unsigned short speedoVal = constrain(map(val, minAnalogRead, maxAnalogRead-5, 0, NUM_LEDS_SPEEDO), 0, NUM_LEDS_SPEEDO);
  for (unsigned int i = 0; i < NUM_LEDS_SPEEDO; ++i) {
    if (i < speedoVal)
      //speedo[i] = CHSV(i*4, 255, 128);
      speedo[i] = CRGB(constrain(i*12, 0, 255), constrain(255-i*12, 0, 255), 0) % (255*0.3);
    else
      speedo[i] = CRGB::Black;
  }
}

void outputThrottlePositionToSerial(unsigned short val) {
  static unsigned long previousUpdate = millis();
  unsigned long currentTime = millis();
  float elapsed = currentTime - previousUpdate;

  if (elapsed < 50) {
    return; 
  }
  previousUpdate = currentTime;
  
  unsigned short normalizedThrottle = constrain(map(val, minAnalogRead, maxAnalogRead, 0, 100), 0, 100);
  Serial.println(normalizedThrottle);
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

void animateFrontWheel(unsigned short rpm, CRGB* strip, unsigned short numLeds, unsigned short offset, unsigned short chaseLength) {
  static unsigned long previousUpdate = millis();
  unsigned long currentTime = millis();
  float elapsed = currentTime - previousUpdate;

  float rotateBy = (elapsed/60000)*rpm;
  static unsigned short chaseCenter = offset;
  unsigned short newChaseCenter = chaseCenter + numLeds*rotateBy;

  if (newChaseCenter == chaseCenter)
    return;

  chaseCenter = newChaseCenter;
  previousUpdate = currentTime;
  if (chaseCenter >= numLeds)
    chaseCenter = 0;

  for (unsigned short i = 0; i < numLeds; ++i) {
    unsigned short real_i = i%numLeds;
    if (isWithinChase(real_i, chaseCenter, numLeds, chaseLength)) { // add energy based on distance from center of chase
      float distance = distanceFromCenterOfChase(real_i, chaseCenter, numLeds);
      unsigned short energy = MAX_ENERGY - min(MAX_ENERGY, distance * 10);
      //strip[real_i] += CRGB(0, energy, energy);
      strip[real_i] += CHSV(currentHue, 255, energy);
    } else { // decay
      strip[real_i].nscale8(DECAY_RATE);
    }
  }
}

// Yes, this is 100% identical to animateFrontWheel, but it looks like trying to share the same function prevents some compiler optimizations and makes everything slower...
void animateBackWheel(unsigned short rpm, CRGB* strip, unsigned short numLeds, unsigned short offset, unsigned short chaseLength) {
  static unsigned long previousUpdate = millis();
  unsigned long currentTime = millis();
  float elapsed = currentTime - previousUpdate;

  float rotateBy = (elapsed/60000)*rpm;
  static unsigned short chaseCenter = offset;
  unsigned short newChaseCenter = chaseCenter + numLeds*rotateBy;

  if (newChaseCenter == chaseCenter)
    return;

  chaseCenter = newChaseCenter;
  previousUpdate = currentTime;
  if (chaseCenter >= numLeds)
    chaseCenter = 0;

  for (unsigned short i = 0; i < numLeds; ++i) {
    unsigned short real_i = i%numLeds;
    if (isWithinChase(real_i, chaseCenter, numLeds, chaseLength)) { // add energy based on distance from center of chase
      float distance = distanceFromCenterOfChase(real_i, chaseCenter, numLeds);
      unsigned short energy = MAX_ENERGY - min(MAX_ENERGY, distance * 10);
      //strip[real_i] += CRGB(0, energy, energy);
      strip[real_i] += CHSV(currentHue, 255, energy);
    } else { // decay
      strip[real_i].nscale8(DECAY_RATE);
    }
  }
}

void animateSideLights(unsigned short rpm, CRGB* strip, unsigned short numLeds) {
  static unsigned long previousUpdate = millis();
  unsigned long currentTime = millis();
  float elapsed = currentTime - previousUpdate;

  if (elapsed < 50) {
    return;
  }
  previousUpdate = currentTime;
  
  unsigned short energy = beatsin8(rpm, 0, MAX_ENERGY);
  CRGB color = CRGB(0, energy, energy) % (255*0.3);
  for (unsigned int i = 0; i < numLeds; ++i) {
    strip[i] = CRGB(0, energy, energy);
  }
}

void shiftColors() {
  if (!colorShifting)
    return;
  static unsigned long previousUpdate = millis();
  unsigned long currentTime = millis();
  float elapsed = currentTime - previousUpdate;

  if (elapsed < 50) {
    return; 
  }
  previousUpdate = currentTime;
    
  ++currentHue;
}

void setup() {
  pinMode(SW_0_PIN, INPUT_PULLUP);
  pinMode(SW_1_PIN, INPUT_PULLUP);
  pinMode(SW_2_PIN, INPUT_PULLUP);
  pinMode(SW_3_PIN, INPUT_PULLUP);

  powerSwitch.setEventHandler(handleButtons);
  
  delay(10);
  analog.update();
  minAnalogRead = analog.getValue();
  
  FastLED.addLeds<SK9822, STRIP_0_DATA, STRIP_0_CLOCK, BGR>(frontWheel, NUM_LEDS_FRONT_WHEEL);
  // NB: attach other side of front to power and GND for STRIP_1, then attach data and clock to STRIP_0
  FastLED.addLeds<SK9822, STRIP_2_DATA, STRIP_2_CLOCK, BGR>(backWheel, NUM_LEDS_BACK_WHEEL);
  // NB: attach other side of rear to power and GND for STRIP_3, then attach data and clock to STRIP_2
  // ==== STRIP_4 NOT CONNECTED ====
  FastLED.addLeds<SK9822, STRIP_5_DATA, STRIP_5_CLOCK, BGR>(sideLights, NUM_LEDS_SIDE_LIGHTS);
  // NB: attach other side of sidelights to power and GND for STRIP_6, then attach data and clock to STRIP_5
  // ==== STRIP_7 NOT CONNECTED ====
  FastLED.addLeds<WS2812B, STRIP_8_DATA, GRB>(speedo, NUM_LEDS_SPEEDO);
  FastLED.addLeds<WS2812B, STRIP_9_DATA, GRB>(headlight, NUM_LEDS_HEADLIGHT);
  
  allLEDsOff();

  if (powerSwitch.isPressedRaw()) {
    globalOn = true;
  }

  Serial.begin(9600);
}

void loop() {
  powerSwitch.check();

  if (!globalOn) {
    delay(25);
    return;
  }
  
  colorButton.check();
  hornButton.check();
  
  analog.update();
  int analogVal = analog.getValue();
  if (analogVal > maxAnalogRead)
    maxAnalogRead = analogVal;

  outputThrottlePositionToSerial(analogVal);
  updateSpeedo(analogVal);
  unsigned short rpm = constrain(map(analogVal, minAnalogRead, maxAnalogRead-5, MIN_RPM, MAX_RPM), MIN_RPM, MAX_RPM);
  animateFrontWheel(rpm, frontWheel, NUM_LEDS_FRONT_WHEEL, FRONT_OFFSET, CHASE_LENGTH_FRONT);
  animateBackWheel(rpm, backWheel, NUM_LEDS_BACK_WHEEL, BACK_OFFSET, CHASE_LENGTH_BACK);
  animateSideLights(rpm, sideLights, NUM_LEDS_SIDE_LIGHTS);
  shiftColors();
  showStrips();
}
