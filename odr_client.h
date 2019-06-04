#pragma once

#define ODR_CLIENT_RAND_MAX_FILENAME 90

typedef struct odr_client_t
{
    char file_name[ODR_CLIENT_RAND_MAX_FILENAME];
    uint16_t port;
    long timestamp;

    struct odr_client_t *next;
} odr_client;

typedef odr_client *odr_client_list;

odr_client *
create_odr_client (char *file_name, uint16_t port);

odr_client *
find_odr_client (odr_client_list t, uint16_t port);

odr_client *
find_odr_client_name (odr_client_list t, char *file_name);
