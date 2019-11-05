#ifndef SERVER_H
#define SERVER_H


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>


ESP8266WebServer SERVER(80);
char* SSID = "LEDGEND_X";
char* PASS = "";


// setupServer sets the server up, lol
void setupServer(uint16_t port);

// handleClient calls SERVER.handleClient
void handleClient();

// these are the route handlers for the server
void handleNotFound();
void handleNetworks();
void handleConnect();
void handleStatus();
void handleRoot();


#endif
