#include "app_settings.h"

#ifndef LOG_TAG
#define LOG_TAG "Dummy"
#endif

static boolean g_serve = TRUE;
#define MAXLINE 64

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

boolean
is_acceptable_request(char *request)
{
    if (0 == memcmp(request, "TIME", 4))
    {
        return TRUE;
    }

    return FALSE;
}

int main()
{
    initialize_signal_handler();
    char *my_file_name = calloc(1, 90);
    memcpy(my_file_name, SERVER_IPC_ABSFILENAME, strlen(SERVER_IPC_ABSFILENAME));
    SOCKET sock = bind_ipc(my_file_name);
    connect(sock, get_sun(ODR_IPC_ABSFILENAME), sizeof(struct sockaddr_un));

    char sip[20] = { '\0' };
    uint16_t sp = 0;
    char buff[MAXLINE] = { '\0' };
    time_t ticks;
    int len = MAXLINE;

    while (TRUE == g_serve)
    {
        len = MAXLINE;
        memset(buff, 0, MAXLINE);
        if (0 < recv_msg(sock, sip, &sp, buff, &len, 5000) &&
                TRUE == is_acceptable_request(buff))
        {
            LOGS("Received %s request from VM%d:%d", buff, get_vm_index(sip), sp);
            ticks = time(NULL);
            memset(buff, 0, MAXLINE);
            snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
            LOGS("Replying with %s", buff);
            send_msg(sock, sip, sp, buff, strlen(buff), FALSE);
        }
    }
    
    close(sock);
    remove(my_file_name);
    free(my_file_name);
    return 0;
}
