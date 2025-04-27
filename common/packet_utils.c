/*
 * packet_utils.c
 *
 *  Created on: Apr 26, 2025
 *      Author: mickey
 */

#include "packet_utils.h"

// date encoding: YYYY.YYYM.MMMD.DDDD
// time encoding: xxxx.xxxx.xxxx.xxxH.HHHH.MMMM.MMSS.SSSS

uint16_t packet_encode_date(uint8_t year, uint8_t month, uint8_t day)
{
	return (day | (month << 5) | (year << 9));
}

uint32_t packet_encode_time(uint8_t hour, uint8_t minutes, uint8_t seconds)
{
	return (seconds | (minutes << 6) | (hour << 12));
}

uint8_t packet_decode_year(uint16_t date)
{
	return (date >> 9) & 127;
}

uint8_t packet_decode_month(uint16_t date)
{
	return (date >> 5) & 15;
}

uint8_t packet_decode_day(uint16_t date)
{
	return date & 31;
}

uint8_t packet_decode_hour(uint32_t time)
{
	return (time >> 12) & 31;
}

uint8_t packet_decode_minutes(uint32_t time)
{
	return (time >> 6) & 63;
}

uint8_t packet_decode_seconds(uint32_t time)
{
	return time & 63;
}

void door_pw_to_str(uint16_t door_pw, char *str_buff)
{
	str_buff[0] = '0' + (door_pw & 15);
	str_buff[1] = '0' + ((door_pw >> 4) & 15);
	str_buff[2] = '0' + ((door_pw >> 8) & 15);
	str_buff[3] = '0' + ((door_pw >> 12) & 15);
}

