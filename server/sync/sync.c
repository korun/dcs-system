/* encoding: UTF-8 */

/** db_sync.c, Ivan Korunkov, 2013 **/

/**
    Compile with keys: -I<INCLUDEDIR> -L<LIBDIR> -lpq
    Where:
    <INCLUDEDIR> - /usr/include/postgresql (try: $ pg_config --includedir)
    <LIBDIR>     - /usr/lib/               (try: $ pg_config --libdir)
**/
#define _XOPEN_SOURCE
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>      /* htonl() */
#include <time.h>
#include <libpq-fe.h>

#include "../../cache/cache.h"
#include "../../hash/hash.h"
#include "../../dbdef.h"
#include "../../strxtoi.h"
#include "sync.h"

#define _GNU_SOURCE

#define TEXT_FRMT           0
#define BINARY_FRMT         1
#define TO_TIMESTAMP(A)     "to_timestamp("A"::int)::timestamp"

/* we need to convert number into network byte order */
static char *order_int(uint32_t num, uint32_t *tmp){
    *tmp = htonl(num);
    return (char *) tmp;
}

static char *chk_time_to_null(time_t num, uint32_t *tmp){
    if(num <= 0) return NULL;
    return order_int((uint32_t) num, tmp);
}

static void store_err_msg(Database *db, PGconn *conn){
    char   *msg;
    //~ size_t  length;
    
    if(!db->catch_err) return;
    
    msg = PQerrorMessage(conn);
    //~ length = strlen(msg);
    db->err_msg = (char *) realloc(db->err_msg, (sizeof(char)) * strlen(msg));
    strcpy(db->err_msg, msg);
}

static time_t str_to_time(const char *str){
    struct tm time_tm;
    
    if(!str) return (time_t) -1;
    
    memset(&time_tm, 0, sizeof(struct tm));

    strptime(str, DB_TIME_FORMAT, &time_tm);

    return mktime(&time_tm);
}

static void _DB_close_res(Database *db, PGconn *conn, PGresult *res){
    store_err_msg(db, conn);
    PQclear(res);
    PQfinish(conn);
}

static int _DB_connect(Database *db, PGconn **conn){
    *conn = PQconnectdb(db->conninfo);
    if(PQstatus(*conn) != CONNECTION_OK){
        _DB_close_res(db, *conn, NULL);
        return 1;
    }
    return 0;
}

static int _DB_commit(Database *db, PGconn *conn, PGresult **res){
    if(!db->begin) return 0;
    *res = PQexec(conn, "COMMIT;");
    if(PQresultStatus(*res) != PGRES_COMMAND_OK) return 1;
    PQclear(*res);
    db->begin = 0;
    return 0;
}

static int _DB_begin(Database *db, PGconn *conn, PGresult **res){
    if(db->begin != 0 && _DB_commit(db, conn, res) != 0)
        return 1;
    *res = PQexec(conn, "BEGIN;");
    if(PQresultStatus(*res) != PGRES_COMMAND_OK) return 1;
    PQclear(*res);
    db->begin = 1;
    return 0;
}

static int _DB_try_begin(Database *db, PGconn *conn, PGresult **res){
    if(db->begin) return 0;
    return _DB_begin(db, conn, res);
}

void DB_init(Database *db, char *conninfo, int catch_err){
    db->err_msg   = NULL;
    db->conninfo  = conninfo;
    db->catch_err = catch_err;
    db->begin     = 0;
}

void DB_destroy(Database *db){
    if(db->err_msg) free(db->err_msg);
}
#if 0
static int _DB_mark_config_del(Database *db, unsigned int id, time_t del_at){
    PGconn   *conn;
    PGresult *res;
    const char *paramValues[2];
    const int   paramLengths[2] = {sizeof(int), sizeof(int)};
    const int   paramFormats[2] = {BINARY_FRMT, BINARY_FRMT};
    uint32_t config_id, time_del_at;
    
    if(_DB_connect(db, &conn)    != 0) return 1;
    if(_DB_begin(db, conn, &res) != 0){ _DB_close_res(db, conn, res); return 1;}
    
    paramValues[0] = chk_time_to_null(del_at, &time_del_at);
    paramValues[1] = order_int(id, &config_id);
            
    res = PQexecParams(conn,
        "UPDATE "CONFIGS_TABNAME" SET deleted_at = "TO_TIMESTAMP("$3")
            " WHERE id = $2::int;",
        2,              /* count of params */
        NULL,           /* let the backend deduce param type */
        paramValues,
        paramLengths,
        paramFormats,
        TEXT_FRMT);             /* text results */
    if(PQresultStatus(res) != PGRES_COMMAND_OK){
        _DB_close_res(db, conn, res);
        return 1;
    }
    PQclear(res);
    
    if(_DB_commit(db, conn, &res) != 0){ _DB_close_res(db, conn, res); return 1;}
    
    PQfinish(conn);
    return 0;
}
#endif
DB_Computer *DB_get_comps(Database *db){
    PGconn      *conn;
    PGresult    *res;
    DB_Computer *comps;
    int nTuples, i;
    
    if(_DB_connect(db, &conn) != 0) return NULL;
    
    res = PQexec(conn, "SELECT id, domain_name, created_at "
                        "FROM "COMPUTERS_TABNAME" "
                        "WHERE deleted_at IS NULL AND "
                             "accepted_at IS NOT NULL;");
    if (PQresultStatus(res) != PGRES_TUPLES_OK){
        _DB_close_res(db, conn, res);
        return NULL;
    }
    
    nTuples = PQntuples(res);
    comps = (DB_Computer *) malloc(sizeof(DB_Computer) * nTuples);
    for(i = 0; i < nTuples; i++){
        //~ printf("%3d ", atoi(PQgetvalue(res, i, 0)));
        comps[i].id         = (unsigned int) atoi(PQgetvalue(res, i, 0));
        //~ printf("%s ", PQgetvalue(res, i, 1));
        strncpy(comps[i].domain,
                PQgetvalue(res, i, 1),
                COMPUTER_DOMAIN_NAME_SIZE);
        //~ printf("'%s':%d\n", PQgetvalue(res, i, 2), str_to_time(PQgetvalue(res, i, 2)));
        comps[i].created_at = str_to_time(PQgetvalue(res, i, 2));
    }
    PQclear(res);
    PQfinish(conn);
    return comps;
}

DB_Detail *DB_get_details(Database *db, unsigned int computer_id){
    PGconn      *conn;
    PGresult    *res;
    DB_Detail   *details;
    int nTuples, i;
    const char *paramValues[1];
    const int   paramFormats[1] = {BINARY_FRMT};
    const int   paramLengths[1] = {sizeof(uint32_t)};
    uint32_t tmp_id;
    
    if(_DB_connect(db, &conn) != 0) return NULL;
    
    paramValues[0] = order_int(computer_id, &tmp_id);
    
    res = PQexecParams(conn,
            "SELECT d.id, d.vendor_id, d.device_id, d.subsystem_id, "
                "d.serial, d.created_at "
                "FROM "CONFIGS_TABNAME" cd "
                "JOIN "DETAILS_TABNAME" d ON d.id = cd.detail_id "
                "WHERE "
                    "cd.computer_id = $1::int AND "
                    "cd.allow_del_at IS NULL;",
            1,              /* count of params */
            NULL,           /* let the backend deduce param type */
            paramValues,
            paramLengths,
            paramFormats,
            TEXT_FRMT);             /* text results */
    if (PQresultStatus(res) != PGRES_TUPLES_OK){
        _DB_close_res(db, conn, res);
        return NULL;
    }
    
    nTuples = PQntuples(res);
    details = (DB_Detail *) malloc(sizeof(DB_Detail) * nTuples);
    for(i = 0; i < nTuples; i++){
        details[i].id           = (unsigned int) atoi(PQgetvalue(res, i, 0));
        details[i].vendor_id    = (unsigned int) atoi(PQgetvalue(res, i, 1));
        details[i].device_id    = (unsigned int) atoi(PQgetvalue(res, i, 2));
        details[i].subsystem_id = (unsigned int) atoi(PQgetvalue(res, i, 3));
        strncpy(details[i].serial,
                PQgetvalue(res, i, 4),
                DETAIL_SERIAL_SIZE);
        details[i].created_at = str_to_time(PQgetvalue(res, i, 5));
        printf("Detail: %4d, %.4x, %.4x, %.8x, '%s', %d\n",
            details[i].id,
            details[i].vendor_id,
            details[i].device_id,
            details[i].subsystem_id,
            details[i].serial,
            (int) details[i].created_at
        );
    }
    PQclear(res);
    PQfinish(conn);
    return details;
}

int DB_put_logs(Database *db, const LC_Log *logs, size_t logs_count){
    PGconn   *conn;
    PGresult *res;
    uint32_t time_cr_at;
    const char *paramValues[3];
    const int   paramFormats[3] = {TEXT_FRMT, TEXT_FRMT, BINARY_FRMT};
          int   paramLengths[3];
    int i;
    printf("put logs...\n");
    if(_DB_connect(db, &conn)    != 0) return 1;
    if(_DB_begin(db, conn, &res) != 0){ _DB_close_res(db, conn, res); return 1;}
    
    paramLengths[0] = COMPUTER_DOMAIN_NAME_SIZE;
    paramLengths[1] = COMPUTER_MD5_SUM_SIZE;
    paramLengths[2] = sizeof(time_cr_at);
    
    for(i = 0; i < logs_count; i++){
        paramValues[0] = logs[i].domain;
        paramValues[1] = logs[i].md5sum;
        paramValues[2] = chk_time_to_null(logs[i].created_at, &time_cr_at);
        
        res = PQexecParams(conn,
            "INSERT INTO "LOGS_TABNAME"(domain_name, md5sum, created_at) "
                "VALUES($1::varchar, "
                    "$2::varchar, "
                    TO_TIMESTAMP("$3")");",
            3,              /* count of params */
            NULL,           /* let the backend deduce param type */
            paramValues,
            paramLengths,
            paramFormats,
            TEXT_FRMT);             /* text results */
        if(PQresultStatus(res) != PGRES_COMMAND_OK){
            _DB_close_res(db, conn, res);
            return 1;
        }
        PQclear(res);
    }
    if(_DB_commit(db, conn, &res) != 0){ _DB_close_res(db, conn, res); return 1;}
    
    PQfinish(conn);
    return 0;
}

int DB_put_configs(Database *db, const LC_Config *confs, size_t configs_count){
    PGconn   *conn;
    PGresult *res;
    const char *paramValues[4];
    const int   paramLengths[4] = {[0 ... 3] = sizeof(int)};
    const int   paramFormats[4] = {[0 ... 3] = BINARY_FRMT};
    uint32_t comp_id, det_id, time_cr_at, time_del_at;
    int i;
    
    if(_DB_connect(db, &conn)    != 0) return 1;
    if(_DB_begin(db, conn, &res) != 0){ _DB_close_res(db, conn, res); return 1;}
    
    for(i = 0; i < configs_count; i++){
        paramValues[0] = order_int(confs[i].computer_id, &comp_id);
        paramValues[1] = order_int(confs[i].detail_id,   &det_id);
        paramValues[2] = chk_time_to_null(confs[i].created_at, &time_cr_at);
        paramValues[3] = chk_time_to_null(confs[i].deleted_at, &time_del_at);
                
        res = PQexecParams(conn,
            "INSERT INTO "CONFIGS_TABNAME
                "(computer_id, detail_id, created_at, deleted_at) "
                "VALUES($1::int, "
                    "$2::int, "
                    TO_TIMESTAMP("$3")", "
                    TO_TIMESTAMP("$4")");",
            4,              /* count of params */
            NULL,           /* let the backend deduce param type */
            paramValues,
            paramLengths,
            paramFormats,
            TEXT_FRMT);             /* text results */
        if(PQresultStatus(res) != PGRES_COMMAND_OK){
            _DB_close_res(db, conn, res);
            return 1;
        }
        PQclear(res);
    }
    if(_DB_commit(db, conn, &res) != 0){ _DB_close_res(db, conn, res); return 1;}
    
    PQfinish(conn);
    return 0;
}

static int _DB_put_comps(Database *db, PGconn *conn, const LC_Computer *comps, size_t comps_count){
    PGresult   *res;
    const char *paramValues[2];
    const int   paramFormats[2] = {TEXT_FRMT, BINARY_FRMT};
    const int   paramLengths[2] = {COMPUTER_DOMAIN_NAME_SIZE, sizeof(time_t)};
    uint32_t    time_cr_at;
    int i;
    printf("put comps to DB...\n");
    for(i = 0; i < comps_count; i++){
        printf("id = %d i = %d\n", comps[i].id, i);
        if(comps[i].id == 0) continue;
        printf("1...\n");
        paramValues[0] = comps[i].domain;
        printf("2...\n");
        paramValues[1] = chk_time_to_null(comps[i].created_at, &time_cr_at);
        printf("Here!\n");
        res = PQexecParams(conn,
            "INSERT INTO "COMPUTERS_TABNAME"(domain_name, created_at) "
                "VALUES($1::varchar, "
                    TO_TIMESTAMP("$2")");",
            2,              /* count of params */
            NULL,           /* let the backend deduce param type */
            paramValues,
            paramLengths,
            paramFormats,
            TEXT_FRMT);             /* text results */
        if(PQresultStatus(res) != PGRES_COMMAND_OK){
            _DB_close_res(db, conn, res);
            return 1;
        }
        PQclear(res);
    }
    printf("For complete!\n");
    
    return 0;
}

static int computers_cmp(const void *key, const void *el){
    return strcmp((char *) key, ((LC_Computer *) el)->domain);
}

int DB_sync_comps(Database *db, LocalCache *cache, const LC_Computer *comps, size_t comps_count){
    PGconn   *conn;
    PGresult *res;
    PGresult *upd_res;
    LC_Computer *finded_comp;
    const char *paramValues[2];
    const int   paramFormats[2] = {BINARY_FRMT, BINARY_FRMT};
    const int   paramLengths[2] = {sizeof(uint32_t), sizeof(time_t)};
    uint32_t time_cr_at;
    int nTuples, i;
    uint32_t     tmp_id;
    time_t       tmp_cr_at;
    printf("sync comps...\n");
    if(_DB_connect(db, &conn)    != 0) return 1;
    
    res = PQexec(conn, "SELECT DISTINCT ON (c.id) "
                        "c.id, c.domain_name, c.created_at, c.deleted_at, "
                        "l.md5sum "
                        "FROM "COMPUTERS_TABNAME" c "
                        "LEFT OUTER JOIN "LOGS_TABNAME" l "
                            "ON c.domain_name LIKE l.domain_name "
                        "ORDER BY c.id, l.created_at DESC;");
    if (PQresultStatus(res) != PGRES_TUPLES_OK){
        _DB_close_res(db, conn, res);
        return 0;
    }
    printf("1...\n");
    nTuples = PQntuples(res);
    for(i = 0; i < nTuples; i++){
        /* Ищем комп из базы в кэше */
        finded_comp = bsearch(PQgetvalue(res, i, 1),    /* domain_name */
                        comps,
                        comps_count,
                        sizeof(LC_Computer),
                        computers_cmp);
        printf("2...\n");
        if(finded_comp){    /* Компьютер из базы найден в кэше. */
            printf("finded_comp\n");
            if(PQgetisnull(res, i, 3)){ /* не удалён в БД */
                /* Синхронизировать параметры */
                tmp_cr_at = str_to_time(PQgetvalue(res, i, 2));     /* created_at */
                tmp_id    = (uint32_t) atoi(PQgetvalue(res, i, 0)); /* db_id */
                Cache_try_begin(cache);
                Cache_upd_comp_id(cache,
                    finded_comp->id,
                    tmp_id,
                    (finded_comp->created_at > tmp_cr_at ? tmp_cr_at : -1)
                );
                if(finded_comp->created_at < tmp_cr_at && finded_comp->created_at > 0){
                    /* если created_at в кэше более ранний - внести в базу */
                    if(_DB_try_begin(db, conn, &upd_res) == 0){
                        paramValues[0] = order_int(tmp_id, &tmp_id);
                        /* не chk_time_to_null, т.к. (finded_comp->created_at > 0) */
                        paramValues[1] = order_int((uint32_t) finded_comp->created_at, &time_cr_at);
                        upd_res = PQexecParams(conn,
                            "UPDATE "COMPUTERS_TABNAME" "
                                "SET created_at = "TO_TIMESTAMP("$2")" "
                                "WHERE id = $1::int;",
                            2,              /* count of params */
                            NULL,           /* let the backend deduce param type */
                            paramValues,
                            paramLengths,
                            paramFormats,
                            TEXT_FRMT);             /* text results */
                        if(PQresultStatus(upd_res) != PGRES_COMMAND_OK){
                            /* syslog ошибка!!! */
                            printf("ERROR!!! UPDATE\n");
                        }
                        PQclear(upd_res);
                    }
                    else{
                        PQclear(upd_res);
                        /* _DB_close_res(db, conn, res); не разрывать соединение!!! */
                        /* syslog ошибка!!! */
                        printf("ERROR!!! Cannot BEGIN!\n");
                    }
                }
            }
            else{ /* удалён в БД - удалить его и из кэша! */
                Cache_try_begin(cache);
                Cache_del_comp(cache, finded_comp->id);
            }
            /* id всех компьютеров, с которыми мы уже поработали устанавливаем в 0,
             * что позволит нам выявить новые компьютеры ( которые есть в кэше,
             * но нет в базе данных) */
            finded_comp->id = 0;
        }
        else if(PQgetisnull(res, i, 3)){   /* Не найден в кэше, и не удалён в БД */
            printf("PQgetisnull(res, i, 3)\n");
            Cache_try_begin(cache);
            /* Загружаем комп из кэша в БД */
            Cache_put_comp(cache,
                (unsigned int) atoi(PQgetvalue(res, i, 0)), /* db_id */
                PQgetvalue(res, i, 1),                      /* domain_name */
                PQgetvalue(res, i, 4),                      /* md5sum */
                str_to_time(PQgetvalue(res, i, 2))          /* created_at */
            );
            /*
            Hash_insert(&hash, PQgetvalue(res, i, 1), strlen(PQgetvalue(res, i, 1)), PQgetvalue(res, i, 4), COMPUTER_MD5_SUM_SIZE);
            */
        }
        /* else {нет в кэше и удалён в БД - делать нечего} */
    }
    PQclear(res);
    
    Cache_commit(cache);
    
    _DB_put_comps(db, conn, comps, comps_count);
    
    if(_DB_commit(db, conn, &upd_res) != 0){
        //~ _DB_close_res(db, conn, res);
        /*syslog()*/
        printf("ERROR!!!\n");
    }
    printf("Error not in commit\n");
    //~ PQclear(upd_res);
    
    PQfinish(conn);
    printf("DB_sync exit...\n");
    return 0;
}
#if 0
int DB_sync_details(Database *db, LocalCache *cache, const LC_Detail *detls, size_t detls_count){
    PGconn   *conn;
    PGresult *res;
    PGresult *upd_res;
    LC_Detail *finded_comp;
    const char *paramValues[2];
    const int   paramFormats[2] = {BINARY_FRMT, BINARY_FRMT};
    const int   paramLengths[2] = {sizeof(uint32_t), sizeof(time_t)};
    uint32_t time_cr_at;
    int nTuples, i;
    uint32_t     tmp_id;
    time_t       tmp_cr_at;
    printf("sync details...\n");
    if(_DB_connect(db, &conn)    != 0) return 1;
    
    res = PQexec(conn, "SELECT id, vendor_id, device_id, subsystem_id, "
                            "revision, serial, created_at, deleted_at, "
                            "class_code FROM "DETAILS_TABNAME";");
    if (PQresultStatus(res) != PGRES_TUPLES_OK){
        _DB_close_res(db, conn, res);
        return 0;
    }
    printf("1...\n");
    nTuples = PQntuples(res);
    for(i = 0; i < nTuples; i++){
        /* Ищем комп из базы в кэше */
        finded_comp = bsearch(PQgetvalue(res, i, 1),    /* domain_name */
                        comps,
                        comps_count,
                        sizeof(LC_Computer),
                        computers_cmp);
        printf("2...\n");
        if(finded_comp){    /* Компьютер из базы найден в кэше. */
            printf("finded_comp\n");
            if(PQgetisnull(res, i, 3)){ /* не удалён в БД */
                /* Синхронизировать параметры */
                tmp_cr_at = str_to_time(PQgetvalue(res, i, 2));     /* created_at */
                tmp_id    = (uint32_t) atoi(PQgetvalue(res, i, 0)); /* db_id */
                Cache_try_begin(cache);
                Cache_upd_comp_id(cache,
                    finded_comp->id,
                    tmp_id,
                    (finded_comp->created_at > tmp_cr_at ? tmp_cr_at : -1)
                );
                if(finded_comp->created_at < tmp_cr_at && finded_comp->created_at > 0){
                    /* если created_at в кэше более ранний - внести в базу */
                    if(_DB_try_begin(db, conn, &upd_res) == 0){
                        paramValues[0] = order_int(tmp_id, &tmp_id);
                        /* не chk_time_to_null, т.к. (finded_comp->created_at > 0) */
                        paramValues[1] = order_int((uint32_t) finded_comp->created_at, &time_cr_at);
                        upd_res = PQexecParams(conn,
                            "UPDATE "COMPUTERS_TABNAME" "
                                "SET created_at = "TO_TIMESTAMP("$2")" "
                                "WHERE id = $1::int;",
                            2,              /* count of params */
                            NULL,           /* let the backend deduce param type */
                            paramValues,
                            paramLengths,
                            paramFormats,
                            TEXT_FRMT);             /* text results */
                        if(PQresultStatus(upd_res) != PGRES_COMMAND_OK){
                            /* syslog ошибка!!! */
                            printf("ERROR!!! UPDATE\n");
                        }
                        PQclear(upd_res);
                    }
                    else{
                        PQclear(upd_res);
                        /* _DB_close_res(db, conn, res); не разрывать соединение!!! */
                        /* syslog ошибка!!! */
                        printf("ERROR!!! Cannot BEGIN!\n");
                    }
                }
            }
            else{ /* удалён в БД - удалить его и из кэша! */
                Cache_try_begin(cache);
                Cache_del_comp(cache, finded_comp->id);
            }
            /* id всех компьютеров, с которыми мы уже поработали устанавливаем в 0,
             * что позволит нам выявить новые компьютеры ( которые есть в кэше,
             * но нет в базе данных) */
            finded_comp->id = 0;
        }
        else if(PQgetisnull(res, i, 3)){   /* Не найден в кэше, и не удалён в БД */
            printf("PQgetisnull(res, i, 3)\n");
            Cache_try_begin(cache);
            /* Загружаем комп из кэша в БД */
            Cache_put_comp(cache,
                (unsigned int) atoi(PQgetvalue(res, i, 0)), /* db_id */
                PQgetvalue(res, i, 1),                      /* domain_name */
                PQgetvalue(res, i, 4),                      /* md5sum */
                str_to_time(PQgetvalue(res, i, 2))          /* created_at */
            );
            /*
            Hash_insert(&hash, PQgetvalue(res, i, 1), strlen(PQgetvalue(res, i, 1)), PQgetvalue(res, i, 4), COMPUTER_MD5_SUM_SIZE);
            */
        }
        /* else {нет в кэше и удалён в БД - делать нечего} */
    }
    PQclear(res);
    
    Cache_commit(cache);
    
    _DB_put_comps(db, conn, comps, comps_count);
    
    if(_DB_commit(db, conn, &upd_res) != 0){
        //~ _DB_close_res(db, conn, res);
        /*syslog()*/
        printf("ERROR!!!\n");
    }
    printf("Error not in commit\n");
    //~ PQclear(upd_res);
    
    PQfinish(conn);
    printf("DB_sync exit...\n");
    return 0;
}
#endif
int DB_store_comp_to_cache(Database *db, LocalCache *cache){
    PGconn      *conn;
    PGresult    *res;
    int nTuples, i;
    printf("store comps to cache...\n");
    if(_DB_connect(db, &conn) != 0) return 0;
    
    res = PQexec(conn, "SELECT DISTINCT ON (c.id) "
                        "c.id, c.domain_name, c.created_at, l.md5sum "
                        "FROM "COMPUTERS_TABNAME" c "
                        "LEFT OUTER JOIN "LOGS_TABNAME" l "
                            "ON c.domain_name LIKE l.domain_name "
                        "WHERE c.deleted_at IS NULL "
                        "ORDER BY c.id, l.created_at DESC;");
    if (PQresultStatus(res) != PGRES_TUPLES_OK){
        _DB_close_res(db, conn, res);
        return 1;
    }
    
    Cache_begin(cache);
    
    nTuples = PQntuples(res);
    for(i = 0; i < nTuples; i++){
        Cache_put_comp(cache,
            (unsigned int) atoi(PQgetvalue(res, i, 0)), /* db_id */
            PQgetvalue(res, i, 1),                      /* domain_name */
            PQgetvalue(res, i, 2),                      /* md5sum */
            str_to_time(PQgetvalue(res, i, 3))          /* created_at */
        );
    }
    
    Cache_commit(cache);
    
    PQclear(res);
    PQfinish(conn);
    
    return 0;
}

int DB_put_details(Database *db, const LC_Detail *details, size_t details_count){
    PGconn   *conn;
    PGresult *res;
    const char *paramValues[5];
          int   paramLengths[5] = {[0 ... 4] = sizeof(int)};
          int   paramFormats[5] = {[0 ... 4] = BINARY_FRMT};
    uint32_t vend_id, dev_id, ssys_id, time_cr_at;
    int i;
    
    paramLengths[3] = DETAIL_SERIAL_SIZE;
    paramFormats[3] = TEXT_FRMT;
    
    if(_DB_connect(db, &conn)    != 0) return 1;
    if(_DB_begin(db, conn, &res) != 0){ _DB_close_res(db, conn, res); return 1;}
    
    for(i = 0; i < details_count; i++){
        paramValues[0] = order_int(details[i].vendor_id, &vend_id);
        paramValues[1] = order_int(details[i].device_id, &dev_id);
        paramValues[2] = order_int(details[i].subsystem_id, &ssys_id);
        paramValues[3] = details[i].serial;
        paramValues[4] = chk_time_to_null(details[i].created_at, &time_cr_at);
                
        res = PQexecParams(conn,
            "INSERT INTO "DETAILS_TABNAME
                "(vendor_id, device_id, subsystem_id, serial, created_at) "
                "VALUES($1::int, "
                    "$2::int, "
                    "(SELECT id FROM subsystems "
                        "WHERE subvendor_id = $3::int >> 16 AND "
                            "subdevice_id = $3::int & 65535), "
                    "$4::varchar, "
                    TO_TIMESTAMP("$5")");",
            5,              /* count of params */
            NULL,           /* let the backend deduce param type */
            paramValues,
            paramLengths,
            paramFormats,
            TEXT_FRMT);             /* text results */
        if(PQresultStatus(res) != PGRES_COMMAND_OK){
            _DB_close_res(db, conn, res);
            return 1;
        }
        PQclear(res);
    }
    if(_DB_commit(db, conn, &res) != 0){ _DB_close_res(db, conn, res); return 1;}
    
    PQfinish(conn);
    return 0;
}

int sync_cache_and_hash(LocalCache *cache, HASH *hash){
    LC_Computer  *cache_comps;
    HASH_ELEMENT *ret;
    size_t       row_count;
    char         val[HASH_ELEMENT_VAL_SIZE];
    char         *md5sum;
    int i = 0, j;
    cache_comps = Cache_get_comps(cache, &row_count, 0, NULL);
    for(; i < row_count; i++){
        md5sum = cache_comps[i].md5sum;
        for(j = 0; j < HASH_ELEMENT_VAL_SIZE; j++){
            val[j] = (char) (unsigned char) strxtoi(md5sum, 2);
            md5sum += 2;
        }
        ret = Hash_insert(hash, cache_comps[i].domain, COMPUTER_DOMAIN_NAME_SIZE,
                                val, HASH_ELEMENT_VAL_SIZE);
        if(ret == NULL){
            /* ERROR */
            return 1;
        }
    }
    free(cache_comps);
    return 0;
}

int sync_cache_and_db(LocalCache *cache){
    Database db;
    LC_Log      *cache_logs;
    LC_Computer *cache_comps;
    //~ LC_Detail   *cache_details;
    size_t  row_count;
    
    DB_init(&db, PQ_CONNECT_INFO, 1);
    
    /* Первым делом выгружаем логи. */
    cache_logs = Cache_get_logs(cache, &row_count, 0, NULL);
    if(row_count > 0){
        DB_put_logs(&db, cache_logs, row_count);
        free(cache_logs);
    }
    printf("Now, get comps...\n");
    /* Синхронизируем компьютеры */
    cache_comps = Cache_get_comps(cache, &row_count, 0, NULL);
    if(row_count > 0){
        DB_sync_comps(&db, cache, cache_comps, row_count);
        free(cache_comps);
    }
    else{
        DB_store_comp_to_cache(&db, cache);
    }
    
    /* Синхронизируем детали *//*
    cache_details = Cache_get_details(cache, &row_count, 0);
    if(row_count > 0){
        DB_sync_details(&db, cache, cache_comps, row_count);
        free(cache_comps);
    }*/
    
    /* Синхронизируем конфигурации */
    
    
    DB_destroy(&db);
    
	return 0;
}
