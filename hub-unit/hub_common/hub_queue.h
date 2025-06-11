#ifndef HUB_QUEUE_H
#define HUB_QUEUE_H

#include "hub_common.h"

typedef struct HubQueue
{
    uint16_t capacity;
    uint16_t length;
    int32_t head_idx;
    int32_t tail_idx;
    pthread_mutex_t mutex;
    DoorPacket_t contents[];
} HubQueue_t;

HubQueue_t *hub_queue_create(uint16_t capacity);
void hub_queue_destroy(HubQueue_t *queue);
int hub_queue_enqueue(HubQueue_t *queue, DoorPacket_t *src);
int hub_queue_dequeue(HubQueue_t *queue, DoorPacket_t *dst);

#endif
