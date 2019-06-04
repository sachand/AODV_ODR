#include "arp_header.h"

void
htona (arp_header *hdr)
{
    hdr->eth_header.ethertype = htons(hdr->eth_header.ethertype);
    hdr->ethertype = htons(hdr->ethertype);
    hdr->hardware_type = htons(hdr->hardware_type);
    hdr->protocol_type = htons(hdr->protocol_type);
    hdr->opcode = htons(hdr->opcode);
}

void
ntoha (arp_header *hdr)
{
    hdr->eth_header.ethertype = ntohs(hdr->eth_header.ethertype);
    hdr->ethertype = ntohs(hdr->ethertype);
    hdr->hardware_type = ntohs(hdr->hardware_type);
    hdr->protocol_type = ntohs(hdr->protocol_type);
    hdr->opcode = ntohs(hdr->opcode);
}
