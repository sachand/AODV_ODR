#include "app_settings.h"

extern float g_staleness;

boolean
is_stale (routing_table_entry *e)
{
    if (NULL == e)
    {
        return TRUE;
    }

    long time_elapsed = (long)get_current_system_time_millis() -
            e->timestamp;
    return (time_elapsed >= g_staleness) ? TRUE : FALSE;
}

boolean
is_better (routing_table_entry *e1, routing_table_entry *e2)
{
    if (TRUE == is_stale(e2))
    {
        return TRUE;
    }

    if (e1->hop_count < e2->hop_count ||
            (e1->hop_count == e2->hop_count &&
            0 != memcmp(e1->next_hop, e2->next_hop, HW_ADDRLEN)))
    {
        return TRUE;
    }

    return FALSE;
}

routing_table_entry *
query_table (routing_table *t, char *destination)
{
    routing_table_entry *e = find_routing_table_entry(*t, destination);
    if (TRUE == is_stale(e))
    {
        if (NULL == e)
        {
            e = create_routing_table_entry();
            strncpy(e->destination, destination, INET_ADDRSTRLEN);
            insert_in_routing_table(t, e);
        }
        else
        {
            purge_entry(t, destination);
        }
    }

    return e;
}

boolean
update_table (routing_table *t, routing_table_entry *e)
{
    routing_table_entry *entry = find_routing_table_entry(*t, e->destination);
    if (TRUE == is_better(e, entry))
    {
        if (NULL == entry)
        {
            entry = create_routing_table_entry(); 
        }
        strncpy(entry->destination, e->destination, INET_ADDRSTRLEN);
        memcpy(entry->next_hop, e->next_hop, HW_ADDRLEN);
        entry->outgoing_if = e->outgoing_if;
        entry->hop_count = e->hop_count;

        entry->timestamp = get_current_system_time_millis();
        insert_in_routing_table(t, entry);
        return TRUE;
    }
    
    return FALSE;
}

boolean
should_relay_broadcast(routing_table_entry *e, uint32_t broadcast_id)
{
    if (e->last_relayed_broadcast < broadcast_id)
    {
        e->last_relayed_broadcast = broadcast_id;
        return TRUE;
    }
    return FALSE;
}
