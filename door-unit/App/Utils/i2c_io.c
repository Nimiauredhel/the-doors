/*
 * i2c_io.c
 *
 *  Created on: Apr 21, 2025
 *      Author: mickey
 */

#include "i2c_io.h"

volatile uint32_t i2c_addr = 0x00;
volatile uint32_t new_i2c_addr = 0x00;

static volatile uint8_t i2c_direction = 0;
static volatile uint32_t i2c_addr_hit_counter = 0;

static volatile bool i2c_enabled = false;

static uint16_t i2c_rx_count = 0;
static uint16_t i2c_tx_count = 0;

static I2CRegisterDefinition_t i2c_tx_register;
static uint8_t *i2c_tx_position;

static uint8_t i2c_tx_buff[I2C_TX_BUFF_SIZE] = {0};
static uint8_t i2c_rx_buff[I2C_RX_BUFF_SIZE] = {0};

static DoorPacket_t i2c_query_result_register = {0};

uint32_t i2c_get_addr_hit_counter(void)
{
	uint32_t ret = i2c_addr_hit_counter;
	if (i2c_addr_hit_counter > 0) i2c_addr_hit_counter -= 1;
	return ret;
}

/*
static uint16_t register_definition_to_packet_size(I2CRegisterDefinition_t reg_def)
{
	switch (reg_def)
	{
	case I2C_REG_EVENT_COUNT:
		return 2;
	case I2C_REG_DATA:
		// TODO: implement actual data size
		return sizeof(DoorPacket_t);
	case I2C_REG_EVENT_HEAD:
	case I2C_REG_QUERY_RESULT:
	case I2C_REG_HUB_COMMAND:
	default:
		return sizeof(DoorPacket_t);
	}
}
*/

static uint8_t* register_definition_to_pointer(I2CRegisterDefinition_t reg_def, uint8_t offset)
{
	switch (reg_def)
	{
	case I2C_REG_EVENT_COUNT:
		return (uint8_t *)event_log_get_length_ptr();
	case I2C_REG_EVENT_HEAD:
		return (uint8_t *)event_log_get_entry(offset);
		// TODO: implement missing regs
	case I2C_REG_QUERY_RESULT:
		return NULL;
	case I2C_REG_HUB_COMMAND:
		return NULL;
	case I2C_REG_DATA:
		return NULL;
	default:
		return NULL;
	}
}

static void process_i2c_rx(I2C_HandleTypeDef *hi2c)
{
	I2CRegisterDefinition_t reg_def = i2c_rx_buff[0];

	switch(reg_def)
	{
	case I2C_REG_EVENT_COUNT:
		comms_report_internal(COMMS_EVENT_RECEIVED, I2C_REG_EVENT_COUNT);
		event_log_clear();
		break;
	case I2C_REG_HUB_COMMAND:
		comms_report_internal(COMMS_EVENT_RECEIVED, I2C_REG_HUB_COMMAND);
		comms_enqueue_command((DoorPacket_t *)(i2c_rx_buff+1));
		break;
	default:
		comms_report_internal(COMMS_EVENT_RECEIVED, I2C_REG_UNKNOWN);
		event_log_append(PACKET_CAT_REPORT, PACKET_REPORT_ERROR, PACKET_ERROR_WRONG_REGISTER, 0);
		break;
	}

	/*
	int num_regs = i2c_rx_count - 1;
	int end_reg = num_regs - 1;

	if (end_reg > 63)
	{
		Error_Handler();
	}

	for (int idx = 0; idx < num_regs; idx++)
	{
		*((uint8_t*)(&i2c_hub_command_register)+idx) = i2c_rx_buff[idx+1];
	}
	*/

	bzero(i2c_rx_buff, I2C_RX_BUFF_SIZE);
	i2c_rx_count = 0;

	HAL_I2C_EnableListen_IT(hi2c);
}

static void process_i2c_tx(I2C_HandleTypeDef *hi2c)
{
	i2c_tx_count = 0;
	comms_report_internal(COMMS_EVENT_SENT, i2c_tx_register);
}

void i2c_io_apply_new_addr(void)
{
	i2c_enabled = false;
	while(HAL_I2C_STATE_BUSY == HAL_I2C_GetState(&hi2c1));
	while(HAL_OK != HAL_I2C_DisableListen_IT(&hi2c1));
	while(HAL_OK != HAL_I2C_DeInit(&hi2c1));
	hi2c1.Init.OwnAddress1 = new_i2c_addr;
	i2c_addr = new_i2c_addr;
	HAL_I2C_Init(&hi2c1);
	while(HAL_OK != HAL_I2C_Init(&hi2c1));
	while(HAL_OK != HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE));
	while(HAL_OK != HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0));
	bzero(i2c_rx_buff, I2C_RX_BUFF_SIZE);
	i2c_rx_count = 0;
	i2c_enabled = true;
	while(HAL_OK != HAL_I2C_EnableListen_IT(&hi2c1));
}

void HAL_I2C_SlaveTxCpltCallback (I2C_HandleTypeDef * hi2c)
{
	if (!i2c_enabled) return;

	i2c_tx_count++;

	if (i2c_tx_count < I2C_TX_BUFF_SIZE)
	{
		if (i2c_tx_count == I2C_TX_BUFF_SIZE - 1)
		{
			HAL_I2C_Slave_Seq_Transmit_DMA(hi2c, i2c_tx_position+i2c_tx_count, 1, I2C_LAST_FRAME);
		}
		else
		{
			HAL_I2C_Slave_Seq_Transmit_DMA(hi2c, i2c_tx_position+i2c_tx_count, 1, I2C_NEXT_FRAME);
		}
	}
	else if (i2c_tx_count == I2C_TX_BUFF_SIZE)
	{
		process_i2c_tx(hi2c);
	}
}

void HAL_I2C_SlaveRxCpltCallback (I2C_HandleTypeDef * hi2c)
{
	if (!i2c_enabled) return;

	if (new_i2c_addr != i2c_addr)
	{
		i2c_io_apply_new_addr();
	}

	i2c_rx_count++;

	if (i2c_rx_count < I2C_RX_BUFF_SIZE)
	{
		if (i2c_rx_count == I2C_RX_BUFF_SIZE - 1)
		{
			HAL_I2C_Slave_Seq_Receive_DMA(hi2c, i2c_rx_buff+i2c_rx_count, 1, I2C_LAST_FRAME);
		}
		else
		{
			HAL_I2C_Slave_Seq_Receive_DMA(hi2c, i2c_rx_buff+i2c_rx_count, 1, I2C_NEXT_FRAME);
		}
	}
	else if (i2c_rx_count == I2C_RX_BUFF_SIZE)
	{
		process_i2c_rx(hi2c);
	}
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef * hi2c)
{
	if (!i2c_enabled) return;

	if (new_i2c_addr != i2c_addr)
	{
		i2c_io_apply_new_addr();
	}

	HAL_I2C_EnableListen_IT(hi2c);
}

extern void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
	if (!i2c_enabled) return;

	i2c_direction = TransferDirection;
	i2c_addr_hit_counter = 100;

	if(i2c_direction == I2C_DIRECTION_TRANSMIT)  // if the master wants to transmit the data
	{
		i2c_rx_buff[0] = 0;
		i2c_rx_count = 0;
		HAL_I2C_Slave_Sequential_Receive_DMA(hi2c, i2c_rx_buff, 1, I2C_FIRST_FRAME);
	}
	else
	{
		i2c_tx_register = i2c_rx_buff[0];
		uint8_t reg_offset = 0;

		// detect event log read and decode offset
		if (i2c_tx_register & I2C_REG_EVENT_HEAD)
		{
			reg_offset = i2c_tx_register >> 1;
			i2c_tx_register = I2C_REG_EVENT_HEAD;
		}

		i2c_tx_position = register_definition_to_pointer(i2c_tx_register, reg_offset);

		HAL_I2C_Slave_Sequential_Transmit_DMA(hi2c, i2c_tx_position, 1, I2C_FIRST_FRAME);

		i2c_rx_buff[0] = 0;
		i2c_tx_count = 0;
	}
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	if (!i2c_enabled) return;

	uint32_t error_code = HAL_I2C_GetError(hi2c);

	if (error_code == 4)  // AF error
	{
		if (i2c_direction == I2C_DIRECTION_TRANSMIT)
		{
			process_i2c_rx(hi2c);
		}
		else
		{
			process_i2c_tx(hi2c);
		}
	}
	/* BERR Error commonly occurs during the Direction switch
	* Here we the software reset bit is set by the HAL error handler
	* Before resetting this bit, we make sure the I2C lines are released and the bus is free
	* I am simply reinitializing the I2C to do so
	*/
	else if (error_code == 1)  // BERR Error
	{
		HAL_I2C_DeInit(hi2c);
		HAL_I2C_Init(hi2c);
		bzero(i2c_rx_buff, I2C_RX_BUFF_SIZE);
		i2c_rx_count = 0;
	}

	HAL_I2C_EnableListen_IT(hi2c);
}

void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *hi2c)
{
	if (!i2c_enabled) return;

	if (new_i2c_addr != i2c_addr)
	{
		i2c_io_apply_new_addr();
	}

	HAL_I2C_EnableListen_IT(hi2c);
}

void i2c_io_init(void)
{
	i2c_addr = hi2c1.Init.OwnAddress1;
	new_i2c_addr = hi2c1.Init.OwnAddress1;
	i2c_enabled = true;
	HAL_I2C_EnableListen_IT(&hi2c1);
}

uint16_t i2c_io_get_device_id(void)
{
	return hi2c1.Init.OwnAddress1;
}
