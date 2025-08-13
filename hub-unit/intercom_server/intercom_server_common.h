#ifndef INTERCOM_SERVER_COMMON_H
#define INTERCOM_SERVER_COMMON_H

#include "hub_common.h"
#include "networking_common.h"

#define CLIENT_SLOTS 32

typedef enum SlotState
{
    SLOTSTATE_VACANT = 0,
    SLOTSTATE_TAKEN = 1,
    SLOTSTATE_ACTIVE = 2,
    SLOTSTATE_GARBAGE = 3,
} SlotState_t;

typedef struct ClientData
{
    SlotState_t slot_state;
    int client_socket;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    pthread_t client_thread_handle;
    HubQueue_t *outbox;
} ClientData_t;

typedef struct ServerDoorList
{
    pthread_mutex_t lock;
    time_t last_updated;
    uint8_t count;
    uint8_t indices[HUB_MAX_DOOR_COUNT];
    char names[HUB_MAX_DOOR_COUNT][UNIT_NAME_MAX_LEN];
} ServerDoorList_t;

extern ServerDoorList_t door_list;

void server_start(void);

#endif
