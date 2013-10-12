/* encoding: UTF-8 */

#include "strxtoi.h"

static int ctoi(const char chr) {
	if(chr >= '0' && chr <= '9') return (int) (chr - '0');
	if(chr >= 'A' && chr <= 'F') return (int) (chr - ('A' - 10));
	if(chr >= 'a' && chr <= 'f') return (int) (chr - ('a' - 10));
	return -1;
}

uint32_t strxtoi(const char *str, size_t length){
	int ret_val = 0;
	int pow_16  = 1;
	for(int i = length - 1; i > -1; i--){
		int digit = ctoi(str[i]);
		if (digit >= 0){
			ret_val += pow_16 * digit;
			pow_16  *= 16;
		}
	}
	return ret_val;
}
