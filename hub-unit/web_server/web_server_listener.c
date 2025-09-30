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
static const char info_html_door_line_fmt[64] = "<p class=\"logline\">[%u] %s [Updated %lds ago]</p>";
static const char info_html_intercom_line_fmt[84] = "<p class=\"logline\">[%u] [%02X:%02X:%02X:%02X:%02X:%02X:] %s [Updated %lds ago]</p>";
static const char info_html_prefix_static[24] = "<div class=\"logbox\">";
static const char info_html_prefix_refresh[76] = "<div class=\"logbox\" hx-target=\"#info\" hx-get=\"%s\" hx-trigger=\"every %us\">";
static const char info_html_suffix[16] = "</div>";

static const char help_text[] =
"<div class=\"logbox\" hx-target=\"#info\" hx-get=\"/help\" hx-trigger=\"load\"><p class=\"infoline\">This is the web interface for the Hub unit managing this DOORS system.</p><p class=\"infoline\">The buttons above control the information displayed in this space.</p></div>";

static const char log_request_strs[HUB_MODULE_COUNT+1][24] =
{
    "/logs/all",
    "/logs/hub_control",
    "/logs/door_manager",
    "/logs/intercom_server",
    "/logs/web_server",
    "/logs/database_service",
};
static const char list_request_strs[2][24] =
{
    "/lists/doors",
    "/lists/intercoms",
};

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
                snprintf(log_buff, sizeof(log_buff), "Test button pushed by a connection with message count of %d.", connection->data[0]);
                log_append(log_buff);
                mg_http_reply(connection, 200, "", "some text");
            }
            else if (mg_match(http_msg_ptr->uri, mg_str("/help"), NULL))
            {
                explicit_bzero(info_html_buff, sizeof(info_html_buff));
                mg_http_reply(connection, 200, "", help_text);
            }
            else if (mg_match(http_msg_ptr->uri, mg_str("/logs/*"), NULL))
            {
                HubModuleId_t log_module = HUB_MODULE_NONE;

                for (uint8_t i = 0; i < (HUB_MODULE_COUNT + 1); i++)
                {
                    if (mg_match(http_msg_ptr->uri, mg_str(log_request_strs[i]), NULL))
                    {
                        log_module = (HubModuleId_t)i;
                        break;
                    }
                }

                explicit_bzero(info_html_buff, sizeof(info_html_buff));
                html_buff_pos += sprintf(html_buff_pos, info_html_prefix_refresh, log_request_strs[log_module], 2);
                html_buff_pos += sprintf(html_buff_pos, "<h2>%s Logs</h2>", log_module == HUB_MODULE_NONE ? "All" : get_module_label(log_module));

                HubLogRing_t *hub_log_ptr = ipc_acquire_hub_log_ptr();

                for (uint16_t i = 0; i < HUB_MAX_LOG_COUNT; i++)
                {
                    uint16_t read_pos = (hub_log_ptr->head + (HUB_MAX_LOG_COUNT - i)) % HUB_MAX_LOG_COUNT;

                    if (hub_log_ptr->logs[read_pos][0] == '\0')
                    {
                        // reached empty slot, stop here
                        break;
                    }

                    // filter to desired module's logs
                    if (log_module != HUB_MODULE_NONE && hub_log_ptr->module_ids[read_pos] != log_module) continue;

                    html_buff_pos += sprintf(html_buff_pos, info_html_log_line_fmt, read_pos,
                        hub_log_ptr->timestamps[read_pos].tm_hour, hub_log_ptr->timestamps[read_pos].tm_min, hub_log_ptr->timestamps[read_pos].tm_sec,
                        get_module_label(hub_log_ptr->module_ids[read_pos]), hub_log_ptr->logs[read_pos]);
                }

                ipc_release_hub_log_ptr();
                html_buff_pos += sprintf(html_buff_pos, info_html_suffix);
                mg_http_reply(connection, 200, "", info_html_buff);
            }
            else if (mg_match(http_msg_ptr->uri, mg_str("/lists/*"), NULL))
            {
                explicit_bzero(info_html_buff, sizeof(info_html_buff));
                int8_t list_id = -1;

                if (mg_match(http_msg_ptr->uri, mg_str(list_request_strs[0]), NULL))
                {
                    list_id = 0;
                }
                else if (mg_match(http_msg_ptr->uri, mg_str(list_request_strs[1]), NULL))
                {
                    list_id = 1;
                }

                if (list_id < 0) return;

                time_t t_now = time(NULL);
                struct tm tm_now = get_datetime();
                uint16_t counted = 0;

                html_buff_pos += sprintf(html_buff_pos, info_html_prefix_refresh, list_request_strs[list_id], 2);
                html_buff_pos += sprintf(html_buff_pos, "<h2>%s List</h2>", list_id == 0 ? "Doors" : "Intercoms");

                if (list_id == 0)
                {
                    HubDoorStates_t *doors = ipc_acquire_door_states_ptr();

                    html_buff_pos += sprintf(html_buff_pos, "<p class=\"infoline\">Count: %u</p><p class=\"infoline\">Logged [%02u:%02u:%02u]</p> ",
                        doors->count, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);

                    for (uint16_t i = 0; i < HUB_MAX_DOOR_COUNT; i++)
                    {
                        if (counted >= doors->count) break;
                        if (doors->last_seen[i] <= 0) continue;
                        counted++;
                        html_buff_pos += sprintf(html_buff_pos, info_html_door_line_fmt, doors->id[i], doors->name[i], t_now - doors->last_seen[i]);
                    }

                    ipc_release_door_states_ptr();
                }
                else if (list_id == 1)
                {
                    HubIntercomStates_t *intercoms = ipc_acquire_intercom_states_ptr();

                    html_buff_pos += sprintf(html_buff_pos, "<p class=\"infoline\">Count: %u</p><p class=\"infoline\">Logged [%02u:%02u:%02u]</p> ",
                        intercoms->count, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);

                    for (uint16_t i = 0; i < HUB_MAX_INTERCOM_COUNT; i++)
                    {
                        if (counted >= intercoms->count) break;
                        if (intercoms->last_seen[i] <= 0) continue;
                        counted++;
                        html_buff_pos += sprintf(html_buff_pos, info_html_intercom_line_fmt, i,
                        intercoms->mac_addresses[i][0], intercoms->mac_addresses[i][1],
                        intercoms->mac_addresses[i][2], intercoms->mac_addresses[i][3],
                        intercoms->mac_addresses[i][4], intercoms->mac_addresses[i][5],
                        intercoms->name[i],     t_now - intercoms->last_seen[i]);
                    }

                    ipc_release_intercom_states_ptr();
                }

                html_buff_pos += sprintf(html_buff_pos, info_html_suffix);
                mg_http_reply(connection, 200, "", info_html_buff);
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
