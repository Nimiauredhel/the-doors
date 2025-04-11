/*
 * door_auth.h
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#ifndef DOOR_AUTH_H_
#define DOOR_AUTH_H_

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "main.h"

void auth_reset_auth(void);
bool auth_is_auth(void);
void auth_check_password(char *rx_msg);
void auth_set_password(char *rx_msg);

#endif /* DOOR_AUTH_H_ */
