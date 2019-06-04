#include "app_settings.h"

#ifndef LOG_TAG
#define LOG_TAG "ODR_Frontend"
#endif

/**
 * Routes a message inside this system. Finds a right ODR client for
 * given msg using its destination port.
 * 
 * @param router        ODR router object
 * @param msg           Message to route in the system
 * 
 * @return Whether the msg was successfully routed. By this we mean the
 *         client's buffer was filled with msg-data.
 * */
boolean
route_to_self (odr_router *router, msg_iovec msg)
{
    msg_header *header = msg[0].iov_base;
    odr_client *oc = find_odr_client(router->client_list, header->destination_port);
    if (NULL == oc)
    {
        errno = ESRCH;
        return FALSE;
    }
    
    if (0 > ipc_send_msg_full(router->ipc_sock, oc->file_name, msg))
    {
        LOGI("Received message for broken or non-existent ODR client.");
        return FALSE;
    }
    else
    {
        oc->timestamp = get_current_system_time_millis();
    }

    return TRUE;
}

/**
 * Routes a data message in the system. Calls odr_route_msg to find a
 * route if not destined to self.
 * */
void
odr_route_data (odr_router *router, msg_iovec msg)
{
    msg_header *header = msg[0].iov_base;
    if (FALSE == is_flag_set(header->flags, MSG_HEADER_FLAG_DAT)) { return; }

    if (TRUE == is_destined_to_self(router, header))
    {
        // we can add ERR here if route to self fails
        if (FALSE == route_to_self(router, msg) &&
                FALSE == is_flag_set(header->flags, MSG_HEADER_FLAG_ERR))
        {
#if PROPAGATE_ERROR == 1
            inform_error(router, msg[0].iov_base);
#endif
        }
    }
    else
    {
        odr_route_msg(router, msg);
    }
}

static void
__do_process(odr_router *router, struct msghdr *msghdr)
{
    msg_iovec msg = msghdr->msg_iov;
    msg_header *header = msg[0].iov_base;

    struct sockaddr_un *sun = msghdr->msg_name;
    LOGV("suname: %s", sun->sun_path);
    header->source_port = register_odr_client(&(router->client_list),
            sun->sun_path, header->source_port);
    memcpy(header->source_id, router->self_canonical_ip, INET_ADDRSTRLEN);
    free(msghdr->msg_name);
    
    if (TRUE == is_flag_set(header->flags, MSG_HEADER_FLAG_FRC))
    {
        purge_entry(&(router->rt_table), header->destination_id);
    }
    odr_route_data(router, msg);
    delete_msg(msg);
}

/**
 * Core of the frontend. Listens on IPC file for DAT requests.
 * */
void *
frontend_do_process (void *args)
{
    struct msghdr *msg = calloc(1, sizeof(struct msghdr));
    odr_router *router = args;
    
    LOGV("router %x %d", router, router->ipc_sock);
    while (FALSE == router->stopping)
    {
        memset(msg, 0, sizeof(struct msghdr));
        int bytes_read = socket_recv_msg_full(router->ipc_sock, msg,
                ODR_IPC_READ_TIMEOUT_MS);

        if (0 > bytes_read)
        {
            LOGE("Error during IPC read: %s", errno_string());
            break;
        }
        else if (0 < bytes_read)
        {
            __do_process(router, msg);
        }
    }

    router->stopping = TRUE;
    free(msg);
    LOGV("backend closing");
    return NULL;
}
