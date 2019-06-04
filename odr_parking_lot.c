#include "app_settings.h"

#ifndef LOG_TAG
#define LOG_TAG "ODR_ParkingLot"
#endif

void
destroy_odr_car (odr_car *e)
{
    if (NULL == e) { return; }

    delete_msg(e->msg);
    free(e);
}

void
destroy_odr_parking_lot (odr_parking_lot l)
{
    odr_car *e, *s;
    for (e = l; NULL != e; e = s)
    {
        s = e->next;
        destroy_odr_car(e);
    }
}

odr_car *
create_odr_car (msg_iovec msg)
{
    odr_car *t = (odr_car *) calloc(1, sizeof(odr_car));
    msg_header *header = msg[0].iov_base;
    t->msg = clone_msg(header, msg[1].iov_base, header->payload_length);
    header = t->msg[0].iov_base;
    header->flags &= ~(MSG_HEADER_FLAG_FRC);
    print_msg_header(header, "PARK");
    return t;
}

odr_car *
find_odr_car (odr_parking_lot opl, msg_header *header)
{
    if (NULL == opl || NULL == header)
    {
        return NULL;
    }

    odr_car *p = NULL;
    msg_header *p_header;
    for (p = opl; NULL != p; p = p->next)
    {
        p_header = p->msg[0].iov_base;
        if (p_header->broadcast_id == header->broadcast_id) { break; }
    }
    
    return p;
}

void
insert_in_odr_parking_lot (odr_parking_lot *t, odr_car *e)
{
    remove_odr_car(t, find_odr_car(*t, e->msg[0].iov_base));
    odr_car *p = *t;
    *t = e;
    e->next = p;
}

void
remove_odr_car (odr_parking_lot *t, odr_car *e)
{
    if (NULL == t || NULL == e)
    {
        return;
    }

    if (*t == e)
    {
        *t = e->next;
        destroy_odr_car(e);
        return;
    }

    odr_car *p, *prev;
    for (prev = *t, p = prev->next; NULL != p; p = p->next)
    {
        if (p == e)
        {
            prev->next = p->next;
            destroy_odr_car(e);
            return;
        }
        prev = p;
    }
}

void
park_msg (routing_table *t, msg_iovec msg)
{
    msg_header *header = msg[0].iov_base;
    routing_table_entry *rte = find_routing_table_entry(*t, header->destination_id);
    
    if (NULL == rte)
    {
        rte = create_routing_table_entry();
        insert_in_routing_table(t, rte);
        memcpy(rte->destination, header->destination_id, INET_ADDRSTRLEN);
    }
    else
    {
        odr_car *c = find_odr_car(rte->parking_lot, header);
        if (NULL != c)
        {
            msg_header *c_header = c->msg[0].iov_base;
            if (c_header->hop_count < header->hop_count) { return; }
        }
    }
    insert_in_odr_parking_lot(&(rte->parking_lot), create_odr_car(msg));
    LOGS("parking");
    print_msg_header(rte->parking_lot->msg[0].iov_base, "CHK");
}
