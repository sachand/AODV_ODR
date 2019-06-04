#pragma once

#include "arp_cache.h"

/**
 * Maximum size of a wired-ethernet message or a message that enters
 * node's kernel's L2.
 * */
#define ETH_MAX_FRAME_SIZE 1516
#define ETH_FRAME_HEADER_SIZE (sizeof(ethernet_frame_header))

/**
 * Representation of an ethernet header
 * */
typedef struct ethernet_frame_header_t
{
    uint8_t destination_hardware_address[HW_ADDRLEN];
    uint8_t source_hardware_address[HW_ADDRLEN];
    uint16_t ethertype;
} ethernet_frame_header;

ethernet_frame_header *
extract_ethernet_frame_header(char *frame);
