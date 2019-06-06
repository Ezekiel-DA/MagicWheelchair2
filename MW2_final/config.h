#define POWER_SWITCH_ID 0
#define HORN_BUTTON_ID  1
#define COLOR_BUTTON_ID 2

#define NUM_LEDS_FRONT_WHEEL 96
#define NUM_LEDS_BACK_WHEEL 120
#define NUM_LEDS_SIDE_LIGHTS 200 // FIXME
#define NUM_LEDS_HEADLIGHT 40
#define NUM_LEDS_SPEEDO 22 // NB: there are technically 24 of these but we want a "dead zone"  at the bottom

#define FRONT_OFFSET 22
#define BACK_OFFSET 82

#define MIN_RPM 15
#define MAX_RPM 300

#define MAX_ENERGY 255
#define DECAY_RATE 0.94*256 // on every step, an LED outside of the chase retains 94% energy from previous step
#define CHASE_LENGTH_FRONT 7
#define CHASE_LENGTH_BACK 7

#define HEADLIGHT_COLORS_COUNT 3 // keep this in sync with the array below
CRGB headlightColors[] = {CRGB::Red, CRGB::Green, CRGB::Blue};
