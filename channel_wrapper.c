/**
 * Task of this component is to provide interface between underlying
 * protocol/channel and application using it.
 * */

#include <math.h>
#include "app_settings.h"

#ifndef LOG_TAG
#define LOG_TAG "ChannelWrapper"
#endif

/**
 * Creates a message that our R-UDP will send. Whatever data is given
 * is buffered. So, caller may free its copy if it doesn't need it anymore.
 *
 * @param header            Message header
 * @param payload           Message payload
 * @param payload_length    Message payload length
 *
 * @return An iovec msg for the details given.
 * */
msg_iovec
create_msg (msg_header *header, char *payload, uint32_t payload_length)
{
    msg_iovec msg = (msg_iovec) calloc(2, sizeof(struct iovec));

    if (NULL == header) { header = get_new_header(); }
    msg[0].iov_base = (char *)header;
    msg[0].iov_len = HEADER_SIZE;

    if (NULL == payload || 0 == payload_length) { return msg; }
    msg[1].iov_base = calloc(1, payload_length);
    msg[1].iov_len = payload_length;
    memcpy(msg[1].iov_base, payload, payload_length);

    return msg;
}

/**
 * Given payload and header, create a clone message for this.
 * (New memory allocated. Use delete_msg when done with this)
 * */
msg_iovec
clone_msg (msg_header *header, char *payload, uint32_t payload_length)
{
    msg_iovec msg = (msg_iovec) calloc(2, sizeof(struct iovec));
    msg_header *header_copy = get_new_header();
    memcpy(header_copy, header, HEADER_SIZE);

    msg[0].iov_base = header_copy;
    msg[0].iov_len = HEADER_SIZE;

    if (NULL == payload || 0 == payload_length) { return msg; }
    char *payload_copy = calloc(1, payload_length);
    memcpy(payload_copy, payload, payload_length);
    msg[1].iov_base = payload_copy;
    msg[1].iov_len = payload_length;

    return msg;
}

/**
 * Creates an empty maximum sized message that this R-UDP supports
 * */
msg_iovec
create_msg_max ()
{
    msg_iovec msg = (msg_iovec) calloc(2, sizeof(struct iovec));

    msg_header *header_copy = get_new_header();    
    char *payload_copy = (char *) calloc(MAX_PAYLOAD_SIZE, 1);

    msg[0].iov_base = (char *)header_copy;
    msg[0].iov_len = HEADER_SIZE;

    msg[1].iov_base = (char *)payload_copy;
    msg[1].iov_len = MAX_PAYLOAD_SIZE;

    return msg;
}

/**
 * Frees given message. Deallocates heap memory.
 * 
 * @param msg       Message to destroy
 * */
void
delete_msg (msg_iovec msg)
{
    if (NULL != msg)
    {
        if (NULL != msg[0].iov_base)
        {
            free(msg[0].iov_base);
        }
        if (NULL != msg[1].iov_base)
        {
            free(msg[1].iov_base);
        }
        free(msg);
    }
}

/**
 * Converts given buffer from application to messages that can be passed
 * over theis R-UDP protocol.
 * 
 * @param p         Endpoint of the connection
 * @param flags     Flags to specify for this buffer. Ideally should be 0.
 * @param buf       Data to send
 * @param len       Length of data to send
 * 
 * @return Number of messages sent for given data. SOCKET_ERROR is returned
 *         if the connection saw some error. Check errno for error.
 * */
int
send_msg (SOCKET sock, char *destination_cip, uint16_t destination_port,
        char *buf, int len, boolean force_route_discovery)
{
    // Sanity check
    if (NULL == buf || 0 >= len)
    {
        errno = EINVAL;
        return SOCKET_ERROR;
    }

    msg_header *header = get_new_header();
    memcpy(header->destination_id, destination_cip, INET_ADDRSTRLEN);
    header->destination_port = destination_port;
    header->payload_length = len;
    header->flags = MSG_HEADER_FLAG_DAT;
    if (TRUE == force_route_discovery)
    {
        header->flags |= MSG_HEADER_FLAG_FRC;
    }
    
    msg_iovec msg = create_msg(header, buf, len);
    LOGV("BUFF2: %s", buf);
    int ret = ipc_send_msg(sock, msg);
    delete_msg(msg);
    return ret;
}

/**
 * Reads a msg from the channel.
 * 
 * @param sock          Socket of caller
 * @param source_cip    Store for message sender IP. Must be at least
 *                      INET_ADDRSTRLEN
 * @param source_port   Store for message sender port
 * @param buf           Buffer pointer to store retrieved data.
 * @param len           Length of this buffer.
 * @param timeout_ms    ms to wait for message 
 * 
 * @return Number of bytes read. SOCKET_ERROR in case of error and 0
 *         if nothing arrived in given time
 * 
 * NOTE:
 * This is timeout-bound blocking.
 * */
int
recv_msg (SOCKET sock, char *source_cip, uint16_t *source_port,
        char *buf, int *len, uint32_t timeout_ms)
{
    // Sanity check
    if (NULL == buf || NULL == len || NULL == source_cip || NULL == source_port
            || 0 >= *len)
    {
        errno = EINVAL;
        return SOCKET_ERROR;
    }

    msg_iovec msg = NULL;
    int bytes_read = ipc_recv_msg(sock, &msg, timeout_ms);
    if (0 < bytes_read)
    {
        msg_header *header = msg[0].iov_base;
        memcpy(source_cip, header->source_id, INET_ADDRSTRLEN);
        *source_port = header->source_port;
        *len = header->payload_length;
        memcpy(buf, msg[1].iov_base, header->payload_length);
        buf[header->payload_length] = '\0';
        if (TRUE == is_flag_set(header->flags, MSG_HEADER_FLAG_ERR))
        {
            bytes_read = SOCKET_ERROR;
        }
    }

    delete_msg(msg);
    return bytes_read;
}
