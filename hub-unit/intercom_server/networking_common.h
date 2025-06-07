#ifndef NETWORKING_COMMON_H
#define NETWORKING_COMMON_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdbool.h>

#define SERVER_DATA_PORT 45678

/**
 * Conveniently wraps the inet_pton() function which converts an IP address
 * to its network binary representation and writes to an existing address struct.
 */
bool parse_address(char *addr_str, struct in_addr *addr_bin);

#endif
