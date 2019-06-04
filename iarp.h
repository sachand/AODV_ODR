#pragma once

typedef struct hwaddr_t
{
    interface index;
    uint16_t  hatype;
    uint8_t   halen;
    uint8_t   l2_address[8];
} hwaddr;

int
areq (struct sockaddr* l3_address, socklen_t len, hwaddr *l2_address,
        int timeout_ms);
