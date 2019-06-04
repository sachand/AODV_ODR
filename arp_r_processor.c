#include "app_settings.h"

static arp_cache g_arp_cache = NULL;

void
reply_for_cache_entry (arp_cache_entry *e, SOCKET frontend_socket)
{
    sendto(frontend_socket, e->l2_address, e->halen, 0,
            &(e->client), sizeof(struct sockaddr_un));
}

void
send_req(arp_cache_entry *e_to_txlate, endpoint *p, SOCKET frontend_socket,
        SOCKET backend_socket)
{
    arp_cache_entry *e = find_arp_cache_entry(g_arp_cache, e_to_txlate->l3_address);

    if (NULL != e)
    {
        reply_for_cache_entry(e, frontend_socket);
    }
    else
    {
        arp_header hdr;
        memset(&hdr, 0, sizeof(arp_header));
        memset(hdr.eth_header.destination_hardware_address, 0xFF, HW_ADDRLEN);
        memcpy(hdr.eth_header.source_hardware_address, p->hardware_address, HW_ADDRLEN);
        hdr.eth_header.ethertype = ETH_P_ARP;
        hdr.ethertype = ETH_P_ARP; // check again
        hdr.hardware_type = 1;
        hdr.protocol_type = 0x800;
        hdr.hardware_size = HW_ADDRLEN;
        hdr.protocol_size = 4;
        hdr.opcode = 1;

        struct sockaddr_ll sll;
        memset(&sll, 0, sizeof(struct sockaddr_ll));
        sll.sll_family = AF_PACKET;
        sll.sll_protocol = htons(ETH_P_ARP);
        sll.sll_ifindex = e_to_txlate->outgoing_if;
        sll.sll_halen = HW_ADDRLEN;
        memset(sll.sll_addr, 0xFF, HW_ADDRLEN);

        struct iovec msg_iov[5];
        
        htona(&hdr);
        msg_iov[0].iov_base = &hdr;
        msg_iov[0].iov_len = sizeof(arp_header);

        msg_iov[1].iov_base = hdr.eth_header.source_hardware_address;
        msg_iov[1].iov_len = HW_ADDRLEN;
        uint32_t dummy1 = get_canonical_nip();
        msg_iov[2].iov_base = &dummy1;
        msg_iov[2].iov_len = 4;

        char brd[HW_ADDRLEN];
        memset(brd, 0xFF, HW_ADDRLEN);
        msg_iov[3].iov_base = brd;
        msg_iov[3].iov_len = HW_ADDRLEN;
        uint32_t dummy2 = e_to_txlate->l3_address;
        msg_iov[4].iov_base = &dummy2;
        msg_iov[4].iov_len = 4;

        struct msghdr msg_msghdr;
        memset(&msg_msghdr, 0, sizeof(struct msghdr));
        msg_msghdr.msg_name = &sll;
        msg_msghdr.msg_namelen = sizeof(struct sockaddr_ll);
        msg_msghdr.msg_iov = msg_iov;
        msg_msghdr.msg_iovlen = 5;
        
        int ret = sendmsg(backend_socket, &msg_msghdr, MSG_DONTWAIT);
        ntoha(&hdr);
    }
}

void
processor_send_req (endpoint *p, SOCKET frontend_socket,
        SOCKET backend_socket)
{
    char msg[256] = { '\0' };
    struct sockaddr_un client;
    socklen_t socklen = sizeof(struct sockaddr_un);
    memset(&client, 0, sizeof(struct sockaddr_un));

    // TODO: Assumes you received whole thing.
    int len;
    int bytes_recv = recvfrom(frontend_socket, &len, 4, 0, &client, &socklen);
    if (bytes_recv <= 0)
    {
        return;
    }
    
    if (len != sizeof(struct sockaddr_in))
    {
        return;
    }

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    socklen = sizeof(struct sockaddr_un);
    bytes_recv = recvfrom(frontend_socket, &sin, len, 0, &client, &socklen);
    if (bytes_recv <= 0)
    {
        return;
    }


    hwaddr l2_addr;
    memset(&l2_addr, 0, sizeof(hwaddr));
    bytes_recv = recvfrom(frontend_socket, &l2_addr, sizeof(hwaddr), 0, &client, &len);
    if (bytes_recv <= 0)
    {
        return;
    }

    arp_cache_entry e;
    memset(&e, 0, sizeof(arp_cache_entry));
    e.l3_address = sin.sin_addr.s_addr;
    e.outgoing_if = l2_addr.index;
    e.hatype = l2_addr.hatype;
    e.client = client;
    send_req(&e, p, frontend_socket, backend_socket);
}

void
processor_recv_reqrep (endpoint *e, SOCKET frontend_socket,
        SOCKET backend_socket)
{
    char msg[256] = { '\0' };
    arp_header hdr;
    memset(&hdr, 0, sizeof(arp_header));
    struct sockaddr_ll sll;

    memset(&sll, 0, sizeof(struct sockaddr_ll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ARP);
    sll.sll_ifindex = e->index;
    socklen_t sin_l = sizeof(struct sockaddr_ll);
    if (0 >= recvfrom(backend_socket, &hdr, sizeof(arp_header), MSG_DONTWAIT,
            (struct sockaddr *)&sll, &sin_l))
    {
        return;
    }

    ntoha(&hdr);
    boolean not_ours = (ETH_P_ARP != hdr.ethertype) ? TRUE : FALSE;

    int addresses_sizes = ((int)hdr.hardware_size + (int)hdr.protocol_size) << 1;

    memset(&sll, 0, sizeof(struct sockaddr_ll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ARP);
    sll.sll_ifindex = e->index;
    if (0 >= recvfrom(backend_socket, msg, addresses_sizes, MSG_DONTWAIT,
            (struct sockaddr *)&sll, &sin_l))
    {
        return;
    }
    
    if (TRUE == not_ours) { return; }

    boolean force_enter = (hdr.opcode == 1) ? FALSE : TRUE;
    arp_cache_entry *ace = create_arp_cache_entry();

    memcpy(&(ace->l2_address), msg, (int)hdr.hardware_size);
    ace->halen = hdr.hardware_size;
    
    uint32_t source_nip = *(uint32_t *)(msg + (int)hdr.hardware_size);
    ace->l3_address = source_nip;

    ace->outgoing_if = e->index;
    insert_in_arp_cache(g_arp_cache, ace, force_enter);

    int target_l3_address_offset = addresses_sizes - (int)hdr.protocol_size;
    if (1 == hdr.opcode &&
            (*(uint32_t *)(msg + target_l3_address_offset)) == get_canonical_nip())
    {
        memcpy(hdr.eth_header.destination_hardware_address,
                hdr.eth_header.source_hardware_address, HW_ADDRLEN);
        memcpy(hdr.eth_header.source_hardware_address, e->hardware_address, HW_ADDRLEN);
        hdr.eth_header.ethertype = ETH_P_ARP;
        hdr.ethertype = ETH_P_ARP; // check again
        hdr.hardware_type = 1;
        hdr.protocol_type = 0x800;
        hdr.hardware_size = HW_ADDRLEN;
        hdr.protocol_size = 4;
        hdr.opcode = 2;

        struct sockaddr_ll sll;
        memset(&sll, 0, sizeof(struct sockaddr_ll));
        sll.sll_family = AF_PACKET;
        sll.sll_protocol = htons(ETH_P_ARP);
        sll.sll_ifindex = e->index;
        sll.sll_halen = HW_ADDRLEN;
        memcpy(sll.sll_addr, hdr.eth_header.destination_hardware_address,
                HW_ADDRLEN);

        struct iovec msg_iov[5];
        
        htona(&hdr);
        msg_iov[0].iov_base = &hdr;
        msg_iov[0].iov_len = sizeof(arp_header);

        msg_iov[1].iov_base = hdr.eth_header.source_hardware_address;
        msg_iov[1].iov_len = HW_ADDRLEN;
        uint32_t dummy1 = get_canonical_nip();
        msg_iov[2].iov_base = &dummy1;
        msg_iov[2].iov_len = 4;

        char brd[HW_ADDRLEN];
        memcpy(brd, hdr.eth_header.destination_hardware_address, HW_ADDRLEN);
        msg_iov[3].iov_base = brd;
        msg_iov[3].iov_len = HW_ADDRLEN;
        uint32_t dummy2 = ace->l3_address;
        msg_iov[4].iov_base = &dummy2;
        msg_iov[4].iov_len = 4;

        struct msghdr msg_msghdr;
        memset(&msg_msghdr, 0, sizeof(struct msghdr));
        msg_msghdr.msg_name = &sll;
        msg_msghdr.msg_namelen = sizeof(struct sockaddr_ll);
        msg_msghdr.msg_iov = msg_iov;
        msg_msghdr.msg_iovlen = 5;
        
        sendmsg(backend_socket, &msg_msghdr, MSG_DONTWAIT);
    }
    else if (2 == hdr.opcode)
    {
        reply_for_cache_entry(find_arp_cache_entry(g_arp_cache, ace->l3_address),
                frontend_socket);
    }
}
