/* $Id$ */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "config.h"
#include "pttstruct.h"
#include "modes.h"
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

int strip_ansi(char *buf, char *str, int mode) {
    register int ansi, count = 0;

    for(ansi = 0; *str /*&& *str != '\n' */; str++) {
	if(*str == 27) {
	    if(mode) {
		if(buf)
		    *buf++ = *str;
		count++;
	    }
	    ansi = 1;
	} else if(ansi && strchr("[;1234567890mfHABCDnsuJKc=n", *str)) {
	    if((mode == NO_RELOAD && !strchr("c=n", *str)) ||
	       (mode == ONLY_COLOR && strchr("[;1234567890m", *str))) {
		if(buf)
		    *buf++ = *str;
		count++;
	    }
	    if(strchr("mHn ", *str))
		ansi = 0;
	} else {
	    ansi =0;
	    if(buf)
		*buf++ = *str;
	    count++;
	}
    }
    if(buf)
	*buf = '\0';
    return count;
}

void b_suckinfile(FILE *fp, char *fname) {
    FILE *sfp;

    if((sfp = fopen(fname, "r"))) {
	char inbuf[256];

	while(fgets(inbuf, sizeof(inbuf), sfp))
	    fputs(inbuf, fp);
	fclose(sfp);
    }
}

int show_file(char *filename, int y, int lines, int mode) {
    FILE *fp;
    char buf[256];
    
    if(y >= 0)
	move(y,0);
    clrtoline(lines + y);
    if((fp=fopen(filename,"r"))) {
	while(fgets(buf,256,fp) && lines--)
	    outs(Ptt_prints(buf,mode));
	fclose(fp);
    } else
	return 0;
    return 1;
}

#ifdef HAVE_SETPROCTITLE

void initsetproctitle(int argc, char **argv, char **envp) {
}

#else

char **Argv = NULL;          /* pointer to argument vector */
char *LastArgv = NULL;       /* end of argv */
extern char **environ;

void initsetproctitle(int argc, char **argv, char **envp) {
    register int i;
    
    /* Move the environment so setproctitle can use the space at
       the top of memory. */
    for(i = 0; envp[i]; i++);
    environ = malloc(sizeof(char *) * (i + 1));
    for(i = 0; envp[i]; i++)
	environ[i] = strdup(envp[i]);
    environ[i] = NULL;
    
    /* Save start and extent of argv for setproctitle. */
    Argv = argv;
    if(i > 0)
	LastArgv = envp[i - 1] + strlen(envp[i - 1]);
    else
	LastArgv = argv[argc - 1] + strlen(argv[argc - 1]);
}

static void do_setproctitle(const char *cmdline) {
    char buf[256], *p;
    int i;
    
    strncpy(buf, cmdline, 256);
    buf[255] = '\0';
    i = strlen(buf);
    if(i > LastArgv - Argv[0] - 2) {
	i = LastArgv - Argv[0] - 2;
    }
    strcpy(Argv[0], buf);
    p = &Argv[0][i];
    while(p < LastArgv)
	*p++='\0';
    Argv[1] = NULL;
}

void setproctitle(const char* format, ...) {
    char buf[256];
    
    va_list args;
    va_start(args, format);
    vsprintf(buf, format,args);
    do_setproctitle(buf);
    va_end(args);
}
#endif
