#include "web_server_listener.h"
#include "mongoose.h"
#include "hub_common.h"

static const struct mg_http_serve_opts server_options =
{
    .root_dir = "./site/",
};

// HTTP server event handler function
static void ev_handler(struct mg_connection *connection, int event_id, void *event_data)
{
    static const char log_event_prefix[16] = "Server event: ";

    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};
    struct mg_http_message *http_msg_ptr = NULL;

    switch (event_id)
    {
        case MG_EV_HTTP_MSG:
            http_msg_ptr = (struct mg_http_message *)event_data;

            if (mg_match(http_msg_ptr->uri, mg_str("/actions/test"), NULL))
            {
                snprintf(log_buff, sizeof(log_buff), "Test button pushed by a connection with message count of %d.", connection->data[0]);
                log_append(log_buff);
            }
            else if (mg_match(http_msg_ptr->uri, mg_str("/help"), NULL))
            {
                mg_http_reply(connection, 200, "", "<iframe id=\"data_frame\" src=\"help.txt\"></iframe>");
            }
            else if (mg_match(http_msg_ptr->uri, mg_str("/logs/*"), NULL))
            {

                if (mg_match(http_msg_ptr->uri, mg_str("*/all"), NULL))
                {
                    mg_http_reply(connection, 200, "", "<iframe id=\"data_frame\" src=\"logs.txt\" hx-get=\"/logs/all\" hx-target=\"#info\" hx-trigger=\"every 2s\"></iframe>");
                }
                else if (mg_match(http_msg_ptr->uri, mg_str("*/hub_control"), NULL))
                {
                    mg_http_reply(connection, 200, "", "<iframe id=\"data_frame\" src=\"Hub-Control.txt\" hx-get=\"/logs/hub_control\" hx-target=\"#info\" hx-trigger=\"every 2s\">></iframe>");
                }
                else if (mg_match(http_msg_ptr->uri, mg_str("*/door_manager"), NULL))
                {
                    mg_http_reply(connection, 200, "", "<iframe id=\"data_frame\" src=\"Door-Manager.txt\" hx-get=\"/logs/door_manager\" hx-target=\"#info\" hx-trigger=\"every 2s\">></iframe>");
                }
                else if (mg_match(http_msg_ptr->uri, mg_str("*/intercom_server"), NULL))
                {
                    mg_http_reply(connection, 200, "", "<iframe id=\"data_frame\" src=\"Intercom-Server.txt\" hx-get=\"/logs/intercom_server\" hx-target=\"#info\" hx-trigger=\"every 2s\">></iframe>");
                }
                else if (mg_match(http_msg_ptr->uri, mg_str("*/web_server"), NULL))
                {
                    mg_http_reply(connection, 200, "", "<iframe id=\"data_frame\" src=\"Web-Server.txt\" hx-get=\"/logs/web_server\" hx-target=\"#info\" hx-trigger=\"every 2s\">></iframe>");
                }
                else if (mg_match(http_msg_ptr->uri, mg_str("*/database_service"), NULL))
                {
                    mg_http_reply(connection, 200, "", "<iframe id=\"data_frame\" src=\"Database-Service.txt\" hx-get=\"/logs/database_service\" hx-target=\"#info\" hx-trigger=\"every 2s\">></iframe>");
                }
            }
            else if (mg_match(http_msg_ptr->uri, mg_str("/lists/*"), NULL))
            {
                if (mg_match(http_msg_ptr->uri, mg_str("*/doors"), NULL))
                {
                    mg_http_reply(connection, 200, "", "<iframe id=\"data_frame\" src=\"doors.txt\" hx-get=\"/lists/doors\" hx-target=\"#info\" hx-trigger=\"every 2s\">></iframe>");
                }
                else if (mg_match(http_msg_ptr->uri, mg_str("*/intercoms"), NULL))
                {
                    mg_http_reply(connection, 200, "", "<iframe id=\"data_frame\" src=\"intercoms.txt\" hx-get=\"/lists/intercoms\" hx-target=\"#info\" hx-trigger=\"every 2s\">></iframe>");
                }
            }
            else
            {
                mg_http_serve_dir(connection, http_msg_ptr, &server_options);
            }

            connection->data[0]++;
            break;
        case MG_EV_OPEN:
            connection->data[0] = 0;
            snprintf(log_buff, sizeof(log_buff), "%sConnection created.", log_event_prefix);
            log_append(log_buff);
            break;
        case MG_EV_ERROR:
            snprintf(log_buff, sizeof(log_buff), "%sError.", log_event_prefix);
            log_append(log_buff);
            break;
        case MG_EV_RESOLVE:
            snprintf(log_buff, sizeof(log_buff), "%sHost name resolved.", log_event_prefix);
            log_append(log_buff);
            break;
        case MG_EV_CONNECT:
            snprintf(log_buff, sizeof(log_buff), "%sConnection established.", log_event_prefix);
            log_append(log_buff);
            break;
        case MG_EV_ACCEPT:
            snprintf(log_buff, sizeof(log_buff), "%sConnection accepted.", log_event_prefix);
            log_append(log_buff);
            break;
        case MG_EV_TLS_HS:
            snprintf(log_buff, sizeof(log_buff), "%sTLS handshake succeeded.", log_event_prefix);
            log_append(log_buff);
            break;
        case MG_EV_CLOSE:
            snprintf(log_buff, sizeof(log_buff), "%sConnection closed.", log_event_prefix);
            log_append(log_buff);
            break;
        default:
            break;
    }
}

void* listener_task(void *arg)
{
    // suppress 'unused variable' warning
    (void)arg;

    struct mg_mgr mgr = {0};

    log_append("Opening web interface.");

    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://0.0.0.0:80", ev_handler, NULL);

    while(!should_terminate)
    {
        mg_mgr_poll(&mgr, 1000);
    }

    log_append("Closing web interface.");

    // TODO: figure out if this is necessary
    mg_mgr_free(&mgr);

    return NULL;
}
