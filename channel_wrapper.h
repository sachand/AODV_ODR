#pragma once

msg_iovec
create_msg (msg_header *header, char *payload, uint32_t payload_length);

msg_iovec
clone_msg (msg_header *header, char *payload, uint32_t payload_length);

msg_iovec
create_msg_max (void);

void
delete_msg (msg_iovec msg);

int
send_msg (SOCKET sock, char *destination_cip, uint16_t destination_port,
        char *buf, int len, boolean force_route_discovery);

int
recv_msg (SOCKET sock, char *source_cip, uint16_t *source_port,
        char *buf, int *len, uint32_t timeout_ms);
