#ifndef _MEM_HELPER_H_
#define _MEM_HELPER_H_

#include "base.h"
#include <stdlib.h>

// allocates memory for given size and set ptr point to that location
// it provides more safe heap allocation
// pass the size of variable and pointer of pointer so you can get value
extern _INLINE_ int memhelper_get(unsigned long size, void **ptr){
	*ptr = malloc(size);
	if(! *ptr){
		return -1;
	}
	return 0;
}

// free heap memory and assign NULL value to ptr
extern _INLINE_ int memhelper_free(void **ptr){
	free(*ptr);
	*ptr = 0;
	return 0;
}

// switch endian format for 16 bit data type
// little endian <-> big endian
extern _INLINE_ unsigned short swap16(unsigned short num){
	return (short)((num >> 8) | (num << 8));
} 

// switch endian format for 32 bit
// little endian <-> big endian
extern _INLINE_ unsigned int swap32(unsigned int num){
	return ((num >> 24 & 0xFF) |
		((num << 8) & 0xFF0000) |
		((num >> 8) & 0xFF00) |
		((num << 24) & 0xFF000000));
}

#endif

