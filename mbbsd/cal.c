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

/* 使用錢的函數 */
int reload_money() {
    passwd_query(usernum, &xuser);
    cuser.money = xuser.money;
    return 0;
}

#define lockreturn(unmode, state) if(lockutmpmode(unmode, state)) return 
#define lockreturn0(unmode, state) if(lockutmpmode(unmode, state)) return 0
#define lockbreak(unmode, state) if(lockutmpmode(unmode, state)) break
extern char trans_buffer[];
extern char save_title[];

/* 使用錢的函數 */
int inumoney(char *tuser, int money) {
    int unum;
    
    if((unum = getuser(tuser))) {
	xuser.money += money;
	passwd_update(unum, &xuser);
	return xuser.money;
    } else
	return -1;
}

int inmoney(int money) {
    passwd_query(usernum, &xuser);
    cuser.money = xuser.money + money;
    passwd_update(usernum, &cuser);
    return cuser.money;
}

static int inmailbox(int m) {
    passwd_query(usernum, &xuser);
    cuser.exmailbox = xuser.exmailbox + m;
    passwd_update(usernum, &cuser);
    return cuser.exmailbox;
}

int deumoney(char *tuser, int money) {
    int unum;
    if((unum = getuser(tuser))) {
	if((unsigned long int)xuser.money <= (unsigned long int)money)
	    xuser.money=0;
	else
	    xuser.money -= money;
	passwd_update(unum, &xuser);
	return xuser.money;
    } else
	return -1;
}

int demoney(int money) {
    passwd_query(usernum, &xuser);
    if((unsigned long int)xuser.money <= (unsigned long int)money)
	cuser.money=0;
    else
	cuser.money = xuser.money - money;
    passwd_update(usernum, &cuser);
    return cuser.money;
}

extern int b_lines;

#if !HAVE_FREECLOAK
/* 花錢選單 */
int p_cloak() {
    char buf[4];
    getdata(b_lines-1, 0,
	    currutmp->invisible ? "確定要現身?[y/N]" : "確定要隱身?[y/N]",
	    buf, 3, LCECHO);
    if(buf[0] != 'y')
	return 0;
    if(cuser.money >= 19) {
	demoney(19);
	currutmp->invisible %= 2;
	outs((currutmp->invisible ^= 1) ? MSG_CLOAKED : MSG_UNCLOAK);
	refresh();
	safe_sleep(1);
    }
    return 0;
}
#endif

int p_from() {
    char ans[4];

    getdata(b_lines-2, 0, "確定要改故鄉?[y/N]", ans, 3, LCECHO);
    if(ans[0] != 'y')
	return 0;
    reload_money();
    if(cuser.money < 49)
	return 0;
    if(getdata_buf(b_lines-1, 0, "請輸入新故鄉:",
		   currutmp->from, 17, DOECHO)) {
	demoney(49);
	currutmp->from_alias=0;
    }
    return 0;
}

int p_exmail() {
    char ans[4],buf[200];
    int n;

    if(cuser.exmailbox >= MAX_EXKEEPMAIL) {
	sprintf(buf,"容量最多增加 %d 封，不能再買了。", MAX_EXKEEPMAIL);
	outs(buf);
	refresh();
	return 0;
    }
    sprintf(buf,"您曾增購 %d 封容量，還要再買多少?",
	    cuser.exmailbox);
    
    getdata_str(b_lines-2, 0, buf, ans, 3, LCECHO, "10");
    
    n = atoi(ans);
    if(!ans[0] || !n)
	return 0;
    if(n + cuser.exmailbox > MAX_EXKEEPMAIL)
	n = MAX_EXKEEPMAIL - cuser.exmailbox;
    reload_money();
    if(cuser.money < n * 1000)
	return 0;
    demoney(n * 1000);
    inmailbox(n);
    return 0;
}

void mail_redenvelop(char* from, char* to, int money, char mode){
    char genbuf[200];
    fileheader_t fhdr;
    time_t now;
    FILE* fp;
    sprintf(genbuf, "home/%c/%s", to[0], to);
    stampfile(genbuf, &fhdr);
    if (!(fp = fopen(genbuf, "w")))
        return;
    now = time(NULL);
    fprintf(fp, "作者: %s\n"
                "標題: 招財進寶\n"
                "時間: %s\n"
                "\033[1;33m親愛的 %s ：\n\n\033[m"
                "\033[1;31m    我包給你一個 %d 元的大紅包喔 ^_^\n\n"
                "    禮輕情意重，請笑納...... ^_^\033[m\n"
                , from, ctime(&now), to, money);
    fclose(fp);
    sprintf(fhdr.title, "招財進寶");
    strcpy(fhdr.owner, from);

    if (mode == 'y')
       vedit(genbuf, NA, NULL);
    sprintf(genbuf, "home/%c/%s/.DIR", to[0], to);       
    append_record(genbuf, &fhdr, sizeof(fhdr));
}

int p_give() {
    int money;
    char id[IDLEN + 1], genbuf[90];
    time_t now = time(0);
    
    move(1,0);
    usercomplete("這位幸運兒的id:", id);
    if(!id[0] || !strcmp(cuser.userid,id) ||
       !getdata(2, 0, "要給多少錢:", genbuf, 7, LCECHO))
	return 0;
    money = atoi(genbuf);
    reload_money();
    if(money > 0 && cuser.money >= money && (inumoney(id, money) != -1)) {
	demoney(money);
	now = time(NULL);
	sprintf(genbuf,"%s\t給%s\t%d\t%s", cuser.userid, id, money,
		ctime(&now));
	log_file(FN_MONEY, genbuf);
	genbuf[0] = 'n';
	getdata(3, 0, "要自行書寫紅包袋嗎？[y/N]", genbuf, 2, LCECHO);
	mail_redenvelop(cuser.userid, id, money, genbuf[0]);
    }
    return 0;
}

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
