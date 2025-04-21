#include "common.h"
#include "i2c_common.h"

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>

#include "../door-control/Common/packet_defs.h"
#include "../door-control/Common/i2c_register_defs.h"

static const char *device_path = "/dev/bone/i2c/2";
static const uint16_t slave_addr = 0x0a;
static const uint16_t polling_interval_sec = 5;

static int device_fd = 0;
static uint8_t rx_buff[256] = {0};

static void i2c_init(void)
{
	// open the i2c device file
	device_fd = open(device_path, O_RDWR, S_IWUSR);
	// assign the slave device address
	ioctl(device_fd, I2C_SLAVE, slave_addr);
}

static void i2c_master_write(const uint8_t reg_addr, const uint8_t *message, const uint8_t len)
{
	// prepare the outgoing message
	uint8_t tx_buff[256] = {0};
	tx_buff[0] = reg_addr;
	strncpy(tx_buff+1, message, sizeof(tx_buff)-1);
	// write to device and close
	write(device_fd, tx_buff, len+1);
}

static void i2c_master_read(const uint8_t reg_addr, const uint8_t len)
{
	for (int i = 0; i < len; i++)
	{
		rx_buff[i] = i2c_smbus_read_byte_data(device_fd, reg_addr+i);
	}
}

static void poll_slave_event_queue(void)
{
	static const uint32_t timeout_sec = 30;
	static const uint9_t zero = 0;

	uint32_t idle_counter = 0;

	printf("Starting polling.\n");

	while(idle_counter < timeout_sec)
	{
		sleep(polling_interval_sec);
		idle_counter += polling_interval_sec;

		i2c_master_read(0, 1);

		if (rx_buff[0] > 0)
		{
			uint8_t queue_length = rx_buff[0];
			printf("Reported %u new events in slave queue! Reading...\n", queue_length);
			i2c_master_read(1, queue_length);

			printf("Events retrieved. Ordering slave device to reset its local queue...\n");
			i2c_master_write(0, &zero, 1);

			printf("The following events have been newly reported:\n");

			for (int i = 0; i < queue_length; i++)
			{
				printf("Report: button pressed %u times.\n", rx_buff[i]);
			}

			printf("Report concluded, polling resumed.\n");

			idle_counter = 0;
		}
	}

	printf("Idle timeout exceeded, terminating.\n");
}

int main(void)
{
	i2c_init();
	poll_slave_event_queue();
	close(device_fd);
}
