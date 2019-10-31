#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>

#include "webpages.h"


ESP8266WebServer server(80);
char* ssid = "LEDGEND_X";
char* pass = "";


WiFiUDP udp;
const uint16_t udp_port = 10107;
char udp_buffer[255];

char* ws_msg_id = "ledgend;";
uint8_t ws_msg_id_len;
bool ws_found = false;
char* ws_ip;

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
    if ( ws_found ) {
        return;
    }

    if ( udp.parsePacket() ) {
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
        ws_found = true;
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
}
