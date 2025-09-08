#include "hub_control_ipc.h"
#include "hub_common.h"

static const struct mq_attr hub_mq_attributes =
{
    .mq_flags = 0,
    .mq_maxmsg = MQ_MSG_COUNT_MAX,
    .mq_msgsize = MQ_MSG_SIZE_MAX,
    .mq_curmsgs = 0,
};

void ipc_init(void)
{
    mqd_t mq_handle;

    mq_handle = mq_open(DOORS_TO_CLIENTS_QUEUE_NAME, O_CREAT | O_EXCL, 0666, &hub_mq_attributes);

    if (mq_handle < 0)
    {
        if (errno == EEXIST)
        {
            log_append("Queue 'doors to clients' already exists.");
        }
        else
        {
            perror("Failed to open or create 'doors to clients' queue");
            log_append("Failed to open or create 'doors to clients' queue");
            exit(EXIT_FAILURE);
        }
    }

    mq_close(mq_handle);

    mq_handle = mq_open(CLIENTS_TO_DOORS_QUEUE_NAME, O_CREAT | O_EXCL, 0666, &hub_mq_attributes);

    if (mq_handle < 0)
    {
        if (errno == EEXIST)
        {
            log_append("Queue 'clients to doors' already exists.");
        }
        else
        {
            perror("Failed to open or create 'clients to doors' queue");
            log_append("Failed to open or create 'clients to doors' queue");
            exit(EXIT_FAILURE);
        }
    }

    mq_close(mq_handle);
}
