/*
 * packet_defs.h
 *
 *  Created on: Apr 20, 2025
 *      Author: mickey
 */

#ifndef PACKET_DEFS_H_
#define PACKET_DEFS_H_

typedef enum DoorPacketCategory
{
	PACKET_CAT_NONE = 0,
	PACKET_CAT_REPORT = 1,
	PACKET_CAT_REQUEST = 2,
	PACKET_CAT_DATA = 3,
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
} DoorReport_t;

typedef enum DoorRequest
{
	PACKET_REQUEST_NONE = 0,
	PACKET_REQUEST_DOOR_OPEN = 1,
	PACKET_REQUEST_DOOR_CLOSE = 2,
	PACKET_REQUEST_BELL = 3,
	PACKET_REQUEST_PHOTO = 4,
} DoorRequest_t;

typedef enum DoorDataType
{
	PACKET_DATA_NONE = 0,
	PACKET_DATA_PHOTO = 1,
} DoorDataType_t;

typedef enum DoorErrorType
{
	PACKET_ERROR_NONE = 0,
	PACKET_ERROR_WRONG_REGISTER = 1,
	PACKET_ERROR_INVALID_PACKET = 2,
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
	// the "stub" fields are here as a reminder that the first two packet types -
	// - are already as big as the third, and can have extra fields at no cost later
	struct {
	} Empty;
	struct {
		DoorReport_t report_id;
		uint8_t source_id;
		uint8_t report_data_8;
		uint32_t report_data_32;
	} Report;
	struct {
		DoorRequest_t request_id;
		uint8_t source_id;
		uint8_t destination_id;
		uint32_t request_data_32;
	} Request;
	struct {
		DoorDataType_t data_type;
		uint16_t source_id;
		uint16_t destination_id;
		uint32_t data_length;
		uint8_t data[];
	} Data;
} DoorPacketBody_t;

typedef struct DoorPacket
{
	DoorPacketHeader_t header;
	DoorPacketBody_t body;
} DoorPacket_t;
#pragma pack(pop)

#endif /* PACKET_DEFS_H_ */
