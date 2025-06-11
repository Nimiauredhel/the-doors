#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"
#include "networking_common.h"
#include "wifi.h"
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

} ClientState_t;

void client_task(void *arg);

#endif
