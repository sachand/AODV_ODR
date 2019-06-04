///@todo Move into socket_common.c

#include "app_settings.h"

#ifndef LOG_TAG
#define LOG_TAG "SocketBinder"
#endif

/**
 * Error codes for bind, accept and listen calls. On failure these calls
 * return these and set errno for the detailed message
 * */
#define BIND_FAILED -1

/**
 * Error log format for create
 * */
#define CREATE_FAIL_LOG_FORMAT "Socket %s failed. Error: %s"

/**
 * Macro that essentially logs out an error message of format CREATE_FAIL_LOG_FORMAT,
 * closes socket and frees heap memory used, if any.
 *
 * @param _x_	Failure message to log
 * @param _y_ 	Port number on which operation was intended
 * @param _z_ 	Socket fd
 * */
#define CLEAN_CREATE(_x_, _y_)       								\
	do 																\
	{ 																\
		LOGE(CREATE_FAIL_LOG_FORMAT, _x_, errno_string()); 	        \
        close(_y_);                                                 \
		return SOCKET_ERROR;    									\
	} while (0); 													\

/**
 * Simple wrapper over socket create and bind. This creates a socket and
 * binds it according to information provided.
 * 
 * @param index     Interface index to bind to
 * 
 * @return Socket fd of bound socket if successful. SOCKET_ERROR otherwise.
 * */
SOCKET
bind_socket (int index)
{
	SOCKET sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (SOCKET_ERROR == sock)
    {
		CLEAN_CREATE("create", sock);
	}

    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(struct sockaddr_ll));

    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ARP);
    sll.sll_ifindex = index;

    set_socket_broadcast(sock);
    if (BIND_FAILED == bind(sock, (struct sockaddr *)&sll, sizeof(struct sockaddr_ll)))
    {
        CLEAN_CREATE("bind", sock);
    }

    return sock;
}

/**
 * Creates bouund socket for IPC given file_name
 * */
SOCKET
bind_ipc (char *file_name, boolean message_oriented)
{
    if (NULL == file_name)
    {
        errno = EINVAL;
        return SOCKET_ERROR;
    }

    SOCKET sock;
    if (TRUE == message_oriented)
    {
        sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
    }
    else
    {
        sock = socket(AF_LOCAL, SOCK_STREAM, 0);
    }
    
    if (SOCKET_ERROR == sock)
    {
		CLEAN_CREATE("create", sock);
	}

    struct sockaddr_un self;
    memset(&self, 0, sizeof(struct sockaddr_un));

    self.sun_family = AF_LOCAL;
    if ('\0' == file_name[0])
    {
        strncpy(self.sun_path, MKSTEMP_TEMPLATE, strlen(MKSTEMP_TEMPLATE));
        mkstemp(self.sun_path);
        unlink(self.sun_path);
    }
    else
    {
        remove(file_name);
        strncpy(self.sun_path, file_name, strlen(file_name));
    }

    if (BIND_FAILED == bind(sock, (struct sockaddr *)&self, sizeof(struct sockaddr_un)))
    {
        CLEAN_CREATE("bind", sock);
    }

    strncpy(file_name, self.sun_path, ODR_CLIENT_RAND_MAX_FILENAME);
    LOGV("socket %d", sock);
    return sock;
}
