/*
 * door_packet.h
 *
 *  Created on: Apr 20, 2025
 *      Author: mickey
 */

#ifndef DOOR_PACKET_H_
#define DOOR_PACKET_H_

typedef enum DoorPacketCategory
{
	PACKET_CAT_NONE = 0,
	PACKET_CAT_REPORT = 1,
	PACKET_CAT_REQUEST = 2,
	PACKET_CAT_DATA = 3,
} DoorPacketCategory_t;

typedef enum DoorPacketReport
{
	PACKET_REPORT_NONE = 0,
	PACKET_REPORT_DOOR_OPEN = 1,
	PACKET_REPORT_DOOR_CLOSE = 2,
	PACKET_REPORT_PASS_CORRECT = 3,
	PACKET_REPORT_PASS_WRONG = 4,
	PACKET_REPORT_DATA_READY = 5,
} DoorPacketReport_t;

typedef enum DoorPacketRequest
{
	PACKET_REQUEST_NONE = 0,
	PACKET_REQUEST_DOOR_OPEN = 1,
	PACKET_REQUEST_DOOR_CLOSE = 2,
	PACKET_REQUEST_BELL = 3,
	PACKET_REQUEST_PHOTO = 4,
} DoorPacketRequest_t;

typedef enum DoorPacketDataType
{
	PACKET_DATA_NONE = 0,
	PACKET_DATA_PHOTO = 1,
} DoorPacketDataType_t;

typedef struct DoorPacketHeader
{
	uint16_t version;
	uint32_t timestamp;
	uint8_t category;
} DoorPacketHeader_t;

typedef union DoorPacketBody
{
#pragma pack(push, 1)
	struct {
	} Empty;
	struct {
		DoorPacketReport_t report_id;
		uint16_t source_id;
	} Report;
	struct {
		DoorPacketRequest_t request_id;
		uint16_t source_id;
		uint16_t destination_id;
	} Request;
	struct {
		DoorPacketDataType_t data_type;
		uint16_t source_id;
		uint16_t destination_id;
		uint32_t data_length;
		uint8_t data[];
	} Data;
#pragma pack(pop)
} DoorPacketBody_t;

#endif /* DOOR_PACKET_H_ */
