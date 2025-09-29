#include "web_server_listener.h"
#include "web_server_ipc.h"
#include "mongoose.h"
#include "hub_common.h"

static const struct mg_http_serve_opts server_options =
{
    .root_dir = "./site/",
};

static const char info_html_line_fmt[32] = "<p class=\"infoline\">%s</p>";
static const char info_html_log_line_fmt[64] = "<p class=\"logline\">[%u][%02u:%02u:%02u][%s]%s</p>";
static const char info_html_prefix_static[24] = "<div class=\"logbox\">";
static const char info_html_prefix_refresh[76] = "<div class=\"logbox\" hx-target=\"#info\" hx-get=\"%s\" hx-trigger=\"every %us\">";
static const char info_html_suffix[16] = "</div>";

static char info_html_buff[sizeof(info_html_prefix_refresh) + sizeof(info_html_suffix)
                        + (HUB_MAX_LOG_COUNT*(HUB_MAX_LOG_MSG_LENGTH+sizeof(info_html_line_fmt)))] = {0};

// HTTP server event handler function
static void ev_handler(struct mg_connection *connection, int event_id, void *event_data)
{
    static const char log_event_prefix[16] = "Server event: ";

    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};

    struct mg_http_message *http_msg_ptr = NULL;
    char *html_buff_pos = info_html_buff;

    switch (event_id)
    {
        case MG_EV_HTTP_MSG:
            http_msg_ptr = (struct mg_http_message *)event_data;

            if (mg_match(http_msg_ptr->uri, mg_str("/actions/test"), NULL))
            {
                mg_http_reply(connection, 200, "Cache-Control: no-cache", "");
                snprintf(log_buff, sizeof(log_buff), "Test button pushed by a connection with message count of %d.", connection->data[0]);
                log_append(log_buff);
            }
            else if (mg_match(http_msg_ptr->uri, mg_str("/help"), NULL))
            {
                explicit_bzero(info_html_buff, sizeof(info_html_buff));
                html_buff_pos += sprintf(html_buff_pos, info_html_line_fmt, "This is the help text!\nThis is the second line!");
                mg_http_reply(connection, 200, "", info_html_buff);
            }
            else if (mg_match(http_msg_ptr->uri, mg_str("/logs/*"), NULL))
            {
                if (mg_match(http_msg_ptr->uri, mg_str("*/all"), NULL))
                {
                    explicit_bzero(info_html_buff, sizeof(info_html_buff));
                    html_buff_pos += sprintf(html_buff_pos, info_html_prefix_refresh, "/logs/all", 2);

                    HubLogRing_t *hub_log_ptr = ipc_acquire_hub_log_ptr();

                    for (uint16_t i = 0; i < HUB_MAX_LOG_COUNT; i++)
                    {
                        uint16_t read_pos = (hub_log_ptr->head + (HUB_MAX_LOG_COUNT - i)) % HUB_MAX_LOG_COUNT;

                        if (hub_log_ptr->logs[read_pos][0] == '\0')
                        {
                            // reached empty slot, stop here
                            break;
                        }

                        html_buff_pos += sprintf(html_buff_pos, info_html_log_line_fmt, read_pos,
                            hub_log_ptr->timestamps[read_pos].tm_hour, hub_log_ptr->timestamps[read_pos].tm_min, hub_log_ptr->timestamps[read_pos].tm_sec,
                            get_module_label(hub_log_ptr->module_ids[read_pos]), hub_log_ptr->logs[read_pos]);
                    }

                    ipc_release_hub_log_ptr();
                    html_buff_pos += sprintf(html_buff_pos, info_html_suffix);
                    mg_http_reply(connection, 200, "", info_html_buff);
                }
                else if (mg_match(http_msg_ptr->uri, mg_str("*/hub_control"), NULL))
                {
                    mg_http_reply(connection, 200, "", "<iframe id=\"data_frame\" src=\"Hub-Control.txt\" hx-get=\"/logs/hub_control\" hx-target=\"#info\" hx-trigger=\"every 2s\"></iframe>");
                }
                else if (mg_match(http_msg_ptr->uri, mg_str("*/door_manager"), NULL))
                {
                    mg_http_reply(connection, 200, "", "<iframe id=\"data_frame\" src=\"Door-Manager.txt\" hx-get=\"/logs/door_manager\" hx-target=\"#info\" hx-trigger=\"every 2s\"></iframe>");
                }
                else if (mg_match(http_msg_ptr->uri, mg_str("*/intercom_server"), NULL))
                {
                    mg_http_reply(connection, 200, "", "<iframe id=\"data_frame\" src=\"Intercom-Server.txt\" hx-get=\"/logs/intercom_server\" hx-target=\"#info\" hx-trigger=\"every 2s\"></iframe>");
                }
                else if (mg_match(http_msg_ptr->uri, mg_str("*/web_server"), NULL))
                {
                    mg_http_reply(connection, 200, "", "<iframe id=\"data_frame\" src=\"Web-Server.txt\" hx-get=\"/logs/web_server\" hx-target=\"#info\" hx-trigger=\"every 2s\"></iframe>");
                }
                else if (mg_match(http_msg_ptr->uri, mg_str("*/database_service"), NULL))
                {
                    mg_http_reply(connection, 200, "", "<iframe id=\"data_frame\" src=\"Database-Service.txt\" hx-get=\"/logs/database_service\" hx-target=\"#info\" hx-trigger=\"every 2s\"></iframe>");
                }
            }
            else if (mg_match(http_msg_ptr->uri, mg_str("/lists/*"), NULL))
            {
                if (mg_match(http_msg_ptr->uri, mg_str("*/doors"), NULL))
                {
                    mg_http_reply(connection, 200, "", "<iframe id=\"data_frame\" src=\"doors.txt\" hx-get=\"/lists/doors\" hx-target=\"#info\" hx-trigger=\"every 2s\"></iframe>");
                }
                else if (mg_match(http_msg_ptr->uri, mg_str("*/intercoms"), NULL))
                {
                    mg_http_reply(connection, 200, "", "<iframe id=\"data_frame\" src=\"intercoms.txt\" hx-get=\"/lists/intercoms\" hx-target=\"#info\" hx-trigger=\"every 2s\"></iframe>");
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
