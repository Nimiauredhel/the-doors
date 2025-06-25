#include "hub_common.h"

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>

#include "packet_defs.h"
#include "packet_utils.h"
#include "i2c_register_defs.h"

#define TARGET_ADDR_OFFSET 0x30
#define TARGET_ADDR_MAX_COUNT 0x0F

static const char *device_path = "/dev/bone/i2c/2";
static const uint8_t polling_interval_sec = 1;

static pthread_t i2c_thread;
static pthread_t ipc_thread;

static int device_fd = -1;
static int clients_to_doors_shmid = -1; 
static HubShmLayout_t *clients_to_doors_ptr = NULL; 
static sem_t *clients_to_doors_sem = NULL; 
static int doors_to_clients_shmid = -1; 
static HubShmLayout_t *doors_to_clients_ptr = NULL; 
static sem_t *doors_to_clients_sem = NULL; 

static HubQueue_t *doors_to_clients_queue;

static bool target_addr_set[TARGET_ADDR_MAX_COUNT] = {false};
static uint8_t target_addr_list[TARGET_ADDR_MAX_COUNT] = {0};

static uint8_t target_addr_last_count = 0;

static uint8_t rx_buff[sizeof(DoorPacket_t) + DOOR_DATA_BYTES_LARGE] = {0};
static DoorPacket_t *packet_ptr = (DoorPacket_t *)&rx_buff;

static void terminate(int ret)
{
    if (device_fd > 0)
    {
        close(device_fd);
    }

    if (doors_to_clients_queue != NULL)
    {
        hub_queue_destroy(doors_to_clients_queue);
    }

    exit(ret);
}

static void i2c_init(void)
{
	char buff[64] = {0};

	system("sh i2c_init.sh");

	sprintf(buff, "Opening I2C device at path %s", device_path);
	syslog_append(buff);

	// open the i2c device file
	device_fd = open(device_path, O_RDWR, S_IWUSR);

	if (device_fd <= 0)
	{
		sprintf(buff, "Error opening I2C device, code: %d", device_fd);
		syslog_append(buff);
        terminate(EXIT_FAILURE);
	}
	else
	{
		sprintf(buff, "Successfully opened I2C device, descriptor: %d", device_fd);
		syslog_append(buff);
	}
}

static void i2c_set_target(uint8_t addr)
{
	// assign the slave device address
	ioctl(device_fd, I2C_SLAVE, addr);
}

static void i2c_master_write(const uint8_t reg_addr, const uint8_t *message, const uint8_t len)
{
	int32_t result = i2c_smbus_write_i2c_block_data(device_fd, reg_addr, len, message);
	if (result != 0) perror("I2C send error");
	//else printf("Sent %ld bytes.\n", len);
}

static int32_t i2c_master_read(const uint8_t reg_addr, const uint8_t len)
{
	int32_t bytes_read = i2c_smbus_read_i2c_block_data(device_fd, reg_addr, len, rx_buff);
	//printf("Read %ld bytes.\n", bytes_read);
    return bytes_read;
}

static void read_data_from_door(uint16_t length)
{
	int32_t bytes_read = i2c_master_read(I2C_REG_DATA, length);
}

static void process_data_from_door(void)
{
    char debug_buff[32] = {0};
    char syslog_buff[128] = {0};

    DoorDataType_t data_type = packet_ptr->body.Data.data_type;
    void *data_ptr = packet_ptr + sizeof(DoorPacket_t);

    switch(data_type)
    {
    case PACKET_DATA_DOOR_INFO:
        sprintf(syslog_buff, "Received door info, door name: %s.",  ((DoorInfo_t *)data_ptr)->name);
        ((DoorInfo_t *)data_ptr)->index = packet_ptr->body.Data.source_id;
        //hub_queue_enqueue(doors_to_clients_queue, packet_ptr);
	break;
    case PACKET_DATA_NONE:
    case PACKET_DATA_CLIENT_INFO:
    case PACKET_DATA_IMAGE:
    case PACKET_DATA_MAX:
    default:
        break;
    }

    syslog_append(syslog_buff);
}

static void send_request_to_door(DoorRequest_t request, uint32_t extra_data, uint16_t priority)
{
	DoorPacket_t request_buffer;
	struct tm dt = get_datetime();

	request_buffer.header.category = PACKET_CAT_REQUEST;
	request_buffer.header.priority = priority;
	request_buffer.header.date =
		packet_encode_date(dt.tm_year-100, dt.tm_mon+1, dt.tm_mday);
	request_buffer.header.time =
		packet_encode_time(dt.tm_hour, dt.tm_min, dt.tm_sec);

	request_buffer.body.Request.request_id = request;
	request_buffer.body.Request.request_data_32 = extra_data;

	i2c_master_write(I2C_REG_HUB_COMMAND, (const uint8_t *)&request_buffer, sizeof(DoorPacket_t));
}

static void process_report(void)
{
	uint16_t idx_temp;
	char debug_buff[32] = {0};
	char syslog_buff[128] = {0};

    DoorReport_t report_type = packet_ptr->body.Report.report_id;

    switch(report_type)
    {
    case PACKET_REPORT_NONE:
         sprintf(syslog_buff, "Door event NONE.");
         break;
    case PACKET_REPORT_DOOR_OPENED:
         sprintf(syslog_buff, "Door Opened");
         break;
    case PACKET_REPORT_DOOR_CLOSED:
         sprintf(syslog_buff, "Door Closed");
         break;
    case PACKET_REPORT_DOOR_BLOCKED:
        sprintf(syslog_buff, "Door Blocked for %u seconds",
        packet_ptr->body.Report.report_data_32);
        break;
    case PACKET_REPORT_PASS_CORRECT:
        bzero(debug_buff, sizeof(debug_buff));
        door_pw_to_str(packet_ptr->body.Report.report_data_16, debug_buff);
        sprintf(syslog_buff, "Correct Password Entered: %s.", debug_buff);
        break;
    case PACKET_REPORT_PASS_WRONG:
        bzero(debug_buff, sizeof(debug_buff));
        door_pw_to_str(packet_ptr->body.Report.report_data_16, debug_buff);
        sprintf(syslog_buff, "Wrong Password Entered: %s.", debug_buff);
        break;
    case PACKET_REPORT_PASS_CHANGED:
        bzero(debug_buff, sizeof(debug_buff));
        door_pw_to_str(packet_ptr->body.Report.report_data_16, debug_buff);
        debug_buff[4] = '-';
        debug_buff[5] = '>';
        door_pw_to_str(packet_ptr->body.Report.report_data_32, debug_buff+6);
        sprintf(syslog_buff, "Password Changed. %s", debug_buff);
        break;
    case PACKET_REPORT_QUERY_RESULT:
        sprintf(syslog_buff, "Query Result [%u][%u]", packet_ptr->body.Report.report_data_16, packet_ptr->body.Report.report_data_32);
        break;
    case PACKET_REPORT_DATA_READY:
        sprintf(syslog_buff, "Data Ready.");
	idx_temp = packet_ptr->body.Report.source_id;
	read_data_from_door(sizeof(DoorPacket_t) + packet_ptr->body.Data.data_length);
	packet_ptr->body.Data.source_id = idx_temp;
	process_data_from_door();
        break;
    case PACKET_REPORT_ERROR:
        sprintf(syslog_buff, "Error Code [%u][%u]", packet_ptr->body.Report.report_data_16, packet_ptr->body.Report.report_data_32);
        break;
    case PACKET_REPORT_TIME_SET:
        sprintf(syslog_buff, "Time/Date Set to [%u|%u]: %02u/%02u/%02u %02u:%02u:%02u.",
        packet_ptr->body.Report.report_data_16,
        packet_ptr->body.Report.report_data_32,
        packet_decode_day(packet_ptr->body.Report.report_data_16),
        packet_decode_month(packet_ptr->body.Report.report_data_16),
        packet_decode_year(packet_ptr->body.Report.report_data_16),
        packet_decode_hour(packet_ptr->body.Report.report_data_32),
        packet_decode_minutes(packet_ptr->body.Report.report_data_32),
        packet_decode_seconds(packet_ptr->body.Report.report_data_32));
        break;
    case PACKET_REPORT_FRESH_BOOT:
        sprintf(syslog_buff, "Fresh Boot. Sending time sync!");
        send_request_to_door(PACKET_REQUEST_SYNC_TIME, 0, 1);
        break;
    default:
        sprintf(syslog_buff, "Unknown Door Event");
        break;
    }

    syslog_append(syslog_buff);
}

static void process_request_from_door(void)
{
	char debug_buff[32] = {0};
	char syslog_buff[128] = {0};

    DoorRequest_t request_type = packet_ptr->body.Request.request_id;

    switch(request_type)
    {
    case PACKET_REQUEST_BELL:
        sprintf(syslog_buff, "Forwarding bell from door %u to client %u.", packet_ptr->body.Request.source_id, packet_ptr->body.Request.destination_id);
        hub_queue_enqueue(doors_to_clients_queue, packet_ptr);
        break;
    case PACKET_REQUEST_SYNC_TIME:
        break;
    case PACKET_REQUEST_SYNC_PASS_USER:
        break;
    case PACKET_REQUEST_SYNC_PASS_ADMIN:
        break;
    case PACKET_REQUEST_PING:
        break;
    case PACKET_REQUEST_NONE:
    case PACKET_REQUEST_DOOR_OPEN:
    case PACKET_REQUEST_DOOR_CLOSE:
    case PACKET_REQUEST_PHOTO:
    case PACKET_REQUEST_RESET_ADDRESS:
    case PACKET_REQUEST_MAX:
    default:
      break;
    }

    syslog_append(syslog_buff);
}

static void poll_slave_event_queue(void)
{
	static const uint8_t zero = 0;

	char debug_buff[32] = {0};
	char syslog_buff[128] = {0};

	//syslog_append("Starting polling");

    sleep(polling_interval_sec);

    for (int i = 0; i < target_addr_last_count; i++)
    {
        i2c_set_target(target_addr_list[i]);

        bzero(rx_buff, sizeof(rx_buff));

        // read event queue length
        // TODO: make this a uint16_t value?
        int32_t bytes_read = i2c_master_read(I2C_REG_EVENT_COUNT, 1);
        uint8_t queue_length = rx_buff[0];
    //sprintf(syslog_buff, "Read %ld bytes from address [0x%X] event count %u", bytes_read, target_addr_list[i], queue_length);
    //syslog_append(syslog_buff);


        if (bytes_read <= 0 || queue_length <= 0) continue;
    //		printf("Reported %u new events in slave queue! Reading...\n", queue_length);

        for (int j = 0; j < queue_length; j++)
        {
            bzero(rx_buff, sizeof(rx_buff));
            i2c_master_read(I2C_REG_EVENT_HEAD | (j << 1), sizeof(DoorPacket_t));

            /*
            printf("[%02u/%02u/%02u][%02u:%02u:%02u]: ",
                    packet_decode_day(packet_ptr->header.date),
                    packet_decode_month(packet_ptr->header.date),
                    packet_decode_year(packet_ptr->header.date),
                    packet_decode_hour(packet_ptr->header.time),
                    packet_decode_minutes(packet_ptr->header.time),
                    packet_decode_seconds(packet_ptr->header.time));
                    */


            switch(packet_ptr->header.category)
            {
            case PACKET_CAT_REPORT:
	        packet_ptr->body.Report.source_id = i;
                process_report();
                break;
            case PACKET_CAT_REQUEST:
	        packet_ptr->body.Request.source_id = i;
                process_request_from_door();
                break;
            case PACKET_CAT_DATA:
		// shouldn't happen
	        packet_ptr->body.Data.source_id = i;
                process_data_from_door();
                break;
            case PACKET_CAT_NONE:
            case PACKET_CAT_MAX:
            default:
              break;
            }

        }

        //printf("Events retrieved. Ordering slave device to reset its local queue...\n");
        i2c_master_write(I2C_REG_EVENT_COUNT, &zero, 1);
        //printf("Report concluded, polling resumed.\n");
    }
}

static void scan_i2c_bus(void)
{
    char syslog_buff[64] = {0};
    int32_t read_bytes = 0;

    target_addr_last_count = 0;
    explicit_bzero(target_addr_set, TARGET_ADDR_MAX_COUNT);
    explicit_bzero(target_addr_list, TARGET_ADDR_MAX_COUNT);

    sprintf(syslog_buff, "Scanning I2C bus for target devices.");
    syslog_append(syslog_buff);

    for (uint8_t i = 0; i < TARGET_ADDR_MAX_COUNT; i++)
    {
        uint8_t addr = i+TARGET_ADDR_OFFSET;
        i2c_set_target(addr);
        read_bytes = i2c_master_read(I2C_REG_EVENT_COUNT, 1);

	//    sprintf(syslog_buff, "Addr [0x%X], read bytes: %d", addr, read_bytes);
	//    syslog_append(syslog_buff);

        if (read_bytes > 0)
        {
            target_addr_set[i] = true;
            target_addr_list[target_addr_last_count] = addr;
            sprintf(syslog_buff, "Detected target device at address [0x%X].", target_addr_list[target_addr_last_count]);
            syslog_append(syslog_buff);
            target_addr_last_count += 1;
        }
    }

    if (target_addr_last_count == 0)
    {
	    sprintf(syslog_buff, "Concluded I2C bus scan, no devices detected.");
	    syslog_append(syslog_buff);
	    exit(EXIT_FAILURE);
    }

    sprintf(syslog_buff, "Concluded I2C bus scan, %d devices detected.", target_addr_last_count);
    syslog_append(syslog_buff);
}

static void forward_request(DoorPacket_t *request)
{
    syslog_append("Forwarding request to door");
    if (target_addr_last_count > 0)
    {
        i2c_set_target(target_addr_list[0]);
    }
    i2c_master_write(I2C_REG_HUB_COMMAND, (const uint8_t *)request, sizeof(DoorPacket_t));
}

static void ipc_loop(void)
{
    DoorPacket_t packet_buff = {0};

    sem_wait(clients_to_doors_sem);
    if (clients_to_doors_ptr->state == SHMSTATE_DIRTY)
    {
        memcpy(&packet_buff, &clients_to_doors_ptr->content, sizeof(DoorPacket_t));
        clients_to_doors_ptr->state = SHMSTATE_CLEAN;
        sem_post(clients_to_doors_sem);
        forward_request(&packet_buff);
    }
    else
    {
        sem_post(clients_to_doors_sem);
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
	    else syslog_append("Dirty shm...");
            sem_post(doors_to_clients_sem);
	    syslog_append("DTC sem released");
	    sleep(1);
        }
    }
}

static void i2c_loop(void)
{
    static const uint16_t rescan_threshold = 100;
    static uint16_t rescan_counter = 0;

    if (rescan_counter <= 0)
    {
        rescan_counter = rescan_threshold;
        scan_i2c_bus();
    }
    else
    {
        rescan_counter--;
    }

    poll_slave_event_queue();
}

static void* i2c_task(void *arg)
{
    for(;;) i2c_loop();
    return NULL;
}

static void* ipc_task(void *arg)
{
    for(;;) ipc_loop();
    return NULL;
}

static void ipc_init(void)
{
    syslog_append("Initializing IPC");

    clients_to_doors_shmid = shmget(CLIENTS_TO_DOORS_SHM_KEY, SHM_PACKET_TOTAL_SIZE, IPC_CREAT | 0666);

    if (clients_to_doors_shmid < 0)
    {
        perror("Failed to get shm with key 324");
        syslog_append("Failed to get shm with key 324");
        exit(EXIT_FAILURE);
    }

    doors_to_clients_shmid = shmget(DOORS_TO_CLIENTS_SHM_KEY, SHM_PACKET_TOTAL_SIZE, IPC_CREAT | 0666);

    if (doors_to_clients_shmid < 0)
    {
        perror("Failed to get shm with key 423");
        syslog_append("Failed to get shm with key 423");
        exit(EXIT_FAILURE);
    }

    clients_to_doors_sem = sem_open(CLIENTS_TO_DOORS_SHM_SEM, O_CREAT, 0600, 0);

    if (clients_to_doors_sem == SEM_FAILED)
    {
        perror("Failed to open sem for shm 324");
        syslog_append("Failed to open sem for shm 324");
        exit(EXIT_FAILURE);
    }

    doors_to_clients_sem = sem_open(DOORS_TO_CLIENTS_SHM_SEM, O_CREAT, 0600, 0);

    if (doors_to_clients_sem == SEM_FAILED)
    {
        perror("Failed to open sem for shm 423");
        syslog_append("Failed to open sem for shm 423");
        exit(EXIT_FAILURE);
    }

    clients_to_doors_ptr = (HubShmLayout_t *)shmat(clients_to_doors_shmid, NULL, 0);

    if (clients_to_doors_ptr == NULL)
    {
        perror("Failed to acquire pointer for shm 324");
        syslog_append("Failed to acquire pointer for shm 324");
        exit(EXIT_FAILURE);
    }

    doors_to_clients_ptr = (HubShmLayout_t *)shmat(doors_to_clients_shmid, NULL, 0);

    if (doors_to_clients_ptr == NULL)
    {
        perror("Failed to acquire pointer for shm 423");
        syslog_append("Failed to acquire pointer for shm 423");
        exit(EXIT_FAILURE);
    }

    doors_to_clients_queue = hub_queue_create(128);

    if (doors_to_clients_queue == NULL)
    {
        perror("Failed to create Doors to Clients Queue");
        syslog_append("Failed to create Doors to Clients Queue");
        exit(EXIT_FAILURE);
    }

    syslog_append("IPC Initialization Complete");

}

int main(void)
{
    char buff[64] = {0};

    syslog_init("Hub Door Manager");
    initialize_signal_handler();
    sprintf(buff, "Starting Door Manager, PID %u", getpid());
    syslog_append(buff);
    ipc_init();
    i2c_init();
    pthread_create(&i2c_thread, NULL, i2c_task, NULL);
    pthread_create(&ipc_thread, NULL, ipc_task, NULL);

    for(;;);
}
