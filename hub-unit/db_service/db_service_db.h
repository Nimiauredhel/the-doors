#ifndef DB_SERVICE_DB_H
#define DB_SERVICE_DB_H

#include "hub_common.h"

bool db_init(void);
void db_deinit(void);
void db_append(DoorPacket_t *source);

#endif
