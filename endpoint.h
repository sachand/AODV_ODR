#pragma once

#include "arp_cache.h"
#include "socket_common.h"

/**
 * Stores information of all appropriate interfaces
 * */
typedef struct endpoint_t
{
    /**
     * Socket with which the hub is bound to that interface
     * */
    SOCKET sock;

    /**
     * Interface index and hardware address for the socket specified in
     * this endpoint
     * */
    interface index;
    uint8_t hardware_address[HW_ADDRLEN];

    /**
     * A human readable description of this structure.
     * 
     * @todo Keeping one for each is a bit of an overkill.
     *       Address if time permits.
     * */
    char desc[128];

    struct endpoint_t *next;
} endpoint;

typedef endpoint *endpoint_list;

endpoint_list
get_endpoint_list ();

endpoint *
find_endpoint (endpoint_list list, interface index);

char *
endpoint_to_string (endpoint *p);

endpoint_list
create_listen_hub ();
