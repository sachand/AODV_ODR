#include "app_settings.h"

routing_table
create_routing_table (char *self_canonical_ip)
{
    routing_table t = create_routing_table_entry();
    memcpy(t->destination, self_canonical_ip, INET_ADDRSTRLEN);
    t->timestamp = LONG_MAX;
    return t;
}

void
destroy_routing_table (routing_table t)
{
    routing_table_entry *e, *s;
    for (e = t; NULL != e; e = s)
    {
        s = e->next;
        destroy_routing_table_entry(e);
    }
}

routing_table_entry *
create_routing_table_entry ()
{
    return (routing_table_entry *) calloc(1, sizeof(routing_table_entry));
}

void
destroy_routing_table_entry (routing_table_entry *e)
{
    if (NULL == e) { return; }

    destroy_odr_parking_lot(e->parking_lot);
    free(e);
}

routing_table_entry *
find_routing_table_entry (routing_table t, char *destination)
{
    routing_table_entry *e;
    for (e = t; NULL != e; e = e->next)
    {
        if (0 == strcmp(e->destination, destination)) { break; }
    }

    return e;
}

void
insert_in_routing_table (routing_table *t, routing_table_entry *e)
{
    if (NULL == find_routing_table_entry(*t, e->destination))
    {
        routing_table_entry *p = *t;
        *t = e;
        e->next = p;
    }
}

void
remove_routing_table_entry (routing_table *t, routing_table_entry *e)
{
    if (NULL == t || NULL == e)
    {
        return;
    }

    if (*t == e)
    {
        *t = e->next;
        destroy_routing_table_entry(e);
        return;
    }

    routing_table_entry *p, *prev;
    for (prev = *t, p = prev->next; NULL != p; p = p->next)
    {
        if (p == e)
        {
            prev->next = p->next;
            destroy_routing_table_entry(e);
            return;
        }
        prev = p;
    }
}

void
purge_entry (routing_table *t, char *destination)
{
    routing_table_entry *e = find_routing_table_entry(*t, destination);
    if (NULL != e && e != *t)
    {
        memset(e->next_hop, 0, HW_ADDRLEN);
        e->outgoing_if = 0;
        e->hop_count = 0;
        e->timestamp = 0;
    }
}
