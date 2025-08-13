#ifndef INTERCOM_SERVER_LISTENER_H
#define INTERCOM_SERVER_LISTENER_H

#include "intercom_server_common.h"

void listener_init(void);
void* listener_task(void *arg);
void send_request(DoorRequest_t request, ClientData_t *client);
void forward_door_to_client_request(DoorPacket_t *request);

#endif
