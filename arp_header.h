#pragma once

#include "ethernet_frame.h"
#include <stdint.h>

/**
 * Partial representation of an ARP header. This struct represents
 * the constant sized parts of header.
 * */
typedef struct arp_header_t
{
    ethernet_frame_header eth_header;
    
    uint16_t ethertype;
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t hardware_size;
    uint8_t protocol_size;
    uint16_t opcode;
} arp_header;

void
htona(arp_header *hdr);

void
ntoha(arp_header *hdr);
