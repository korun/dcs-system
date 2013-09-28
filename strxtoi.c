/* encoding: UTF-8 */

#include <stdio.h>
#include <stdlib.h>

static int ctoi(const char chr) {
	if(chr >= '0' && chr <= '9') return (int) (chr - '0');
	if(chr >= 'A' && chr <= 'F') return (int) (chr - ('A' - 10));
	if(chr >= 'a' && chr <= 'f') return (int) (chr - ('a' - 10));
	return -1;
}

int strxtoi(const char *str, size_t length){
	int i, digit, pow_16;
	int ret_val = 0;
	pow_16 = 1;
	for(i = length - 1; i > -1; i--){
		digit = ctoi(str[i]);
		if (digit >= 0){
			ret_val += pow_16 * digit;
			pow_16  *= 16;
		}
	}
	return ret_val;
}
