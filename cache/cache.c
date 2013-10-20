/* encoding: UTF-8 */

/** cache.c, Ivan Korunkov, 2013 **/

/**
    Привет
**/

#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>

#include "cache.h"

#ifndef _XOPEN_SOURCE
    #define _XOPEN_SOURCE   /* glibc2 needs this (strptime) */
#endif

#define MIN(A, B) ((A) > (B) ? (B) : (A))

#define SQLITE_NOW_DATETIME "datetime('now', 'localtime')"

/*  Вспомогательная функция, проверяющая наличие какой-либо таблицы в кэше.
    Возвращает 0 - если таблицы нет, 1 - если таблица есть.                 */
static int _Cache_have_table(
        LocalCache  *cache,
        const char  *table_name
    ){
    char  *query;
    char **result;
    int    rc, res_row, res_col;
    
    query = sqlite3_mprintf("SELECT id FROM %s LIMIT 1;", table_name);

    rc = sqlite3_get_table(cache->db, query, &result, &res_row, &res_col, NULL);

    sqlite3_free(query);
    sqlite3_free_table(result);

    /*  0    no such table
        1    table exists   */
    return (rc == SQLITE_OK);
}

static int _Cache_create_table(LocalCache *cache, const char *query, const char *table_name){
    int rc;
    rc = sqlite3_exec(cache->db, query, NULL, 0, NULL);
    if( rc != SQLITE_OK && !_Cache_have_table(cache, table_name)){
        /* In parent */
        /* Cache_destroy(cache); */
        /* Delete 'path' file from HD, and try init again. */
        return LC_CODE_TABLE_ERR;
    }
    return 0;
}

int Cache_init(const char *path, LocalCache *cache) {
    int rc;

    rc = sqlite3_open(path, &(cache->db));
    if(rc != SQLITE_OK){
        /* In parent: */
        /* SYSLOG! "Can't open database: %s\n", sqlite3_errmsg(cache->db) */
        /* Cache_destroy(cache->db);*/
        return LC_CODE_BADF_ERR;
    }

    rc = _Cache_create_table(cache,
        "CREATE TABLE "COMPUTERS_TABNAME"("
            "id          INTEGER PRIMARY KEY, "
            "db_id       INTEGER UNIQUE, "
            "domain      TEXT    UNIQUE, "
            "md5sum      TEXT, "
            "created_at  TEXT DEFAULT("SQLITE_NOW_DATETIME")"
        ");", COMPUTERS_TABNAME);
    if(rc < 0)    return rc;

    rc = _Cache_create_table(cache,
        "CREATE TABLE "DETAILS_TABNAME"("
            "id           INTEGER PRIMARY KEY, "
            "vendor_id    INTEGER, "
            "device_id    INTEGER, "
            "subsystem_id INTEGER, "
            "class_code   INTEGER, "
            "revision     INTEGER, "
            "bus_addr     TEXT, "
            "serial       TEXT, "
            "created_at   TEXT DEFAULT("SQLITE_NOW_DATETIME"), "
            "params       TEXT"
        ");", DETAILS_TABNAME);
    if(rc < 0)    return rc;

    rc = _Cache_create_table(cache,
        "CREATE TABLE "CONFIGS_TABNAME"("
            "id           INTEGER PRIMARY KEY, "
            "computer_id  INTEGER, "
            "detail_id    INTEGER, "
            "created_at   TEXT DEFAULT("SQLITE_NOW_DATETIME"), "
            "deleted_at   TEXT"
        ");", CONFIGS_TABNAME);
    if(rc < 0)    return rc;

    rc = _Cache_create_table(cache,
        "CREATE TABLE "LOGS_TABNAME"("
            "id           INTEGER PRIMARY KEY, "
            "domain       TEXT, "
            "md5sum       TEXT, "
            "created_at   TEXT DEFAULT("SQLITE_NOW_DATETIME")"
        ");", LOGS_TABNAME);
    if(rc < 0)    return rc;

    return 0;
}

void Cache_destroy(LocalCache *cache) {
    if(cache->db){
        sqlite3_close(cache->db);
        cache->db = NULL;
    }
}

static int    _Cache_time_to_str(time_t timestamp, char *str){
    struct tm *time_tm;

    if(!timestamp) return 0;

    time_tm = localtime(&timestamp);

    return strftime(str, LC_TIMESTAMP_STRLEN + 1, LC_TIME_FORMAT, time_tm);
}
static time_t _Cache_str_to_time(const char *str){
    struct tm time_tm;
    
    if(!str) return (time_t) -1;
    
    memset(&time_tm, 0, sizeof(struct tm));

    strptime(str, LC_TIME_FORMAT, &time_tm);

    return mktime(&time_tm);
}

int Cache_begin(LocalCache *cache){
    int rc;
    if(cache->begin != 0 && (rc = Cache_commit(cache)) != SQLITE_OK)
        return rc;
    rc = sqlite3_exec(cache->db, "BEGIN;", NULL, 0, NULL);
    if(rc == SQLITE_OK)
        cache->begin = 1;
    return rc;
}

int Cache_commit(LocalCache *cache){
    int rc;
    if(!cache->begin) return SQLITE_OK;
    rc = sqlite3_exec(cache->db, "COMMIT;", NULL, 0, NULL);
    if(rc == SQLITE_OK)
        cache->begin = 0;
    return rc;
}

int Cache_try_begin(LocalCache *cache){
    if(cache->begin) return SQLITE_OK;
    return Cache_begin(cache);
}

int Cache_put_log(LocalCache *cache, const char *domain, const char *md5sum){
    int   rc;
    char *query;

    query = sqlite3_mprintf("INSERT INTO "LOGS_TABNAME"(domain, md5sum) "
                                "VALUES(%Q, %Q);", domain, md5sum);
    
    rc    = sqlite3_exec(cache->db, query, NULL, 0, NULL);

    sqlite3_free(query);

    if(rc != SQLITE_OK) return LC_CODE_INSERT_ERR; /* См. sqlite3_errmsg */

    return 0;
}

int Cache_put_comp(
        LocalCache  *cache,
        unsigned int db_id,
        const char  *domain_name,
        const char  *md5sum,
        time_t       created_at
    ){
    int   rc;
    char *query;
    char *str_created_at;
    char  time_buffer[LC_TIMESTAMP_STRLEN + 3]; /* + "\'\0" */
    char  getdatetime[] = SQLITE_NOW_DATETIME;

    if(_Cache_time_to_str(created_at, time_buffer + 1)){
        time_buffer[0] = '\'';
        str_created_at = time_buffer;
        time_buffer[LC_TIMESTAMP_STRLEN + 1] = '\'';
        time_buffer[LC_TIMESTAMP_STRLEN + 2] = '\0';
    }
    else
        str_created_at = getdatetime;

    query = sqlite3_mprintf("INSERT INTO "COMPUTERS_TABNAME
                                "(db_id, domain, md5sum, created_at) "
                                "VALUES(%d, %Q, %Q, %s);",
                                db_id, domain_name, md5sum, str_created_at);

    rc    = sqlite3_exec(cache->db, query, NULL, 0, NULL);

    sqlite3_free(query);

    if(rc != SQLITE_OK) return LC_CODE_INSERT_ERR; /* См. sqlite3_errmsg */

    return 0;
}

int Cache_put_detail(
        LocalCache      *cache,
        const LC_Detail *detail
    ){
    int   rc;
    char *query;

    query = sqlite3_mprintf("INSERT INTO "DETAILS_TABNAME
                                "(vendor_id, device_id, subsystem_id, "
                                "class_code, revision, bus_addr, serial, params) "
                                "VALUES(%d, %d, %d, %d, %Q, %Q, %Q);",
                                detail->vendor_id,
                                detail->device_id,
                                detail->subsystem_id,
                                detail->class_code,
                                detail->revision,
                                detail->bus_addr,
                                detail->serial,
                                detail->params
                            );

    rc    = sqlite3_exec(cache->db, query, NULL, 0, NULL);

    sqlite3_free(query);

    if(rc != SQLITE_OK) return LC_CODE_INSERT_ERR; /* См. sqlite3_errmsg */

    return 0;
}

int Cache_put_config(
        LocalCache   *cache,
        unsigned int  computer_id,
        unsigned int  detail_id,
        time_t        deleted_at
    ){
    int   rc;
    char *query;
    char *str_deleted_at = NULL;
    char  time_buffer[LC_TIMESTAMP_STRLEN + 1]; /* + '\0' */

    if(_Cache_time_to_str(deleted_at, time_buffer))
        str_deleted_at = time_buffer;

    query = sqlite3_mprintf("INSERT INTO "CONFIGS_TABNAME
                                "(computer_id, detail_id, deleted_at) "
                                "VALUES(%d, %d, %Q);",
                                computer_id, detail_id, str_deleted_at);

    rc    = sqlite3_exec(cache->db, query, NULL, 0, NULL);

    sqlite3_free(query);

    if(rc != SQLITE_OK) return LC_CODE_INSERT_ERR; /* См. sqlite3_errmsg */

    return 0;
}

int Cache_del_comp(LocalCache *cache, unsigned int id){
    int   rc;
    char *query;
    query = sqlite3_mprintf("DELETE FROM "COMPUTERS_TABNAME" WHERE id = %d;", id);
    rc    = sqlite3_exec(cache->db, query, NULL, 0, NULL);
    sqlite3_free(query);
    return rc;
}

int Cache_del_logs(LocalCache *cache){
    return sqlite3_exec(cache->db, "DELETE FROM "LOGS_TABNAME";", NULL, 0, NULL);
}

int Cache_del_details(LocalCache *cache){
    return sqlite3_exec(cache->db, "DELETE FROM "DETAILS_TABNAME";", NULL, 0, NULL);
}

int Cache_del_configs(LocalCache *cache){
    return sqlite3_exec(cache->db, "DELETE FROM "CONFIGS_TABNAME";", NULL, 0, NULL);
}

int Cache_upd_comp_id(
        LocalCache  *cache,
        unsigned int id,
        unsigned int db_id,
        time_t       created_at
    ){
    int   rc;
    char *query;
    char *str_created_at;
    char  time_buffer[LC_TIMESTAMP_STRLEN + 1];

    if(db_id){
        if(created_at >= 0){
            _Cache_time_to_str(created_at, time_buffer);
            str_created_at = time_buffer;
            time_buffer[LC_TIMESTAMP_STRLEN] = '\0';
                query = sqlite3_mprintf("UPDATE "COMPUTERS_TABNAME" "
                                        "SET (db_id, created_at) = (%d, %q) "
                                        "WHERE id = %d;", db_id, str_created_at, id);
        }
        else{
            query = sqlite3_mprintf("UPDATE "COMPUTERS_TABNAME" "
                                        "SET db_id = %d "
                                        "WHERE id = %d;", db_id, id);
        }
    }
    else{
        return 0;
    }
    
    rc    = sqlite3_exec(cache->db, query, NULL, 0, NULL);

    sqlite3_free(query);
    
    if(rc != SQLITE_OK) return LC_CODE_UPDATE_ERR; /* См. sqlite3_errmsg */
    
    return 0;
}

int Cache_upd_comp(
        LocalCache  *cache,
        unsigned int id,
        unsigned int db_id,
        const char  *md5sum
    ){
    int   rc;
    char *query;

    if(id){
        query = sqlite3_mprintf("UPDATE "COMPUTERS_TABNAME" SET md5sum = %Q WHERE id = %d;", md5sum, id);
    }
    else if(db_id){
        query = sqlite3_mprintf("UPDATE "COMPUTERS_TABNAME" SET md5sum = %Q WHERE db_id = %d;", md5sum, db_id);
    }
    else return 0;
    
    rc    = sqlite3_exec(cache->db, query, NULL, 0, NULL);

    sqlite3_free(query);
    
    if(rc != SQLITE_OK) return LC_CODE_UPDATE_ERR; /* См. sqlite3_errmsg */
    
    return 0;
}

int Cache_set_config_del(LocalCache *cache, unsigned int id){
    int   rc;
    char *query;

    if(id){
        query = sqlite3_mprintf("UPDATE "CONFIGS_TABNAME" SET deleted_at = "SQLITE_NOW_DATETIME" WHERE id = %d;", id);
    }
    else return 0;
    
    rc    = sqlite3_exec(cache->db, query, NULL, 0, NULL);

    sqlite3_free(query);
    
    if(rc != SQLITE_OK) return LC_CODE_UPDATE_ERR; /* См. sqlite3_errmsg */
    
    return 0;
}

static void _Cache_set_log(sqlite3_stmt *res, void *element){
    LC_Log *log;
    int     char_count;
    
    log = (LC_Log *) element;
    
    /* id */
    log->id = (unsigned int) sqlite3_column_int(res, 0);

    /* domain name */
    char_count = snprintf(log->domain,
                            COMPUTER_DOMAIN_NAME_SIZE,
                            "%s", sqlite3_column_text(res, 1));
    log->domain[MIN(COMPUTER_DOMAIN_NAME_SIZE,
                            char_count)] = '\0';

    /* md5-key */ /* not check char_count because md5 MUST have length = 32 */
    snprintf(log->md5sum,
            COMPUTER_MD5_SUM_SIZE,
            "%s", sqlite3_column_text(res, 2));
    log->md5sum[COMPUTER_MD5_SUM_SIZE] = '\0';

    /* convert time from string into time_t */
    log->created_at =
        _Cache_str_to_time((const char *) sqlite3_column_text(res, 3));
}

static void _Cache_set_comp(sqlite3_stmt *res, void *element){
    LC_Computer *comp;
    int     char_count;
    
    comp = (LC_Computer *) element;
    
    /* id */
    comp->id    = (unsigned int) sqlite3_column_int(res, 0);

    /* data base id */
    comp->db_id = (unsigned int) sqlite3_column_int(res, 1);

    /* domain name */
    char_count = snprintf(comp->domain, COMPUTER_DOMAIN_NAME_SIZE,
                          "%s", sqlite3_column_text(res, 2));
    comp->domain[MIN(COMPUTER_DOMAIN_NAME_SIZE,
                            char_count)] = '\0';

    /* md5-key */
    snprintf(comp->md5sum, COMPUTER_MD5_SUM_SIZE,
             "%s", sqlite3_column_text(res, 3));
    comp->md5sum[COMPUTER_MD5_SUM_SIZE] = '\0';

    /* convert time from string into time_t */
    comp->created_at =
        _Cache_str_to_time((const char *) sqlite3_column_text(res, 4));
}

static void _Cache_set_detail(sqlite3_stmt *res, void *element){
    LC_Detail *detail;
    int     char_count;
    
    detail = (LC_Detail *) element;
    
    /* id */
    detail->id = (unsigned int) sqlite3_column_int(res, 0);
    /* vendor_id */
    detail->vendor_id =
        (unsigned int) sqlite3_column_int(res, 1);
    /* device_id */
    detail->device_id =
        (unsigned int) sqlite3_column_int(res, 2);
    /* subsystem_id */
    detail->subsystem_id =
        (unsigned int) sqlite3_column_int(res, 3);
    
    detail->class_code =
        (unsigned int) sqlite3_column_int(res, 4);
    detail->revision =
        (unsigned int) sqlite3_column_int(res, 5);
    
    /* serial */
    char_count = snprintf(detail->serial,
                          DETAIL_SERIAL_SIZE,
                          "%s", sqlite3_column_text(res, 6));
    detail->serial[MIN(DETAIL_SERIAL_SIZE,
                             char_count)] = '\0';

    /* convert time from string into time_t */
    detail->created_at =
        _Cache_str_to_time((const char *) sqlite3_column_text(res, 7));
}

static void _Cache_set_config(sqlite3_stmt *res, void *element){
    LC_Config *config;
    
    config = (LC_Config *) element;
    
    /* id */
    config->id = (unsigned int) sqlite3_column_int(res, 0);
    /* computer_id */
    config->computer_id =
        (unsigned int) sqlite3_column_int(res, 1);
    /* detail_id */
    config->detail_id =
        (unsigned int) sqlite3_column_int(res, 2);
    
    /* convert time from string into time_t */
    config->created_at =
        _Cache_str_to_time((const char *) sqlite3_column_text(res, 3));
    
    /* convert time from string into time_t */
    config->deleted_at =
        _Cache_str_to_time((const char *) sqlite3_column_text(res, 4));
}

static size_t _Cache_get_to_array(
        LocalCache  *cache,
        void       **array,
        size_t       arr_size,
        size_t       el_size,
        void (*callback)(sqlite3_stmt *, void *),
        const char  *query
    ){
    int           rc, i;
    sqlite3_stmt *res;
    void         *tmp_result = NULL;

    rc = sqlite3_prepare_v2(cache->db, query, strlen(query), &res, NULL);

    if (rc == SQLITE_OK) {

        *array = malloc(arr_size * el_size);
        if(!*array){
            /* Не хватает памяти. errno = ENOMEM */
            return 0;
        }

        i = 0;

        while(sqlite3_step(res) == SQLITE_ROW){

            if(i == arr_size){
                arr_size += LC_CHUNK_SIZE;
                tmp_result = realloc(*array, arr_size * el_size);
                if(!tmp_result){
                    /* Не хватает памяти. errno = ENOMEM */
                    free(*array);
                    sqlite3_finalize(res);
                    return 0;
                }
                *array = tmp_result;
            }

            (*callback)(res, (void *) ((int) *array + i * el_size));

            i++;
        }

        sqlite3_finalize(res);
        /*  Если количество записанных в массив элементов равно нулю,
            realloc самостоятельно почистит выделенную память.
            Иначе realloc безопастно уменьшит размер массива до нужного,
            и освободит неиспользуемую память.
            [XXXXXXXXXXXXXXXXXXX........]
             \     elements    /\ free /        */
        if(i < arr_size){
            *array = realloc(*array, i * el_size);
        }
        return i;
    }
    return 0;
}

LC_Log *Cache_get_logs(
        LocalCache *cache,
        size_t      *el_count,
        unsigned int id,
        const char  *domain
    ){
    size_t        arr_size;
    char         *query;
    LC_Log       *finded_logs = NULL;

    *el_count = 0;

    if(id){
        query = sqlite3_mprintf("SELECT id, domain, md5sum, created_at "
                                    "FROM "LOGS_TABNAME" WHERE id = %d LIMIT 1;", id);
        arr_size = 1;
    }
    else if(domain){
        query = sqlite3_mprintf("SELECT id, domain, md5sum, created_at "
                                    "FROM "LOGS_TABNAME" WHERE domain LIKE '%q' "
                                    "LIMIT 1;", domain);
        arr_size = 1;
    }
    else{
        query = sqlite3_mprintf("SELECT id, domain, md5sum, created_at "
                                    "FROM "LOGS_TABNAME" "
                                    "ORDER BY domain, created_at, id;");
        arr_size = LC_CHUNK_SIZE;
    }
    
    *el_count = _Cache_get_to_array(cache,
                                    (void **) &finded_logs,
                                    arr_size, sizeof(LC_Log),
                                    _Cache_set_log, query);
    sqlite3_free((void *) query);
    return finded_logs;
}

LC_Computer *Cache_get_comps(
        LocalCache *cache,
        size_t      *el_count,
        unsigned int db_id,
        const char  *domain
    ){
    size_t        arr_size;
    char         *query;
    LC_Computer  *finded_comp = NULL;

    if(db_id){
        query = sqlite3_mprintf("SELECT id, db_id, domain, md5sum, created_at "
                                    "FROM "COMPUTERS_TABNAME" "
                                    "WHERE db_id = %d LIMIT 1;", db_id);
        arr_size = 1;
    }
    else if(domain){
        query = sqlite3_mprintf("SELECT id, db_id, domain, md5sum, created_at "
                                    "FROM "COMPUTERS_TABNAME" "
                                    "WHERE domain LIKE '%q' LIMIT 1;", domain);
        arr_size = 1;
    }
    else{
        query = sqlite3_mprintf("SELECT id, db_id, domain, md5sum, created_at "
                                    "FROM "COMPUTERS_TABNAME" ORDER BY domain;");
        arr_size = LC_CHUNK_SIZE;
    }
    
    *el_count = _Cache_get_to_array(cache,
                                    (void **) &finded_comp,
                                    arr_size, sizeof(LC_Computer),
                                    _Cache_set_comp, query);
    sqlite3_free(query);
    return finded_comp;
}

LC_Detail *Cache_get_details(
        LocalCache *cache,
        size_t      *el_count,
        unsigned int id
    ){
    size_t        arr_size;
    char         *query;
    LC_Detail    *finded_details = NULL;

    *el_count = 0;

    if(id){
        query = sqlite3_mprintf("SELECT id, vendor_id, device_id, "
                                    "subsystem_id, class_code, revision, "
                                    "serial, created_at "
                                    "FROM "DETAILS_TABNAME" WHERE id = %d LIMIT 1;", id);
        arr_size = 1;
    }
    else{
        query = sqlite3_mprintf("SELECT id, vendor_id, device_id, "
                                "subsystem_id, class_code, revision, "
                                "serial, created_at "
                                "FROM "DETAILS_TABNAME" "
                                "ORDER BY vendor_id, device_id, subsystem_id;");
        arr_size = LC_CHUNK_SIZE;
    }

    *el_count = _Cache_get_to_array(cache,
                                    (void **) &finded_details,
                                    arr_size, sizeof(LC_Detail),
                                    _Cache_set_detail, query);
    sqlite3_free(query);
    return finded_details;
}

int Cache_get_detail_id(
        LocalCache *cache,
        LC_Detail  *detail
    ){
    sqlite3_stmt *res;
    char         *query;

    query = sqlite3_mprintf("SELECT id "
                                "FROM "DETAILS_TABNAME" "
                                "WHERE vendor_id = %d AND "
                                    "device_id = %d AND "
                                    "subsystem_id = %d AND "
                                    "class_code = %d AND "
                                    "revision = %d AND "
                                    "serial LIKE %Q "
                                "ORDER BY created_at DESC "
                                "LIMIT 1;",
                                detail->vendor_id,
                                detail->device_id,
                                detail->subsystem_id,
                                detail->class_code,
                                detail->revision,
                                detail->serial
                            );

    int rc = sqlite3_prepare_v2(cache->db, query, strlen(query), &res, NULL);

    if (rc == SQLITE_OK) {
        if(sqlite3_step(res) == SQLITE_ROW)
            detail->id = (uint32_t) sqlite3_column_int(res, 0);
        sqlite3_finalize(res);
    }
    
    sqlite3_free(query);
    return rc;
}

LC_Config *Cache_get_configs(
        LocalCache *cache,
        size_t      *el_count,
        unsigned int computer_id,
        unsigned int detail_id
    ){
    char         *query;
    LC_Config    *finded_configs = NULL;

    *el_count = 0;
    
    if(computer_id && detail_id){
        query = sqlite3_mprintf("SELECT id, computer_id, detail_id, "
                                    "created_at, deleted_at "
                                    "FROM "CONFIGS_TABNAME" "
                                    "WHERE computer_id = %d AND "
                                    "detail_id = %d ORDER BY created_at;",
                                    computer_id, detail_id);
    }
    else{
        query = sqlite3_mprintf("SELECT id, computer_id, detail_id, "
                                    "created_at, deleted_at "
                                    "FROM "CONFIGS_TABNAME" ORDER BY "
                                    "computer_id, detail_id, created_at;",
                                    computer_id, detail_id);
    }

    *el_count = _Cache_get_to_array(cache,
                                    (void **) &finded_configs,
                                    LC_CHUNK_SIZE, sizeof(LC_Config),
                                    _Cache_set_config, query);
    sqlite3_free(query);
    return finded_configs;
}
