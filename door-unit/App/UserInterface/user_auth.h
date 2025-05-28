/*
 * door_auth.h
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#ifndef USER_AUTH_H_
#define USER_AUTH_H_

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "main.h"

#include "uart_io.h"
#include "audio.h"

#include "event_log.h"
#include "user_interface.h"

void auth_reset_auth(void);
bool auth_is_user_auth(void);
bool auth_is_admin_auth(void);
void auth_check_password(const char *input_str, bool admin);
void auth_set_password(const char *input_str);

#endif /* USER_AUTH_H_ */
