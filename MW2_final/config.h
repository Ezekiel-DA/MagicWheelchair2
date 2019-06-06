#define POWER_SWITCH_ID 0
#define HORN_BUTTON_ID  1
#define COLOR_BUTTON_ID 2

#define NUM_LEDS_FRONT_WHEEL 96
#define NUM_LEDS_BACK_WHEEL 120
#define NUM_LEDS_SIDE_LIGHTS 200 // FIXME 73 + 
#define NUM_LEDS_HEADLIGHT 40
#define NUM_LEDS_SPEEDO_RING 24 // this is the full amount of LEDs in the ring
#define NUM_LEDS_SPEEDO 19 // this is how many LEDs in the ring are used for the speedo; the rest will be used as indicators

#define FRONT_OFFSET 22
#define BACK_OFFSET 82

#define MIN_RPM 12
#define MAX_RPM 140

#define MAX_ENERGY 140
#define DECAY_RATE 0.94*256 // on every step, an LED outside of the chase retains 94% energy from previous step
#define CHASE_LENGTH_FRONT 7
#define CHASE_LENGTH_BACK 7

#define STARTING_HUE 128 // aqua
