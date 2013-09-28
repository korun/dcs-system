/* encoding: UTF-8 */

/*
 * db_sync_test.c
 * 
 * Copyright 2013 Ivan Korunkov <kia84@mail.msiu.ru>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

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

#include "db_sync.h"

static char *gen_md5(char *str){
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

static char *gen_sn(char *str){
    int i, j;
    str[0] = 'S';
    str[1] = 'N';
    str[2] = '-';
    for(i = 3; i < 12; i++){
        j = rand();
        while(j > 'Z' - '/') j /= 10;
        j += '/';
        if(j > '9' && j < 'A') j += j - '/';
        str[i] = (char) j;
    }
    str[i] = '\0';
    return str;
}

int main(void){
    int i, arr_size = 2;
	Database db;
	LC_Log    logs[arr_size];
	LC_Config confs[arr_size];
    LC_Detail detls[arr_size];
    char md5_sum[33];
    int cids[] = {1, 7, 14, 18};
    int dids[] = {11, 12, 13, 14, 15, 17, 18, 19, 20, 21, 22, 23};
    
    srand(time(NULL));
    
    DB_init(&db, "dbname=device_control_database", 1);
    
    for(i = 0; i < arr_size; i++){
        gethostname(logs[i].domain, COMPUTER_DOMAIN_NAME_SIZE);
        strncpy(logs[i].md5sum, gen_md5(md5_sum), COMPUTER_MD5_SUM_SIZE + 1);
        logs[i].created_at = time(NULL) + i;
        
        confs[i].computer_id = cids[rand() % 4];
        confs[i].detail_id   = dids[rand() % 12];
        confs[i].created_at  = time(NULL);
        confs[i].deleted_at  = -1;
        //~ printf("Config #%2d = %3d %3d %12d %12d\n", i,
            //~ confs[i].computer_id,
            //~ confs[i].detail_id,
            //~ confs[i].created_at,
            //~ confs[i].deleted_at
        //~ );
        
        detls[i].vendor_id = 0x10de;
        detls[i].device_id = 0x0141;
        detls[i].subsystem_id = 0x14583124;
        strncpy(detls[i].serial, gen_sn(md5_sum), DETAIL_SERIAL_SIZE + 1);
        detls[i].created_at = time(NULL);
    }
    
    DB_put_logs(&db, logs, arr_size);
    if(db.err_msg) fprintf(stderr, db.err_msg);
    
    DB_put_configs(&db, confs, arr_size);
    if(db.err_msg) fprintf(stderr, db.err_msg);
    
    free(DB_get_comps(&db));
    if(db.err_msg) fprintf(stderr, db.err_msg);
    
    free(DB_get_details(&db, 1));
    if(db.err_msg) fprintf(stderr, db.err_msg);
    
    LC_Computer comp;
    gethostname(comp.domain, COMPUTER_DOMAIN_NAME_SIZE);
    comp.created_at = time(NULL);
    DB_put_comps(&db, &comp, 1);
    if(db.err_msg) fprintf(stderr, db.err_msg);
    
    DB_put_details(&db, detls, arr_size);
    if(db.err_msg) fprintf(stderr, db.err_msg);
    
    DB_destroy(&db);
    
	return EXIT_SUCCESS;
}
