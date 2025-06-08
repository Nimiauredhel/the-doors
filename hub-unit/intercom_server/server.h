#ifndef SERVER_H
#define SERVER_H

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
    //int next_slot_idx;
    int client_socket;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    pthread_t client_thread_handle;
} ClientData_t;

void server_start(void);

#endif
