#include "app_settings.h"

#define LOG_TAG "Core ARP"

static boolean g_serve = TRUE;
SOCKET g_frontend_socket = SOCKET_ERROR;
SOCKET g_backend_socket = SOCKET_ERROR;
endpoint_list g_endpoints = NULL;

static void
termination_handler (int sig, siginfo_t *si, void *unused)
{
    switch(sig)
    {
    case SIGINT :
    case SIGHUP :
    case SIGTERM :
    case SIGPIPE :
        g_serve = FALSE;
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

static int
wait_for_activity (SOCKET front, SOCKET back, fd_set *rdfds)
{
    FD_ZERO(rdfds);
    FD_SET(front, rdfds);
    FD_SET(back, rdfds);
    int max_fd = front > back ? front : back;

    struct timeval timeout = ms_to_timeval(5000);
    return select(max_fd + 1, rdfds, NULL, NULL, &timeout);
}


int
main (int argc, char *argv[])
{
    initialize_signal_handler();
    
    g_endpoints = get_endpoint_list();
    g_backend_socket = bind_socket(g_endpoints->index);
    if (SOCKET_ERROR == g_backend_socket)
    {
        LOGE("Could not create RAW socket to listen for ARP REQ. Error: %s",
                errno_string());
        goto cleanup;
    }

    g_frontend_socket = bind_ipc(ARP_IPC_ABSFILENAME, FALSE);
    if (SOCKET_ERROR == g_frontend_socket)
    {
        LOGE("Could not create STREAM socket for IPC. Error: %s",
                errno_string());
        goto cleanup;
    }

    if (SOCKET_ERROR == listen(g_frontend_socket, 10))
    {
        LOGE("Could not set STREAM socket to listen for IPC. Error: %s",
                errno_string());
        goto cleanup;
    }

    fd_set rdfds;
    while (TRUE == g_serve)
    {
        int rsc = wait_for_activity(g_frontend_socket, g_backend_socket, &rdfds); 
        if (0 > rsc)
        {
            LOGE("Error while waiting on select on sockets: %s", errno_string());
            break;
        }
        else if (0 == rsc) { continue; }
    
        if (0 != FD_ISSET(g_frontend_socket, &rdfds))
        {
            processor_send_req(g_endpoints, g_frontend_socket, g_backend_socket);
        }
        
        if (0 != FD_ISSET(g_backend_socket, &rdfds))
        {
            processor_recv_reqrep(g_endpoints, g_frontend_socket, g_backend_socket);
        }
    }

cleanup :
    free_endpoint_list(g_endpoints);
    close(g_frontend_socket);
    close(g_backend_socket);
    return 0;
}
