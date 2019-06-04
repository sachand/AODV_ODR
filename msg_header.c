#include "app_settings.h"

#ifndef LOG_TAG
#define LOG_TAG "MsgHeader"
#endif

msg_header *
get_new_header ()
{
    return (msg_header *) calloc(1, sizeof(msg_header));
}

char *
msg_header_flags_to_string (uint16_t flags)
{
    static char flag_string[80];
    char *temp = flag_string;
    memset(temp, 0, 80);

    temp += sprintf(temp, "%s", "_");
    
    if (0 != (flags & MSG_HEADER_FLAG_REQ))
    {
        temp += sprintf(temp, "%s", "REQ_");
    }
    if (0 != (flags & MSG_HEADER_FLAG_REP))
    {
        temp += sprintf(temp, "%s", "REP_");
    }
    if (0 != (flags & MSG_HEADER_FLAG_DAT))
    {
        temp += sprintf(temp, "%s", "DAT_");
    }
    if (0 != (flags & MSG_HEADER_FLAG_FRC))
    {
        temp += sprintf(temp, "%s", "FRC_");
    }
    if (0 != (flags & MSG_HEADER_FLAG_ERR))
    {
        temp += sprintf(temp, "%s", "ERR_");
    }

    return flag_string;
}

int
get_vm_index (char *vm_canonocal_ip)
{
    if (0 != memcmp(VM_PREFIX, vm_canonocal_ip, strlen(VM_PREFIX)))
    {
        return 0;
    }

    switch (vm_canonocal_ip[strlen(VM_PREFIX)])
    {
    case '0': return 10;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    }

    return 0;
}

void
print_msg_header (msg_header *header, char *prefix)
{
    LOGS("%s Dest=VM%d:%u Src=VM%d:%u BroadcastId=%d Flags=%s Len=%u Hop=%d", prefix,
            get_vm_index(header->destination_id), header->destination_port,
            get_vm_index(header->source_id), header->source_port, header->broadcast_id,
            msg_header_flags_to_string(header->flags), header->payload_length,
            header->hop_count);
}

/**
 * Swaps desitnation and source id and port in the header
 * */
void
swap_roles (msg_header *header)
{
    char temp_net[INET_ADDRSTRLEN];
    uint16_t temp_port;
    
    memcpy(temp_net, header->source_id, INET_ADDRSTRLEN);
    temp_port = header->source_port;
    memcpy(header->source_id, header->destination_id, INET_ADDRSTRLEN);
    header->source_port = header->destination_port;
    memcpy(header->destination_id, temp_net, INET_ADDRSTRLEN);
    header->destination_port = temp_port;
}

void
htonm (msg_header *header)
{
    header->source_port = htons(header->source_port);
    header->destination_port = htons(header->destination_port);
    header->broadcast_id = htonl(header->broadcast_id);
    header->flags = htons(header->flags);
    header->hop_count = htons(header->hop_count);
    header->payload_length = htons(header->payload_length);
}

void
ntohm (msg_header *header)
{
    header->source_port = ntohs(header->source_port);
    header->destination_port = ntohs(header->destination_port);
    header->broadcast_id = ntohl(header->broadcast_id);
    header->flags = ntohs(header->flags);
    header->hop_count = ntohs(header->hop_count);
    header->payload_length = ntohs(header->payload_length);
}
