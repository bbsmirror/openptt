/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "config.h"
#include "pttstruct.h"
#include "common.h"
#include "modes.h"
#include "perm.h"
#include "proto.h"

extern struct utmpfile_t *utmpshm;
extern int usernum;

extern userinfo_t *currutmp;
extern char *ModeTypeTable[];

extern userec_t cuser;
extern userec_t xuser;

extern char trans_buffer[];
extern char save_title[];

static int inmailbox(int m) {
    passwd_query(usernum, &xuser);
    cuser.exmailbox = xuser.exmailbox + m;
    passwd_update(usernum, &cuser);
    return cuser.exmailbox;
}

extern int b_lines;

int p_touch_boards() {
    touch_boards();
    move(b_lines - 1, 0);
    outs("BCACHE 更新完成\n");
    pressanykey();
    return 0;
}

int p_sysinfo() {
    char buf[100];
    long int total,used;
    float p;
    
    move(b_lines-1,0);
    clrtoeol();
    cpuload(buf);
    outs("CPU 負荷 : ");
    outs(buf);
    
    p = swapused(&total,&used);
    sprintf(buf, " 虛擬記憶體使用率: %.3f  (全部:%ldMB 用掉:%ldMB)\n",
	    p, total >> 20, used >> 20);
    outs(buf);
    pressanykey();
    return 0;
}

/* 小計算機 */
static void ccount(float *a, float b, int cmode) {
    switch(cmode) {
    case 0:
    case 1:
    case 2:
        *a += b;
        break;
    case 3:
        *a -= b;
        break;
    case 4:
        *a *= b;
        break;
    case 5:
        *a /= b;
        break;
    }
}

int cal() {
    float a = 0;
    char flo = 0, ch = 0;
    char mode[6] = {' ','=','+','-','*','/'} , cmode = 0;
    char buf[100] = "[            0] [ ] ", b[20] = "0";
    
    move(b_lines - 1, 0);
    clrtoeol();
    outs(buf);
    move(b_lines, 0);
    clrtoeol();
    outs("\033[44m 小計算機  \033[31;47m      (0123456789+-*/=) "
	 "\033[30m輸入     \033[31m  "
	 "(Q)\033[30m 離開                   \033[m");
    while(1) {
	ch = igetch();
	switch(ch) {
	case '\r':
            ch = '=';
	case '=':
	case '+':
	case '-':
	case '*':
	case '/':
            ccount(&a, atof(b), cmode);
            flo = 0;
            b[0] = '0';
            b[1] = 0;
            move(b_lines - 1, 0);
            sprintf(buf, "[%13.2f] [%c] ", a, ch);
            outs(buf);
            break;
	case '.':
            if(!flo)
		flo = 1;
            else
		break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '0':
	    if(strlen(b) > 13)
		break;
	    if(flo || b[0] != '0')
		sprintf(b,"%s%c",b,ch);
	    else
		b[0]=ch;
	    move(b_lines - 1, 0);
	    sprintf(buf, "[%13s] [%c]", b, mode[(int)cmode]);
	    outs(buf);
	    break;
	case 'q':
	    return 0;
	}
	
	switch(ch) {
	case '=':
	    a = 0;
	    cmode = 0;
	    break;
	case '+':
	    cmode = 2;
	    break;
	case '-':
	    cmode = 3;
	    break;
	case '*':
	    cmode = 4;
	    break;
	case '/':
	    cmode = 5;
	    break;
	}
    }
}
