#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"
#include "networking_common.h"
#include "wifi.h"

typedef enum ClientState
{
    CLIENTSTATE_NONE = 0,
    CLIENTSTATE_INIT = 1,
    CLIENTSTATE_CONNECTING = 2,
    CLIENTSTATE_ONLINE = 3,

} ClientState_t;

void client_task(void *arg);

#endif
