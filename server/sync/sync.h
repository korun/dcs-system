/* encoding: UTF-8 */

/** db_sync.c, Ivan Korunkov, 2013 **/

/**
    Compile with keys: -I<INCLUDEDIR> -L<LIBDIR> -lpq
    Where:
    <INCLUDEDIR> - /usr/include/postgresql (try: $ pg_config --includedir)
    <LIBDIR>     - /usr/lib/               (try: $ pg_config --libdir)
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>      /* htonl() */
#include <time.h>
#include <libpq-fe.h>

#include "../../cache/cache.h"
#include "../../hash/hash.h"
#include "../../dbdef.h"

#define DB_TIME_FORMAT      "%Y-%m-%d %H:%M:%S"

typedef struct {
    char *conninfo;
    char *err_msg;
    int   catch_err; /* 0 - no, 1 - yes */
    int   begin;     /* есть незакрытый BEGIN? */
} Database;

typedef struct {
    unsigned int id;
    char         domain[COMPUTER_DOMAIN_NAME_SIZE + 1]; /* + '\0' */
    time_t       created_at;
} DB_Computer;

typedef struct {
    unsigned int id;
    unsigned int vendor_id;
    unsigned int device_id;
    unsigned int subsystem_id;
    /*unsigned int revision;*/
    char         serial[DETAIL_SERIAL_SIZE + 1]; /* + '\0' */
    time_t       created_at;
} DB_Detail;

int sync_cache_and_hash(LocalCache *cache, HASH *hash);
int sync_cache_and_db(LocalCache *cache);

/*
void DB_init(Database *db, const char *conninfo, int catch_err);
void DB_destroy(Database *db);
DB_Computer *DB_get_comps(Database *db);
DB_Detail *DB_get_details(Database *db, unsigned int computer_id);
int DB_put_logs(Database *db, const LC_Log *logs, size_t logs_count);
int DB_put_configs(Database *db, const LC_Config *confs, size_t configs_count);
int DB_put_comps(Database *db, const LC_Computer *comps, size_t comps_count);
int DB_put_details(Database *db, const LC_Detail *details, size_t details_count);
*/
