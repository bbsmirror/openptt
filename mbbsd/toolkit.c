/* $Id$ */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include "config.h"
#include "pttstruct.h"
#include "proto.h"

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

int IsSNum(char *a) {
    int i;
    
    for(i = 0; a[i]; i++)
	if(a[i] > '9' || a[i] < '0')
	    return 0;
    return 1;
}

int show_file(char *filename, int y, int lines, int mode) {
    FILE *fp;
    char buf[512];
    
    if(y >= 0)
	move(y,0);
    clrtoline(lines + y);
    if((fp=fopen(filename,"r"))) {
	while(fgets(buf,sizeof(buf),fp) && lines--)
	    outs(Ptt_prints(buf,mode));
	fclose(fp);
    } else
	return 0;
    return 1;
}
