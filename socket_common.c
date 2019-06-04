#include "app_settings.h"

#ifndef LOG_TAG
#define LOG_TAG "SockCommon"
#endif

inline int
set_socket_broadcast (SOCKET sock)
{
	int optval = 1;
	return setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *) &optval,
			sizeof(optval));
}

int
ready_socket_count (SOCKET sock, uint32_t ms)
{
    struct timeval timeout = ms_to_timeval(ms);

    fd_set rdfds;
    FD_ZERO(&rdfds);
    FD_SET(sock, &rdfds);

    return select(sock + 1, &rdfds, NULL, NULL, &timeout);
}

int
wait_for_ready_socket(SOCKET sock)
{
    fd_set rdfds;
    FD_ZERO(&rdfds);
    FD_SET(sock, &rdfds);

    return select(sock + 1, &rdfds, NULL, NULL, NULL);
}

int
wait_for_ready_sockets(endpoint_list list, fd_set *rdfds)
{
    FD_ZERO(rdfds);
    int max_fd = 0;

    endpoint *temp;
    for (temp = list; NULL != temp; temp = temp->next)
    {
        FD_SET(temp->sock, rdfds);
        max_fd = max_fd < temp->sock ? temp->sock : max_fd;
    }

    struct timeval timeout = ms_to_timeval(5000);
    return select(max_fd + 1, rdfds, NULL, NULL, &timeout);
}

void
unconnect_socket (SOCKET sock)
{
    struct sockaddr_in sin_unconnect;
    memset(&sin_unconnect, 0, sizeof(struct sockaddr_in));
    sin_unconnect.sin_family = AF_UNSPEC;
    
    connect(sock, &sin_unconnect, sizeof(struct sockaddr_in));
}

struct sockaddr_un*
get_sun (char *file_name)
{
    struct sockaddr_un *sun = calloc(1, sizeof(struct sockaddr_un));
    if (NULL != sun)
    {
        sun->sun_family = AF_LOCAL;
        memcpy(sun->sun_path, file_name, strlen(file_name));
    }

    return sun;
}
