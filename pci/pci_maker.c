/* encoding: UTF-8 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "strxtoi.h"

#define CODE_SIZE       4
#define NAME_MAX_SIZE   128
#define MALLOC_START_VAL 16384
#define COMPARE_NUM	    5

#define TYPE_VENDOR     0
#define TYPE_DEVICE     1
#define TYPE_SUBSYSTEM  2

#define DEFAULT_PATH		"/usr/share/hwdata/pci.ids"

typedef struct {
	int  code;          /* or subvendor */
	int  vendor_code;   /* or subdevice */
	int  device_id;     /* for sybsystem only */
	char name[NAME_MAX_SIZE];
} DEVICE_NAMESPACE;

DEVICE_NAMESPACE * subsystems;
int subsystems_size  = 0;
int last_subsystem_i = 0;

DEVICE_NAMESPACE * devices;
int devices_size  = 0;
int last_device_i = 0;
int last_vendor;

void print_name (const DEVICE_NAMESPACE * row){
	int i = 0;
	while(row->name[i] != '\0'){
		if(row->name[i] == '\'') printf("''");
		else printf("%c", row->name[i]);
		i++;
	}
}

void print_vendor (const DEVICE_NAMESPACE * row){
	printf("(%d,\t'", row->code);
	print_name(row);
	printf("')");
}

void print_device (const DEVICE_NAMESPACE * row, int id){
	printf("(%d,\t%d,\t%d,\t'", id + 1, row->code, row->vendor_code);
	print_name(row);
	printf("')");
}

void print_subsystem (const int i, const DEVICE_NAMESPACE * row){
	printf("(%d,\t%d,\t%d,\t%d,\t'", i, row->device_id, row->code, row->vendor_code);
	print_name(row);
	printf("')");
}

void check_and_realloc (DEVICE_NAMESPACE ** array, int * array_size, const int last_i){
	if(!*array){
		*array_size = MALLOC_START_VAL;
		*array = (DEVICE_NAMESPACE *) malloc(*array_size * sizeof(DEVICE_NAMESPACE));
	}
	else if(*array_size - last_i <= COMPARE_NUM){
		*array_size <<= 1;
		*array = (DEVICE_NAMESPACE *) realloc(*array, *array_size * sizeof(DEVICE_NAMESPACE));
	}
}

void shift_and_trim (char ** str){
	*str += CODE_SIZE;
	while(**str == ' ') ++*str;
}

void fill_in_row (
		char * str,
		int type,
		DEVICE_NAMESPACE * row,
		DEVICE_NAMESPACE ** array,
		int * array_size,
		int last_i
){
	char * buf_pointer;
	DEVICE_NAMESPACE * local_row;
	
	buf_pointer = str + type;
	
	if(type){
		check_and_realloc(array, array_size, last_i);
		local_row = *array + last_i;
	}
	else{
		local_row = row;
	}
	
	local_row->code = strxtoi(buf_pointer, CODE_SIZE);
	shift_and_trim(&buf_pointer);
	
	if(type == TYPE_DEVICE){
		local_row->vendor_code = last_vendor;
	}
	else if(type == TYPE_SUBSYSTEM){
		local_row->vendor_code = strxtoi(buf_pointer, CODE_SIZE);
		local_row->device_id   = last_device_i;
		shift_and_trim(&buf_pointer);
	}
	sprintf(local_row->name, "%s", buf_pointer);
}

int split_row (char * buf, DEVICE_NAMESPACE * row){
	
	row->code = 0;
	bzero(row->name, NAME_MAX_SIZE);
	if(buf[0] == '\t'){
		if(buf[1] == '\t'){	/* subsystem */
			fill_in_row(buf, TYPE_SUBSYSTEM, NULL, &subsystems, &subsystems_size, last_subsystem_i);
			last_subsystem_i++;
		}
		else{			/* device */
			fill_in_row(buf, TYPE_DEVICE, NULL, &devices, &devices_size, last_device_i);
			last_device_i++;
		}
	}
	else{				/* vendor */
		fill_in_row(buf, TYPE_VENDOR, row, NULL, NULL, 0);
		last_vendor = row->code;
		return 1;
	}
	return 0;
}

int main (int argc, char ** argv) {
	
	FILE * pci_fd;
	DEVICE_NAMESPACE row;
	char buf[256];
	char c;
	int ignore_mod = 0;
	int i = 0;
	int first = 1;
	
	if (argc < 2){
		/*printf("Usage: pci_maker pci.ids\n");
		exit(1);*/
		pci_fd = fopen(DEFAULT_PATH, "r");
	}
	else{
		pci_fd = fopen(argv[1], "r");
	}
	if(!pci_fd){
		perror(NULL);
		exit(EXIT_FAILURE);
	}
	
	bzero(buf, 256);
	
	printf("BEGIN;\n\n");
	
	printf("DROP   TABLE IF     EXISTS subsystems;\n");
	printf("DROP   TABLE IF     EXISTS devices;\n");
	printf("DROP   TABLE IF     EXISTS vendors;\n\n");
	
	printf("CREATE TABLE IF NOT EXISTS vendors(\n"
			"\tid integer PRIMARY KEY,\t-- Hex digit in integer format (0x0e11 -> 3601)\n"
			"\tname varchar(%d) NOT NULL CHECK (trim(both from name) NOT LIKE '')\n"
		");\n", NAME_MAX_SIZE);
	printf("CREATE TABLE IF NOT EXISTS devices(\n"
			"\tid integer PRIMARY KEY,\t-- like a 'serial'\n"
			"\tvendor_id integer REFERENCES vendors(id),\n"
			"\tdevice_id integer,\t\t-- Hex digit in integer format (0x8801 -> 34817)\n"
			"\tname varchar(%d) NOT NULL CHECK (trim(both from name) NOT LIKE '')\n"
		");\n", NAME_MAX_SIZE);
	
	printf("CREATE TABLE IF NOT EXISTS subsystems(\n"
			"\tid integer PRIMARY KEY,\n"
			"\tdevice_id integer REFERENCES devices(id),\n"
			"\tsubvendor_id integer,\t-- Hex digit in integer format (0x0e11 -> 3601)\n"
			"\tsubdevice_id integer,\t-- Hex digit in integer format (0x4091 -> 16529)\n"
			"\tname varchar(%d) NOT NULL CHECK (trim(both from name) NOT LIKE '')\n"
		");\n\n", NAME_MAX_SIZE);
	
	printf("INSERT INTO vendors(id, name)\n\tVALUES");
	
	c = (char) fgetc(pci_fd);
	while(c != EOF){
		if(i == 0){
			if(c == '#') ignore_mod = 1;
			else if(!ignore_mod && c == 'C') break;
		}
		
		if(c == '\n'){
			ignore_mod = 0;
			if(buf[0]){
				split_row(buf, &row);
				if(row.name[0] >= '!' && row.name[0] <= '~'){
					printf(first ? "\t" : ",\n\t\t");
					print_vendor(&row);
					first = 0;
				}
			}
			bzero(buf, 256);
			i = 0;
		}
		else if(!ignore_mod){
			if(i >= 255){
				buf[255] = '\0';
				i = 0;
			}
			buf[i] = c;
			i++;
		}
		
		c = (char) fgetc(pci_fd);
	}
	
	printf(";\n\n");
	printf("INSERT INTO devices(id, device_id, vendor_id, name)\n\tVALUES");
	
	i = 0;
	first = 1;
	while(i < last_device_i){
		printf(first ? "\t" : ",\n\t\t");
		print_device(devices + i, i);
        i++;
		first = 0;
	}
	free(devices);
	printf(";\n\n");
	printf("INSERT INTO subsystems(id, device_id, subvendor_id, subdevice_id, name)\n\tVALUES");
	
	i = 0;
	first = 1;
	while(i < last_subsystem_i){
		printf(first ? "\t" : ",\n\t\t");
		print_subsystem(i + 1, subsystems + i);
		i++;
		first = 0;
	}
	free(subsystems);
	printf(";\n\n");
	
	printf("COMMIT;\n");
	
	fclose(pci_fd);
	
	return EXIT_SUCCESS;
}
