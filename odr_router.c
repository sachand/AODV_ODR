#include "app_settings.h"

#ifndef LOG_TAG
#define LOG_TAG "Router"
#endif

/**
 * Global router object. It is only global so that signal handlers can
 * signal it to stop.
 * */
odr_router *g_router = NULL;

/**
 * Staleness factor stored in ms
 * */
float g_staleness = (0.0f * 1000);

extern void *
frontend_do_process (void *args);
extern void *
backend_do_process (void *args);

static void
termination_handler (int sig, siginfo_t *si, void *unused)
{
    switch(sig)
    {
    case SIGINT :
    case SIGHUP :
    case SIGTERM :
    case SIGPIPE :
        cancel_router(g_router);
    }
}

static void
initialize_signal_handler ()
{
	struct sigaction new_action;
	memset(&new_action, 0, sizeof(new_action));

	new_action.sa_sigaction = termination_handler;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = SA_SIGINFO;

	sigaction(SIGINT, &new_action, NULL);
	sigaction(SIGHUP, &new_action, NULL);
	sigaction(SIGTERM, &new_action, NULL);
	sigaction(SIGPIPE, &new_action, NULL);
}

/**
 * Creates the ODR router. It binds a socket to the constant well known
 * file for ODR IPC. Also includes creation of odr_client_list in which
 * server is added default.
 * 
 * @param interface_hub     List of all interfaces in this node.
 * 
 * @return Router object if initialization successful. NULL otherwise.
 * */
static odr_router *
create_odr_router (endpoint_list interface_hub)
{
    odr_router *router = calloc(1, sizeof(odr_router));
    char *odr_ipc_absfilename = calloc(1, ODR_CLIENT_RAND_MAX_FILENAME);
    strncpy(odr_ipc_absfilename, ODR_IPC_ABSFILENAME, ODR_CLIENT_RAND_MAX_FILENAME);
    router->ipc_sock = bind_ipc(odr_ipc_absfilename);
    free(odr_ipc_absfilename);
    
    if (SOCKET_ERROR == router->ipc_sock)
    {
        free(router);
        return NULL;
    }

    get_canonical_ip(router->self_canonical_ip);
    LOGS("Can IP: %s", router->self_canonical_ip);
    router->rt_table = create_routing_table(router->self_canonical_ip);
    router->interface_hub = interface_hub;
    router->stopping = FALSE;
    insert_in_odr_client_list(&(router->client_list), create_odr_client(
            SERVER_IPC_ABSFILENAME, DEFAULT_PORT_NUMBER));
    sem_init(&(router->route_signal), 0, 0);

    return router;
}

/**
 * Signal router ops to stop.
 * */
void
cancel_router (odr_router *router)
{
    if (NULL != router)
    {
        router->stopping = TRUE;
        sem_post(&(router->route_signal));
    }
}

/**
 * Opposite of create_odr_router above
 * */
void
destroy_odr_router (odr_router *router)
{
    if (NULL == router) { return; }

    cancel_router(router);
    close(router->ipc_sock);
    destroy_routing_table(router->rt_table);
    destroy_odr_client_list(router->client_list);
    free_endpoint_list(router->interface_hub);
    sem_destroy(&(router->route_signal));
    free(router);
}

int
main (int argc, char *argv[])
{
    if (2 <= argc)
    {
        sscanf(argv[1], "%f", &g_staleness);
        if (0 > g_staleness)
        {
            g_staleness = 0.0f;
        }
        g_staleness *= 1000;
    }
    LOGS("Staleness set to %f milliseconds", g_staleness);

    endpoint_list list = create_listen_hub();
    g_router = create_odr_router(list);
    initialize_signal_handler();

    if (NULL != g_router)
    {
        pthread_t backend, frontend;

        LOGV("%s", errno_to_string(pthread_create(&backend, NULL, &backend_do_process, g_router)));
        LOGV("%lu", backend);

        LOGV("%s", errno_to_string(pthread_create(&frontend, NULL, &frontend_do_process, g_router)));
        LOGV("%lu", frontend);

        pthread_join(backend, NULL);
        pthread_join(frontend, NULL);
    }

    remove(ODR_IPC_ABSFILENAME);
    destroy_odr_router(g_router);

    return 0;
}
