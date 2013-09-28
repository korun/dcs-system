/* encoding: UTF-8 */

/** dbdef.h, Ivan Korunkov, 2013 **/

/**
    Здесь находятся константы, определяющие размер полей в некоторых таблицах,
    встречающихся и в базе данных и в локальном кэше.
**/

#ifndef	_DBDEF_H
#define _DBDEF_H

/* Параметры для подключения к базе данных. */
#define PQ_CONNECT_DBNAME   "dbname=device_control_database"
#define PQ_CONNECT_INFO     PQ_CONNECT_DBNAME

/* Названия таблиц в базе данных */
#define COMPUTERS_TABNAME   "computers"
#define DETAILS_TABNAME     "details"
#define CONFIGS_TABNAME     "computers_details"
#define LOGS_TABNAME        "logs"

/*  Максимальная длина доменного имени машины.   */
#define COMPUTER_DOMAIN_NAME_SIZE    256

/*  Длина поля с md5-суммой (всегда должно быть равно 32, иначе это не md5).  */
#define COMPUTER_MD5_SUM_SIZE        32

/*  Максимальная длина серийного номера детали.  */
#define DETAIL_SERIAL_SIZE           128

/*
#define STATE_NEW               0b00
#define STATE_NEED_DELETE       0b01
#define STATE_CURRENT           0b10
#define STATE_DELETED           0b11
#define GET_STATE(rec)          ((rec).accepted_at ? 0b10 : 0b00) | ((rec).deleted_at ? 0b01 : 0b00)
*/

#endif /* dbdef.h */
