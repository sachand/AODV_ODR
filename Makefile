CC = gcc

CFLAGS = -w -g -I/home/saksham/Downloads/unpv13e/lib

RM = rm -f

SYSTEM_LIBS = \
	-lpthread \
	-lm \
	-lrt \
	/home/saksham/Downloads/unpv13e/libunp.a\

TARGET_LIBS = \
	arp_core \

#The original utils
SOURCES_UTILS = \
	logger.c \
	np_time.c \
	extras.c \

#Shared files with headers
SOURCES_COMMON = \
	${SOURCES_UTILS} \
	endpoint.c \
	msg_header.c \
	socket_common.c \

HEADERS_COMMON = $(SOURCES_COMMON:.c=.h)

#Shared file but no headers
SOURCES_BASE = \
	${SOURCES_COMMON} \
	get_hw_addrs.c \
	socket_binder.c \

#ARP's files
SOURCES_ARP = \
	${SOURCES_BASE} \
	arp_core.c \
	arp_cache.c \
	arp_header.c \
	arp_r_processor.c \
	iarp.c \

#ODR's files
SOURCES_ODR_ROUTER = \
	${SOURCES_BASE} \
	odr_router.c \
	ethernet_frame_processor.c \
	odr_client.c \
	odr_parking_lot.c \
	router_top_backend.c \
	router_top_frontend.c \
	routing_table.c \
	routing_table_provider.c \

#Client's files
SOURCES_CLIENT = \
	${SOURCES_BASE} \
	client.c \

#Server's files
SOURCES_SERVER = \
	${SOURCES_BASE} \
	server.c \

.PHONY: all
all: ${TARGET_LIBS} clean rename

server : server.o
	${CC} ${CFLAGS} -DLOG_TAG="\"SERVER\"" -o server ${SOURCES_SERVER:.c=.o} ${SYSTEM_LIBS}
server.o : ${SOURCES_SERVER}
	${CC} ${CFLAGS} -DLOG_TAG="\"SERVER\"" -c ${SOURCES_SERVER}

client : client.o
	${CC} ${CFLAGS} -DLOG_TAG="\"CLIENT\"" -o client ${SOURCES_CLIENT:.c=.o} ${SYSTEM_LIBS}
client.o : ${SOURCES_CLIENT}
	${CC} ${CFLAGS} -DLOG_TAG="\"CLIENT\"" -c ${SOURCES_CLIENT}

odr_router : odr_router.o
	${CC} ${CFLAGS} -DLOG_TAG="\"ROUTER\"" -o odr_router ${SOURCES_ODR_ROUTER:.c=.o} ${SYSTEM_LIBS}
odr_router.o : odr_router.c
	${CC} ${CFLAGS} -DLOG_TAG="\"ROUTER\"" -c ${SOURCES_ODR_ROUTER}

arp_core : arp_core.o
	${CC} ${CFLAGS} -o arp_core ${SOURCES_ARP:.c=.o} ${SYSTEM_LIBS}
arp_core.o : arp_core.c
	${CC} ${CFLAGS} -c ${SOURCES_ARP}

.PHONY: clean
clean:
	${RM} *.o

.PHONY: rename
rename:
	mv arp_core arp_${USER}

