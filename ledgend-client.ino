#include "ledserv_client.h"
#include "webpages.h"
#include "server.h"
#include "leds.h"

#define PORT        10107


void setup() {
    Serial.begin(9600);
    Serial.println("\n");

    setupLeds();
    setupServer(PORT);
    setupLedservClient(PORT);
}


void loop() {
    handleClient();
    loopLedservClient();
}
