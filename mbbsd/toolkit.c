/* $Id$ */
#include <ctype.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "config.h"
#include "pttstruct.h"

unsigned StringHash(unsigned char *s) {
    unsigned int v=0;
    while(*s) {
	v = (v << 8) | (v >> 24);
	v ^= toupper(*s++);	/* note this is case insensitive */
    }
    return (v * 2654435769UL) >> (32 - HASH_BITS);
}

int IsNum(char *a, int n) {
    int i;

    for(i = 0; i < n; i++)
	if (a[i] > '9' || a[i] < '0' )
	    return 0;
    return 1;
}

void mprotect_utmp(int lock) {
    static int count = 0;
    extern struct utmpfile_t *utmpshm;
    
    if(lock) {
	count++;
	if(count == 1)
	    mprotect(utmpshm, sizeof(*utmpshm), PROT_READ);
    } else {
	count--;
	if(count == 0)
	    mprotect(utmpshm, sizeof(*utmpshm), PROT_READ | PROT_WRITE);
    }
}
