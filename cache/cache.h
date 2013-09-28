/* encoding: UTF-8 */

/** cache.h, Ivan Korunkov, 2013 **/

/**
    Здесь объявлены функции, позволяющие серверу работать с локальным кэшем.
    Кэш представляет собой статическую базу данных sqlite3, для которой заданы
    таблицы, и функции для взаимодействия с данными из этих таблиц.
**/

/** Структура кэша (таблицы):
    Кэш содержит в себе таблицу машин - клиентов сервера, а также таблицу их
    деталей. Одна деталь за свою "жизнь" может побывать в разных машинах.
    Связь "многое-ко-многому" между машиной и её деталями организуется
    посредством таблицы CONFIGS_TABNAME.
    COMPUTERS_TABNAME 1..* <----CONFIGS_TABNAME----> 0..* DETAILS_TABNAME
    
    Кроме того, кэш содержит логи на каждую машину.
    COMPUTERS_TABNAME 1 <---- 0..* LOGS_TABNAME
    
    Постоянно заполненной остаётся только таблица COMPUTERS_TABNAME, остальные
    таблицы очищаются после первой возможной синхронизации с базой данных.
    
    COMPUTERS_TABNAME(
        id           INTEGER PRIMARY KEY,
        db_id        INTEGER UNIQUE, -- id в основной базе данных
        domain       TEXT    UNIQUE, -- не больше COMPUTER_DOMAIN_NAME_SIZE
        md5sum       TEXT,           -- не больше COMPUTER_MD5_SUM_SIZE
        created_at   TEXT DEFAULT(datetime('now', 'localtime'))
    );
    DETAILS_TABNAME(
        id           INTEGER PRIMARY KEY,
        vendor_id    INTEGER,
        device_id    INTEGER,
        subsystem_id INTEGER,
        serial       TEXT,           -- не больше DETAIL_SERIAL_SIZE
        created_at   TEXT DEFAULT(datetime('now', 'localtime'))
    );
    CONFIGS_TABNAME(
        id           INTEGER PRIMARY KEY,
        computer_id  INTEGER,
        detail_id    INTEGER,
        created_at   TEXT DEFAULT(datetime('now', 'localtime')),
        deleted_at   TEXT
    );
    LOGS_TABNAME(
        id           INTEGER PRIMARY KEY,
        domain       TEXT,           -- не больше COMPUTER_DOMAIN_NAME_SIZE
        md5sum       TEXT,           -- не больше COMPUTER_MD5_SUM_SIZE
        created_at   TEXT DEFAULT(datetime('now', 'localtime'))
    );
    
    Sqlite3 использует динамическую типизацию, поэтому типы полей в таблицах,
    по сути, ограничиваются следующими: NULL, INTEGER, REAL, TEXT, BLOB.
    
    В sqlite3 нельзя задать конкретную длину строки, поэтому вся проверка
    ложится на функции, использующие доступ к кэшу.
**/

#ifndef	_CACHE_H
#define _CACHE_H

#define _XOPEN_SOURCE
#include <time.h>       /* Содержит time_t и функции преобразования. */

#include "../dbdef.h"   /* Содержит информацию о таблицах в БД.  */
#include "sqlite3.h"    /* sqlite3 API */

/*  Функции, получающие данные из таблиц, выделяют память для массива
    возвращаемых данных. Так как количество строк, получемых запросом, заранее
    не известно - нам необходимо выделять память последовательно, некоторыми
    блоками, по мере получения данных.
    LC_CHUNK_SIZE - это количество элементов, содержащихся в одном блоке
    выделяемой памяти.  */
#define LC_CHUNK_SIZE       64

/*  Строковое представление даты и времени. Именно этот формат используется
    в sqlite, и именно его мы будем использовать при преобразовании даты
    и времени (strftime, strptime).  */
#define LC_TIME_FORMAT      "%Y-%m-%d %H:%M:%S"

/*  Длина LC_TIME_FORMAT.  */
#define LC_TIMESTAMP_STRLEN 19  /* "YYYY-mm-dd HH:MM:SS" */

/*~~~~~~~~~~~~~~~~~~~~~~~~ Коды возвращаемых ошибок ~~~~~~~~~~~~~~~~~~~~~~~~*/
/*  Ниже представлены коды возвращаемых ошибок. Если нужно узнать код ошибки,
    который вернули sqlite-функции, используйте sqlite3_errcode             */
#define LC_CODE_BADF_ERR    -1  /* Невозможно открыть sqlite3-файл кэша.    */
#define LC_CODE_TABLE_ERR   -2  /* Невозможно создать таблицу.              */
#define LC_CODE_INSERT_ERR  -3  /* Невозможно добавить запись в таблицу.    */
#define LC_CODE_UPDATE_ERR  -4  /* Невозможно очистить таблицу.             */
/*~~~~~~~~~~~~~~~~~~~~~~~ /Коды возвращаемых ошибок ~~~~~~~~~~~~~~~~~~~~~~~~*/

/*  Данная структура описывает кэш как таковой. Она необходима для работы
    всех функций доступа к кэшу. */
typedef struct {
    /*int sync;*/   /* флаг (mutex) синхронизации. */
    sqlite3 *db;
    int  begin;     /* есть незакрытый BEGIN? */
} LocalCache;

/*  Нижеперечисленные структуры описывают элементы таблиц кэша.
    Каждая из них представляет собой одну запись, полученную из таблицы.
    Все поля соответственно преобразуются (т.е. числа в int, строки в (char *)
    timestamp в time_t).  */
typedef struct {
    unsigned int id;
    unsigned int db_id;
    char         domain[COMPUTER_DOMAIN_NAME_SIZE + 1]; /* + '\0' */
    char         md5sum[COMPUTER_MD5_SUM_SIZE     + 1]; /* + '\0' */
    time_t       created_at;
} LC_Computer;

typedef struct {
    unsigned int id;
    unsigned int vendor_id;
    unsigned int device_id;
    unsigned int subsystem_id;
    unsigned int class_code;
    unsigned int revision;
    char         serial[DETAIL_SERIAL_SIZE + 1]; /* + '\0' */
    time_t       created_at;
} LC_Detail;

typedef struct {
    unsigned int id;
    unsigned int computer_id;
    unsigned int detail_id;
    time_t       created_at;
    time_t       deleted_at;
} LC_Config;

typedef struct {
    unsigned int id;
    char         domain[COMPUTER_DOMAIN_NAME_SIZE + 1]; /* + '\0' */
    char         md5sum[COMPUTER_MD5_SUM_SIZE     + 1]; /* + '\0' */
    time_t       created_at;
} LC_Log;

/*  Cache_init
    Функция, производящая первичную инициализацию кэша.
    Выделяет память, создаёт файл кеша (если его нет), создаёт необходимые
    таблицы для дальнейшей работы.
    
    const char  *path  - путь до файла кэша.
    LocalCache *cache - указатель на структуру типа LocalCache, которую
    необходимо инициализировать.
    
    ВНИМЕНИЕ: функция Cache_init выделяет память для обеспечения возможности
    работы с локальной базой данных.
    Для избежания утечек памяти, после работы с хэшем обязательно
    используйте Cache_destroy.
    
    Не используйте Cache_init более одного раза подряд для одного и того же
    хэша, без вызова Cache_destroy.
    
    Возвращает:
        0                 - если всё прошло успешно;
        LC_CODE_BADF_ERR  - если произошла ошибка создания / открытия файла;
        LC_CODE_TABLE_ERR - если невозможно создать стандартную таблицу.
*/
int Cache_init(const char *path, LocalCache *cache);

/*  Cache_destroy
    Функция, производящая освобождение занятой кэшем памяти.
    Сбрасывает все установленные в структуре LocalCache поля.
    
    Единственный аргумент - указатель на существующий кэш.
*/
void Cache_destroy(LocalCache *cache);

/*  Функции для начала и завершения транзакции.
    Единственный аргумент - указатель на существующий кэш.
*/
int Cache_begin(LocalCache  *cache);
int Cache_commit(LocalCache *cache);
int Cache_try_begin(LocalCache *cache);

/*  Серия функций Cache_put_*
    Каждая из этих функций добавляет в соответствующую таблицу новую запись.
    
    Общий аргумент: LocalCache *cache - указатель на существующий кэш.
    
    Строки, передаваемые этим функциям должны гарантированно заканчиваться
    символом '\0'. Кроме того, эти функции не проверяют длину строковых
    аргументов (domain_name, md5sum, serial), так что это нужно делать
    самостоятельно, ДО их вызова.
    
    Некоторые аргументы не являются обязательными, вместо них можно
    передавать 0. Если передать 0 в качестве аргумента created_at, будет
    использовано значение по умолчанию (текущие дата и время).
    
    Возвращают они 0 - в случае успеха, либо LC_CODE_INSERT_ERR - если
    добавление такого элемента невозожно.
*/
int Cache_put_log(
        LocalCache *cache,
        const char *domain,
        const char *md5sum
    );
int Cache_put_comp(
        LocalCache   *cache,
        unsigned int  db_id,
        const char   *domain_name,
        const char   *md5sum,
        time_t        created_at    /* необязательный аргумент, 0 - DEFAULT */
    );
int Cache_put_detail(
        LocalCache   *cache,
        unsigned int  vendor_id,
        unsigned int  device_id,
        unsigned int  subsystem_id,
        const char   *serial        /* необязательный аргумент, NULL */
    );
int Cache_put_config(
        LocalCache   *cache,
        unsigned int  computer_id,
        unsigned int  detail_id,
        time_t        deleted_at    /* необязательный аргумент, 0 */
    );

/*  Серия функций Cache_del_*
    Каждая из этих функций очищает соответствующую таблицу в кэше
    (т.е. удаляет из неё все записи).
    
    Общий аргумент: LocalCache *cache - указатель на существующий хэш.
    
    Функции вернут SQLITE_OK (0) - в случае успеха, либо код 
    sqlite-ошибки.
*/
int Cache_del_logs   (LocalCache *cache);
int Cache_del_details(LocalCache *cache);
int Cache_del_configs(LocalCache *cache);

/******************************************************************************/
int Cache_upd_comp(
        LocalCache  *cache,
        unsigned int id,
        unsigned int db_id,
        const char  *md5sum
    );
int Cache_set_config_del(
        LocalCache  *cache,
        unsigned int id
    );
LC_Log *Cache_get_logs(
        LocalCache  *cache,
        size_t      *el_count,
        unsigned int id,
        const char  *domain
    );
LC_Computer *Cache_get_comps(
        LocalCache  *cache,
        size_t      *el_count,
        unsigned int db_id,
        const char  *domain
    );
LC_Detail *Cache_get_details(
        LocalCache  *cache,
        size_t      *el_count,
        unsigned int id
    );
LC_Config *Cache_get_configs(
        LocalCache  *cache,
        size_t      *el_count,
        unsigned int computer_id,
        unsigned int detail_id
    );

int Cache_upd_comp_id(
        LocalCache  *cache,
        unsigned int id,
        unsigned int db_id,
        time_t       created_at
    );
int Cache_del_comp(LocalCache *cache, unsigned int id);
#endif /* cache.h */
