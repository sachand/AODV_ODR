#include "app_settings.h"

#ifndef LOG_TAG
#define LOG_TAG "ODR_Frontend"
#endif

/**
 * Store for current broadcast ID. Assumed atomic in terms of usage - single
 * threaded access.
 * */
static uint32_t g_broadcast_id = 0;

/**
 * Broadcasts a REQ message for the given destination.
 * 
 * @param router        ODR router object
 * @param destination   Destination for which we want to find route
 * @param rte           Routing table entry in the router for the given
 *                      destination.
 * 
 * @return Returns the number of interfaces broadcasted thie info on.
 * SOCKET_ERROR if some error occured
 * */
int
broadcast_req (odr_router *router, char *destination, routing_table_entry *rte,
        boolean forced, endpoint *e)
{
    if (NULL == router || NULL == destination || NULL == rte)
    {
        errno = EINVAL;
        return SOCKET_ERROR;
    }
    
    if (TRUE == rte->finding_route) { return; }

    LOGV("Broacasting req");
    rte->finding_route = TRUE;

    msg_header *h = get_new_header();
    LOGV("%s %s", router->self_canonical_ip, destination);
    strncpy(h->source_id, router->self_canonical_ip, INET_ADDRSTRLEN);
    strncpy(h->destination_id, destination, INET_ADDRSTRLEN);
    h->broadcast_id = ++g_broadcast_id;
    if (0 == h->broadcast_id) { h->broadcast_id = ++g_broadcast_id; }
    h->flags = MSG_HEADER_FLAG_REQ;
    if (TRUE == forced)
    {
        h->flags |= MSG_HEADER_FLAG_FRC;
    }
    msg_iovec msg = create_msg(h, NULL, 0);
    print_msg_header(h, "BRDC");
    
    int ret = socket_l2_flood_msg(router->interface_hub, e, msg);
    delete_msg(msg);
    return ret;
}

/**
 * Routes messages to the outside world. Caters to two scenarios:
 *  o If an ODR client wants to send a message out of this system.
 *  o The backend wants to route a REP or DAT but can't find RTE
 *    for the given destination. Thereby asking this to initiate
 *    route discovery
 * 
 * Does not cater to messages that are supposed to be routed inside.
 * Will silently ignore such messages.
 * */
void
odr_route_msg (odr_router *router, msg_iovec msg)
{
    msg_header *header = msg[0].iov_base;
    if (TRUE == is_destined_to_self(router, header)) { return; }
    
    routing_table_entry *rte = query_table(&(router->rt_table),
            header->destination_id);
    if (FALSE == is_stale(rte))
    {
        LOGV("Found entry %s", header->destination_id);
        endpoint *e = find_endpoint(router->interface_hub, rte->outgoing_if);
        socket_l2_send_unicast(e, msg, rte->next_hop);
    }
    else
    {
        LOGV("Did not find entry %s", header->destination_id);
        park_msg(&(router->rt_table), msg);
        broadcast_req(router, header->destination_id, rte,
                is_flag_set(header->flags, MSG_HEADER_FLAG_FRC), NULL);
    }
}

void
inform_error (odr_router *router, msg_header *m_header)
{
    msg_iovec msg = clone_msg(m_header, &errno, sizeof(errno));
    m_header = msg[0].iov_base;
    swap_roles(m_header);
    m_header->flags |= MSG_HEADER_FLAG_ERR;
    m_header->flags |= MSG_HEADER_FLAG_DAT;
    m_header->payload_length = sizeof(errno);
    
    odr_route_msg(router, msg);
}

/**
 * Core of the backend. Listens on all the interfaces. Reads one eth frame
 * from one interface at a time and services it.
 * */
void *
backend_do_process (void *args)
{
    odr_router *router = (odr_router *)args;
    endpoint_list interface_hub = router->interface_hub;
    char *buffer = calloc(1, ETH_MAX_FRAME_SIZE);

    struct sockaddr_ll sll;
    LOGV("router3 %x %d", router, router->ipc_sock);
    do
    {
        fd_set rdfds;
        WATCH_CALL(wait_for_ready_sockets(interface_hub, &rdfds), "select");

        endpoint *t;
        for (t = interface_hub; NULL != t; t = t->next)
        {
            if (0 == FD_ISSET(t->sock, &rdfds))
            {
                continue;
            }

            memset(&sll, 0, sizeof(struct sockaddr_ll));
            sll.sll_family = AF_PACKET;
            sll.sll_protocol = htons(ETH_P_ODR);
            sll.sll_ifindex = t->index;
            socklen_t sin_l = sizeof(struct sockaddr_ll);
            if (0 < recvfrom(t->sock, buffer, ETH_MAX_FRAME_SIZE, MSG_DONTWAIT,
                    (struct sockaddr *)&sll, &sin_l))
            {
                process_incoming_ethernet_frame(router, buffer, t->index);
                memset(buffer, 0, ETH_MAX_FRAME_SIZE);
            }
        }
    } while (FALSE == router->stopping);


    router->stopping = TRUE;
    free(buffer);
    LOGV("backend closing");
    return NULL;
}
