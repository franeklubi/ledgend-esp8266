#include "ledserv_client.h"


void setupLedservClient(uint16_t port) {
    OP_PORT = port;

    udp.begin(OP_PORT);

    ws_msg_id_len = strlen(ws_msg_id);
}


void loopLedservClient() {
    web_socket.loop();
    listenUDP();
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

        parseIP(len);
    }
}


void parseIP(uint8_t buffer_len) {
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

    web_socket.begin(ws_ip, OP_PORT, "/ws");
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

    parsePayload(payload, length);
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
