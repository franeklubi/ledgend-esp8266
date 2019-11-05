#ifndef LEDSERV_CLIENT_H
#define LEDSERV_CLIENT_H


#include <WiFiUdp.h>
#include <WebSocketsClient.h>


// type Change holds the changed led's index and it's r, g, and b values
typedef struct {
    uint16_t address;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Change;


WiFiUDP udp;
char udp_buffer[255];
uint16_t OP_PORT;


char* ws_msg_id = "ledgend;";
uint8_t ws_msg_id_len;
bool ws_address_found = false;
char* ws_ip;

WebSocketsClient web_socket;


void setupLedservClient();

void listenUDP();

void loopLedservClient();

void handleWebsocketEvent(WStype_t type, uint8_t* payload, uint32_t length);

void parsePayload(uint8_t* payload, uint32_t length);

void connectToWebsocket();

void parseIP(uint8_t buffer_len);


#endif
