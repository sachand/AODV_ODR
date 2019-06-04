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

int main()
{
    initialize_signal_handler();

    char *my_file_name = calloc(1, 90);
    SOCKET sock = bind_ipc(my_file_name);
    connect(sock, get_sun(ODR_IPC_ABSFILENAME), sizeof(struct sockaddr_un));
    
    int opt;
    char *server;
    char sip[INET_ADDRSTRLEN] = { '\0' };
    uint16_t sp = 0;
    char buff[MAXLINE] = { '\0' };
    time_t ticks;
    int len = MAXLINE;

    do
    {
        opt = 0;
        LOGS("Enter VM#(1-10) or 0 to quit: ");
        scanf("%d", &opt);
        
        if (0 == opt) { break; }

        switch (opt)
        {
        case 1: server = VM1; break;
        case 2: server = VM2; break;
        case 3: server = VM3; break;
        case 4: server = VM4; break;
        case 5: server = VM5; break;
        case 6: server = VM6; break;
        case 7: server = VM7; break;
        case 8: server = VM8; break;
        case 9: server = VM9; break;
        case 10: server = VM10; break;
        default: continue;
        }

        memset(sip, 0, INET_ADDRSTRLEN);
        sp = 0;
        memset(buff, 0, MAXLINE);
        len = MAXLINE;
	
        LOGS("Requesting server at VM%d.", opt);
        send_msg(sock, server, DEFAULT_PORT_NUMBER, "TIME", 4, FALSE);
        int bytes_read = recv_msg(sock, sip, &sp, buff, &len, CLIENT_READ_TIMEOUT_MS);

        if (0 < bytes_read)
        {
            LOGS("Response from server at VM%d: %s",
                    get_vm_index(sip), buff);
        }
        else if (0 == bytes_read)
        {
            LOGS("Timeout while waiting for server at VM%d. FRC Retrying",
                    opt);
            send_msg(sock, server, DEFAULT_PORT_NUMBER, "TIME", 4, TRUE);
            bytes_read = recv_msg(sock, sip, &sp, buff, &len, CLIENT_READ_TIMEOUT_MS);
            if (0 < bytes_read)
            {
                LOGS("Response from server at VM%d: %s",
                        get_vm_index(sip), buff);
            }
            else if (0 == bytes_read)
            {
                LOGS("Giving up on server at VM%d", opt);
            }
            else
            {
                if (MAXLINE == len)
                {
                    LOGE("Local error: %s", errno_string());
                }
                else
                {
                    LOGE("Remote error: %s", errno_to_string((int)*buff));
                }
            }
        }
        else
        {
            if (MAXLINE == len)
            {
                LOGE("Local error: %s", errno_string());
            }
            else
            {
                LOGE("Remote error: %s", errno_to_string((int)*buff));
            }
        }
    } while (TRUE == g_serve);
    
    close(sock);
    remove(my_file_name);
    free(my_file_name);
    return 0;
}
