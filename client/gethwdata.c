/* encoding: UTF-8 */

/*
 * gethwdata.c
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

#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../client_server.h"
#include "../strxtoi.h"

//~ #define CMD_DMIDECODE   "dmidecode -t 1 | egrep \"(Manufacturer|Product Name|Serial Number)\""
//~ #define CMD_DMIDECODE   "dmidecode -t 1"
#define CMD_BIOSDECODE  "biosdecode | grep slot"
//~ #define CMD_BIOSDECODE   "cat ~kia84/Device_Control_System/client/fake/biosdecode | grep slot"
#define CMD_LSPCI        "env -i lspci -n -vvv"
//~ #define CMD_LSPCI        "cat ~kia84/Device_Control_System/client/fake/lspci"
#define CMD_DMIDECODE(T) "dmidecode -t "T""
//~ #define CMD_DMIDECODE(T) "cat ~kia84/Device_Control_System/client/fake/dmidecode_t_"T""

#define check_to_fill(BUF) ((BUF) != NULL && \
                            strstr((BUF), "To Be Filled By O.E.M.") == NULL && \
                            strstr((BUF), "No Module") == NULL && \
                            strstr((BUF), "Unknown") == NULL && \
                            strstr((BUF), "Empty") == NULL)

#define GETHW_DEFAULT_EL_COUNT  32
#define GETHW_BUF_SIZE          1024
#define GETHW_PARAMS_CHUNK_SIZE 32
/*
	typedef struct {
		unsigned int id;
		unsigned int vendor_id;
		unsigned int device_id;
		unsigned int subsystem_id;
		unsigned int class_code;
		unsigned int revision;
		char         bus_addr[6];
		char         serial[DETAIL_SERIAL_SIZE + 1];
        size_t       params_length;
        char        *params;
	} CL_Detail;
*/

static int bus_cmp(const void *m1, const void *m2){
	return strncmp(((CL_Detail *) m1)->bus_addr, ((CL_Detail *) m2)->bus_addr, 5);
}

static int bus_str_cmp(const void *key, const void *m2){
	return strncmp((char *) key, (char *) ((CL_Detail *) m2)->bus_addr, 5);
}

static int final_cmp(const void *m1, const void *m2){
	CL_Detail *d1 = (CL_Detail *) m1;
	CL_Detail *d2 = (CL_Detail *) m2;
    int rc = 0;
    rc = (d1->vendor_id - d2->vendor_id);
    if(rc) return rc;
    rc = (d1->device_id - d2->device_id);
    if(rc) return rc;
    rc = (d1->subsystem_id - d2->subsystem_id);
    if(rc) return rc;
    rc = (d1->class_code - d2->class_code);
    if(rc) return rc;
    rc = (d1->revision - d2->revision);
    if(rc) return rc;
    rc = strncmp(d1->bus_addr, d2->bus_addr, DETAIL_SERIAL_SIZE);
    if(rc) return rc;
    rc = strcmp(d1->params, d2->params);
    if(rc) return rc;
    return strncmp(d1->bus_addr, d2->bus_addr, 5);
}

// static void *expand_array(size_t new_count){}

static void check_and_alloc_params(CL_Detail *d, size_t size){
    if(d->params != NULL){
        int fact_size = strlen(d->params);
        if(d->params_length - fact_size < size){
            d->params_length += (GETHW_PARAMS_CHUNK_SIZE > size ? GETHW_PARAMS_CHUNK_SIZE : size);
            d->params = realloc(d->params, d->params_length);
        }
    }
    else{
        d->params_length = GETHW_PARAMS_CHUNK_SIZE;
        d->params = calloc(sizeof(char), d->params_length);
    }
}

CL_Detail *gethwdata(size_t *arr_size){
    FILE      *fd;
	CL_Detail *details      = NULL;
	size_t     details_size = GETHW_DEFAULT_EL_COUNT;
	unsigned int last_detail_index = 0;
    char       buf[GETHW_BUF_SIZE];
	
	details = (CL_Detail *) calloc(details_size, sizeof(CL_Detail));
	
	// get all non 'onboard' PCI-devices
    fd = popen(CMD_BIOSDECODE, "r");
    while(fgets(buf, GETHW_BUF_SIZE, fd) != NULL){
        char *id_start = strchr(buf, 'I');
		if(id_start){
			if(last_detail_index >= details_size){
				details_size += GETHW_DEFAULT_EL_COUNT;
				details = (CL_Detail *)
							realloc(details, sizeof(CL_Detail) * details_size);
			}
			id_start += 3;
			strncpy((char *) details[last_detail_index].bus_addr, id_start, 5);
			details[last_detail_index].bus_addr[5] = '\0';
			last_detail_index++;
		}
    }
    pclose(fd);
	
	// last_detail_index == count of elements
	*arr_size = last_detail_index;
	qsort(details, *arr_size, sizeof(CL_Detail), bus_cmp);
	
	// get info about it
	int finded = 0;
	CL_Detail *res;
    
	fd = popen(CMD_LSPCI, "r");
	while(fgets(buf, GETHW_BUF_SIZE, fd) != NULL){
		if(buf[0] == '\n'){
			finded = 0;
			continue;
		}
		if(finded == 0){
			res = bsearch(buf, details, *arr_size,
						sizeof(CL_Detail), bus_str_cmp);
			if(!res) continue;
			finded = 1;
			char *pr_if = strstr(buf, "prog-if");
            if(pr_if){
                pr_if  += 8;
                buf[12] = *pr_if++;
                buf[13] = *pr_if;
            }
            else{
                // lspci don't have info about prog. IF
                // so, we fill it default values 'FF'
                buf[12] = 'f';
                buf[13] = 'f';
            }
			res->class_code = (unsigned int) strxtoi(buf +  8, 6);
			res->vendor_id  = (unsigned int) strxtoi(buf + 14, 4);
			res->device_id  = (unsigned int) strxtoi(buf + 19, 4);
			res->revision   = (unsigned int) strxtoi(buf + 29, 2);
			continue;
		}
		char *text_start = buf;
		while(*text_start == '\t') text_start++;
		if(strncmp(text_start, "Subsystem", 9) == 0){
			res->subsystem_id = (unsigned int) strxtoi(text_start + 11, 9);
			continue;
		}
		text_start = strstr(text_start, "Device Serial Number");
		if(text_start){
			strncpy(res->serial, text_start + 21, DETAIL_SERIAL_SIZE + 1);
			// strncpy don't return chars count:
			int size = strlen(res->serial);
			if(res->serial[size - 1] == '\n')
				res->serial[size - 1] = '\0';
		}
	}
	pclose(fd);
	
    if(last_detail_index >= details_size){
        details_size += GETHW_DEFAULT_EL_COUNT;
        details = (CL_Detail *)
                    realloc(details, sizeof(CL_Detail) * details_size);
    }
    res = details + last_detail_index;
    *arr_size = last_detail_index + 1;
	fd = popen(CMD_DMIDECODE("4"), "r"); // CPU
	while(fgets(buf, GETHW_BUF_SIZE, fd) != NULL){
		if(buf[0] == '#' || buf[0] == '\n') continue;
        char *tmp_p;
        int   sub_size;
        tmp_p = strstr(buf, "Manufacturer:");
        if(check_to_fill(tmp_p)){
            sub_size = strlen(tmp_p);
            if(tmp_p[sub_size - 1] == '\n'){
                do {sub_size--;} while(tmp_p[sub_size - 1] == ' ');
            }
            check_and_alloc_params(res, sub_size + 2);
            if(res->params[0] != '\0') strncat(res->params, ", ", 2);
            strncat(res->params, tmp_p, sub_size);
        }
        tmp_p = strstr(buf, "Version:");
        if(check_to_fill(tmp_p)){
            sub_size = strlen(tmp_p);
            if(tmp_p[sub_size - 1] == '\n'){
                do {sub_size--;} while(tmp_p[sub_size - 1] == ' ');
            }
            check_and_alloc_params(res, sub_size + 2);
            if(res->params[0] != '\0') strncat(res->params, ", ", 2);
            strncat(res->params, tmp_p, sub_size);
        }
        tmp_p = strstr(buf, "Max Speed:");
        if(check_to_fill(tmp_p)){
            tmp_p += 4;
            sub_size = strlen(tmp_p);
            if(tmp_p[sub_size - 1] == '\n'){
                do {sub_size--;} while(tmp_p[sub_size - 1] == ' ');
            }
            check_and_alloc_params(res, sub_size + 2);
            if(res->params[0] != '\0') strncat(res->params, ", ", 2);
            strncat(res->params, tmp_p, sub_size);
        }
        tmp_p = strstr(buf, "Serial Number:");
        if(check_to_fill(tmp_p)){
            tmp_p += 15;
            sub_size = strlen(tmp_p);
            if(tmp_p[sub_size - 1] == '\n'){
                do {sub_size--;} while(tmp_p[sub_size - 1] == ' ');
            }
            sub_size = (sub_size < DETAIL_SERIAL_SIZE ? sub_size : DETAIL_SERIAL_SIZE);
            strncpy(res->serial, tmp_p, sub_size);
        }
	}
	pclose(fd);
	
    //~ CL_Detail *cpu = res;
    
    last_detail_index++;
    if(last_detail_index >= details_size){
        details_size += GETHW_DEFAULT_EL_COUNT;
        details = (CL_Detail *)
                    realloc(details, sizeof(CL_Detail) * details_size);
    }
    //~ res = details + last_detail_index;
    *arr_size = last_detail_index + 1;
	fd = popen(CMD_DMIDECODE("17"), "r"); // MEM
    finded = -1;
	while(fgets(buf, GETHW_BUF_SIZE, fd) != NULL){
		if(buf[0] == '#' || buf[0] == '\n') continue;
        char *tmp_p;
        int   sub_size;
        
        tmp_p = strstr(buf, "Handle:");
        if(tmp_p){
            finded = 0;
            continue;
        }
        
		if(finded == 0){
			res = details + last_detail_index;
			finded = 1;
			continue;
		}
        if(finded > 0){
            tmp_p = strstr(buf, "Size:");
            if(check_to_fill(tmp_p)){
                sub_size = strlen(tmp_p);
                if(tmp_p[sub_size - 1] == '\n'){
                    do {sub_size--;} while(tmp_p[sub_size - 1] == ' ');
                }
                check_and_alloc_params(res, sub_size + 2);
                //~ if(res->params[0] != '\0') strncat(res->params, ", ", 2);
                strncpy(res->params, tmp_p, sub_size);
                res->params[sub_size] = '\0';
                last_detail_index++;
            }
            tmp_p = strstr(buf, "Type:");
            if(check_to_fill(tmp_p)){
                sub_size = strlen(tmp_p);
                if(tmp_p[sub_size - 1] == '\n'){
                    do {sub_size--;} while(tmp_p[sub_size - 1] == ' ');
                }
                check_and_alloc_params(res, sub_size + 2);
                if(res->params[0] != '\0') strncat(res->params, ", ", 2);
                strncat(res->params, tmp_p, sub_size);
            }
            tmp_p = strstr(buf, "Speed:");
            if(check_to_fill(tmp_p)){
                sub_size = strlen(tmp_p);
                if(tmp_p[sub_size - 1] == '\n'){
                    do {sub_size--;} while(tmp_p[sub_size - 1] == ' ');
                }
                check_and_alloc_params(res, sub_size + 2);
                if(res->params[0] != '\0') strncat(res->params, ", ", 2);
                strncat(res->params, tmp_p, sub_size);
            }
            tmp_p = strstr(buf, "Manufacturer:");
            if(check_to_fill(tmp_p)){
                sub_size = strlen(tmp_p);
                if(tmp_p[sub_size - 1] == '\n'){
                    do {sub_size--;} while(tmp_p[sub_size - 1] == ' ');
                }
                check_and_alloc_params(res, sub_size + 2);
                if(res->params[0] != '\0') strncat(res->params, ", ", 2);
                strncat(res->params, tmp_p, sub_size);
            }
            tmp_p = strstr(buf, "Part Number:");
            if(check_to_fill(tmp_p)){
                sub_size = strlen(tmp_p);
                if(tmp_p[sub_size - 1] == '\n'){
                    do {sub_size--;} while(tmp_p[sub_size - 1] == ' ');
                }
                check_and_alloc_params(res, sub_size + 2);
                if(res->params[0] != '\0') strncat(res->params, ", ", 2);
                strncat(res->params, tmp_p, sub_size);
            }
            tmp_p = strstr(buf, "Serial Number:");
            if(check_to_fill(tmp_p)){
                tmp_p += 15;
                sub_size = strlen(tmp_p);
                if(tmp_p[sub_size - 1] == '\n'){
                    do {sub_size--;} while(tmp_p[sub_size - 1] == ' ');
                }
                sub_size = (sub_size < DETAIL_SERIAL_SIZE ? sub_size : DETAIL_SERIAL_SIZE);
                strncpy(res->serial, tmp_p, sub_size);
            }
        }
	}
	pclose(fd);
    
	details = (CL_Detail *) realloc(details, sizeof(CL_Detail) * (*arr_size));
    
    for(int i = 0; i < (*arr_size); i++){
        if(details[i].params != NULL){
            details[i].params_length = strlen(details[i].params);
            details[i].params = realloc(details[i].params, details[i].params_length);
            if(details[i].params_length == 0) details[i].params = NULL;
        }
        details[i].serial_length = strlen(details[i].serial);
    }
    
    qsort(details, *arr_size, sizeof(CL_Detail), final_cmp);
    
    return details;
}
#if 0
int main(int argc, char *argv[]){
	CL_Detail *tmp;
	size_t     tmp_size;
	tmp = gethwdata(&tmp_size);
	
	printf("Size = %d\n", tmp_size);
    printf("-------------\n");
	for(int i = 0; i < tmp_size; i++){
		printf("ID's:\t%.4x:%.4x:%.8x\n",
			tmp[i].vendor_id,
			tmp[i].device_id,
			tmp[i].subsystem_id);
		printf("Class:\t%.6x (rev: %.2x)\n",
			tmp[i].class_code, tmp[i].revision);
		printf("Bus:\t%s\nSerial:\t'%s'\n",
			tmp[i].bus_addr, tmp[i].serial);
        printf("Params[%d]:\t%s\n", tmp[i].params_length,
			tmp[i].params);
        printf("-------------\n");
	}
	
	free(tmp);
	return EXIT_SUCCESS;
}
#endif
