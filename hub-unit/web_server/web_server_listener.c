#include "web_server_listener.h"
#include "mongoose.h"
#include "hub_common.h"

// HTTP server event handler function
static void ev_handler(struct mg_connection *c, int ev, void *ev_data)
{
    if (ev == MG_EV_HTTP_MSG)
    {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        struct mg_http_serve_opts opts = { .root_dir = "./site/" };
        mg_http_serve_dir(c, hm, &opts);
    }
}

void* listener_task(void *arg)
{
    // suppress 'unused variable' warning
    (void)arg;

    struct mg_mgr mgr = {0};

    log_append("Opening web interface.");

    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://0.0.0.0:8000", ev_handler, NULL);

    while(!should_terminate)
    {
        mg_mgr_poll(&mgr, 1000);
    }

    log_append("Closing web interface.");

    // TODO: figure out if this is necessary
    mg_mgr_free(&mgr);

    return NULL;
}
