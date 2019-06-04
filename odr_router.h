#pragma once

/**
 * Representation of a router
 * */
typedef struct odr_router_t
{
    /**
     * SOCKET for IPC. Will be bound to ODR_IPC_ABSFILENAME
     * */
    SOCKET ipc_sock;
    
    /**
     * Canonical IP address of this node. We obtain this by going through
     * interface list and using eth0's IP as canonical
     * */
    char self_canonical_ip[INET_ADDRSTRLEN];
    
    /**
     * Router's routing table
     * */
    routing_table rt_table;
    
    /**
     * List of ODR's clients
     * */
    odr_client_list client_list;
    
    /**
     * List of all interfaces that are of worth for ODR on this node.
     * */
    endpoint_list interface_hub;
    
    /**
     * Flag to specify if the router is stopping
     * */
    boolean stopping;
    
    /**
     * Semaphore to ensure synchronization b/w frontend and backend
     * 
     * @todo Is this needed?
     * */
    sem_t route_signal;
} odr_router;
