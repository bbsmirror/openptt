/* $Id$ */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include "config.h"
#include "pttstruct.h"
#include "proto.h"

extern struct utmpfile_t *utmpshm;
extern userec_t cuser;

char *Ptt_prints(char *str, int mode) {
    char *p,*q,strbuf[1024];
    p=str; q=strbuf;
    while(*p) {
        if(q-strbuf>512) {
            break; /* buffer overflow */
        }
        if(*p!='\033') {
            *q++=*p++;
        } else {
            if(*(p+1)!='*') {
                *q++=*p++;
                *q++=*p++;
            } else {
                p+=3;
                switch(*(p-1)) {
                    case 's':
                        strcpy(q,cuser.userid);
                        break;
                    case 't': {
                        time_t now=time(0);
                        strcpy(q,Cdate(&now));
                        }
                        break;
                    case 'u':
                        sprintf(q,"%d",utmpshm->number);
                        break;
                    case 'b':
                        sprintf(q,"%d/%d",cuser.month,cuser.day);
                        break;
                    case 'l':
                        sprintf(q,"%d",cuser.numlogins);
                        break;
                    case 'p':
                        sprintf(q,"%d",cuser.numposts);
                        break;
                    case 'n':
                        sprintf(q,"%s",cuser.username);
                        break;
                    case 'm':
                        sprintf(q,"%ld",cuser.money);
                        break;
                    default:
                        *q++='\033';
                        *q++='*';
                        *q=0;
                        p--;
                }
                while(*q) q++;
            }
        }
    }
    *q=0;
    strbuf[511]=0; /* truncate string length */
    strip_ansi(str, strbuf ,mode);
    return str;
}

int Rename(char* src, char* dst) {
    if(rename(src, dst) == 0)
	return 0;    
    return -1;
}

int Link(char* src, char* dst) {
    char cmd[200];
    
    if(strcmp(src, BBSHOME "/home") == 0)
    	return 1;
    if(link(src, dst) == 0)
	return 0;
    
    sprintf(cmd, "/bin/cp -R %s %s", src, dst);
    return system(cmd);
}

char *my_ctime(const time_t *t) {
    struct tm *tp;
    static char ans[100];

    tp = localtime(t);
    sprintf(ans, "%02d/%02d/%02d %02d:%02d:%02d", (tp->tm_year % 100),
	    tp->tm_mon + 1,tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
    return ans;
}
