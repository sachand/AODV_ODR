#pragma once

#define HEADER_SIZE (sizeof(msg_header))

typedef struct iovec *msg_iovec;

/**
 * Defines a header. Headers are of the form:
 *
 *  0                   1                   2                   3   
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                               |
 *  |                          Source ID                            |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                               |
 *  |                        Destination ID                         |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |           Source Port         |       Destination Port        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Broadcast ID                          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                     |E|F|D|R|R|                               |
 *  |        Unused       |R|R|A|E|E|           Hop Count           |
 *  |                     |R|C|T|P|Q|                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        Payload Length                         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * */
typedef struct msg_header_t
{
    char source_id[INET_ADDRSTRLEN];
    char destination_id[INET_ADDRSTRLEN];
    uint16_t source_port;
    uint16_t destination_port;

    uint32_t broadcast_id;
    uint16_t flags;
    uint16_t hop_count;
    uint32_t payload_length;
} msg_header;

msg_header *
get_new_header ();

char *
msg_header_flags_to_string (uint16_t flags);

void
print_msg_header (msg_header *header, char *prefix);

void
htonm (msg_header *header);

void
ntohm (msg_header *header);
