#include "hub_queue.h"

static void increment_head(HubQueue_t *queue)
{
    if (queue->length == 0)
    {
        queue->head_idx = 0;
        queue->tail_idx = 0;
    }
    else
    {
        queue->head_idx++;
        if (queue->head_idx >= queue->capacity) queue->head_idx = 0;
    }
}

static void increment_tail(HubQueue_t *queue)
{
    if (queue->length == 0)
    {
        queue->head_idx = 0;
        queue->tail_idx = 0;
    }
    else
    {
        queue->tail_idx++;
        if (queue->tail_idx >= queue->capacity) queue->tail_idx = 0;
    }
}

HubQueue_t *hub_queue_create(uint16_t capacity)
{
    HubQueue_t *queue_ptr = malloc(sizeof(HubQueue_t) + (capacity * sizeof(DoorPacket_t)));

    queue_ptr->capacity = capacity;
    queue_ptr->head_idx = 0;
    queue_ptr->tail_idx = 0;
    queue_ptr->length = 0;
    pthread_mutex_init(&queue_ptr->mutex, NULL);
    log_append("hub queue created");

    return queue_ptr;
}

void hub_queue_destroy(HubQueue_t *queue)
{
    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);
    free(queue);
    log_append("hub queue destroyed");
}

int hub_queue_enqueue(HubQueue_t *queue, DoorPacket_t *src)
{
    pthread_mutex_lock(&queue->mutex);

    if (queue->length >= queue->capacity)
    {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    increment_tail(queue);
    queue->contents[queue->tail_idx] = *src;
    queue->length++;
    int ret = queue->length;

    pthread_mutex_unlock(&queue->mutex);
    log_append("hub queue enqueued");
    return ret;
}

int hub_queue_dequeue(HubQueue_t *queue, DoorPacket_t *dst)
{
    pthread_mutex_lock(&queue->mutex);
    if (queue->length <= 0)
    {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    *dst = queue->contents[queue->head_idx];
    queue->length--;
    increment_head(queue);
    int len = queue->length;

    pthread_mutex_unlock(&queue->mutex);
    log_append("hub queue dequeued");
    return len;
}
