#include "app_settings.h"

#define LOG_TAG "IARP"

int
areq (struct sockaddr* l3_address, socklen_t len, hwaddr *l2_addr,
        int timeout_ms)
{
    int ret = SOCKET_ERROR;
    char *my_file_name = calloc(1, 90);
    SOCKET sock = bind_ipc(my_file_name, FALSE);
    struct sockaddr_un *arp_sun = get_sun(ARP_IPC_ABSFILENAME);
    connect(sock, arp_sun, sizeof(struct sockaddr_un));

    arp_cache_entry e;
    memset(&e, 0, sizeof(arp_cache_entry));
    int msg_size = sizeof(int) + len + sizeof(hwaddr);
    char *msg = calloc(1, msg_size);
    char *temp = msg;
    
    int ilen = len;
    memcpy(temp, &ilen, sizeof(socklen_t));
    temp += sizeof(socklen_t);
    memcpy(temp, l3_address, len);
    temp += len;
    memcpy(temp, l2_addr, sizeof(hwaddr));
    
    if (0 >= send(sock, msg, msg_size, 0))
    {
        LOGE("Send to ARP failed");
    }
    else
    {
        int rsc = ready_socket_count(sock, timeout_ms);
        if (0 < rsc)
        {
            // TODO: Assumes you read everything
            int bytes_recv = recv(sock, msg, HW_ADDRLEN, MSG_WAITALL);
            if (0 < bytes_recv)
            {
                memcpy(l2_addr->l2_address, msg, HW_ADDRLEN);
                ret = 1;
            }
            else if (0 == bytes_recv)
            {
                LOGI("Connection closed by ARP");
            }
            else
            {
                LOGE("Error while receiving from ARP: %s", errno_string());
            }
        }
        else if (0 == rsc)
        {
            LOGI("Did not hear anything from ARP");
            errno = ETIMEDOUT;
        }
        else
        {
            LOGE("Error while waiting for ARP reply: %s", errno_string());
        }
    }
    
    free(arp_sun);
    free(my_file_name);
    free(msg);
    return ret;
}
