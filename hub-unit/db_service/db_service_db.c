#include "db_service_db.h"
#include <sqlite3.h>

#define DB_PATH "doors-hub.db"

static sqlite3_stmt *stmt_append_packet = NULL;

static sqlite3 *open_db(void)
{
    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};

    sqlite3 *db = NULL;
    int ret = sqlite3_open(DB_PATH, &db);

    if (ret != SQLITE_OK)
    {
        snprintf (log_buff, sizeof(log_buff), "Failed to open DB: %s\n", sqlite3_errmsg(db));
        log_append(log_buff);
        return NULL;
    }

    return db;
}

bool db_init(void)
{
    static const char db_str_create_packets_table[] =
    {
        "CREATE TABLE IF NOT EXISTS packets ("
        "time INTEGER NOT NULL, "
        "date INTEGER NOT NULL, "
        "category INTEGER NOT NULL, "
        "subcategory INTEGER NOT NULL, "
        "source_id INTEGER NOT NULL, "
        "data_16 INTEGER NOT NULL, "
        "data_32 INTEGER NOT NULL );"
    };

    static const char db_str_append_packet[] =
    {
        "INSERT INTO packets VALUES(?, ?, ?, ?, ?, ?, ?)"
    };

    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};

    sqlite3 *db = open_db();

    if (db == NULL)
    {
        goto open_failure;
    }

    char *sqlite_error_msg;

    if (SQLITE_OK != sqlite3_exec(db, db_str_create_packets_table, NULL, NULL, &sqlite_error_msg))
    {
        snprintf (log_buff, sizeof(log_buff), "Error creating packets table: %s\n", sqlite_error_msg);
        log_append(log_buff);
        goto exec_failure;
    }

    int ret;
    ret = sqlite3_prepare_v2(db, db_str_append_packet, strlen(db_str_append_packet), &stmt_append_packet, NULL);

    if(ret != SQLITE_OK)
    {
        snprintf (log_buff, sizeof(log_buff), "Error preparing append packet statement: %s\n", sqlite3_errstr(ret));
        log_append(log_buff);
        goto prepare_failure;
    }

    sqlite3_close(db);
    return true;

prepare_failure:
    if (stmt_append_packet != NULL) sqlite3_finalize(stmt_append_packet);
exec_failure:
    sqlite3_free(sqlite_error_msg);
    sqlite3_close(db);
open_failure:
    // TODO: use TERMR_ERROR
    //why_terminate = TERMR_ERROR;
    should_terminate = true;
    return false;
}

void db_deinit(void)
{
    if (stmt_append_packet != NULL) sqlite3_finalize(stmt_append_packet);
}

void db_append(DoorPacket_t *source)
{
    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};

    sqlite3_bind_int(stmt_append_packet, 1, source->header.time);
    sqlite3_bind_int(stmt_append_packet, 2, source->header.date);
    sqlite3_bind_int(stmt_append_packet, 3, source->header.category);
    sqlite3_bind_int(stmt_append_packet, 4, source->body.Report.report_id);
    sqlite3_bind_int(stmt_append_packet, 5, source->body.Report.source_id);
    sqlite3_bind_int(stmt_append_packet, 6, source->body.Report.report_data_16);
    sqlite3_bind_int(stmt_append_packet, 7, source->body.Report.report_data_32);

    int ret = sqlite3_step(stmt_append_packet);

    if (ret != SQLITE_DONE)
    {
        snprintf (log_buff, sizeof(log_buff), "Statement step error: %s\n", sqlite3_errstr(ret));
        log_append(log_buff);
    }

    sqlite3_reset(stmt_append_packet);
}
