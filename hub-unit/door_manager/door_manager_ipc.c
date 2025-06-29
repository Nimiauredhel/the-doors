#include "door_manager_ipc.h"

static int clients_to_doors_shmid = -1; 
static HubShmLayout_t *clients_to_doors_ptr = NULL; 
static sem_t *clients_to_doors_sem = NULL; 
static int doors_to_clients_shmid = -1; 
static HubShmLayout_t *doors_to_clients_ptr = NULL; 
static sem_t *doors_to_clients_sem = NULL; 

static void ipc_loop(void)
{
    static DoorPacket_t packet_buff = {0};

    sem_wait(clients_to_doors_sem);

    if (clients_to_doors_ptr->state == SHMSTATE_DIRTY)
    {
        bzero(&packet_buff, sizeof(packet_buff));
        memcpy(&packet_buff, &clients_to_doors_ptr->content, sizeof(DoorPacket_t));
        clients_to_doors_ptr->state = SHMSTATE_CLEAN;
        sem_post(clients_to_doors_sem);
        i2c_forward_request(&packet_buff);
    }
    else
    {
        sem_post(clients_to_doors_sem);
        sleep(1);
    }

    if (hub_queue_dequeue(doors_to_clients_queue, &packet_buff) >= 0)
    {
        bool sent = false;

        while(!sent)
        {
            syslog_append("Trying to forward from queue to shm.");
            sem_wait(doors_to_clients_sem);
            syslog_append("DTC sem acquired");

            if (doors_to_clients_ptr->state == SHMSTATE_CLEAN)
            {
                syslog_append("Clean shm, writing !");
                memcpy(&doors_to_clients_ptr->content, &packet_buff, sizeof(DoorPacket_t));
                doors_to_clients_ptr->state = SHMSTATE_DIRTY;
                sent = true;
            }
            else
            {
                syslog_append("Dirty shm...");
            }

            sem_post(doors_to_clients_sem);
            syslog_append("DTC sem released");
            sleep(1);
        }
    }
}

void ipc_terminate(void)
{
}

void* ipc_task(void *arg)
{
    for(;;) ipc_loop();
    return NULL;
}

void ipc_init(void)
{
    syslog_append("Initializing IPC");

    clients_to_doors_shmid = shmget(CLIENTS_TO_DOORS_SHM_KEY, SHM_PACKET_TOTAL_SIZE, IPC_CREAT | 0666);

    if (clients_to_doors_shmid < 0)
    {
        perror("Failed to get shm with key 324");
        syslog_append("Failed to get shm with key 324");
        common_terminate(EXIT_FAILURE);
    }

    doors_to_clients_shmid = shmget(DOORS_TO_CLIENTS_SHM_KEY, SHM_PACKET_TOTAL_SIZE, IPC_CREAT | 0666);

    if (doors_to_clients_shmid < 0)
    {
        perror("Failed to get shm with key 423");
        syslog_append("Failed to get shm with key 423");
        common_terminate(EXIT_FAILURE);
    }

    clients_to_doors_sem = sem_open(CLIENTS_TO_DOORS_SHM_SEM, O_CREAT, 0600, 0);

    if (clients_to_doors_sem == SEM_FAILED)
    {
        perror("Failed to open sem for shm 324");
        syslog_append("Failed to open sem for shm 324");
        common_terminate(EXIT_FAILURE);
    }

    doors_to_clients_sem = sem_open(DOORS_TO_CLIENTS_SHM_SEM, O_CREAT, 0600, 0);

    if (doors_to_clients_sem == SEM_FAILED)
    {
        perror("Failed to open sem for shm 423");
        syslog_append("Failed to open sem for shm 423");
        common_terminate(EXIT_FAILURE);
    }

    clients_to_doors_ptr = (HubShmLayout_t *)shmat(clients_to_doors_shmid, NULL, 0);

    if (clients_to_doors_ptr == NULL)
    {
        perror("Failed to acquire pointer for shm 324");
        syslog_append("Failed to acquire pointer for shm 324");
        common_terminate(EXIT_FAILURE);
    }

    doors_to_clients_ptr = (HubShmLayout_t *)shmat(doors_to_clients_shmid, NULL, 0);

    if (doors_to_clients_ptr == NULL)
    {
        perror("Failed to acquire pointer for shm 423");
        syslog_append("Failed to acquire pointer for shm 423");
        common_terminate(EXIT_FAILURE);
    }

    syslog_append("IPC Initialization Complete");

}
