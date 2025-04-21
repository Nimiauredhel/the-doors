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
	PACKET_REPORT_PASS_CORRECT = 3,
	PACKET_REPORT_PASS_WRONG = 4,
	PACKET_REPORT_PASS_CHANGED = 5,
	PACKET_REPORT_DATA_READY = 6,
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

typedef struct DoorPacketHeader
{
	uint16_t version;
	uint32_t timestamp;
	uint8_t category;
} DoorPacketHeader_t;

typedef union DoorPacketBody
{
	// the "stub" fields are here as a reminder that the first two packet types -
	// - are already as big as the third, and can have extra fields at no cost later
#pragma pack(push, 1)
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
#pragma pack(pop)
} DoorPacketBody_t;

typedef struct DoorPacket
{
	DoorPacketHeader_t header;
	DoorPacketBody_t body;
} DoorPacket_t;

#endif /* PACKET_DEFS_H_ */
