#pragma once

#include <pthread.h>
#include <semaphore.h>
#include <limits.h>
#include <netpacket/packet.h>
#include <sys/un.h>

#include "socket_common.h"
#include "endpoint.h"
#include "logger.h"
#include "extras.h"
#include "np_time.h"
#include "msg_header.h"
#include "ethernet_frame.h"
#include "arp_cache.h"
#include "arp_header.h"
#include "iarp.h"
/**
 * ODR's ethertype
 * */
#define ETH_P_ARP (0x4969)

/**
 * Timeout for read at client. After this timeout, client will send the
 * "forced" message
 * */
#define CLIENT_READ_TIMEOUT_MS 300000

/**
 * Sets whether remote error is to be propagated back to sender ODR.
 * For e.g., if sender-ODR sends message to receiver-ODR
 * for a port that receiver-ODR does not have an entry for.
 * */
#define PROPAGATE_ERROR 1

/**
 * Template for temporary IPC files.
 * */
#define MKSTEMP_TEMPLATE "/tmp/schandXXXXXX\0"

/**
 * ODR's and server's IPC details.
 * */
#define ODR_IPC_ABSFILENAME "/tmp/ODRschand\0"
#define ARP_IPC_ABSFILENAME "/tmp/ARPschand\0"
#define SERVER_IPC_ABSFILENAME "/tmp/SVRschand\0"
#define ODR_IPC_READ_TIMEOUT_MS 5000
#define ODR_CLIENT_RAND_MAX_FILENAME 90

/**
 * Server default port number.
 * */
#define DEFAULT_PORT_NUMBER 1

/**
 * Protocol flags
 * */
#define MSG_HEADER_FLAG_REQ (1 << 0)
#define MSG_HEADER_FLAG_REP (1 << 1)
#define MSG_HEADER_FLAG_DAT (1 << 2)
#define MSG_HEADER_FLAG_FRC (1 << 3)
#define MSG_HEADER_FLAG_ERR (1 << 4)

#define MAX_MESSAGE_SIZE 512
#define MAX_PAYLOAD_SIZE (MAX_MESSAGE_SIZE - HEADER_SIZE)

/**
 * We don't have ARP. Static pool of VM IPs
 * */
#define VM_PREFIX "130.245.156.2\0"
#define VM1     "130.245.156.21\0"
#define VM2	    "130.245.156.22\0"
#define VM3	    "130.245.156.23\0"
#define VM4	    "130.245.156.24\0"
#define VM5	    "130.245.156.25\0"
#define VM6	    "130.245.156.26\0"
#define VM7	    "130.245.156.27\0"
#define VM8	    "130.245.156.28\0"
#define VM9	    "130.245.156.29\0"
#define VM10	"130.245.156.20\0"
