#pragma once

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>

typedef enum boolean_t
{
	FALSE,
	TRUE,
} boolean;

char *
errno_to_string(int err);

char *
errno_string();

inline struct timeval
ms_to_timeval(uint32_t ms);

char *
signum_to_string(int sig);
