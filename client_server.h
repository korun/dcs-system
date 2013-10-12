/* encoding: UTF-8 */

/** client_server.h, Ivan Korunkov, 2013 **/

/**
    Здесь находятся константы, используемые как клиентом так и сервером.
**/

#include <stdint.h>
#include <arpa/inet.h>
#include "dbdef.h"

#ifndef	_CLIENT_SERVER_H
#define _CLIENT_SERVER_H

#define DEFAULT_SERVER_ADDR                "tcp://*:8000"
#define DEFAULT_SERVER_ADDR_FOR_CLIENT     "tcp://localhost:8000"

#define MAX_SERVER_ADDR_SIZE    256

#define MSG_DIGEST_SIZE         16
#define MSG_CTRL_BYTE           MSG_DIGEST_SIZE /* т.к. нумерация начинается с 0 */
#define MSG_SEPARATOR           '|'
#define MSG_SALT_PATH           "/home/kia84/Documents/dcs/salt"
//~ #define MSG_SALT_PATH           "/home/kia84/Device_Control_System/salt"

/* Type - signed char (!) */
#define DCS_CLIENT_REQ_MD5      0x01
#define DCS_CLIENT_REQ_FULL     0x02

/* Соотв. индексам в массиве! */
#define DCS_SERVER_REPLY_OK     0x00
#define DCS_SERVER_REPLY_FULL   0x01
#define DCS_SERVER_REPLY_SIZE   1       /* do NOT change this */
#define DCS_SERVER_REPLY_COUNT  2

/* Макрос, повторяющий указанно действие (ACTion) указанное количество раз (N),
 * и пока выполняется следующее условие (CONDition) */
#define TRY_N_TIMES(N, ACT, COND, S, SYSLL)                      \
    for(int i = 0; i < (N) && (COND); i++) {                     \
            syslog((SYSLL), "trying "S": %s.", strerror(errno)); \
            sleep(i + 1);                                        \
            (ACT);                                               \
        }

#define get_uint32_from(POINTER) ntohl((uint32_t) *((uint32_t *) (POINTER)))
#define get_uint16_from(POINTER) ntohs((uint16_t) *((uint16_t *) (POINTER)))
#define get_uint8_from(POINTER)       ((uint8_t)  *((uint8_t  *) (POINTER)))

typedef struct {
    uint32_t id;
    uint16_t vendor_id;
    uint16_t device_id;
    uint32_t subsystem_id;
    uint32_t class_code;
    uint8_t  revision;
    char     bus_addr[6];
    size_t   serial_length;
    char     serial[DETAIL_SERIAL_SIZE + 1]; /* + '\0' */
    size_t   params_length;
    char    *params;
} CL_Detail;

#endif /* client_server.h */
