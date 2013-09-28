/* encoding: UTF-8 */

/** db_sync.c, Ivan Korunkov, 2013 **/

/**
    Compile with keys: -I<INCLUDEDIR> -L<LIBDIR> -lpq
    Where:
    <INCLUDEDIR> - /usr/include/postgresql (try: $ pg_config --includedir)
    <LIBDIR>     - /usr/lib/               (try: $ pg_config --libdir)
**/

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>      /* htonl() */
#include <time.h>
#include <libpq-fe.h>

#include "../cache/cache.h"
#include "../dbdef.h"
#include "db_sync.h"

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
    if(!db->catch_err) return;
    
    char   *msg;
    //~ size_t  length;
    
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

static int _DB_begin(Database *db, PGconn *conn, PGresult **res){
    *res = PQexec(conn, "BEGIN;");
    if(PQresultStatus(*res) != PGRES_COMMAND_OK){
        _DB_close_res(db, conn, *res);
        return 1;
    }
    PQclear(*res);
    return 0;
}

static int _DB_commit(Database *db, PGconn *conn, PGresult **res){
    *res = PQexec(conn, "COMMIT;");
    if(PQresultStatus(*res) != PGRES_COMMAND_OK){
        _DB_close_res(db, conn, *res);
        return 1;
    }
    PQclear(*res);
    return 0;
}

void DB_init(Database *db, const char *conninfo, int catch_err){
    db->err_msg   = NULL;
    db->conninfo  = conninfo;
    db->catch_err = catch_err;
}

void DB_destroy(Database *db){
    if(db->err_msg) free(db->err_msg);
}

int _DB_mark_config_del(Database *db, unsigned int id, time_t del_at){
    PGconn   *conn;
    PGresult *res;
    const char *paramValues[2];
    const int   paramLengths[2] = {[0 ... 1] = sizeof(int)};
    const int   paramFormats[2] = {1, 1};
    uint32_t config_id, time_del_at;
    int i;
    
    if(_DB_connect(db, &conn)    != 0) return 1;
    if(_DB_begin(db, conn, &res) != 0) return 1;
    
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
        0);             /* text results */
    if(PQresultStatus(res) != PGRES_COMMAND_OK){
        _DB_close_res(db, conn, res);
        return 1;
    }
    PQclear(res);
    
    if(_DB_commit(db, conn, &res) != 0) return 1;
    
    PQfinish(conn);
    return 0;
}

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
    const int   paramFormats[1] = {1};
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
            0);             /* text results */
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
            details[i].created_at
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
    const int   paramFormats[3] = {0, 0, 1};
          int   paramLengths[3];
    int i;
    
    if(_DB_connect(db, &conn)    != 0) return 1;
    if(_DB_begin(db, conn, &res) != 0) return 1;
    
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
            0);             /* text results */
        if(PQresultStatus(res) != PGRES_COMMAND_OK){
            _DB_close_res(db, conn, res);
            return 1;
        }
        PQclear(res);
    }
    if(_DB_commit(db, conn, &res) != 0) return 1;
    
    PQfinish(conn);
    return 0;
}

int DB_put_configs(Database *db, const LC_Config *confs, size_t configs_count){
    PGconn   *conn;
    PGresult *res;
    const char *paramValues[4];
    const int   paramLengths[4] = {[0 ... 3] = sizeof(int)};
    const int   paramFormats[4] = {1, 1, 1, 1};
    uint32_t comp_id, det_id, time_cr_at, time_del_at;
    int i;
    
    if(_DB_connect(db, &conn)    != 0) return 1;
    if(_DB_begin(db, conn, &res) != 0) return 1;
    
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
            0);             /* text results */
        if(PQresultStatus(res) != PGRES_COMMAND_OK){
            _DB_close_res(db, conn, res);
            return 1;
        }
        PQclear(res);
    }
    if(_DB_commit(db, conn, &res) != 0) return 1;
    
    PQfinish(conn);
    return 0;
}

int DB_put_comps(Database *db, const LC_Computer *comps, size_t comps_count){
    PGconn   *conn;
    PGresult *res;
    const char *paramValues[2];
    const int   paramFormats[2] = {0, 1};
    const int   paramLengths[2] = {COMPUTER_DOMAIN_NAME_SIZE, sizeof(time_t)};
    uint32_t time_cr_at;
    int i;
    
    if(_DB_connect(db, &conn)    != 0) return 1;
    if(_DB_begin(db, conn, &res) != 0) return 1;
    
    for(i = 0; i < comps_count; i++){
        paramValues[0] = comps[i].domain;
        paramValues[1] = chk_time_to_null(comps[i].created_at, &time_cr_at);
        res = PQexecParams(conn,
            "INSERT INTO "COMPUTERS_TABNAME"(domain_name, created_at) "
                "VALUES($1::varchar, "
                    TO_TIMESTAMP("$2")");",
            2,              /* count of params */
            NULL,           /* let the backend deduce param type */
            paramValues,
            paramLengths,
            paramFormats,
            0);             /* text results */
        if(PQresultStatus(res) != PGRES_COMMAND_OK){
            _DB_close_res(db, conn, res);
            return 1;
        }
        PQclear(res);
    }
    if(_DB_commit(db, conn, &res) != 0) return 1;
    
    PQfinish(conn);
    return 0;
}

static int computers_cmp(const void *key, const void *el){
    return strcmp((char *) key, ((LC_Computer *) el)->domain);
}

int DB_sync_comps(Database *db, LocalCache *cache, const LC_Computer *comps, size_t comps_count){
    PGconn   *conn;
    PGresult *res;
    LC_Computer *finded_comp;
    const char *paramValues[2];
    const int   paramFormats[2] = {0, 1};
    const int   paramLengths[2] = {COMPUTER_DOMAIN_NAME_SIZE, sizeof(time_t)};
    uint32_t time_cr_at;
    int nTuples, i;
    size_t array_size;
    
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
        return NULL;
    }
    
    array_size = sizeof(LC_Computer) * comps_count;
    nTuples = PQntuples(res);
    for(i = 0; i < nTuples; i++){
        /* Ищем комп из базы в кэше */
        finded_comp = bsearch(PQgetvalue(res, i, 1),    /* domain_name */
                        comps,
                        array_size,
                        sizeof(LC_Computer),
                        computers_cmp);
        
        if(finded_comp){    /* Компьютер из базы найден в кэше. */
            if(PQgetisnull(res, i, 3)){ /* не удалён в БД */
                /* Синхронизировать параметры */
                
            }
            else{ /* удалён в БД - удалить его и из кэша! */
                Cache_try_begin(myCache);
                Cache_del_comp(myCache, finded_comp->id);
            }
        }
        else if(PQgetisnull(res, i, 3)){   /* Не найден в кэше, и не удалён в БД */
            Cache_try_begin(myCache);
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
    
    Cache_commit(myCache);
    
    PQfinish(conn);
    return 0;
    
    /********************************************************************************/
    if(_DB_begin(db, conn, &res) != 0) return 1;
    
    for(i = 0; i < comps_count; i++){
        paramValues[0] = comps[i].domain;
        paramValues[1] = chk_time_to_null(comps[i].created_at, &time_cr_at);
        res = PQexecParams(conn,
            "INSERT INTO "COMPUTERS_TABNAME"(domain_name, created_at) "
                "VALUES($1::varchar, "
                    TO_TIMESTAMP("$2")");",
            2,              /* count of params */
            NULL,           /* let the backend deduce param type */
            paramValues,
            paramLengths,
            paramFormats,
            0);             /* text results */
        if(PQresultStatus(res) != PGRES_COMMAND_OK){
            _DB_close_res(db, conn, res);
            return 1;
        }
        PQclear(res);
    }
    if(_DB_commit(db, conn, &res) != 0) return 1;
    /********************************************************************************/
    if(_DB_connect(db, &conn) != 0) return NULL;
    
    res = PQexec(conn, "SELECT id, domain_name, created_at "
                        "FROM "COMPUTERS_TABNAME" "
                        "WHERE deleted_at IS NULL;");
    if (PQresultStatus(res) != PGRES_TUPLES_OK){
        _DB_close_res(db, conn, res);
        return NULL;
    }
    
    nTuples = PQntuples(res);
    for(i = 0; i < nTuples; i++){
        comps[i].id         = (unsigned int) atoi(PQgetvalue(res, i, 0));
        strncpy(comps[i].domain,
                PQgetvalue(res, i, 1),
                COMPUTER_DOMAIN_NAME_SIZE);
        comps[i].created_at = str_to_time(PQgetvalue(res, i, 2));
    }
    PQclear(res);
    PQfinish(conn);
    return comps;
    /********************************************************************************/
}

int DB_store_comp_to_cache(Database *db, LocalCache *cache){
    PGconn      *conn;
    PGresult    *res;
    int nTuples, i;
    
    if(_DB_connect(db, &conn) != 0) return NULL;
    
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
    
    Cache_begin(&myCache);
    
    nTuples = PQntuples(res);
    for(i = 0; i < nTuples; i++){
        Cache_put_comp(cache,
            (unsigned int) atoi(PQgetvalue(res, i, 0)), /* db_id */
            PQgetvalue(res, i, 1),                      /* domain_name */
            PQgetvalue(res, i, 2),                      /* md5sum */
            str_to_time(PQgetvalue(res, i, 3))          /* created_at */
        );
    }
    
    Cache_commit(&myCache);
    
    PQclear(res);
    PQfinish(conn);
    
    return 0;
}

int DB_put_details(Database *db, const LC_Detail *details, size_t details_count){
    PGconn   *conn;
    PGresult *res;
    const char *paramValues[5];
          int   paramLengths[5] = {[0 ... 4] = sizeof(int)};
    const int   paramFormats[5] = {1, 1, 1, 0, 1};
    uint32_t vend_id, dev_id, ssys_id, time_cr_at;
    int i;
    
    paramLengths[3] = DETAIL_SERIAL_SIZE;
    
    if(_DB_connect(db, &conn)    != 0) return 1;
    if(_DB_begin(db, conn, &res) != 0) return 1;
    
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
            0);             /* text results */
        if(PQresultStatus(res) != PGRES_COMMAND_OK){
            _DB_close_res(db, conn, res);
            return 1;
        }
        PQclear(res);
    }
    if(_DB_commit(db, conn, &res) != 0) return 1;
    
    PQfinish(conn);
    return 0;
}
