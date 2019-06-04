#include "app_settings.h"

arp_cache_entry *
create_arp_cache_entry ()
{
    return (arp_cache_entry *) calloc(1, sizeof(arp_cache_entry));
}

void
destroy_arp_cache_entry (arp_cache_entry *e)
{
    if (NULL == e) { return; }

    free(e);
}

void
destroy_arp_cache (arp_cache t)
{
    arp_cache_entry *e, *s;
    for (e = t; NULL != e; e = s)
    {
        s = e->next;
        destroy_arp_cache_entry(e);
    }
}

arp_cache_entry *
find_arp_cache_entry (arp_cache t, uint32_t l3_address)
{
    arp_cache_entry *e;
    for (e = t; NULL != e; e = e->next)
    {
        if (e->l3_address == l3_address) { break; }
    }

    return e;
}

void
__update_cache_entry(arp_cache_entry *e_old, arp_cache_entry *e_new)
{
    e_old->l3_address = e_new->l3_address;
    memcpy(e_old->l2_address, e_new->l2_address, HW_ADDRLEN);
    e_old->outgoing_if = e_new->outgoing_if;
    e_old->halen = e_new->halen;
}

void
insert_in_arp_cache (arp_cache *t, arp_cache_entry *e_new, boolean force)
{
    arp_cache_entry *e_old = find_arp_cache_entry(*t, e_new->l3_address);
    
    if (NULL != e_old)
    {
        __update_cache_entry(e_old, e_new);
    }
    else
    {
        if (TRUE == force)
        {
            arp_cache_entry *entry = create_arp_cache_entry();
            memcpy(entry, e_new, sizeof(arp_cache_entry));
            arp_cache_entry *p = *t;
            *t = entry;
            entry->next = p;
        }
    }
}
