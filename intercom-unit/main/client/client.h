#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"
#include "networking_common.h"
#include "wifi.h"
#include "utils.h"
#include "packet_defs.h"
#include "packet_utils.h"
#include "gui.h"

#define HUB_AP_SSID "DOORS-HUB"
#define HUB_AP_PASS "mAlabama"
#define HUB_SERVER_IP "192.168.8.1"
#define HUB_SERVER_PORT 45678

#define CLIENT_MAX_DOOR_COUNT (8)

typedef enum ClientState
{
    CLIENTSTATE_NONE = 0,
    CLIENTSTATE_INIT = 1,
    CLIENTSTATE_CONNECTING = 2,
    CLIENTSTATE_CONNECTED = 3,
    CLIENTSTATE_BELL = 4,
} ClientState_t;

typedef struct ClientDoorList
{
    SemaphoreHandle_t lock;
    time_t last_updated;
    uint8_t count;
    uint8_t indices[CLIENT_MAX_DOOR_COUNT];
    char names[CLIENT_MAX_DOOR_COUNT][UNIT_NAME_MAX_LEN];
} ClientDoorList_t;

ClientDoorList_t *client_acquire_door_list_ptr(void);
void client_release_door_list_ptr(void);
int8_t client_get_current_bell_idx(void);
esp_netif_ip_info_t client_get_ip_info(void);
int client_send_request(DoorRequest_t request, uint16_t destination);
ClientState_t client_get_state(void);
void client_task(void *arg);

#endif
