#ifndef NETWORKING_COMMON_H
#define NETWORKING_COMMON_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>

#define PORT_BUFF_LENGTH 8
#define ADDRESS_BUFF_LENGTH 40
#define NAME_BUFF_LENGTH 32

/**
 * The permitted ranges for random ephemeral data port selection.
 */
#define CLIENT_DATA_PORT_MIN 50000
#define CLIENT_DATA_PORT_MAX 59999

/**
 * Conveniently wraps the inet_pton() function which converts an IP address
 * to its network binary representation and writes to an existing address struct.
 */
bool parse_address(char *addr_str, struct in_addr *addr_bin);

#endif
