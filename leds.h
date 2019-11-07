#ifndef LEDS_H
#define LEDS_H


#include <FastLED.h>


#define NO_LEDS     15
#define LEDS_PIN    5


// type Change holds the changed led's index and it's r, g, and b values
typedef struct {
    uint16_t address;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Change;


CRGB leds[NO_LEDS];


void setupLeds();

void applyChanges(Change* changes, uint16_t no_changes);


#endif
