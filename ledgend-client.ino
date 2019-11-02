#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <WebSocketsClient.h>

#include "webpages.h"


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


    if ( length < 5 ) {
        Serial.println("Payload too short!");
        return;
    }

    uint8_t preambule = payload[0];
    uint8_t address_bytes = preambule & 0x0F;
    // for later use if needed
    // uint8_t other_options = (preambule & 0xF0) >> 4;

    if ( (length-1)%(address_bytes+3) != 0 ) {
        Serial.println("Invalid payload!");
        return;
    }

    uint8_t* leds = payload+1;
    for ( uint32_t x = 1; x < length; x += 3+address_bytes ) {
        uint16_t led_address = 0;
        led_address = payload[x];
        if ( address_bytes == 2 ) {
            led_address = (led_address << 8) | uint16_t(payload[x+1]);
        }

        uint8_t r = payload[x+1+address_bytes-1];
        uint8_t g = payload[x+2+address_bytes-1];
        uint8_t b = payload[x+3+address_bytes-1];

        Serial.printf("%i: %i, %i, %i\n\r", led_address, r, g, b);
    }
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
}


void loop() {
    server.handleClient();
    listenUDP();
    web_socket.loop();
}
