#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

/* request packet */
#define RMSGSIZE 43 /* 42 + 1 */

#define MSGTYPE_LEN 1
#define FILENAME_LEN 16
#define CHUNK_LEN 5
#define CADDRESS_LEN 15
#define CPORT_LEN 5

/* CTL packet */
#define CMSGSIZE 31 /* 30 + 1 */
/* MSGTYPE_LEN 1 */
#define CTLTYPE_LEN 5
#define CTLMSG_LEN 24

/* DATA packet */
#define DMSGSIZE 1001 /* 1000 + 1 */
/* #define MSGTYPE_LEN 1 */
/* #define CHUNK_LEN   5 */
#define DATA_LEN 994


/* ports */
#define SERVER_RECV_PORT 7311
#define CLIENT_RECV_PORT 7314


/* number of tries before giving up */
#define MAX_TRIES 10

/* number of seconds for socket timeout */
#define SOCK_TIMEOUT 1




