#pragma once

#include "extras.h"
#include "socket_common.h"

#define HW_ADDRLEN (6)

typedef uint8_t interface;

typedef struct arp_cache_entry_t
{
    uint32_t l3_address;
    uint8_t l2_address[8];
    uint8_t halen;
    interface outgoing_if;
    uint16_t hatype;
    struct sockaddr_un client;
    
    struct arp_cache_entry_t *next;
} arp_cache_entry;

typedef arp_cache_entry *arp_cache;

void
destroy_arp_cache (arp_cache t);

arp_cache_entry *
create_arp_cache_entry ();

void
destroy_arp_cache_entry (arp_cache_entry *e);

arp_cache_entry *
find_arp_cache_entry (arp_cache t, uint32_t destination);

void
insert_in_arp_cache (arp_cache *t, arp_cache_entry *e, boolean force);
