/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include "config.h"
#include "pttstruct.h"
#include "common.h"
#include "modes.h"
#include "perm.h"
#include "proto.h"

extern struct utmpfile_t *utmpshm;
extern int usernum;

/* 防堵 Multi play */
static int count_multiplay(int unmode) {
    register int i, j;
    register userinfo_t *uentp;
    extern struct utmpfile_t *utmpshm;

    for(i = j = 0; i < USHM_SIZE; i++) {
	uentp = &(utmpshm->uinfo[i]);
	if(uentp->uid == usernum)
	    if(uentp->lockmode == unmode)
		j++;
    }
    return j;
}

extern userinfo_t *currutmp;
extern char *ModeTypeTable[];

int lockutmpmode(int unmode, int state) {
    int errorno = 0;
    
    if(currutmp->lockmode)
	errorno = 1;
    else if(count_multiplay(unmode))
	errorno = 2;
    
    if(errorno && !(state == LOCK_THIS && errorno == LOCK_MULTI)) {
	clear();
	move(10,20);
	if(errorno == 1)
	    prints("請先離開 %s 才能再 %s ",
		   ModeTypeTable[currutmp->lockmode],
		   ModeTypeTable[unmode]);
	else 
	    prints("抱歉! 您已有其他線相同的ID正在%s",
		   ModeTypeTable[unmode]);
	pressanykey();
	return errorno;
    }
    
    setutmpmode(unmode);
    currutmp->lockmode = unmode;
    return 0;
}

int unlockutmpmode() {
    currutmp->lockmode = 0;
    return 0;
}

extern userec_t cuser;
extern userec_t xuser;

#define lockreturn(unmode, state) if(lockutmpmode(unmode, state)) return 
#define lockreturn0(unmode, state) if(lockutmpmode(unmode, state)) return 0
#define lockbreak(unmode, state) if(lockutmpmode(unmode, state)) break
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
    
    move(b_lines-1,0);
    clrtoeol();
    cpuload(buf);
    outs("CPU 負荷 : ");
    outs(buf);
    outs("\n");
    
    pressanykey();
    return 0;
}
