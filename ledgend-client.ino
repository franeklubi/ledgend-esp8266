#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <WebSocketsClient.h>
#include <FastLED.h>

#include "webpages.h"


#define NO_LEDS 15
#define LEDS_PIN 5
CRGB leds[NO_LEDS];


ESP8266WebServer server(80);
char* ssid = "LEDGEND_X";
char* pass = "";


WiFiUDP udp;
const uint16_t udp_port = 10107;
char udp_buffer[255];


char* ws_msg_id = "ledgend;";
uint8_t ws_msg_id_len;
bool ws_address_found = false;
char* ws_ip;

WebSocketsClient web_socket;


typedef struct {
    uint16_t address;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Change;


void handleRoot() {
    server.send(200, "text/html", html_root);
}


void handleStatus() {
    if ( WiFi.status() == WL_CONNECTED ) {
        server.send(
            200, "text/plain",
            String(WiFi.SSID())+" @ "+WiFi.localIP().toString()
        );
    } else {
        server.send(200, "text/plain", "false");
    }
}


void handleNetworks() {
    int n = WiFi.scanNetworks();
    String json_time = "{\"networks\":[";
    for ( int x = 0; x < n; x++ ) {
        json_time += "[\"";
        json_time += String(WiFi.SSID(x));
        json_time += "\",";
        json_time += String(WiFi.RSSI(x));
        json_time += "]";
        if ( x != n-1 ) {
            json_time += ',';
        }
    }
    json_time += "]}";

    server.send(200, "application/json", json_time);
}


void handleConnect() {
    String network_SSID = server.arg("SSID");
    String network_PASS = server.arg("PASS");

    WiFi.begin(network_SSID, network_PASS, 1);
    delay(8000);

    server.send(
        200,
        "text/plain",
        String(WiFi.SSID())+" @ "+WiFi.localIP().toString()
    );
}


void handleNotFound() {
    server.sendHeader("Location", "/");
    server.send(303);
}


void listenUDP() {
    if ( udp.parsePacket() ) {
        if ( ws_address_found ) {
            return;
        }

        uint8_t len = udp.read(udp_buffer, 255);

        // if len is not equal 0
        if ( len ) {
            // null terminating the string
            udp_buffer[len] = 0;
        }

        handleIP(len);
    }
}


void handleIP(uint8_t buffer_len) {
    if ( buffer_len <= ws_msg_id_len ) {
        return;
    }

    int8_t cmp = strncmp(ws_msg_id, udp_buffer, ws_msg_id_len);
    if ( cmp == 0 ) {
        ws_ip = udp_buffer+ws_msg_id_len;
        ws_address_found = true;

        Serial.printf("Received websocket address: %s\n\r", ws_ip);
        connectToWebsocket();
    }
}


void connectToWebsocket() {
    Serial.println("Attempting ws connection");
    if ( !ws_address_found ) {
        Serial.println("Ws address not found yet!");
        return;
    }

    web_socket.begin(ws_ip, udp_port, "/ws");
    web_socket.onEvent(handleWebsocketEvent);
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


void parsePayload(uint8_t* payload, uint32_t length) {
    if ( length < 5 ) {
        Serial.println("Payload too short!");
        return;
    }

    // draw out the preambule
    uint8_t preambule = payload[0];

    // leds' information start after preambule
    uint32_t leds_length = length-1;
    uint8_t* leds = payload+1;

    // reading the preambule
    uint8_t address_bytes = preambule & 0x0F;
    // for later use if needed
    // uint8_t other_options = (preambule & 0xF0) >> 4;

    // r+g+b + number of address bytes
    uint8_t change_size = 3+address_bytes;

    if ( leds_length%change_size != 0 ) {
        Serial.println("Invalid payload!");
        return;
    }

    // allocating memory for Changes
    uint16_t no_changes = leds_length/change_size;
    void* temp_changes = malloc(no_changes*sizeof(Change));
    if ( !temp_changes ) {
        Serial.println("Can't malloc for changes!");
        return;
    }
    Change* changes = (Change*) temp_changes;


    // parsing payload for Changes
    for ( uint16_t x = 0; x < no_changes; x++ ) {
        Change new_change;

        uint32_t curr_index = x*(3+address_bytes);

        new_change.address = leds[curr_index];
        if ( address_bytes == 2 ) {
            new_change.address =
                (new_change.address << 8) | uint16_t(leds[curr_index+1]);
        }

        new_change.r = leds[curr_index+1+address_bytes-1];
        new_change.g = leds[curr_index+2+address_bytes-1];
        new_change.b = leds[curr_index+3+address_bytes-1];

        changes[x] = new_change;
    }

    applyChanges(changes, no_changes);

    free(changes);
}


void handleWebsocketEvent(WStype_t type, uint8_t* payload, uint32_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("Websocket disconnected\n\r");
            ws_address_found = false;
            return;

        case WStype_CONNECTED:
            Serial.printf("Websocket connected to url: %s\n\r", payload);
            return;

        case WStype_TEXT:
            Serial.println("TEXT");
            Serial.printf("Message from server: %s", payload);
            return;

        case WStype_BIN:
            break;

        default:
            return;
    }

    parsePayload(payload, length);
}


void setup() {
    Serial.begin(9600);
    Serial.println("\n");

    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect();

    ssid[8] = random(9)+0x30;
    Serial.println(ssid);

    WiFi.softAP(ssid, pass, 2);
    Serial.println("AP STARTED");

    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", HTTP_GET, handleRoot);
    server.on("/networks", HTTP_GET, handleNetworks);
    server.on("/connect", HTTP_GET, handleConnect);
    server.on("/status", HTTP_GET, handleStatus);

    server.onNotFound(handleNotFound);

    server.begin();

    udp.begin(udp_port);

    ws_msg_id_len = strlen(ws_msg_id);

    FastLED.addLeds<WS2812B, LEDS_PIN>(leds, NO_LEDS);
}


void loop() {
    server.handleClient();
    listenUDP();
    web_socket.loop();
}
