#include "app_settings.h"
#include "hw_addrs.h"

#ifndef LOG_TAG
#define LOG_TAG "Endpoint"
#endif

/**
 * Checks if interface is loopback
 * */
boolean
is_loopback (struct hwa_info *i)
{
    return (0 == strcmp(i->if_name, "lo\0")) ? TRUE : FALSE;
}

/**
 * Checks if interface is loopback
 * */
boolean
is_canonical_interface (struct hwa_info *i)
{
    return (0 == strcmp(i->if_name, "eth0")) ? TRUE : FALSE;
}

/**
 * Checks if hardware address is valid
 * */
boolean
is_address_valid (struct hwa_info *i)
{
    boolean zero_address = TRUE;

    int j;
    for (j = 0; j < HW_ADDRLEN; ++j)
    {
        if ('\0' != i->if_haddr[j])
        {
            zero_address = FALSE;
            break;
        }
    }

    return (TRUE == zero_address) ? FALSE : TRUE;
}

/**
 * Removes AND destroys an endpoint entry from given list.
 * */
void
remove_endpoint (endpoint_list *list, endpoint *record)
{
    if (record == *list)
    {
        *list = record->next;
    }
    else
    {
        endpoint *t, *s;
        for (t = *list; NULL != t; t = t->next)
        {
            if (t == record)
            {
                s->next = t->next;
                break;
            }
            s = t;
        }
    }

    free(record);
}

/**
 * Inserts an endpoint in the given list in decreasing order of
 * interface index
 * */
static void
insert_endpoint (endpoint_list *list, endpoint *record)
{
    if (NULL == record) { return; }

    if (NULL == *list)
    {
        *list = record;
        return;
    }

    endpoint *t, *prev = NULL;
    boolean recorded = FALSE;
    for (t = *list; NULL != t; t = t->next)
    {
        if (t->index < record->index)
        {
            recorded = TRUE;
            if (NULL != prev)
            {
                prev->next = record;
            }
            else
            {
                *list = record;
            }
            record->next = t;
            break;
        }
        prev = t;
    }

    if (FALSE == recorded)
    {
        prev->next = record;
        record->next = NULL;
    }
}

/**
 * Creates and returns a list of all appropriate endpoints - up, unicast
 * and not wildcard.
 * */
endpoint_list
get_endpoint_list ()
{
    struct hwa_info *head = get_hw_addrs(AF_INET, 1);
    endpoint_list if_list = NULL;

    struct hwa_info *t = NULL;
    for (t = head; NULL != t; t = t->hwa_next)
    {
		LOGI("Analyzing interface: %s", t->if_name);
        
        if (TRUE == is_loopback(t))
        {
            LOGI("Skipping interface %s. IS LOOPBACK", t->if_name);
            continue;
        }

        if (FALSE == is_canonical_interface(t))
        {
            LOGI("Skipping interface %s. IS NOT CANONICAL", t->if_name);
            continue;
        }

        if (FALSE == is_address_valid(t))
        {
            LOGI("Skipping interface %s. ADDRESS INVALID", t->if_name);
            continue;
        }

        LOGI("Monitoring interface %s", t->if_name);
        endpoint *if_record = (endpoint *) calloc(1, sizeof(endpoint));
        memcpy(if_record->hardware_address, t->if_haddr, HW_ADDRLEN);
        if_record->index = t->if_index;

        insert_endpoint(&if_list, if_record);
    }

    printf("\n");
    free_hwa_info(head);
    return if_list;
}

void
get_canonical_ip (char *ip)
{
    struct hwa_info *head = get_hw_addrs(AF_INET, 1);
    struct hwa_info *t = NULL;
    for (t = head; NULL != t; t = t->hwa_next)
    {
        if (TRUE == is_canonical_interface(t))
        {
            struct sockaddr_in *sa = t->ip_addr;
            inet_ntop(AF_INET, &(sa->sin_addr), ip, INET_ADDRSTRLEN);
            break;
        }
    }

    free_hwa_info(head);
}

uint32_t
get_canonical_nip ()
{
    struct hwa_info *head = get_hw_addrs(AF_INET, 1);
    struct hwa_info *t = NULL;
    for (t = head; NULL != t; t = t->hwa_next)
    {
        if (TRUE == is_canonical_interface(t))
        {
            struct sockaddr_in *sa = t->ip_addr;
            free_hwa_info(head);
            return sa->sin_addr.s_addr;
        }
    }

    free_hwa_info(head);
    return 0;
}

/**
 * Destroys the given endpoint list
 * */
void
free_endpoint_list (endpoint_list l)
{
    if (NULL == l) { return; }

    if (NULL == l->next)
    {
        close(l->sock);
        free(l);
        return;
    }

    endpoint *t;
    for (t = l->next; NULL != t; t = t->next)
    {
        close(l->sock);
        free(l);
        l = t;
    }

    close(l->sock);
    free(l);
}

endpoint *
find_endpoint (endpoint_list list, interface index)
{
    endpoint *t;
    for (t = list; NULL != t; t = t->next)
    {
        if (index == t->index) { break; }
    }
    
    return t;
}

/**
 * Does this node have only one interface. Useful while flooding
 * */
boolean
have_only_one_interface (endpoint_list list)
{
    return (NULL == list || NULL == list->next) ? TRUE : FALSE;
}

/**
 * Utility function to get human readable description of given endpoint
 * */
char *
endpoint_to_string (endpoint *p)
{
    if ('\0' == p->desc[0])
    {
        char *temp = p->desc;
        temp += sprintf(temp, "Index: %u ", p->index);
        temp += sprintf(temp, "Hardware Address: ");
        int i;
        for (i = 0; i < HW_ADDRLEN; ++i)
        {
            temp += sprintf(temp, "%.2x:", p->hardware_address[i]);
        }
        --temp;
        *temp = '\0';
    }

    return (p->desc);
}

endpoint_list
create_listen_hub ()
{
    endpoint_list if_list = get_endpoint_list();
    endpoint *t = if_list;

    while (NULL != t)
    {
        SOCKET sock = bind_socket(t->index);
        if (SOCKET_ERROR == sock)
        {
            LOGE("Couldn't be bound to following endpoint. Skipping:%s",
                    endpoint_to_string(t));
            endpoint *temp = t->next;
            remove_endpoint(&if_list, t);
            t = temp;
            continue;
        }

        t->sock = sock;
        LOGS("Binding socket on %s", endpoint_to_string(t));
        t = t->next;
    }
    printf("\n");

    return if_list;
}
