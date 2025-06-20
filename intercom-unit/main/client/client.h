#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"
#include "networking_common.h"
#include "wifi.h"
#include "utils.h"
#include "packet_defs.h"
#include "packet_utils.h"

#define HUB_AP_SSID "DOORS-HUB"
#define HUB_AP_PASS "mAlabama"
#define HUB_SERVER_IP "192.168.8.1"
#define HUB_SERVER_PORT 45678

typedef enum ClientState
{
    CLIENTSTATE_NONE = 0,
    CLIENTSTATE_INIT = 1,
    CLIENTSTATE_CONNECTING = 2,
    CLIENTSTATE_CONNECTED = 3,
    CLIENTSTATE_BELL = 4,
} ClientState_t;

int send_request(DoorRequest_t request, uint16_t destination);
ClientState_t client_get_state(void);
void client_task(void *arg);

#endif
