#include "leds.h"

#define NO_LEDS     15
#define LEDS_PIN    5
CRGB leds[NO_LEDS];


void setupLeds() {
    FastLED.addLeds<WS2812B, LEDS_PIN>(leds, NO_LEDS);
}


void applyChanges(Change* changes, uint16_t no_changes) {
    for ( uint32_t x = 0; x < no_changes; x++ ) {
        if ( changes[x].address >= NO_LEDS ) {
            Serial.println("Led address exceeds the number of leds");
            return;
        }


        leds[changes[x].address] = CRGB(
            changes[x].g,
            changes[x].r,
            changes[x].b
        );
    }
    FastLED.show();
}
