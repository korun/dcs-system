/* encoding: UTF-8 */

#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "../dbdef.h"
#include "cache.h"

#define error_std_text(s) strerror((s) ? errno : 0)
#define error_sql_text(s, db) ((s) ? sqlite3_errmsg((db)) : strerror(0))

#define SYNC_TEST

#define MIN(A, B) ((A) > (B) ? (B) : (A))

#ifndef _XOPEN_SOURCE
    #define _XOPEN_SOURCE     /* glibc2 needs this (strptime) */
#endif

#define CONVERT_TIME(i, s, tm)                \
    strptime(s, "%Y-%m-%d %H:%M:%S", &(tm));    \
    i = mktime(&(tm));

#ifdef SYNC_TEST
    
    #include <time.h>
    #include <sys/time.h>
    
    typedef struct {
        unsigned int id;
        char domain[COMPUTER_DOMAIN_NAME_SIZE + 1];
        char md5sum[COMPUTER_MD5_SUM_SIZE     + 1]; /* + '\0' */
        time_t created_at;
        time_t accepted_at;
        time_t deleted_at;
    } DB_COMPUTER;
    
    DB_COMPUTER base_computers[5];
    
    typedef struct {
        unsigned int id;
        unsigned int vendor_id;
        unsigned int device_id;
        unsigned int subsystem_id;
        char serial[DETAIL_SERIAL_SIZE + 1]; /* + '\0' */
        time_t created_at;
        time_t accepted_at;
        time_t deleted_at;
    } DB_DETAIL;
    
    DB_DETAIL base_details[13];
    
    typedef struct {
        unsigned int id;
        unsigned int computer_id;
        unsigned int detail_id;
        time_t created_at;
        time_t deleted_at;
        time_t accepted_at;
        time_t allow_del_at;
        time_t validated_at;
    } DB_CONFIG;
    
    DB_CONFIG base_config[19];
#endif

char *gen_md5(char * str){
    int i, j;
    for(i = 0; i < COMPUTER_MD5_SUM_SIZE; i++){
        j = rand();
        while(j > 'Z' - '/') j /= 10;
        j += '/';
        if(j > '9' && j < 'A') j += j - '/';
        str[i] = (char) j;
    }
    str[i] = '\0';
    return str;
}

void put_log(LocalCache *cache, const char *domain, const char *md5){
    int rc;
    
    rc = Cache_put_log(cache, domain, md5);
    printf("Put log: %d\t- %s.\n", rc, error_sql_text(rc, cache->db));
}

void put_comp(LocalCache * cache, int id, const char * domain, const char * md5, time_t time){
    int rc;
    
    rc = Cache_put_comp(cache, id, domain, md5, time);
    printf("Put comp: %d\t- %s.\n", rc, error_sql_text(rc, cache->db));
}

void put_detail(LocalCache * cache, int vid, int did, int sid, const char * md5){
    int rc;
    
    rc = Cache_put_detail(cache, vid, did, sid, md5);
    printf("Put detail: %d\t- %s.\n", rc, error_sql_text(rc, cache->db));
}

void put_config(LocalCache * cache, int cid, int did, time_t time){
    int rc;
    
    rc = Cache_put_config(cache, cid, did, time);
    printf("Put config: %d\t- %s.\n", rc, error_sql_text(rc, cache->db));
}

#ifdef SYNC_TEST
char *gen_sn(char * str){
    int i, j;
    str[0] = 'S';
    str[1] = 'N';
    str[2] = '-';
    for(i = 3; i < MIN(DETAIL_SERIAL_SIZE, 12); i++){
        j = rand();
        while(j > 'Z' - '/') j /= 10;
        j += '/';
        if(j > '9' && j < 'A') j += j - '/';
        str[i] = (char) j;
    }
    str[i] = '\0';
    return str;
}

void set_comp(
        int i,
        unsigned int id,
        const char *domain,
        const char *created_at,
        const char *accepted_at,
        const char *deleted_at
    ){
    char md5_sum[COMPUTER_MD5_SUM_SIZE + 1];
    struct tm tm;
    time_t tmp_time;
    
    memset(&tm, 0, sizeof(struct tm));
    
    base_computers[i].id = id;
    
    snprintf(base_computers[i].domain, COMPUTER_DOMAIN_NAME_SIZE, "%s", domain);
    
    snprintf(base_computers[i].md5sum, COMPUTER_MD5_SUM_SIZE, "%s", gen_md5(md5_sum));
    
    tmp_time = 0;
    if(created_at){ CONVERT_TIME(tmp_time, created_at, tm); }
    base_computers[i].created_at  = tmp_time;
    
    tmp_time = 0;
    if(accepted_at){ CONVERT_TIME(tmp_time, accepted_at, tm); }
    base_computers[i].accepted_at = tmp_time;
    
    tmp_time = 0;
    if(deleted_at){ CONVERT_TIME(tmp_time, deleted_at, tm); }
    base_computers[i].deleted_at  = tmp_time;
    /*
    printf("Set comp: <#COMPUTER id:%.2d, domain:'%s',     \tmd5sum:'%s', created_at:%.10d, accepted_at:%.10d, deleted_at:%.10d>\n",
        base_computers[i].id,
        base_computers[i].domain,
        base_computers[i].md5sum,
        base_computers[i].created_at,
        base_computers[i].accepted_at,
        base_computers[i].deleted_at);*/
}

void set_detail(
        int i,
        unsigned int id,
        unsigned int vid,
        unsigned int did,
        unsigned int sid,
        const char * created_at,
        const char * accepted_at,
        const char * deleted_at
    ){
    char serial_no[DETAIL_SERIAL_SIZE + 1];
    struct tm tm;
    time_t tmp_time;
    
    memset(&tm, 0, sizeof(struct tm));
    
    base_details[i].id = id;
    base_details[i].vendor_id = vid;
    base_details[i].device_id = did;
    base_details[i].subsystem_id = sid;
    
    snprintf(base_details[i].serial, DETAIL_SERIAL_SIZE, "%s", gen_sn(serial_no));
    
    tmp_time = 0;
    if(created_at){ CONVERT_TIME(tmp_time, created_at, tm); }
    base_details[i].created_at  = tmp_time;
    
    tmp_time = 0;
    if(accepted_at){ CONVERT_TIME(tmp_time, accepted_at, tm); }
    base_details[i].accepted_at = tmp_time;
    
    tmp_time = 0;
    if(deleted_at){ CONVERT_TIME(tmp_time, deleted_at, tm); }
    base_details[i].deleted_at  = tmp_time;
    /*
    printf("Set detail: <#DETAIL id:%.2d, vid:%.4x, did:%.4x, sid:%.8x, serial:'%s', created_at:%.10d, accepted_at:%.10d, deleted_at:%.10d>\n",
        base_details[i].id,
        base_details[i].vendor_id,
        base_details[i].device_id,
        base_details[i].subsystem_id,
        base_details[i].serial,
        base_details[i].created_at,
        base_details[i].accepted_at,
        base_details[i].deleted_at);*/
}

void set_config(
        int i,
        unsigned int id,
        unsigned int computer_id,
        unsigned int detail_id,
        const char * created_at,
        const char * deleted_at,
        const char * accepted_at,
        const char * allow_del_at,
        const char * validated_at
    ){
    struct tm tm;
    time_t tmp_time;
    
    memset(&tm, 0, sizeof(struct tm));
    
    base_config[i].id = id;
    base_config[i].computer_id = computer_id;
    base_config[i].detail_id = detail_id;
    
    tmp_time = 0;
    if(created_at){ CONVERT_TIME(tmp_time, created_at, tm); }
    base_config[i].created_at  = tmp_time;
    
    tmp_time = 0;
    if(accepted_at){ CONVERT_TIME(tmp_time, accepted_at, tm); }
    base_config[i].accepted_at = tmp_time;
    
    tmp_time = 0;
    if(deleted_at){ CONVERT_TIME(tmp_time, deleted_at, tm); }
    base_config[i].deleted_at  = tmp_time;
    
    tmp_time = 0;
    if(allow_del_at){ CONVERT_TIME(tmp_time, deleted_at, tm); }
    base_config[i].allow_del_at  = tmp_time;
    
    tmp_time = 0;
    if(validated_at){ CONVERT_TIME(tmp_time, deleted_at, tm); }
    base_config[i].validated_at  = tmp_time;
    /*
    printf("Set config: <#CONFIG id:%.2d, computer_id:%.2d, detail_id:%.2d, created_at:%.10d, accepted_at:%.10d, deleted_at:%.10d, allow_del_at:%.10d, validated_at:%.10d>\n",
        base_config[i].id,
        base_config[i].computer_id,
        base_config[i].detail_id,
        base_config[i].created_at,
        base_config[i].accepted_at,
        base_config[i].deleted_at,
        base_config[i].allow_del_at,
        base_config[i].validated_at);*/
}
#endif

int main(void){
    LocalCache myCache;
    char md5_sum[33];
    int rc, i, j;
    LC_Log *log_array;
    
    size_t arr_size;
    LC_Computer *comp_array;
    LC_Detail *detail_array;
    LC_Config *config_arr;
    
    srand((unsigned int) time(NULL));
    
    #ifndef SYNC_TEST

    printf("Start cache testing...\n");
    
    rc = Cache_init("temp.bd", &myCache);
    printf("REInit cache: %d\t- %s.\n", rc, error_std_text(rc));
    printf("HO-HO-HO %s\n", sqlite3_errmsg(myCache.db));
    put_log(&myCache, "ice21@msiu.ru", gen_md5(md5_sum));
    
    /* Server insert new machine 00 */
    put_comp(&myCache, rand(), "ice21@msiu.ru", gen_md5(md5_sum), NULL);
    put_comp(&myCache, rand(), "monster15@msiu.ru", gen_md5(md5_sum), NULL);
    /* Server get DB machine     10 */
    put_comp(&myCache, rand(), "ice11@msiu.ru", gen_md5(md5_sum), "2012-12-21 00:00:00.321656");
    /* Server get DB machine     11 */
    put_comp(&myCache, 145, "monster14@msiu.ru", gen_md5(md5_sum), "2002-05-17 14:52:13.123486");
    
    for(i = 0; i < 15; i++)
        put_detail(&myCache, rand(), rand(), rand(), gen_md5(md5_sum));
    
    put_config(&myCache, rand(), rand(), NULL);
    put_config(&myCache, rand(), rand(), "2012-12-21 00:00:00.321656");
    
    printf("destroy...\n");
    Cache_destroy(&myCache);
    
    #else
    
    set_comp(0, 1,  "ice01@msiu.ru",      "1992-11-01 12:52:35", "1992-12-09 02:41:28", NULL);
    set_comp(1, 18, "lib18@msiu.ru",      "2003-02-21 09:17:59", "2003-06-19 20:39:30", NULL);
    set_comp(2, 7,  "fire07@msiu.ru",     "1998-05-24 22:45:56", "1998-09-04 14:28:23", NULL);
    set_comp(3, 14, "monster14@msiu.ru",  "2000-11-04 09:29:42", "2000-11-20 22:41:37", NULL);
    set_comp(4, 5,  "gorynich05@msiu.ru", "1993-07-07 05:20:55", "1993-07-10 18:49:03", "1995-09-05 03:03:52");
    
    /* ice01 */
    set_detail(0, 11, 0x1002, 0x791e, 0x14627327, "1992-11-01 12:52:35", "1992-12-09 02:41:28", NULL);    /* Video  */
    set_detail(1, 12, 0x2646, 0x0000, 0x00000000, "1992-11-01 12:52:35", "1992-12-09 02:41:28", NULL);    /* Memory */
    set_detail(2, 13, 0x8086, 0x0602, 0x00000000, "1992-11-01 12:52:35", "1992-12-09 02:41:28", NULL);    /* Proc   */
    
    /* gorynich05 */
    set_detail(3, 14, 0x1002, 0x94c1, 0x10280d02, "1993-07-07 05:20:55", "1993-07-10 18:49:03", NULL);    /* Video  */
    set_detail(4, 15, 0x2646, 0x0000, 0x00000000, "1993-07-07 05:20:55", "1993-07-10 18:49:03", NULL);    /* Memory */
    set_detail(5, 16, 0x8086, 0x0602, 0x00000000, "1993-07-07 05:20:55", "1993-07-10 18:49:03", "1995-09-05 03:03:52");    /* Proc (X.X) */
    
    /* fire07 */
    set_detail(6, 17, 0x8086, 0x0602, 0x00000000, "1998-05-24 22:45:56", "1998-09-04 14:28:23", NULL);    /* Proc   */
    
    /* monster14 */
    set_detail(7, 18, 0x1002, 0x954F, 0x22711787, "2000-11-04 09:29:42", "2000-11-20 22:41:37", NULL);    /* Video  */
    set_detail(8, 19, 0x2646, 0x0000, 0x00000000, "2000-11-04 09:29:42", "2000-11-20 22:41:37", NULL);    /* Memory */
    set_detail(9, 20, 0x8086, 0x0602, 0x00000000, "2000-11-04 09:29:42", "2000-11-20 22:41:37", NULL);    /* Proc   */
    
    /* lib18 */
    set_detail(10, 21, 0x1002, 0x94c3, 0x1746e400, "2003-02-21 09:17:59", "2003-06-19 20:39:30", NULL);    /* Video  */
    
    /* new for monster14 (2003) */
    set_detail(11, 22, 0x2646, 0x0000, 0x00000000, "2003-02-21 09:17:59", "2003-06-19 20:39:30", NULL);    /* Memory */
    set_detail(12, 23, 0x8086, 0x0602, 0x00000000, "2003-02-21 09:17:59", "2003-06-19 20:39:30", NULL);    /* Proc   */
    
    /* ice01 base configuration */ /* cr_at | del_at | acc_at | allow_del_at | val_at */
    set_config(0, 24, 1, 11, "1992-11-01 12:52:35", NULL, "1992-12-09 02:41:28", NULL, NULL);
    set_config(1, 25, 1, 12, "1992-11-01 12:52:35", "2003-02-21 09:17:59", "1992-12-09 02:41:28", "2003-06-19 20:39:30", NULL);
    set_config(2, 26, 1, 13, "1992-11-01 12:52:35", "2003-02-21 09:17:59", "1992-12-09 02:41:28", "2003-06-19 20:39:30", NULL);
    /* gorynich05 base configuration */
    set_config(3, 30, 5, 14, "1993-07-07 05:20:55", "1995-09-05 03:03:52", "1993-07-10 18:49:03", "1995-09-07 12:52:01", NULL);
    set_config(4, 31, 5, 15, "1993-07-07 05:20:55", "1995-09-05 03:03:52", "1993-07-10 18:49:03", "1995-09-07 12:52:01", NULL);
    set_config(5, 32, 5, 16, "1993-07-07 05:20:55", "1995-09-05 03:03:52", "1993-07-10 18:49:03", "1995-09-07 12:52:01", NULL);    /* Proc dead (X.X) */
    /* fire07 base configuration */
    set_config(6, 40, 7, 17, "1998-05-24 22:45:56", NULL, "1998-09-04 14:28:23", NULL, NULL);
    /* monster14 base configuration */
    set_config(7, 44, 14, 18, "2000-11-04 09:29:42", NULL, "2000-11-20 22:41:37", NULL, NULL);
    set_config(8, 45, 14, 19, "2000-11-04 09:29:42", "2003-02-21 09:17:59", "2000-11-20 22:41:37", "2003-06-19 20:39:30", NULL);
    set_config(9, 46, 14, 20, "2000-11-04 09:29:42", "2003-02-21 09:17:59", "2000-11-20 22:41:37", "2003-06-19 20:39:30", NULL);
    /* lib18 base configuration */
    set_config(10, 53, 18, 21, "2003-02-21 09:17:59", NULL, "2003-06-19 20:39:30", NULL, NULL);
    
    /* store -> fire07 */
    set_config(11, 61, 7, 14, "1998-05-24 22:45:56", NULL, "1998-09-04 14:28:23", NULL, NULL);
    set_config(12, 62, 7, 21, "1998-05-24 22:45:56", NULL, "1998-09-04 14:28:23", NULL, NULL);
    /* ice01 -> lib18 */
    set_config(13, 68, 18, 12, "2003-02-21 09:17:59", NULL, "2003-06-19 20:39:30", NULL, NULL);
    set_config(14, 69, 18, 13, "2003-02-21 09:17:59", NULL, "2003-06-19 20:39:30", NULL, NULL);
    /* monster14 -> ice01 */
    set_config(15, 70, 1,  19, "2003-02-21 09:17:59", NULL, "2003-06-19 20:39:30", NULL, NULL);
    set_config(16, 71, 1,  20, "2003-02-21 09:17:59", NULL, "2003-06-19 20:39:30", NULL, NULL);
    /* store -> monster14 */
    set_config(17, 72, 14, 22, "2003-02-21 09:17:59", NULL, "2003-06-19 20:39:30", NULL, NULL);
    set_config(18, 73, 14, 23, "2003-02-21 09:17:59", NULL, "2003-06-19 20:39:30", NULL, NULL);
    
    rc = Cache_init("temp.bd", &myCache);
    printf("Init cache: %d\t- %s.\n", rc, error_std_text(rc));
    
    Cache_begin(&myCache);
    
    /* First start */
    for(i = 0; i < 5; i++)
        put_comp(&myCache, base_computers[i].id, base_computers[i].domain, base_computers[i].md5sum, base_computers[i].created_at);
    
    for(i = 0; i < 15; i++)
        put_log(&myCache, base_computers[i % 5].domain, gen_md5(md5_sum));
    
    for(i = 0; i < 13; i++)
        put_detail(&myCache, base_details[i].vendor_id, base_details[i].device_id, base_details[i].subsystem_id, gen_md5(md5_sum));
    
    Cache_commit(&myCache);
    
    /*put_config(&myCache, base_config[i].computer_id, base_config[i].detail_id, base_config[i].deleted_at);*/
    
    printf("now get all logs!\n");
    log_array = Cache_get_logs(&myCache, &arr_size, 0, NULL);
    for(i = 0; i < arr_size; i++)
        printf("<#LC_Log id:%.2d, domain:'%s', md5sum:'%s', created_at:%d>\n", log_array[i].id, log_array[i].domain, log_array[i].md5sum, (int) log_array[i].created_at);
    free(log_array);
    
    printf("now get all comps!\n");
    comp_array = Cache_get_comps(&myCache,&arr_size,0,NULL);
    
    Cache_begin(&myCache);
    for(i = 0; i < arr_size; i++){
        printf("<#LC_Computer id:%.2d, db_id:%.2d, domain:'%s', md5sum:'%s', created_at:%d>\n", comp_array[i].id, comp_array[i].db_id, comp_array[i].domain, comp_array[i].md5sum, (int) comp_array[i].created_at);
        for(j = 0; j < 19; j++){
            if(base_config[j].computer_id == comp_array[i].db_id){
                put_config(&myCache, comp_array[i].id, base_config[j].detail_id - 10, base_config[j].deleted_at);
            }
        }
    }
    Cache_commit(&myCache);
    free(comp_array);
    
    printf("now get all details!\n");
    detail_array = Cache_get_details(&myCache,&arr_size,0);
    for(i = 0; i < arr_size; i++)
        printf("<#LC_Detail id:%.2d, vendor_id:%.2d, device_id:%.2d, subsystem_id:%.2d, serial:'%s', created_at:%d>\n",
            detail_array[i].id,
            detail_array[i].vendor_id,
            detail_array[i].device_id,
            detail_array[i].subsystem_id,
            detail_array[i].serial,
            (int) detail_array[i].created_at);
    free(detail_array);
    
    printf("now get all configs!\n");
    config_arr = Cache_get_configs(&myCache,&arr_size,0,0);
    for(i = 0; i < arr_size; i++)
        printf("<#LC_Config id:%.2d, computer_id:%.2d, detail_id:%.2d, created_at:%d, deleted_at:%d>\n",
            config_arr[i].id,
            config_arr[i].computer_id,
            config_arr[i].detail_id,
            (int) config_arr[i].created_at,
            (int) config_arr[i].deleted_at);
    free(config_arr);
    
    printf("update comps...\n");
    
    Cache_upd_comp(&myCache,2,0,gen_md5(md5_sum));
    Cache_upd_comp(&myCache,0,14,gen_md5(md5_sum));
    
    comp_array = Cache_get_comps(&myCache,&arr_size,0,NULL);
    for(i = 0; i < arr_size; i++)
        printf("<#LC_Computer id:%.2d, db_id:%.2d, domain:'%s', md5sum:'%s', created_at:%d>\n", comp_array[i].id, comp_array[i].db_id, comp_array[i].domain, comp_array[i].md5sum, (int) comp_array[i].created_at);
    free(comp_array);
    /*
    Cache_del_logs(&myCache);
    Cache_del_details(&myCache);
    Cache_del_configs(&myCache);
    */
    printf("destroy...\n");
    Cache_destroy(&myCache);
    
    #endif
    
    return EXIT_SUCCESS;
}
