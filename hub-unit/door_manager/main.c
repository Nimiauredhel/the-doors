#include "../hub_common.h"

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>

#include "packet_defs.h"
#include "packet_utils.h"
#include "i2c_register_defs.h"

static const char *device_path = "/dev/bone/i2c/2";
static const uint16_t slave_addr = 0x0a;
static const uint16_t polling_interval_sec = 1;

static int device_fd = 0;

static uint8_t rx_buff[255] = {0};
static DoorPacket_t packet_buff = {0};

static struct tm get_datetime(void)
{
    time_t rawtime;
    struct tm *timeinfo;
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    return *timeinfo;
}

static void i2c_init(void)
{
	// open the i2c device file
	device_fd = open(device_path, O_RDWR, S_IWUSR);
	// assign the slave device address
	ioctl(device_fd, I2C_SLAVE, slave_addr);
}

static void i2c_master_write(const uint8_t reg_addr, const uint8_t *message, const uint8_t len)
{
	int32_t result = i2c_smbus_write_i2c_block_data(device_fd, reg_addr, len, message);
	if (result != 0) perror("I2C send error");
	//else printf("Sent %ld bytes.\n", len);
}

static void i2c_master_read(const uint8_t reg_addr, const uint8_t len)
{
	int32_t bytes_read = i2c_smbus_read_i2c_block_data(device_fd, reg_addr, len, rx_buff);
	//printf("Read %ld bytes.\n", bytes_read);
}

static void send_request(DoorRequest_t request, uint32_t extra_data, uint16_t priority)
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

static void poll_slave_event_queue(void)
{
	static const uint32_t timeout_sec = 60;
	static const uint8_t zero = 0;

	char debug_buff[32] = {0};
	uint32_t idle_counter = 0;

	printf("Starting polling.\n");

	while(idle_counter < timeout_sec)
	{
		sleep(polling_interval_sec);
		bzero(rx_buff, sizeof(rx_buff));
		idle_counter += polling_interval_sec;

        // read event queue length, a uint16_t value
		i2c_master_read(I2C_REG_EVENT_COUNT, 1);
		uint8_t queue_length = rx_buff[0];

		if (queue_length <= 0) continue;
	//		printf("Reported %u new events in slave queue! Reading...\n", queue_length);

		for (int i = 0; i < queue_length; i++)
		{
			bzero(rx_buff, sizeof(rx_buff));
			i2c_master_read(I2C_REG_EVENT_HEAD | (i << 1), sizeof(DoorPacket_t));

			memcpy(&packet_buff, rx_buff, sizeof(DoorPacket_t));

			printf("[%02u/%02u/%02u][%02u:%02u:%02u]: ",
					packet_decode_day(packet_buff.header.date),
					packet_decode_month(packet_buff.header.date),
					packet_decode_year(packet_buff.header.date),
					packet_decode_hour(packet_buff.header.time),
					packet_decode_minutes(packet_buff.header.time),
					packet_decode_seconds(packet_buff.header.time));

			DoorReport_t report_type = packet_buff.body.Report.report_id;

			switch(report_type)
			{
			case PACKET_REPORT_NONE:
			       printf("NONE.\n");
			       break;
			case PACKET_REPORT_DOOR_OPENED:
			       printf("Door Opened.\n");
			       break;
			case PACKET_REPORT_DOOR_CLOSED:
			       printf("Door Closed.\n");
			       break;
			case PACKET_REPORT_DOOR_BLOCKED:
				printf("Door Blocked for %lu seconds.\n",
				packet_buff.body.Report.report_data_32);
			    break;
			case PACKET_REPORT_PASS_CORRECT:
			       bzero(debug_buff, sizeof(debug_buff));
			       door_pw_to_str(packet_buff.body.Report.report_data_16, debug_buff);
			       printf("Correct Password Entered: %s.\n", debug_buff);
			       break;
			case PACKET_REPORT_PASS_WRONG:
			       bzero(debug_buff, sizeof(debug_buff));
			       door_pw_to_str(packet_buff.body.Report.report_data_16, debug_buff);
				printf("Wrong Password Entered: %s.\n", debug_buff);
				break;
			case PACKET_REPORT_PASS_CHANGED:
				bzero(debug_buff, sizeof(debug_buff));
				door_pw_to_str(packet_buff.body.Report.report_data_16, debug_buff);
				debug_buff[4] = '-';
				debug_buff[5] = '>';
				door_pw_to_str(packet_buff.body.Report.report_data_32, debug_buff+6);
				printf("Password Changed. %s\n", debug_buff);
				break;
			case PACKET_REPORT_QUERY_RESULT:
				printf("Query Result [%u][%u]", packet_buff.body.Report.report_data_16, packet_buff.body.Report.report_data_32);
				break;
			case PACKET_REPORT_DATA_READY:
				printf("Data Ready.\n");
				break;
			case PACKET_REPORT_ERROR:
				printf("Error Code [%u][%u]\n", packet_buff.body.Report.report_data_16, packet_buff.body.Report.report_data_32);
				break;
			case PACKET_REPORT_TIME_SET:
				printf("Time/Date Set to [%u|%u]: %02u/%02u/%02u %02u:%02u:%02u.\n",
				packet_buff.body.Report.report_data_16,
				packet_buff.body.Report.report_data_32,
				packet_decode_day(packet_buff.body.Report.report_data_16),
				packet_decode_month(packet_buff.body.Report.report_data_16),
				packet_decode_year(packet_buff.body.Report.report_data_16),
				packet_decode_hour(packet_buff.body.Report.report_data_32),
				packet_decode_minutes(packet_buff.body.Report.report_data_32),
				packet_decode_seconds(packet_buff.body.Report.report_data_32));
				break;
			case PACKET_REPORT_FRESH_BOOT:
				printf("Fresh Boot. Sending time sync!\n");
				send_request(PACKET_REQUEST_SYNC_TIME, 0, 1);
				break;
			}
		}

	//	printf("Events retrieved. Ordering slave device to reset its local queue...\n");
		i2c_master_write(I2C_REG_EVENT_COUNT, &zero, 1);
		//printf("Report concluded, polling resumed.\n");

		idle_counter = 0;
	}

	printf("Idle timeout exceeded, terminating.\n");
}

int main(void)
{
	printf("Size of header: %d\n", sizeof(DoorPacketHeader_t));
	printf("Size of body: %d\n", sizeof(DoorPacketBody_t));
	printf("Size of packet: %d\n", sizeof(DoorPacket_t));
	i2c_init();
	sleep(1);
	poll_slave_event_queue();
	close(device_fd);
}
