/*
 * packet_defs.h
 *
 *  Created on: Apr 20, 2025
 *      Author: mickey
 */

#ifndef PACKET_DEFS_H_
#define PACKET_DEFS_H_

#include <stdint.h>
#include <stdbool.h>

// max size of data segment containing: door info, client info
#define DOOR_DATA_BYTES_SMALL (sizeof(ClientInfo_t))
// max size of data segment containing: image
#define DOOR_DATA_BYTES_LARGE (76800)

typedef enum DoorPacketCategory
{
	PACKET_CAT_NONE = 0,
	PACKET_CAT_REPORT = 1,
	PACKET_CAT_REQUEST = 2,
	PACKET_CAT_DATA = 3,
	PACKET_CAT_MAX = INT32_MAX,
} DoorPacketCategory_t;

typedef enum DoorReport
{
	PACKET_REPORT_NONE = 0,
	PACKET_REPORT_DOOR_OPENED = 1,
	PACKET_REPORT_DOOR_CLOSED = 2,
	PACKET_REPORT_DOOR_BLOCKED = 3,
	PACKET_REPORT_PASS_CORRECT = 4,
	PACKET_REPORT_PASS_WRONG = 5,
	PACKET_REPORT_PASS_CHANGED = 6,
	PACKET_REPORT_QUERY_RESULT = 7,
	PACKET_REPORT_DATA_READY = 8,
	PACKET_REPORT_ERROR = 9,
	PACKET_REPORT_TIME_SET = 10,
	PACKET_REPORT_FRESH_BOOT = 11,
	PACKET_REPORT_NEW_ADDRESS = 12,
	PACKET_REPORT_ADMIN_PASS_CORRECT = 13,
	PACKET_REPORT_ADMIN_PASS_WRONG = 14,
	PACKET_REPORT_PONG = 15,
	PACKET_REPORT_MAX = INT32_MAX,
} DoorReport_t;

typedef enum DoorRequest
{
	PACKET_REQUEST_NONE = 0,
	PACKET_REQUEST_DOOR_OPEN = 1,
	PACKET_REQUEST_DOOR_CLOSE = 2,
	PACKET_REQUEST_BELL = 3,
	PACKET_REQUEST_PHOTO = 4,
	PACKET_REQUEST_SYNC_TIME = 5,
	PACKET_REQUEST_SYNC_PASS_USER = 6,
	PACKET_REQUEST_SYNC_PASS_ADMIN = 7,
	PACKET_REQUEST_RESET_ADDRESS = 12,
	PACKET_REQUEST_PING = 15,
	PACKET_REQUEST_RESET_UNIT = 16,
	PACKET_REQUEST_FORCE_ADMIN = 16,
	PACKET_REQUEST_MAX = INT32_MAX,
} DoorRequest_t;

typedef enum DoorDataType
{
	PACKET_DATA_NONE = 0,
	PACKET_DATA_DOOR_INFO = 1,
	PACKET_DATA_CLIENT_INFO = 2,
	PACKET_DATA_IMAGE = 3,
	PACKET_DATA_MAX = INT32_MAX,
} DoorDataType_t;

typedef enum DoorErrorType
{
	PACKET_ERROR_NONE = 0,
	PACKET_ERROR_WRONG_REGISTER = 1,
	PACKET_ERROR_INVALID_PACKET = 2,
	PACKET_ERROR_MAX = INT32_MAX,
} DoorErrorType_t;

#pragma pack(push, 4)
typedef struct DoorPacketHeader
{
	uint32_t version;
	uint32_t time;
	uint16_t date;
	uint16_t priority;
	DoorPacketCategory_t category;
} DoorPacketHeader_t;

typedef union DoorPacketBody
{
	struct ReportBody
    {
		DoorReport_t report_id;
		uint16_t source_id;
		uint16_t report_data_16;
		uint32_t report_data_32;
	} Report;
	struct RequestBody
    {
		DoorRequest_t request_id;
		uint16_t source_id;
		uint16_t destination_id;
		uint32_t request_data_32;
	} Request;
	struct DataInfo
    {
		DoorDataType_t data_type;
		uint16_t source_id;
		uint16_t destination_id;
		uint32_t data_length;
	} Data;
} DoorPacketBody_t;

typedef struct DoorPacket
{
	DoorPacketHeader_t header;
	DoorPacketBody_t body;
} DoorPacket_t;

typedef struct DoorInfo
{
    uint16_t index;
    uint8_t i2c_address;
    char name[32];
} DoorInfo_t;

typedef struct ClientInfo
{
    uint16_t index;
    uint8_t mac_address[6];
    char name[32];
} ClientInfo_t;

#pragma pack(pop)

#endif /* PACKET_DEFS_H_ */
