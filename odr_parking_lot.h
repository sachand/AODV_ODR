#pragma once

#include "msg_header.h"

typedef struct odr_car_t
{
    msg_iovec msg;

    struct odr_car_t *next;
} odr_car;

typedef odr_car* odr_parking_lot;

odr_car *
create_odr_car (msg_iovec msg);
