#include "app_settings.h"

#ifndef LOG_TAG
#define LOG_TAG "IPC_Provider"
#endif

static struct sockaddr_un *g_odr_channel = NULL;

/**
 * Performs core IPC send over the file socket.
 * 
 * @param sock          Socket fd with caller
 * @param destination   UD sockaddr for destination
 * @param msg           Message to send
 * */
int
__ipc_send_msg (SOCKET sock, struct sockaddr_un *destination, msg_iovec msg)
{
    struct msghdr msg_cover;
    memset(&msg_cover, 0, sizeof(struct msghdr));
    msg_cover.msg_name = destination;
    msg_cover.msg_namelen = sizeof(struct sockaddr_un);
    msg_cover.msg_iov = msg;
    msg_cover.msg_iovlen = 2;
    
    LOGV("socket s %d", sock);
    return sendmsg(sock, &msg_cover, 0);
}

/**
 * Interface API to send a message to ODR service underneath.
 * 
 * This is blocking. Data is placed on ODR's IPC channel and then
 * this function returns.
 * 
 * @param sock      Socket fd from which message is to be sent
 * @param msg       Message to send to ODR
 * 
 * @return Number of bytes written on ODR's IPC channel. SOCKET_ERROR in
 *         case of error.
 * */
int
ipc_send_msg (SOCKET sock, msg_iovec msg)
{
    if (0 >= sock || NULL == msg)
    {
        errno = EINVAL;
        return SOCKET_ERROR;
    }

    if (NULL == g_odr_channel)
    {
        g_odr_channel = get_sun(ODR_IPC_ABSFILENAME);
    }
    
    return __ipc_send_msg(sock, g_odr_channel, msg);
}

/**
 * Interface API to read a message from ODR service underneath.
 * 
 * @param sock          Socket fd from which message is to be read
 * @param msg           Message to be received from ODR
 * @param timeout_ms    Read timeout
 * 
 * @return Number of bytes read from ODR's IPC channel. SOCKET_ERROR in
 *         case of error.
 * */
int
ipc_recv_msg (SOCKET sock, msg_iovec *msg, uint32_t timeout_ms)
{
    if (0 >= sock)
    {
        errno = EINVAL;
        return SOCKET_ERROR;
    }

    if (NULL == g_odr_channel)
    {
        g_odr_channel = get_sun(ODR_IPC_ABSFILENAME);
    }
    
    return socket_recv_msg_default(sock, msg, timeout_ms);
}

/**
 * Interface API to send a message from ODR service underneath.
 * 
 * This is blocking. Data is placed on recepient's IPC channel and then
 * this function returns.
 * 
 * @param destination   File name to which message is to be sent
 * @param msg           Message to send to ODR
 * 
 * @return Number of bytes written on ODR's IPC channel. SOCKET_ERROR in
 *         case of error.
 * */
int
ipc_send_msg_full (SOCKET sock, char *destination, msg_iovec msg)
{
    if (NULL == destination || NULL == msg)
    {
        errno = EINVAL;
        return SOCKET_ERROR;
    }

    struct sockaddr_un *sun = get_sun(destination);
    int ret = __ipc_send_msg(sock, sun, msg);
    free(sun);
    return ret;
}
