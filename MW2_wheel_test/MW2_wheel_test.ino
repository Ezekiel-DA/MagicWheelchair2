#include <FastLED.h>

#define NUM_LEDS 120
#define CHASE_LENGTH 5
#define MAX_ENERGY 100
#define DECAY_RATE 0.75

CRGB strip[NUM_LEDS];

short chaseCenter = 0;

void setAll(CRGB c) {
  for (unsigned int i = 0; i < NUM_LEDS; ++i) {
    strip[i] = c;
  }
}

void showStrip(unsigned int time) {
  FastLED.show();
  FastLED.delay(time);
}
  
void setup() {
  delay(250);
  FastLED.addLeds<SK9822, 30, 31, BGR>(strip, NUM_LEDS);
  setAll(CRGB::Black);
  showStrip(0);
}

float distanceFromCenterOfChase(unsigned int pos) {
  float distance = abs((float)pos - chaseCenter);
  if (distance >= NUM_LEDS/2)
    distance = NUM_LEDS - distance;
   
  return distance;
}

bool isWithinChase(unsigned int pos) {
  return distanceFromCenterOfChase(pos) <= CHASE_LENGTH/2;
}

void loop() {
  for (unsigned int i = 0; i < NUM_LEDS; ++i) {
    if (isWithinChase(i)) { // add energy based on distance from center of chase
      float distance = distanceFromCenterOfChase(i);
      short energy = MAX_ENERGY - min(MAX_ENERGY, distance * 10);
      strip[i%NUM_LEDS] += CRGB(0, energy, energy);
    } else { // decay
      strip[i%NUM_LEDS] %= DECAY_RATE*256;
    }
  }

  ++chaseCenter;
  if (chaseCenter >= NUM_LEDS)
    chaseCenter = 0;

  showStrip(16);
}
