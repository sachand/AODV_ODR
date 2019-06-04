#include "app_settings.h"

#ifndef LOG_TAG
#define LOG_TAG "ODR_Client"
#endif

static uint16_t g_next_ephemeral_port = DEFAULT_PORT_NUMBER + 1;
static const uint16_t g_seq_last_port = 0xFFFF;

void
destroy_odr_client (odr_client *e)
{
    free(e);
}

void
destroy_odr_client_list (odr_client_list l)
{
    odr_client *e, *s;
    for (e = l; NULL != e; e = s)
    {
        s = e->next;
        destroy_odr_client(e);
    }
}

odr_client *
create_odr_client (char *file_name, uint16_t port)
{
    odr_client *t = (odr_client *) calloc(1, sizeof(odr_client));
    strncpy(t->file_name, file_name, ODR_CLIENT_RAND_MAX_FILENAME);
    t->port = port;
}

odr_client *
find_odr_client (odr_client_list t, uint16_t port)
{
    odr_client *e;
    for (e = t; NULL != e; e = e->next)
    {
        if (port == e->port) { break; }
    }

    return e;
}

odr_client *
find_odr_client_name (odr_client_list t, char *file_name)
{
    odr_client *e;
    for (e = t; NULL != e; e = e->next)
    {
        if (0 == strcmp(e->file_name, file_name)) { break; }
    }

    return e;
}

void
insert_in_odr_client_list (odr_client_list *t, odr_client *e)
{
    odr_client *p = find_odr_client(*t, e->port);
    if (NULL == p)
    {
        p = *t;
        *t = e;
        e->next = p;
    }
    else
    {
        strncpy(p->file_name, e->file_name, ODR_CLIENT_RAND_MAX_FILENAME);
    }
}

odr_client *
get_oldest_odr_client(odr_client_list list)
{
    if (NULL == list) { return NULL; }

    odr_client *min = NULL;
    odr_client *t;
    for (t = list->next; NULL != t; t = t->next)
    {
        if (NULL == min)
        {
            min = t;
            continue;
        }
        else
        {
            if (min->timestamp > t->timestamp) { min = t; }
        }
    }
    
    return min;
}

uint16_t
get_next_ephemeral_port (odr_client_list list)
{
    if (0 == memcmp(&g_next_ephemeral_port, &g_seq_last_port, sizeof(uint16_t)))
    {
        odr_client *oc = get_oldest_odr_client(list);
        return oc->port;
    }
    else
    {
        return ++g_next_ephemeral_port;
    }
}

uint16_t
register_odr_client (odr_client_list *list, char *file_name, uint16_t port)
{
    odr_client *oc = NULL;
    if (0 != port)
    {
        oc = find_odr_client(*list, port);
    }
    else
    {
        oc = find_odr_client_name(*list, file_name);
        port = (NULL == oc) ? get_next_ephemeral_port(*list) : oc->port;
    }

    if (NULL == oc)
    {
        oc = create_odr_client(file_name, port);
        insert_in_odr_client_list(list, oc);
    }
    else
    {
        strncpy(oc->file_name, file_name, ODR_CLIENT_RAND_MAX_FILENAME);
    }

    oc->timestamp = get_current_system_time_millis();
    return port;
}
