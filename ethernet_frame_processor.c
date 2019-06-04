#include "app_settings.h"

#ifndef LOG_TAG
#define LOG_TAG "EthFrameProc"
#endif

/**
 * Creates an ethernet frame header.
 * 
 * @param frame     Ethernet frame received over raw sock
 * 
 * @return Pointer to freshly alloc-ed memory that contains frame header.
 *         Caller's responsibility to free the memory
 * */
ethernet_frame_header *
extract_ethernet_frame_header(char *frame)
{
    ethernet_frame_header *h = calloc(1, sizeof(ethernet_frame_header));
    memcpy(h->destination_hardware_address, frame, HW_ADDRLEN);
    frame += HW_ADDRLEN;
    memcpy(h->source_hardware_address, frame, HW_ADDRLEN);
    frame += HW_ADDRLEN;

    sscanf(frame, "%u", h->ethertype);
    h->ethertype = ntohl(h->ethertype);

    return h;
}

// A copy of this is in socket_common.c for SEND. We need two copies because
// two threads access this.
static char *
get_hardware_address_string (uint8_t *addr, boolean first)
{
    static char addr_string[40];
    char *temp = (TRUE == first) ? addr_string : addr_string + 20;
    memset(temp, 0, 20);
    int i;
    for (i = 0; i < HW_ADDRLEN; ++i)
    {
        temp += sprintf(temp, "%.2x:", addr[i]);
    }
    --temp;
    *temp = '\0';
    return (TRUE == first) ? addr_string : addr_string + 20;
}

/**
 * Whether this node is the source
 * */
boolean
is_created_by_self (odr_router *router, msg_header *header)
{
    return (0 == strcmp(router->self_canonical_ip, header->source_id))
            ? TRUE : FALSE;
}

/**
 * Whether this node is the destination
 * */
boolean
is_destined_to_self (odr_router *router, msg_header *header)
{
    return (0 == strcmp(router->self_canonical_ip, header->destination_id))
            ? TRUE : FALSE;
}

/**
 * To be invoked when a message is received by ODR. It extracts important
 * information from the message like route to source.
 * 
 * @param router            Router object
 * @param eth_header        Ethernet header object
 * @param m_header          msg-header object
 * @param interface_index   Interface index in which this message arrived.
 * 
 * @return TRUE if information in this message is new and better, FALSE
 *         otherwise.
 * */
boolean
extract_base_info (odr_router *router, ethernet_frame_header *eth_header,
        msg_header *header, interface interface_index)
{
    routing_table_entry rte;
    memset(&rte, 0, sizeof(routing_table_entry));
    memcpy(rte.destination, header->source_id, INET_ADDRSTRLEN);
    memcpy(rte.next_hop, eth_header->source_hardware_address, HW_ADDRLEN);
    rte.outgoing_if = interface_index;
    rte.hop_count = header->hop_count + 1;
    
    return update_table(&(router->rt_table), &rte);
}

/**
 * Relays a broadcast REQ.
 * 
 * @param router            Router object
 * @param m_header          msg-header object
 * @param interface_index   Interface index in which this message arrived.
 * @param better_route      Whether the received message gave better info to
 *                          the routing table
 * @param rte               Routing table entry for the message's destination
 * 
 * @return Number of interfaces on which broadcast was done
 * */
int
relay_broadcast (odr_router *router, msg_header *m_header, interface interface_index,
        boolean better_route, routing_table_entry *rte)
{
    endpoint_list list = router->interface_hub;

    if (TRUE == have_only_one_interface(list) || (
            FALSE == should_relay_broadcast(rte, m_header->broadcast_id) &&
            FALSE == better_route))
    {
        return 0;
    }
    
    struct iovec msg_iov[2];
    msg_iov[0].iov_base = m_header;
    msg_iov[0].iov_len = HEADER_SIZE;
    msg_iov[1].iov_base = m_header + 1;
    msg_iov[1].iov_len = m_header->payload_length;
    
    return socket_l2_flood_msg(list, find_endpoint(list, interface_index),
            msg_iov);
}

/**
 * Processes an incoming message with REQ bit set
 * 
 * @param router            Router object
 * @param m_header          msg-header object
 * */
void
process_incoming_dat (odr_router *router, msg_header *m_header)
{
    msg_iovec msg = clone_msg(m_header, m_header + 1,
            m_header->payload_length);
    m_header = msg[0].iov_base;
    
    if (TRUE == is_destined_to_self(router, m_header))
    {
        odr_route_data(router, msg);
    }
    else
    {
        odr_route_msg(router, msg);
    }
    delete_msg(msg);
}

/**
 * Processes an incoming message with REP bit set
 * 
 * @param router            Router object
 * @param eth_header        Ethernet header object
 * @param m_header          msg-header object
 * @param interface_index   Interface index in which this message arrived.
 * @param better_route      Whether the received message gave better info to
 *                          the routing table
 * @param rte               Routing table entry for the message's destination
 * */
void
process_incoming_rep (odr_router *router, ethernet_frame_header *eth_header,
        msg_header *m_header, interface interface_index,
        routing_table_entry *rte)
{
    endpoint_list list = router->interface_hub;
    endpoint *e = find_endpoint(list, interface_index);
    msg_iovec msg = clone_msg(m_header, NULL, 0);
    m_header = msg[0].iov_base;
    
    if (TRUE == is_destined_to_self(router, m_header))
    {
        odr_car *car;
        rte = find_routing_table_entry(router->rt_table, m_header->source_id);
        for (car = rte->parking_lot; NULL != car; car = car->next)
        {
            LOGV("Unparking");
            socket_l2_send_unicast(e, car->msg, eth_header->source_hardware_address);
        }
        destroy_odr_parking_lot(rte->parking_lot);
        rte->parking_lot = NULL;
        rte->finding_route = FALSE;
    }
    else
    {
        if (FALSE == is_stale(rte))
        {
            LOGS("Not Stale entry");
            socket_l2_send_unicast(find_endpoint(router->interface_hub,
                    rte->outgoing_if), msg, rte->next_hop);
        }
        else
        {
            LOGS("Stale entry");
            park_msg(&(router->rt_table), msg);
            broadcast_req(router, m_header->destination_id, rte,
                is_flag_set(m_header->flags, MSG_HEADER_FLAG_FRC), e);
        }
    }
    delete_msg(msg);
}

/**
 * Processes an incoming message with REQ bit set
 * 
 * @param router            Router object
 * @param eth_header        Ethernet header object
 * @param m_header          msg-header object
 * @param interface_index   Interface index in which this message arrived.
 * @param better_route      Whether the received message gave better info to
 *                          the routing table
 * @param rte               Routing table entry for the message's destination
 * */
void
process_incoming_req (odr_router *router, ethernet_frame_header *eth_header,
        msg_header *m_header, interface interface_index, boolean better_route,
        routing_table_entry *rte)
{
    endpoint_list list = router->interface_hub;
    endpoint *e = find_endpoint(list, interface_index);
    msg_iovec msg = clone_msg(m_header, NULL, 0);
    m_header = msg[0].iov_base;
    boolean rep_sent = is_flag_set(m_header->flags, MSG_HEADER_FLAG_REP);
    routing_table_entry *source_rte = query_table(&(router->rt_table),
            m_header->source_id);
    
    if (FALSE == is_stale(rte))
    {
        LOGS("Not Stale entry");
        m_header->flags |= MSG_HEADER_FLAG_REP;
        if (TRUE == better_route &&
                FALSE == is_flag_set(m_header->flags, MSG_HEADER_FLAG_FRC) &&
                FALSE == is_destined_to_self(router, m_header))
        {
            socket_l2_flood_msg(list, e, msg);
        }

        //if (FALSE == rep_sent &&
        //        source_rte->last_replied_broadcast < m_header->broadcast_id ||
        //        (source_rte->last_replied_broadcast == m_header->broadcast_id &&
        //        TRUE == better_route))
        if (FALSE == rep_sent)
        {
            source_rte->last_replied_broadcast = m_header->broadcast_id;
            m_header->hop_count = rte->hop_count;
            swap_roles(m_header);
            m_header->flags &= ~(MSG_HEADER_FLAG_REQ);

            socket_l2_send_unicast(e, msg, eth_header->source_hardware_address);
        }
    }
    else
    {
        LOGS("Stale entry");
        relay_broadcast(router, m_header, interface_index, better_route,
                source_rte);
    }
    delete_msg(msg);
}

/**
 * --------------------- THE CORE --------------------- 
 * Reads an incoming ethernet frame and sends appropriate reply for it.
 * Note that reading a frame and replying for it is an atomic operation
 * iff the message can be immediately serviced.
 * 
 * @param router            Router object
 * @param frame             Received ethernet frame
 * @param interface_index   Interface index in which this message arrived.
 * */
void
process_incoming_ethernet_frame (odr_router *router, char *frame,
        interface interface_index)
{
    ethernet_frame_header *eth_header = extract_ethernet_frame_header(frame);
    frame += ETH_FRAME_HEADER_SIZE;
    
    msg_header *m_header = (msg_header *)frame;
    frame += HEADER_SIZE;

    ntohm(m_header);
    LOGS("RECV FRAME L2: src=%s dst=%s L3: Src=VM%d:%u Dest=VM%d:%u BroadcastId=%d Flags=%s Len=%u Hop=%d",
            get_hardware_address_string(eth_header->source_hardware_address, TRUE),
            get_hardware_address_string(eth_header->destination_hardware_address, FALSE),
            get_vm_index(m_header->source_id), m_header->source_port,
            get_vm_index(m_header->destination_id), m_header->destination_port,
            m_header->broadcast_id,
            msg_header_flags_to_string(m_header->flags), m_header->payload_length,
            m_header->hop_count);

    if (TRUE == is_flag_set(m_header->flags, MSG_HEADER_FLAG_FRC))
    {
        if (FALSE == is_flag_set(m_header->flags, MSG_HEADER_FLAG_REP))
        {
            purge_entry(&(router->rt_table), m_header->destination_id);
        }
        purge_entry(&(router->rt_table), m_header->source_id);
    }

    // Extract useful info from the msg_header, if any
    m_header->hop_count += 1;
    boolean better_route = extract_base_info(router, eth_header,
            m_header, interface_index);
    routing_table_entry *rte = query_table(&(router->rt_table),
            m_header->destination_id);

    msg_header_flags_to_string(m_header->flags);
    if (TRUE == is_flag_set(m_header->flags, MSG_HEADER_FLAG_REQ) &&
            FALSE == is_created_by_self(router, m_header))
    {
        process_incoming_req(router, eth_header, m_header, interface_index,
                better_route, rte);
    }
    else if (TRUE == is_flag_set(m_header->flags, MSG_HEADER_FLAG_REP))
    {
        process_incoming_rep(router, eth_header, m_header, interface_index,
                rte);
    }
    else if (TRUE == is_flag_set(m_header->flags, MSG_HEADER_FLAG_DAT))
    {
        process_incoming_dat(router, m_header);
    }

    free(eth_header);
}
