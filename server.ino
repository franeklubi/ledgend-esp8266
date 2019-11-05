#include "server.h"


void setupServer(uint16_t port) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect();

    SSID[8] = random(9)+0x30;
    Serial.println(SSID);

    WiFi.softAP(SSID, PASS, 2);
    Serial.println("AP STARTED");

    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());

    SERVER.on("/networks", HTTP_GET, handleNetworks);
    SERVER.on("/connect", HTTP_GET, handleConnect);
    SERVER.on("/status", HTTP_GET, handleStatus);
    SERVER.on("/", HTTP_GET, handleRoot);
    SERVER.onNotFound(handleNotFound);

    SERVER.begin();
}


void handleClient() {
    SERVER.handleClient();
}


void handleRoot() {
    SERVER.send(200, "text/html", html_root);
}


void handleStatus() {
    if ( WiFi.status() == WL_CONNECTED ) {
        SERVER.send(
            200, "text/plain",
            String(WiFi.SSID())+" @ "+WiFi.localIP().toString()
        );
    } else {
        SERVER.send(200, "text/plain", "false");
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

    SERVER.send(200, "application/json", json_time);
}


void handleConnect() {
    String network_SSID = SERVER.arg("SSID");
    String network_PASS = SERVER.arg("PASS");

    WiFi.begin(network_SSID, network_PASS, 1);
    delay(8000);

    SERVER.send(
        200,
        "text/plain",
        String(WiFi.SSID())+" @ "+WiFi.localIP().toString()
    );
}


void handleNotFound() {
    SERVER.sendHeader("Location", "/");
    SERVER.send(303);
}
