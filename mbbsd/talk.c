/* $Id$ */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "config.h"
#include "pttstruct.h"
#include "common.h"
#include "perm.h"
#include "modes.h"
#include "proto.h"

#define QCAST   int (*)(const void *, const void *)

extern userinfo_t *currutmp;
extern char *ModeTypeTable[MAX_MODES];
extern char *fn_overrides;
extern int usernum;
extern char *msg_sure_ny;
extern char *msg_cancel;
extern unsigned int currstat;
extern char *fn_writelog;
extern pid_t currpid;
extern int b_lines;		/* Screen bottom line number: t_lines-1 */
extern int t_lines, t_columns;	/* Screen size / width */
extern char *fn_talklog;
extern char currauthor[IDLEN + 2];
extern char *msg_usr_left;
extern char *msg_uid;
extern char *BBSName;
extern int p_lines;		/* a Page of Screen line numbers: tlines-4 */
extern char fromhost[];
extern char *err_uid;
extern int talkrequest;
extern char *msg_shortulist;
extern char *msg_nobody;
extern boardheader_t *bcache;
extern int curr_idle_timeout;
extern userec_t cuser;
extern userec_t xuser;

FILE *fp_writelog = NULL;

static char *IdleTypeTable[] = {
    "偶在花呆啦", "情人來電", "覓食中", "拜見周公", "假死狀態", "我在思考"
};
static char *sig_des[] = {
    "鬥雞", "聊天", "", "下棋"
};

#define MAX_SHOW_MODE 2
#define M_INT 15		/* monitor mode update interval */
#define P_INT 20		/* interval to check for page req. in
				 * talk/chat */
#define IRH 1
#define HRM 2
#define IFH 2
#define HFM 4

#define MYREJ     1
#define HISREJ    2
#define BOARDFRI  1
#define MYFRI     2
#define HISFRI    4

typedef struct talkwin_t {
    int curcol, curln;
    int sline, eline;
} talkwin_t;

typedef struct pickup_t {
    userinfo_t *ui;
    time_t idle;
    unsigned int friend;
} pickup_t;

extern int bind( /* int,struct sockaddr *, int */ );
extern char *getuserid();
extern struct utmpfile_t *utmpshm;
extern char watermode, no_oldmsg, oldmsg_count;
extern msgque_t oldmsg[MAX_REVIEW];
extern char *friend_file[8];

/* 記錄 friend 的 user number */
#define PICKUP_WAYS     5

static int pickup_way = 0;
int friendcount;
static int friends_number;
static int override_number;
int rejected_number;
static int bfriends_number;
static char *fcolor[11] = {
    "", "\033[36m", "\033[32m", "\033[1;32m",
    "\033[33m", "\033[1;33m", "\033[1;37m", "\033[1;37m",
    "\033[31m", "\033[1;35m", "\033[1;36m"
};
static char save_page_requestor[40];
static char page_requestor[40];
static FILE *flog;

static int is_hidden(char *user) {
    int tuid;
    userinfo_t *uentp;

    if ((!(tuid = getuser(user)))
	|| (!(uentp = search_ulist(cmpuids, tuid)))
	|| ((!uentp->invisible || HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_SEECLOAK))
	    && (((!PERM_HIDE(uentp) && !PERM_HIDE(currutmp)) ||
		 PERM_HIDE(currutmp))
		&& !(is_rejected(uentp) & HRM && !(is_friend(uentp) & HFM)))))
	return 0;		/* 交談 xxx */
    else
	return 1;		/* 自言自語 */
}

char *modestring(userinfo_t * uentp, int simple) {
    static char modestr[40];
    static char *notonline = "不在站上";
    register int mode = uentp->mode;
    register char *word;
    int isrej, isfri;

/* for debugging */
    if (mode >= MAX_MODES)
    {
	syslog(LOG_WARNING, "what!? mode = %d", mode);
	word = ModeTypeTable[mode % MAX_MODES];
    }
    else
	word = ModeTypeTable[mode];
    isrej = is_rejected(uentp);
    isfri = is_friend(uentp);
    if (!(HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_SEECLOAK)) &&
	(
	    (uentp->invisible || (isrej & HRM)) &&
	    !((isfri & HFM) && (isrej & HRM))
	    )
	)
	return notonline;
    else if (mode == EDITING)
    {
	sprintf(modestr, "E:%s",
		ModeTypeTable[uentp->destuid < EDITING ? uentp->destuid :
			     EDITING]);
	word = modestr;
    }
    else if (!mode && *uentp->chatid == 1)
    {
	if (!simple)
	    sprintf(modestr, "回應 %s", getuserid(uentp->destuid));
	else
	    sprintf(modestr, "回應呼叫");
    }
    else if (!mode && *uentp->chatid == 2)
	if (uentp->msgcount < 10)
	{
	    char *cnum[10] =
	    {"", "一", "兩", "三", "四", "五", "六", "七",
	     "八", "九"};
	    sprintf(modestr, "中%s顆水球", cnum[uentp->msgcount]);
	}
	else
	    sprintf(modestr, "不行了 @_@");
    else if (!mode && *uentp->chatid == 3)
	sprintf(modestr, "水球準備中");
    else if (!mode)
	return (uentp->destuid == 6) ? uentp->chatid :
	IdleTypeTable[(0 <= uentp->destuid && uentp->destuid < 6) ?
		     uentp->destuid : 0];
    else if (simple)
	return word;
    else if (uentp->in_chat && mode == CHATING)
	sprintf(modestr, "%s (%s)", word, uentp->chatid);
    else if (mode == TALK)
    {
	if (is_hidden(getuserid(uentp->destuid)))	/* Leeym 對方(紫色)隱形 */
	    sprintf(modestr, "%s", "交談 空氣");	/* Leeym 大家自己發揮吧！ */
	else
	    sprintf(modestr, "%s %s", word, getuserid(uentp->destuid));
    }
    else if (mode != PAGE && mode != TQUERY)
	return word;
    else
	sprintf(modestr, "%s %s", word, getuserid(uentp->destuid));

    return (modestr);
}

int cmpuids(int uid, userinfo_t * urec) {
    return (uid == urec->uid);
}

/* Leeym 從 FireBird 移植改寫過來的 */
static int cmppids(pid_t pid, userinfo_t * urec) {
    return (pid == urec->pid);
}

/* routines for Talk->Friend */
static int can_override(char *userid, char *whoasks) {
    char buf[STRLEN];

    sethomefile(buf, userid, fn_overrides);
    return belong(buf, whoasks);
}

int is_friend(userinfo_t * ui) {
    register int unum, hit, *myfriends;

/* 判斷對方是否為我的朋友 ? */
    unum = ui->uid;
    myfriends = currutmp->friend;
    while ((hit = *myfriends++))
    {
	if (unum == hit)
	{
	    hit = IFH;
	    break;
	}
    }

/* 看板好友 */
    if (currutmp->brc_id && ui->brc_id == currutmp->brc_id)
    {
	hit |= 1;
    }

/* 判斷我是否為對方的朋友 ? */
    myfriends = ui->friend;
    while ((unum = *myfriends++))
    {
	if (unum == usernum)
	{
	    hit |= HFM;
	    break;
	}
    }
    return hit;
}

/* 被拒絕 */
int is_rejected(userinfo_t * ui) {
    register int unum, hit, *myrejects;

    if (PERM_HIDE(ui))
	return 0;
/* 判斷對方是否為我的仇人 ? */

    unum = ui->uid;
    myrejects = currutmp->reject;
    while ((hit = *myrejects++))
    {
	if (unum == hit)
	{
	    hit = 1;
	    break;
	}
    }

/* 判斷我是否為對方的仇人 ? */
    myrejects = ui->reject;
    while ((unum = *myrejects++))
    {
	if (unum == usernum)
	{
	    hit |= 2;
	    break;
	}
    }
    return hit;
}

int isvisible(userinfo_t * uentp, int isfri, int isrej) {
    if (uentp->userid[0] == 0)
	return 0;

    if (PERM_HIDE(uentp) && !(PERM_HIDE(currutmp)))	/* 對方紫色隱形而你沒有 */
	return 0;
    else if (HAS_PERM(PERM_SYSOP))	/* 站長看的見任何人 */
	return 1;

    if (uentp->invisible &&
	!HAS_PERM(PERM_SEECLOAK) &&	/* 沒有看見忍者的權限 */
	!((isrej & HISREJ) && (isfri & HISFRI)))	/* 沒同時設好友壞人 */
	return 0;

    return ((isrej & HISREJ) && !(isfri & HISFRI)) ? 0 : 1;
}

/* 真實動作 */
static void my_kick(userinfo_t * uentp) {
    char genbuf[200];

    getdata(1, 0, msg_sure_ny, genbuf, 4, LCECHO);
    clrtoeol();
    if (genbuf[0] == 'y')
    {
	sprintf(genbuf, "%s (%s)", uentp->userid, uentp->username);
	log_usies("KICK ", genbuf);
	if((uentp->pid <= 0 || kill(uentp->pid, SIGHUP) == -1) && (errno == ESRCH))
	    purge_utmp(uentp);
	outs("踢出去囉");
    }
    else
	outs(msg_cancel);
    pressanykey();
}

int my_query(char *uident) {
    extern char currmaildir[];
    userec_t muser;
    int tuid;
    userinfo_t *uentp;
    
    if ((tuid = getuser(uident)))
    {
	memcpy(&muser, &xuser, sizeof(muser));
	move(1, 0);
	clrtobot();
	move(1, 0);
	setutmpmode(TQUERY);
	currutmp->destuid = tuid;

	prints("《ＩＤ暱稱》%s(%s)\n", muser.userid, muser.username);
	prints("《上站次數》%d次", muser.numlogins);
	move(2, 40);
	prints("《文章篇數》%d篇\n", muser.numposts);

	uentp = (userinfo_t *) search_ulist(cmpuids, tuid);
	prints("\033[1;33m《目前動態》%-28.28s\033[m",
	       (uentp && isvisible(uentp, is_friend(uentp), is_rejected(uentp))) ?
	       modestring(uentp, 0) : "不在站上");

	sethomedir(currmaildir, muser.userid);
	outs(chkmail(1) ? "《私人信箱》有新進信件還沒看\n" :
	     "《私人信箱》所有信件都看過了\n");
	sethomedir(currmaildir, cuser.userid);
	chkmail(1);
	prints("《上次上站》%-28.28s《上次故鄉》%s\n",
	       Cdate(&muser.lastlogin),
	       (muser.lasthost[0] ? muser.lasthost : "(不詳)"));

	if (can_override(muser.userid, cuser.userid) || HAS_PERM(PERM_SYSOP) ||
	    !strcmp(muser.userid, cuser.userid))
	{
	    char *sex[8] =
	    {MSG_BIG_BOY, MSG_BIG_GIRL,
	     MSG_LITTLE_BOY, MSG_LITTLE_GIRL,
	     MSG_MAN, MSG_WOMAN, MSG_PLANT, MSG_MIME};

	    prints("《 性  別 》%-28.28s\n", sex[muser.sex % 8]);
	}
	showplans(uident);
	pressanykey();
	return FULLUPDATE;
    }
    return DONOTHING;
}

static char t_last_write[200] = "";

/* 
   被呼叫的時機:
   1. 丟群組水球 flag = 1 (pre-edit)
   2. 回水球     flag = 0
   3. 上站aloha  flag = 2 (pre-edit)
   4. 廣播       flag = 3 if SYSOP, otherwise flag = 1 (pre-edit)
   5. 丟水球     flag = 0
*/
int my_write(pid_t pid, char *prompt, char *id, int flag) {
    int len, currstat0 = currstat;
    char msg[80], destid[IDLEN + 1];
    char genbuf[200], buf[200], c0 = currutmp->chatid[0];
    unsigned char mode0 = currutmp->mode;
    time_t now;
    struct tm *ptime;
    userinfo_t *uin;
    extern msgque_t oldmsg[MAX_REVIEW];
    
    uin = (userinfo_t *)search_ulist(cmppids, pid);
    strcpy(destid, id);
    
    if(!uin && !(flag == 0 && oldmsg_count > 0)) {
	outmsg("\033[1;33;41m糟糕! 對方已落跑了(不在站上)! \033[37m~>_<~\033[m");
	clrtoeol();
	refresh();
	watermode = -1;
	return 0;
    }
    
    currutmp->mode = 0;
    currutmp->chatid[0] = 3;
    currstat = XMODE;
    
    time(&now);
    ptime = localtime(&now);
    
    if(flag == 0) {
	/* 一般水球 */
	watermode = 0;
	if(!(len = getdata(0, 0, prompt, msg, 56, DOECHO))) {
	    outmsg("\033[1;33;42m算了! 放你一馬...\033[m");
	    clrtoeol();
	    refresh();
	    currutmp->chatid[0] = c0;
	    currutmp->mode = mode0;
	    currstat = currstat0;
	    watermode = -1;
	    return 0;
	}
	
	if(watermode > 0) {
	    int i;
	    
	    i = (no_oldmsg - watermode + MAX_REVIEW) % MAX_REVIEW;
	    uin = (userinfo_t *)search_ulist(cmppids, oldmsg[i].last_pid);
	    strcpy(destid, oldmsg[i].last_userid);
	}
    } else {
	/* pre-edit 的水球 */
	strcpy(msg, prompt);
	len = strlen(msg);
    }
    
    watermode = -1;
    strip_ansi(msg, msg, 0);
    if(uin && *uin->userid && flag == 0) {
	sprintf(buf, "丟給 %s : %s [Y/n]?", uin->userid, msg);
	getdata(0, 0, buf, genbuf, 3, LCECHO);
	if(genbuf[0] == 'n') {
	    outmsg("\033[1;33;42m算了! 放你一馬...\033[m");
	    clrtoeol();
	    refresh();
	    currutmp->chatid[0] = c0;
	    currutmp->mode = mode0;
	    currstat = currstat0;
	    watermode = -1;
	    return 0;
	}
    }
    
    if(!uin || !*uin->userid || strcasecmp(destid, uin->userid)) {
	outmsg("\033[1;33;41m糟糕! 對方已落跑了(不在站上)! \033[37m~>_<~\033[m");
	clrtoeol();
	refresh();
	currutmp->chatid[0] = c0;
	currutmp->mode = mode0;
	currstat = currstat0;
	return 0;
    }
    
    time(&now);
    if(flag != 2) { /* aloha 的水球不用存下來 */
	/* 存到自己的水球檔 */
	if(fp_writelog == NULL) {
	    sethomefile(genbuf, cuser.userid, fn_writelog);
	    fp_writelog = fopen(genbuf, "a");
	}
	if(fp_writelog) {
	    fprintf(fp_writelog, "To %s: %s [%s]\n", 
		    uin->userid, msg, Cdatelite(&now));
	    sprintf(t_last_write, "To %s: %s\n", uin->userid, msg);
	}
    }
    
    if(flag == 3 && uin->msgcount) {
	/* 不懂 */
	uin->destuip = currutmp - &utmpshm->uinfo[0];
	uin->sig = 2;
	if(uin->pid > 0) kill(uin->pid, SIGUSR1);
    } else if(flag != 2 &&
	      !HAS_PERM(PERM_SYSOP) &&
	      (uin->pager == 3 || 
	       uin->pager == 2 || 
	       (uin->pager == 4 &&
		!(is_friend(uin) & 4))))
	outmsg("\033[1;33;41m糟糕! 對方防水了! \033[37m~>_<~\033[m");
    else {
	if(uin->msgcount < MAX_MSGS) {
	    unsigned char pager0 = uin->pager;
	    
	    uin->pager = 2;
	    uin->msgs[uin->msgcount].last_pid = currpid;
	    strcpy(uin->msgs[uin->msgcount].last_userid, cuser.userid);
	    strcpy(uin->msgs[uin->msgcount++].last_call_in, msg);
	    uin->pager = pager0;
	} else if (flag != 2)
	    outmsg("\033[1;33;41m糟糕! 對方不行了! (收到太多水球) \033[37m@_@\033[m");
	
	if(uin->msgcount == 1 && (uin->pid <= 0 || kill(uin->pid, SIGUSR2) == -1) && flag != 2)
	    outmsg("\033[1;33;41m糟糕! 沒打中! \033[37m~>_<~\033[m");
	else if(uin->msgcount == 1 && flag != 2)
	    outmsg("\033[1;33;44m水球砸過去了! \033[37m*^o^*\033[m");
	else if(uin->msgcount > 1 && uin->msgcount < MAX_MSGS && flag != 2)
	    outmsg("\033[1;33;44m再補上一粒! \033[37m*^o^*\033[m");
    }
    
    clrtoeol();
    refresh();
    
    currutmp->chatid[0] = c0;
    currutmp->mode = mode0;
    currstat = currstat0;
    return 1;
}

static char t_display_new_flag = 0;
void t_display_new() {
    int i;
    char buf[200];

    if (t_display_new_flag)
	return;
    else
	t_display_new_flag = 1;

    if (oldmsg_count && watermode > 0)
    {
	move(1, 0);
	outs(
	    "───────水─球─回─顧─────────"
	    "用[Ctrl-R Ctrl-T]鍵切換─────");
	for (i = 0; i < oldmsg_count; i++)
	{
	    int a = (no_oldmsg - i - 1 + MAX_REVIEW) % MAX_REVIEW;

	    move(i + 2, 0);
	    clrtoeol();
	    if (watermode - 1 != i)
		sprintf(buf, "\033[1;33;46m %s \033[37;45m %s \033[m",
			oldmsg[a].last_userid, oldmsg[a].last_call_in);
	    else
		sprintf(buf, "\033[1;44m>\033[1;33;47m%s "
			"\033[37;45m %s \033[m",
			oldmsg[a].last_userid, oldmsg[a].last_call_in);
	    outs(buf);
	}

	if (t_last_write[0])
	{
	    move(i + 2, 0);
	    clrtoeol();
	    outs(t_last_write);
	    i++;
	}
	move(i + 2, 0);
	outs("───────────────────────"
	     "─────────────────");
    }
    t_display_new_flag = 0;
}

int t_display() {
    char genbuf[200], ans[4];

    if(fp_writelog) {
	fclose(fp_writelog);
	fp_writelog = NULL;
    }
    setuserfile(genbuf, fn_writelog);
    if (more(genbuf, YEA) != -1)
    {
	getdata(b_lines - 1, 0, "清除(C) 移至備忘錄(M) 保留(R) (C/M/R)?[R]",
		ans, 3, LCECHO);
	if (*ans == 'm')
	{
	    fileheader_t mymail;
	    char title[128], buf[80];

	    sethomepath(buf, cuser.userid);
	    stampfile(buf, &mymail);

	    mymail.savemode = 'H';	/* hold-mail flag */
	    mymail.filemode = FILE_READ;
	    strcpy(mymail.owner, "[備.忘.錄]");
	    strcpy(mymail.title, "熱線\033[37;41m記錄\033[m");
	    sethomedir(title, cuser.userid);
	    Rename(genbuf, buf);
	    append_record(title, &mymail, sizeof(mymail));
	}
	else if (*ans == 'c')
	    unlink(genbuf);
	return FULLUPDATE;
    }
    return DONOTHING;
}

static void do_talk_nextline(talkwin_t * twin) {
    twin->curcol = 0;
    if (twin->curln < twin->eline)
	++(twin->curln);
    else
	region_scroll_up(twin->sline, twin->eline);
    move(twin->curln, twin->curcol);
}

static void do_talk_char(talkwin_t * twin, int ch) {
    extern screenline_t *big_picture;
    screenline_t *line;
    int i;
    char ch0, buf[81];

    if (isprint2(ch))
    {
	ch0 = big_picture[twin->curln].data[twin->curcol];
	if (big_picture[twin->curln].len < 79)
	    move(twin->curln, twin->curcol);
	else
	    do_talk_nextline(twin);
	outc(ch);
	++(twin->curcol);
	line = big_picture + twin->curln;
	if (twin->curcol < line->len)
	{			/* insert */
	    ++(line->len);
	    memcpy(buf, line->data + twin->curcol, 80);
	    save_cursor();
	    do_move(twin->curcol, twin->curln);
	    ochar(line->data[twin->curcol] = ch0);
	    for (i = twin->curcol + 1; i < line->len; i++)
		ochar(line->data[i] = buf[i - twin->curcol - 1]);
	    restore_cursor();
	}
	line->data[line->len] = 0;
	return;
    }

    switch (ch)
    {
    case Ctrl('H'):
    case '\177':
	if (twin->curcol == 0)
	    return;
	line = big_picture + twin->curln;
	--(twin->curcol);
	if (twin->curcol < line->len)
	{
	    --(line->len);
	    save_cursor();
	    do_move(twin->curcol, twin->curln);
	    for (i = twin->curcol; i < line->len; i++)
		ochar(line->data[i] = line->data[i + 1]);
	    line->data[i] = 0;
	    ochar(' ');
	    restore_cursor();
	}
	move(twin->curln, twin->curcol);
	return;
    case Ctrl('D'):
	line = big_picture + twin->curln;
	if (twin->curcol < line->len)
	{
	    --(line->len);
	    save_cursor();
	    do_move(twin->curcol, twin->curln);
	    for (i = twin->curcol; i < line->len; i++)
		ochar(line->data[i] = line->data[i + 1]);
	    line->data[i] = 0;
	    ochar(' ');
	    restore_cursor();
	}
	return;
    case Ctrl('G'):
	bell();
	return;
    case Ctrl('B'):
	if (twin->curcol > 0)
	{
	    --(twin->curcol);
	    move(twin->curln, twin->curcol);
	}
	return;
    case Ctrl('F'):
	if (twin->curcol < 79)
	{
	    ++(twin->curcol);
	    move(twin->curln, twin->curcol);
	}
	return;
    case KEY_TAB:
	twin->curcol += 8;
	if (twin->curcol > 80)
	    twin->curcol = 80;
	move(twin->curln, twin->curcol);
	return;
    case Ctrl('A'):
	twin->curcol = 0;
	move(twin->curln, twin->curcol);
	return;
    case Ctrl('K'):
	clrtoeol();
	return;
    case Ctrl('Y'):
	twin->curcol = 0;
	move(twin->curln, twin->curcol);
	clrtoeol();
	return;
    case Ctrl('E'):
	twin->curcol = big_picture[twin->curln].len;
	move(twin->curln, twin->curcol);
	return;
    case Ctrl('M'):
    case Ctrl('J'):
	line = big_picture + twin->curln;
	strncpy(buf, line->data, line->len);
	buf[line->len] = 0;
	do_talk_nextline(twin);
	break;
    case Ctrl('P'):
	line = big_picture + twin->curln;
	strncpy(buf, line->data, line->len);
	buf[line->len] = 0;
	if (twin->curln > twin->sline)
	{
	    --(twin->curln);
	    move(twin->curln, twin->curcol);
	}
	break;
    case Ctrl('N'):
	line = big_picture + twin->curln;
	strncpy(buf, line->data, line->len);
	buf[line->len] = 0;
	if (twin->curln < twin->eline)
	{
	    ++(twin->curln);
	    move(twin->curln, twin->curcol);
	}
	break;
    }
    trim(buf);
    if (*buf)
	fprintf(flog, "%s%s: %s%s\n",
		(twin->eline == b_lines - 1) ? "\033[1;35m" : "",
		(twin->eline == b_lines - 1) ?
		getuserid(currutmp->destuid) : cuser.userid, buf,
		(ch == Ctrl('P')) ? "\033[37;45m(Up)\033[m" : "\033[m");
}

static void do_talk(int fd) {
    struct talkwin_t mywin, itswin;
    char mid_line[128], data[200];
    int i, datac, ch;
    int im_leaving = 0;
    FILE *log;
    struct tm *ptime;
    time_t now;
    char genbuf[200], fpath[100];

    time(&now);
    ptime = localtime(&now);

    sethomepath(fpath, cuser.userid);
    strcpy(fpath, tempnam(fpath, "talk_"));
    flog = fopen(fpath, "w");

    setuserfile(genbuf, fn_talklog);

    if ((log = fopen(genbuf, "w")))
	fprintf(log, "[%d/%d %d:%02d] & %s\n",
		ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour,
		ptime->tm_min, save_page_requestor);
    setutmpmode(TALK);

    ch = 58 - strlen(save_page_requestor);
    sprintf(genbuf, "%s【%s", cuser.userid, cuser.username);
    i = ch - strlen(genbuf);
    if (i >= 0)
	i = (i >> 1) + 1;
    else
    {
	genbuf[ch] = '\0';
	i = 1;
    }
    memset(data, ' ', i);
    data[i] = '\0';

    sprintf(mid_line, "\033[1;46;37m  談天說地  \033[45m%s%s】"
	    " 與  %s%s\033[0m", data, genbuf, save_page_requestor, data);

    memset(&mywin, 0, sizeof(mywin));
    memset(&itswin, 0, sizeof(itswin));

    i = b_lines >> 1;
    mywin.eline = i - 1;
    itswin.curln = itswin.sline = i + 1;
    itswin.eline = b_lines - 1;

    clear();
    move(i, 0);
    outs(mid_line);
    move(0, 0);

    add_io(fd, 0);

    while (1)
    {
	ch = igetkey();
	if (ch == I_OTHERDATA)
	{
	    datac = recv(fd, data, sizeof(data), 0);
	    if (datac <= 0)
		break;
	    for (i = 0; i < datac; i++)
		do_talk_char(&itswin, data[i]);
	}
	else
	{
	    if (ch == Ctrl('C'))
	    {
		if (im_leaving)
		    break;
		move(b_lines, 0);
		clrtoeol();
		outs("再按一次 Ctrl-C 就正式中止談話囉！");
		im_leaving = 1;
		continue;
	    }
	    if (im_leaving)
	    {
		move(b_lines, 0);
		clrtoeol();
		im_leaving = 0;
	    }
	    switch (ch)
	    {
	    case KEY_LEFT:	/* 把2byte的鍵改為一byte */
		ch = Ctrl('B');
		break;
	    case KEY_RIGHT:
		ch = Ctrl('F');
		break;
	    case KEY_UP:
		ch = Ctrl('P');
		break;
	    case KEY_DOWN:
		ch = Ctrl('N');
		break;
	    }
	    data[0] = (char) ch;
	    if (send(fd, data, 1, 0) != 1)
		break;
	    if (log)
		fprintf(log, "%c", (ch == Ctrl('M')) ? '\n' : (char) *data);
	    do_talk_char(&mywin, *data);
	}
    }
    if (log)
	fclose(log);

    add_io(0, 0);
    close(fd);

    if (flog)
    {
	char ans[4];
	extern screenline_t *big_picture;
	extern unsigned char scr_lns;
	int i;

	time(&now);
	fprintf(flog, "\n\033[33;44m離別畫面 [%s] ...     \033[m\n",
		Cdatelite(&now));
	for (i = 0; i < scr_lns; i++)
	    fprintf(flog, "%.*s\n", big_picture[i].len, big_picture[i].data);
	fclose(flog);
	more(fpath, NA);
	getdata(b_lines - 1, 0, "清除(C) 移至備忘錄(M). (C/M)?[C]",
		ans, 4, LCECHO);
	if (*ans == 'm')
	{
	    fileheader_t mymail;
	    char title[128];

	    sethomepath(genbuf, cuser.userid);
	    stampfile(genbuf, &mymail);
	    mymail.savemode = 'H';	/* hold-mail flag */
	    mymail.filemode = FILE_READ;
	    strcpy(mymail.owner, "[備.忘.錄]");
	    sprintf(mymail.title, "對話記錄 \033[1;36m(%s)\033[m",
		    getuserid(currutmp->destuid));
	    sethomedir(title, cuser.userid);
	    Rename(fpath, genbuf);
	    append_record(title, &mymail, sizeof(mymail));
	}
	else
	    unlink(fpath);
	flog = 0;
    }
    setutmpmode(XINFO);
}

#define lockreturn(unmode, state) if(lockutmpmode(unmode, state)) return 

static void my_talk(userinfo_t * uin) {
    int sock, msgsock, length, ch;
    struct sockaddr_in server;
    pid_t pid;
    char c;
    char genbuf[4];

    unsigned char mode0 = currutmp->mode;

    ch = uin->mode;
    strcpy(currauthor, uin->userid);

    if (ch == EDITING || ch == TALK || ch == CHATING || ch == PAGE ||
	ch == MAILALL || ch == MONITOR ||
	(!ch && (uin->chatid[0] == 1 || uin->chatid[0] == 3)))
    {
	outs("人家在忙啦");
    }
    else if (!HAS_PERM(PERM_SYSOP) &&
	     (
		 ((is_rejected(uin) & HRM) && (!(is_friend(uin) & HFM))) ||
		 (!uin->pager && !is_friend(uin) & HFM)
		 )
	)
    {
	outs("對方關掉呼叫器了");
    }
    else if (!HAS_PERM(PERM_SYSOP) &&
	     (
		 ((is_rejected(uin) & HRM) && !(is_friend(uin) & HFM)) ||
		 uin->pager == 2
		 )
	)
    {
	outs("對方拔掉呼叫器了");
    }
    else if (!HAS_PERM(PERM_SYSOP) &&
	     !(is_friend(uin) & HFM) && uin->pager == 4)
    {
	outs("對方只接受好友的呼叫");
    }
    else if (!(pid = uin->pid) /*|| (kill(pid, 0) == -1) */ )
    {
//      resetutmpent();
	outs(msg_usr_left);
    }
    else
    {
	showplans(uin->userid);
	getdata(2, 0, "要和他(她) (T)談天(P)鬥寵物"
		"(N)沒事找錯人了?[N] ", genbuf, 4, LCECHO);
	switch (*genbuf)
	{
	case 'y':
	case 't':
	    uin->sig = SIG_TALK;
	    break;
	default:
	    return;
	}

	uin->turn = 1;
	currutmp->turn = 0;
	strcpy(uin->mateid, currutmp->userid);
	strcpy(currutmp->mateid, uin->userid);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
	    perror("sock err");
	    unlockutmpmode();
	    return;
	}
	server.sin_family = PF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = 0;
	if (bind(sock, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
	    close(sock);
	    perror("bind err");
	    unlockutmpmode();
	    return;
	}
	length = sizeof(server);
	if (getsockname(sock, (struct sockaddr *) &server, &length) < 0)
	{
	    close(sock);
	    perror("sock name err");
	    unlockutmpmode();
	    return;
	}
	currutmp->sockactive = YEA;
	currutmp->sockaddr = server.sin_port;
	currutmp->destuid = uin->uid;
	setutmpmode(PAGE);
	uin->destuip = currutmp - &utmpshm->uinfo[0];
	if(pid > 0) kill(pid, SIGUSR1);
	clear();
	prints("正呼叫 %s.....\n鍵入 Ctrl-D 中止....", uin->userid);

	listen(sock, 1);
	add_io(sock, 5);
	while (1)
	{
	    ch = igetch();
	    if (ch == I_TIMEOUT)
	    {
		ch = uin->mode;
		if (!ch && uin->chatid[0] == 1 &&
		    uin->destuip == currutmp - &utmpshm->uinfo[0])
		{
		    bell();
		    outmsg("對方回應中...");
		    refresh();
		}
		else if (ch == EDITING || ch == TALK || ch == CHATING ||
			 ch == PAGE || ch == MAILALL || ch == MONITOR ||
			 (!ch && (uin->chatid[0] == 1 ||
				  uin->chatid[0] == 3)))
		{
		    add_io(0, 0);
		    close(sock);
		    currutmp->sockactive = currutmp->destuid = 0;
		    outmsg("人家在忙啦");
		    pressanykey();
		    unlockutmpmode();
		    return;
		}
		else
		{
#ifdef Linux
		    add_io(sock, 20);	/* added for linux... achen */
#endif
		    move(0, 0);
		    outs("再");
		    bell();

		    uin->destuip = currutmp - &utmpshm->uinfo[0];
		    if(pid <= 0 || kill(pid, SIGUSR1) == -1)
		    {
#ifdef Linux
			add_io(sock, 20);	/* added 4 linux... achen */
#endif
			outmsg(msg_usr_left);
			refresh();
			pressanykey();
			unlockutmpmode();
			return;
		    }
		    continue;
		}
	    }

	    if (ch == I_OTHERDATA)
		break;

	    if (ch == '\004')
	    {
		add_io(0, 0);
		close(sock);
		currutmp->sockactive = currutmp->destuid = 0;
		unlockutmpmode();
		return;
	    }
	}

	msgsock = accept(sock, (struct sockaddr *) 0, (int *) 0);
	if (msgsock == -1)
	{
	    perror("accept");
	    unlockutmpmode();
	    return;
	}
	add_io(0, 0);
	close(sock);
	currutmp->sockactive = NA;
	read(msgsock, &c, sizeof c);

	if (c == 'y')
	{
	    sprintf(save_page_requestor, "%s (%s)",
		    uin->userid, uin->username);
	    switch (uin->sig)
	    {
	    case SIG_TALK:
	    default:
		do_talk(msgsock);
	    }
	}
	else
	{
	    move(9, 9);
	    outs("【回音】 ");
	    switch (c)
	    {
	    case 'a':
		outs("我現在很忙，請等一會兒再 call 我，好嗎？");
		break;
	    case 'b':
		prints("對不起，我有事情不能跟你 %s....", sig_des[uin->sig]);
		break;
	    case 'd':
		outs("我要離站囉..下次再聊吧..........");
		break;
	    case 'c':
		outs("請不要吵我好嗎？");
		break;
	    case 'e':
		outs("找我有事嗎？請先來信唷....");
		break;
	    case 'f':
	    {
		char msgbuf[60];

		read(msgsock, msgbuf, 60);
		prints("對不起，我現在不能跟你 %s，因為\n", sig_des[uin->sig]);
		move(10, 18);
		outs(msgbuf);
	    }
	    break;
	    case '1':
		prints("%s？先拿100銀兩來..", sig_des[uin->sig]);
		break;
	    case '2':
		prints("%s？先拿1000銀兩來..", sig_des[uin->sig]);
		break;
	    default:
		prints("我現在不想 %s 啦.....:)", sig_des[uin->sig]);
	    }
	    close(msgsock);
	}
    }
    currutmp->mode = mode0;
    currutmp->destuid = 0;
    unlockutmpmode();
    pressanykey();
}

/* 選單式聊天介面 */
#define US_PICKUP       1234
#define US_RESORT       1233
#define US_ACTION       1232
#define US_REDRAW       1231

static void t_showhelp() {
    clear();

    outs("\033[36m【 休閒聊天使用說明 】\033[m\n\n"
	 "(←)(e)         結束離開             (h)             看使用說明\n"
	 "(↑)/(↓)(n)    上下移動             (TAB)           切換排序方式\n"
	 "(PgUp)(^B)      上頁選單             ( )(PgDn)(^F)   下頁選單\n"
	 "(Hm)/($)(Ed)    首/尾                (s)             "
	 "來源/好友描述/戰績 切換\n"
	 "(m)             寄信                 (q/c)(/)        "
	 "查詢網友/寵物/找尋網友\n"
	 "(r)             閱\讀信件             (l)             看上次熱訊\n"
	 "(f)             全部/好友列表        (數字)          跳至該使用者\n"
	 "(p/N)           切換呼叫器/更改暱稱  (g)             給錢\n"
	 "(a/d/o)         好友 增加/刪除/修改  [33m(i)             切換心情[m");

    if (HAS_PERM(PERM_PAGE))
    {
	outs("\n\n\033[36m【 交談專用鍵 】\033[m\n\n"
	     "(→)(t)(Enter)  跟他／她聊天\n"
	     "(w)             熱線 Call in\n"
	     "(b)             對好友廣播 (一定要在好友列表中)\n"
	     "(^R)            即時回應 (有人 Call in 你時)");
    }

    if (HAS_PERM(PERM_SYSOP))
    {
	outs("\n\n\033[36m【 站長專用鍵 】\033[m\n\n");
	if (HAS_PERM(PERM_SYSOP))
	    outs("(u)/(H)         設定使用者資料/切換隱形模式\n");
	outs("(R)/(K)         查詢使用者的真實姓名/把壞蛋踢出去\n");
    }
    pressanykey();
}

static int listcuent(userinfo_t * uentp) {
    if ((uentp->uid != usernum) &&
	(!uentp->invisible || HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_SEECLOAK)))
	AddNameList(uentp->userid);
    return 0;
}

static void creat_list() {
    CreateNameList();
    apply_ulist(listcuent);
}

static int search_pickup(int num, int actor, pickup_t pklist[]) {
    char genbuf[IDLEN + 2];

    move(1, 0);
    creat_list();
    namecomplete(msg_uid, genbuf);
    if (genbuf[0])
    {
	int n = (num + 1) % actor;
	while (n != num)
	{
	    if (!strcmp(pklist[n].ui->userid, genbuf))
		return n;
	    if (++n >= actor)
		n = 0;
	}
    }
    return -1;
}

static int pickup_cmp(pickup_t * i, pickup_t * j) {
    switch (pickup_way)
    {
    case 0:
    {
	register int friend;

	if ((friend = j->friend - i->friend))
	    return friend;
    }
    case 1:
	return strcasecmp(i->ui->userid, j->ui->userid);
    case 2:
	return (i->ui->mode - j->ui->mode);
    case 3:
	return (i->idle - j->idle);
    case 4:
	return strcasecmp(i->ui->from, j->ui->from);
    }
    return 0;
}

/* Kaede show friend description */
static char *friend_descript(char *uident) {
    static char *space_buf = "                    ";
    static char desc_buf[80];
    char fpath[80], name[IDLEN + 2], *desc, *ptr;
    int len, flag;
    FILE *fp;
    char genbuf[200];

    setuserfile(fpath, friend_file[0]);

    if ((fp = fopen(fpath, "r")))
    {
	sprintf(name, "%s ", uident);
	len = strlen(name);
	desc = genbuf + 13;

	while ((flag = (int) fgets(genbuf, STRLEN, fp)))
	{
	    if (!memcmp(genbuf, name, len))
	    {
		if ((ptr = strchr(desc, '\n')))
		    ptr[0] = '\0';
		if (desc)
		    break;
	    }
	}
	fclose(fp);
	if (desc && flag)
	    strcpy(desc_buf, desc);
	else
	    return space_buf;

	return desc_buf;
    }
    else
	return space_buf;
}

static char *descript(int show_mode, userinfo_t * uentp, time_t diff) {
    switch (show_mode)
    {
    case 1:
	return friend_descript(uentp->userid);
    case 0:
	return (((uentp->pager != 2 && uentp->pager != 3 && diff) ||
		 HAS_PERM(PERM_SYSOP)) ? uentp->from : "*");
    default:
	return "";
    }
}

static void pickup_user() {
    static int real_name = 0;
    static int show_mode = 0;
    static int show_uid = 0;
    static int show_board = 0;
    static int show_pid = 0;
    static int num = 0;
#ifdef SHOWMIND
    static int show_mind = 0;
#endif
    char genbuf[200];

    register userinfo_t *uentp;
    register pid_t pid0 = 0;	/* Ptt 定位 */
    register int id0 = 0;	/* US_PICKUP時的游標用 */
    register int state = US_PICKUP, hate, ch;
    register int actor = 0, head, foot, bmind = -1;
    int isfri, isrej;
    int badman = 0;
    int savemode = currstat;
    time_t diff, freshtime;
    pickup_t pklist[USHM_SIZE];	/* parameter Ptt註 */
/* num : 現在的游標位 */
/* foot: 此頁的腳腳 */
    char buf[20];		/* actor:共有多少user */
    char pagerchar[5] = "* -Wf";
    char *msg_pickup_way[PICKUP_WAYS] =
    {
	"嗨！朋友",
	"網友代號",
	"網友動態",
	"發呆時間",
	"來自何方"
    };
    char *MODE_STRING[MAX_SHOW_MODE] =
    {
	"故鄉",
	"好友描述"
    };

    while (1)
    {
	if (utmpshm->uptime > freshtime || state == US_PICKUP)
	{
	    state = US_PICKUP;
	    time(&freshtime);
	    bfriends_number = friends_number = override_number =
		rejected_number = actor = ch = 0;
	    while (ch < USHM_SIZE)
	    {
		uentp = &(utmpshm->uinfo[ch++]);
		if (uentp->pid)
		{
		    isrej = head = is_rejected(uentp);
		    isfri = is_friend(uentp);

		    if (!isvisible(uentp, isfri, isrej) ||
			((cuser.uflag & FRIEND_FLAG) &&
			 (!isfri ||
			  ((isrej & MYREJ) && !(isfri & MYFRI)))))
			continue;

		    /* Heat990613:心情排序 */
		    if (bmind != -1 && bmind != uentp->mind)
			continue;


		    if ((isrej & MYREJ) && !(isfri & MYFRI))
			rejected_number++;

		    head = isfri;

		    if (isfri & MYFRI)
			friends_number++;
		    if (isfri & HISFRI)
			override_number++;
		    if (isfri & BOARDFRI)
			bfriends_number++;

#ifdef SHOW_IDLE_TIME
		    diff = freshtime - uentp->lastact;
#ifdef DOTIMEOUT
		    /* prevent fault /dev mount from kicking out users */
//                  if((diff > IDLE_TIMEOUT + 10) &&
		    if ((diff > curr_idle_timeout + 10) &&
			(diff < 60 * 60 * 24 * 5))
		    {
			if ((uentp->pid <= 0 || kill(uentp->pid, SIGHUP) == -1) &&
			    (errno == ESRCH))
			    purge_utmp(uentp);
			continue;
		    }
#endif
		    pklist[actor].idle = diff;
#endif
		    pklist[actor].friend = head;
		    pklist[actor].ui = uentp;
		    actor++;
		}
	    }
	    badman = rejected_number;

	    if (!actor)
	    {
		getdata(b_lines - 1, 0,
			"你的朋友還沒上站，要看看一般網友嗎(Y/N)？[Y]",
			genbuf, 4, LCECHO);
		if (genbuf[0] != 'n')
		{
		    cuser.uflag &= ~FRIEND_FLAG;
		    bmind = -1;
		    continue;
		}
		return;
	    }
	}

	if (state >= US_RESORT)
	    qsort(pklist, actor, sizeof(pickup_t), (QCAST) pickup_cmp);
	if (state >= US_ACTION)
	{
	    showtitle((cuser.uflag & FRIEND_FLAG) ? "好友列表" : "休閒聊天",
		      BBSName);
	    prints("  排序：[%s] 上站人數：%-4d\033[1;32m我的朋友：%-3d"
		   "\033[33m與我為友：%-3d\033[36m板友：%-4d\033[31m壞人："
		   "%-2d\033[m\n"
		   "\033[7m  %s P%c代號         %-17s%-17s%-13s%-10s\033[m\n",
		   msg_pickup_way[pickup_way], actor, friends_number,
		   override_number, bfriends_number,
		   badman,
		   show_uid ? "UID" : "No.", 
		   (HAS_PERM(PERM_SEECLOAK) || HAS_PERM(PERM_SYSOP)) ? 'C' : ' ',
		   real_name ? "姓名" : "暱稱",
		   MODE_STRING[show_mode],
		   show_board ? "Board" : "動態",
		   show_pid ? "       PID" : (show_mind ? "心情  發呆" : "備註  發呆")
		);
	}
	else
	{
	    move(3, 0);
	    clrtobot();
	}
	if (pid0)
	    for (ch = 0; ch < actor; ch++)
	    {
		if (pid0 == (pklist[ch].ui)->pid &&
		    id0 == 256 * pklist[ch].ui->userid[0] +
		    pklist[ch].ui->userid[1])
		{
		    num = ch;
		}
	    }
	if (num < 0)
	    num = 0;
	else if (num >= actor)
	    num = actor - 1;
	head = (num / p_lines) * p_lines;
	foot = head + p_lines;
	if (foot > actor)
	    foot = actor;
	for (ch = head; ch < foot; ch++)
	{
	    uentp = pklist[ch].ui;

	    if (!uentp->pid)
	    {
//Ptt:減少sort次數 state = US_PICKUP; 
		//                 break;
		continue;
	    }
#ifdef SHOW_IDLE_TIME
	    diff = pklist[ch].idle;
	    if (diff > 59990) diff = 59990;   /* Doma: 以免一大串的發呆時間 */
	    if (diff > 0)
		sprintf(buf, "%3ld'%02ld", diff / 60, diff % 60);
	    else
		buf[0] = '\0';
#else
	    buf[0] = '\0';
#endif

#ifdef SHOWPID
	    if (show_pid)
		sprintf(buf, "%6d", uentp->pid);
#endif
	    state = (currutmp == uentp) ? 10 : pklist[ch].friend;

	    if (PERM_HIDE(uentp))
		state = 9;

	    hate = is_rejected(uentp);
	    diff = uentp->pager & !(hate & HRM);
	    prints("%5d %c%c%s%-13s%-17.16s\033[m%-17.16s%-13.13s%s%s\n",
#ifdef SHOWUID
		   show_uid ? uentp->uid :
#endif
		   (ch + 1),
		   (hate & HRM) ? 'X' :
		   pagerchar[uentp->pager % 5],
		   (uentp->invisible ? ')' : ' '),
		   (
		       (hate & IRH) && !is_friend(uentp)
		       )? fcolor[8] : fcolor[state],
		   /* %s */
		   uentp->userid,

		   /* %-13s 暱稱 */
#ifdef REALINFO
		   real_name ? uentp->realname :
#endif
		   uentp->username,
		   /* %-17.16s 故鄉 */
		   descript(show_mode, uentp, diff),

		   /* %-17.16s 看板 */
#ifdef SHOWBOARD
		   show_board ? (uentp->brc_id == 0 ? "" :
				 bcache[uentp->brc_id - 1].brdname) :
#endif
		   /* %-13.13s */
		   modestring(uentp, 0),
		   /* %4s 備註 */
		   ((uentp->userlevel & PERM_VIOLATELAW) ? "通緝" : (uentp->birth ? "壽星"
								     : "    ")),
		   /* %s 發呆 */
		   buf);
	}
	if (state == US_PICKUP)
	    continue;

	move(b_lines, 0);
	outs("\033[31;47m(TAB/f)\033[30m排序/好友 \033[31m(t)\033[30m聊天 "
	     "\033[31m(a/d/o)\033[30m交友 \033[31m(q)\033[30m查詢 "
	     "\033[31m(w)\033[30m水球 \033[31m(m)\033[30m寄信 \033[31m(h)"
	     "\033[30m線上輔助 \033[m");
	state = 0;
	while (!state)
	{
	    ch = cursor_key(num + 3 - head, 0);
	    if (ch == KEY_RIGHT || ch == '\n' || ch == '\r')
		ch = 't';

	    switch (ch)
	    {
	    case KEY_LEFT:
	    case 'e':
	    case 'E':
		return;

	    case KEY_TAB:
		pickup_way = (pickup_way + 1) % PICKUP_WAYS;
		state = US_RESORT;
		num = 0;
		break;

	    case KEY_DOWN:
	    case 'n':
	    case 'j':
		if (++num < actor)
		{
		    if (num >= foot)
			state = US_REDRAW;
		    break;
		}
	    case '0':
	    case KEY_HOME:
		num = 0;
		if (head)
		    state = US_REDRAW;
		break;
#if 0
		/* 這功能似乎會導致shared memory的混亂, 先暫時拿掉 */
	    case 'N':
		if (HAS_PERM(PERM_BASIC))
		{
		    char buf[100];

		    sprintf(buf, "暱稱 [%s]：", currutmp->username);
		    if (!getdata(1, 0, buf, currutmp->username, 17, DOECHO))
			strcpy(currutmp->username, cuser.username);

		    state = US_REDRAW;
		}
		break;
#endif
	    case 'H':
		if (HAS_PERM(PERM_SYSOP))
		{
		    currutmp->userlevel ^= PERM_DENYPOST;
		    state = US_REDRAW;
		}
		break;
	    case 'D':
		if (HAS_PERM(PERM_SYSOP))
		{
		    char buf[100];

		    sprintf(buf, "代號 [%s]：", currutmp->userid);
		    if (!getdata(1, 0, buf, currutmp->userid, IDLEN + 1,
				 DOECHO))
			strcpy(currutmp->userid, cuser.userid);
		    state = US_REDRAW;
		}
		break;
	    case 'F':
		if (HAS_PERM(PERM_SYSOP))
		{
		    char buf[100];

		    sprintf(buf, "故鄉 [%s]：", currutmp->from);
		    if (!getdata(1, 0, buf, currutmp->from, 17, DOECHO))
			strncpy(currutmp->from, fromhost, 23);
		    state = US_REDRAW;
		}
		break;
	    case 'C':
#if !HAVE_FREECLOAK
		if (HAS_PERM(PERM_CLOAK))
#endif
		{
		    currutmp->invisible ^= 1;
		    state = US_REDRAW;
		}
		break;
	    case ' ':
	    case KEY_PGDN:
	    case Ctrl('F'):
		if (foot < actor)
		{
		    num += p_lines;
		    state = US_REDRAW;
		    break;
		}
		if (head)
		    num = 0;
		state = US_PICKUP;
		break;
	    case KEY_UP:
	    case 'k':
		if (--num < head)
		{
		    if (num < 0)
		    {
			num = actor - 1;
			if (actor == foot)
			    break;
		    }
		    state = US_REDRAW;
		}
		break;
	    case KEY_PGUP:
	    case Ctrl('B'):
	    case 'P':
		if (head)
		{
		    num -= p_lines;
		    state = US_REDRAW;
		    break;
		}

	    case KEY_END:
	    case '$':
		num = actor - 1;
		if (foot < actor)
		    state = US_REDRAW;
		break;

	    case '/':
	    {
		int tmp;
		if ((tmp = search_pickup(num, actor, pklist)) >= 0)
		    num = tmp;
		state = US_ACTION;
	    }
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
	    {		/* Thor: 可以打數字跳到該人 */
		int tmp;
		if ((tmp = search_num(ch, actor - 1)) >= 0)
		    num = tmp;
		state = US_REDRAW;
	    }
	    break;
#ifdef SHOWMIND
	    case 'M':
		show_mind ^= 1;
		state = US_PICKUP;
		break;
#endif
#ifdef REALINFO
	    case 'R':		/* 顯示真實姓名 */
		if (HAS_PERM(PERM_SYSOP))
		    real_name ^= 1;
		state = US_PICKUP;
		break;
#endif
#ifdef SHOWUID
	    case 'U':
		if (HAS_PERM(PERM_SYSOP))
		    show_uid ^= 1;
		state = US_PICKUP;
		break;
#endif
#ifdef  SHOWBOARD
	    case 'Y':
		if (HAS_PERM(PERM_SYSOP))
		    show_board ^= 1;
		state = US_PICKUP;
		break;
#endif
#ifdef  SHOWPID
	    case '#':
		if (HAS_PERM(PERM_SYSOP))
		    show_pid ^= 1;
		state = US_PICKUP;
		break;
#endif

	    case 'b':		/* broadcast */
		if (cuser.uflag & FRIEND_FLAG || HAS_PERM(PERM_SYSOP))
		{
		    int actor_pos = actor;
		    char ans[4];

		    state = US_PICKUP;
		    if (!getdata(0, 0, "廣播訊息:", genbuf, 60, DOECHO))
			break;
		    if (getdata(0, 0, "確定廣播? [Y]", ans, 4, LCECHO) &&
			*ans == 'n')
			break;
		    while (actor_pos)
		    {
			uentp = pklist[--actor_pos].ui;
			if (uentp->pid &&
			    currpid != uentp->pid &&
			    uentp->pid > 0 && kill(uentp->pid, 0) != -1 &&
			    (HAS_PERM(PERM_SYSOP) ||
			     (uentp->pager != 3 &&
			      (uentp->pager != 4 || is_friend(uentp) & HFM))))
			    my_write(uentp->pid, genbuf, uentp->userid, HAS_PERM(PERM_SYSOP) ? 3 : 1);
		    }
		}
		break;
	    case 's':		/* 顯示好友描述 */
		show_mode = (++show_mode) % MAX_SHOW_MODE;
		state = US_PICKUP;
		break;
	    case 'u':		/* 線上修改資料 */
	    case 'K':		/* 把壞蛋踢出去 */
		if (!HAS_PERM(PERM_ACCOUNTS))
		    continue;
		state = US_ACTION;
		break;
	    case Ctrl('S'):
		state = US_ACTION;
		break;
	    case 't':
	    case 'w':
		if (!(cuser.userlevel & PERM_LOGINOK))
		    continue;
		state = US_ACTION;
		break;
	    case 'a':
	    case 'd':
	    case 'o':
	    case 'f':
	    case 'g':
		if (!HAS_PERM(PERM_LOGINOK))
		    /* 註冊才有 Friend */
		    break;
		if (ch == 'f')
		{
		    cuser.uflag ^= FRIEND_FLAG;
		    state = US_PICKUP;
		    break;
		}
		state = US_ACTION;
		break;
	    case 'q':
	    case 'c':
	    case 'm':
	    case 'r':
	    case 'l':
		/* guest 只能 query */
		if (!cuser.userlevel && ch != 'q' && ch != 'l')
		    break;
	    case 'h':
		state = US_ACTION;
		break;
	    case 'p':
		if (HAS_PERM(PERM_BASIC))
		{
		    t_pager();
		    state = US_REDRAW;
		}
		break;
	    default:		/* refresh screen */
		state = US_REDRAW;
	    }
	}

	if (state != US_ACTION)
	{
	    pid0 = 0;
	    continue;
	}

	/* Ptt decide cur */
	uentp = pklist[num].ui;
	pid0 = uentp->pid;
	id0 = 256 * uentp->userid[0] + uentp->userid[1];

	if (ch == 'w')
	{
	    if ((uentp->pid != currpid) &&
		(HAS_PERM(PERM_SYSOP) ||
		 (uentp->pager != 3 &&
		  (is_friend(uentp) & HFM || uentp->pager != 4))))
	    {
		cursor_show(num + 3 - head, 0);
		sprintf(genbuf, "Call-In %s ：", uentp->userid);
		my_write(uentp->pid, genbuf, uentp->userid, 0);
	    }
	}
	else if (ch == 'l')
	{			/* Thor: 看 Last call in */
	    t_display();
	}
	else
	{
	    switch (ch)
	    {
	    case 'r':
		m_read();
		break;
	    case 'a':
		friend_add(uentp->userid, FRIEND_OVERRIDE);
		friend_load();
		state = US_PICKUP;
		break;
	    case 'd':
		friend_delete(uentp->userid, FRIEND_OVERRIDE);
		friend_load();
		state = US_PICKUP;
		break;
	    case 'o':
		t_override();
		state = US_PICKUP;
		break;
	    case 'K':
		if (uentp->pid > 0 && kill(uentp->pid, 0) != -1)
		{
		    move(1, 0);
		    clrtobot();
		    move(2, 0);
		    my_kick(uentp);
		    state = US_PICKUP;
		}
		break;
	    case 'm':
		stand_title("寄  信");
		prints("[寄信] 收信人：%s", uentp->userid);
		my_send(uentp->userid);
		break;
	    case 'q':
		strcpy(currauthor, uentp->userid);
		my_query(uentp->userid);
		break;
	    case 'u':		/* Thor: 可線上查看及修改使用者 */
	    {
		int id;
		userec_t muser;

		strcpy(currauthor, uentp->userid);
		stand_title("使用者設定");
		move(1, 0);
		if ((id = getuser(uentp->userid)))
		{
		    memcpy(&muser, &xuser, sizeof(muser));
		    user_display(&muser, 1);
		    uinfo_query(&muser, 1, id);
		}
	    }
	    break;

	    case 'h':		/* Thor: 看 Help */
		t_showhelp();
		break;

	    case 't':
		if (uentp->pid != currpid &&
		    (strcmp(uentp->userid, cuser.userid)))
		{
		    move(1, 0);
		    clrtobot();
		    move(3, 0);
		    my_talk(uentp);
		    state = US_PICKUP;
		}
		break;
	    }
	}
	setutmpmode(savemode);
    }
}

int t_users() {
    int destuid0 = currutmp->destuid;
    int mode0 = currutmp->mode;
    int stat0 = currstat;

    if (chkmailbox())
	return 0;
    setutmpmode(LUSERS);
    pickup_user();
    currutmp->mode = mode0;
    currutmp->destuid = destuid0;
    currstat = stat0;
    return 0;
}

int t_pager() {
    currutmp->pager = (currutmp->pager + 1) % 5;
    return 0;
}

int t_idle() {
    int destuid0 = currutmp->destuid;
    int mode0 = currutmp->mode;
    int stat0 = currstat;
    char genbuf[20];
    char buf[80], passbuf[PASSLEN];

    setutmpmode(IDLE);
    getdata(b_lines - 1, 0, "理由：[0]發呆 (1)接電話 (2)覓食 (3)打瞌睡 "
	    "(4)裝死 (5)羅丹 (6)其他 (Q)沒事？", genbuf, 3, DOECHO);
    if (genbuf[0] == 'q' || genbuf[0] == 'Q')
    {
	currutmp->mode = mode0;
	currstat = stat0;
	return 0;
    }
    else if (genbuf[0] >= '1' && genbuf[0] <= '6')
	currutmp->destuid = genbuf[0] - '0';
    else
	currutmp->destuid = 0;

    if (currutmp->destuid == 6)
	if (!cuser.userlevel ||
	    !getdata(b_lines - 1, 0, "發呆的理由：", currutmp->chatid, 11,
		     DOECHO))
	    currutmp->destuid = 0;
    do
    {
	move(b_lines - 2, 0);
	clrtoeol();
	sprintf(buf, "(鎖定螢幕)發呆原因: %s", (currutmp->destuid != 6) ?
		IdleTypeTable[currutmp->destuid] : currutmp->chatid);
	outs(buf);
	refresh();
	getdata(b_lines - 1, 0, MSG_PASSWD, passbuf, PASSLEN, NOECHO);
	passbuf[8] = '\0';
    }
    while (!checkpasswd(cuser.passwd, passbuf) &&
	   strcmp(STR_GUEST, cuser.userid));

    currutmp->mode = mode0;
    currutmp->destuid = destuid0;
    currstat = stat0;

    return 0;
}

int t_query() {
    char uident[STRLEN];

    stand_title("查詢網友");
    usercomplete(msg_uid, uident);
    if (uident[0])
	my_query(uident);
    return 0;
}

int t_talk() {
    char uident[16];
    int tuid, unum, ucount;
    userinfo_t *uentp;
    char genbuf[4];

    if (count_ulist() <= 1)
    {
	outs("目前線上只有您一人，快邀請朋友來光臨【" BBSNAME "】吧！");
	return XEASY;
    }
    stand_title("打開話匣子");
    creat_list();
    namecomplete(msg_uid, uident);
    if (uident[0] == '\0')
	return 0;

    move(3, 0);
    if (!(tuid = searchuser(uident)) || tuid == usernum)
    {
	outs(err_uid);
	pressanykey();
	return 0;
    }

/* multi-login check */
    unum = 1;
    while ((ucount = count_logins(cmpuids, tuid, 0)) > 1)
    {
	outs("(0) 不想 talk 了...\n");
	count_logins(cmpuids, tuid, 1);
	getdata(1, 33, "請選擇一個聊天對象 [0]：", genbuf, 4, DOECHO);
	unum = atoi(genbuf);
	if (unum == 0)
	    return 0;
	move(3, 0);
	clrtobot();
	if (unum > 0 && unum <= ucount)
	    break;
    }

    if ((uentp = (userinfo_t *) search_ulistn(cmpuids, tuid, unum)))
	my_talk(uentp);

    return 0;
}

/* 有人來串門子了，回應呼叫器 */
static userinfo_t *uip;
void talkreply() {
    struct hostent *h;
    char hostname[STRLEN], buf[4];
    struct sockaddr_in sin;
    char genbuf[200];
    int a, sig = currutmp->sig;

    talkrequest = NA;
    uip = &utmpshm->uinfo[currutmp->destuip];
    sprintf(page_requestor, "%s (%s)", uip->userid, uip->username);
    currutmp->destuid = uip->uid;
    currstat = XMODE;		/* 避免出現動畫 */

    clear();

    prints("\n\n");
    prints("       (Y) 讓我們 %s 吧！"
	   "     (A) 我現在很忙，請等一會兒再 call 我\n", sig_des[sig]);
    prints("       (N) 我現在不想 %s"
	   "      (B) 對不起，我有事情不能跟你 %s\n",
	   sig_des[sig], sig_des[sig]);
    prints("       (C) 請不要吵我好嗎？"
	   "     (D) 我要離站囉..下次再聊吧.......\n");
    prints("       (E) 有事嗎？請先來信"
	   "     (F) \033[1;33m我自己輸入理由好了...\033[m\n");
    prints("       (1) %s？先拿100銀兩來"
	   "  (2) %s？先拿1000銀兩來..\n\n", sig_des[sig], sig_des[sig]);

    getuser(uip->userid);
    currutmp->msgs[0].last_pid = uip->pid;
    strcpy(currutmp->msgs[0].last_userid, uip->userid);
    strcpy(currutmp->msgs[0].last_call_in, "呼叫、呼叫，聽到請回答 (Ctrl-R)");
    prints("對方來自 [%s]，共上站 %d 次，文章 %d 篇\n",
	   uip->from, xuser.numlogins, xuser.numposts);
    showplans(uip->userid);
    show_last_call_in(0);

    sprintf(genbuf, "你想跟 %s %s啊？請選擇(Y/N/A/B/C/D/E/F/1/2)[N] ",
	    page_requestor, sig_des[sig]);
    getdata(0, 0, genbuf, buf, 4, LCECHO);

    if (uip->mode != PAGE)
    {
	sprintf(genbuf, "%s已停止呼叫，按Enter繼續...", page_requestor);
	getdata(0, 0, genbuf, buf, 4, LCECHO);
	return;
    }
    currutmp->msgcount = 0;
    strcpy(save_page_requestor, page_requestor);
    memset(page_requestor, 0, sizeof(page_requestor));
    gethostname(hostname, STRLEN);

    if (!(h = gethostbyname(hostname)))
    {
	perror("gethostbyname");
	return;
    }
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = h->h_addrtype;
    memcpy(&sin.sin_addr, h->h_addr, h->h_length);
    sin.sin_port = uip->sockaddr;
    a = socket(sin.sin_family, SOCK_STREAM, 0);
    if ((connect(a, (struct sockaddr *) &sin, sizeof(sin))))
    {
	perror("connect err");
	return;
    }
    if (!buf[0] || !strchr("yabcdef12", buf[0]))
	buf[0] = 'n';
    write(a, buf, 1);
    if (buf[0] == 'f' || buf[0] == 'F')
    {
	if (!getdata(b_lines, 0, "不能的原因：", genbuf, 60, DOECHO))
	    strcpy(genbuf, "不告訴你咧 !! ^o^");
	write(a, genbuf, 60);
    }
    uip->destuip = currutmp - &utmpshm->uinfo[0];
    if (buf[0] == 'y')
	switch (sig)
	{
	case SIG_TALK:
	default:
	    do_talk(a);
	}
    else
	close(a);
    clear();
}

/* 網友動態簡表 */
static int shortulist(userinfo_t * uentp) {
    static int lineno, fullactive, linecnt;
    static int moreactive, page, num;
    char uentry[50];
    int state;

    if (!lineno)
    {
	lineno = 3;
	page = moreactive ? (page + p_lines * 3) : 0;
	linecnt = num = moreactive = 0;
	move(1, 70);
	prints("Page: %d", page / (p_lines) / 3 + 1);
	move(lineno, 0);
    }

    if (uentp == NULL)
    {
	int finaltally;

	clrtoeol();
	move(++lineno, 0);
	clrtobot();
	finaltally = fullactive;
	lineno = fullactive = 0;
	return finaltally;
    }
    if ((!HAS_PERM(PERM_SYSOP) &&
	 !HAS_PERM(PERM_SEECLOAK) &&
	 uentp->invisible) ||
	((is_rejected(uentp) & HRM) &&
	 !HAS_PERM(PERM_SYSOP)))
    {
	if (lineno >= b_lines)
	    return 0;
	if (num++ < page)
	    return 0;
	memset(uentry, ' ', 25);
	uentry[25] = '\0';
    }
    else
    {
	fullactive++;
	if (lineno >= b_lines)
	{
	    moreactive = 1;
	    return 0;
	}
	if (num++ < page)
	    return 0;

	state = (currutmp == uentp) ? 10 : is_friend(uentp);

	if (PERM_HIDE(uentp))
	    state = 9;

	sprintf(uentry, "%s%-13s%c%-10s%s ", fcolor[state],
		uentp->userid, uentp->invisible ? '#' : ' ',
		modestring(uentp, 1), state ? "\033[0m" : "");
    }
    if (++linecnt < 3)
    {
	strcat(uentry, "│");
	outs(uentry);
    }
    else
    {
	outs(uentry);
	linecnt = 0;
	clrtoeol();
	move(++lineno, 0);
    }
    return 0;
}

static void do_list(char *modestr) {
    int count;

    showtitle(modestr, BBSName);
    if (currstat == MONITOR)
	prints("每隔 %d 秒更新一次，請按[Ctrl-C]或[Ctrl-D]離開", M_INT);

    outc('\n');
    outs(msg_shortulist);

    bfriends_number = friends_number = override_number = 0;
    if (apply_ulist(shortulist) == -1)
	outs(msg_nobody);
    else
    {
	time_t thetime = time(NULL);

	count = shortulist(NULL);
	move(b_lines, 0);
	prints("\033[1;37;46m  上站總人數：%-7d\033[32m我的朋友：%-6d"
	       "\033[33m與我為友：%-8d\033[30m%-23s\033[m",
	       count, friends_number, override_number, Cdate(&thetime));
	refresh();
    }
}

/* 監看使用情形 */
static int idle_monitor_time;

static void sig_catcher(int sig) {
    if (currstat != MONITOR)
    {
#ifdef DOTIMEOUT
	init_alarm();
#else
	signal(SIGALRM, SIG_IGN);
#endif
	return;
    }
    if (signal(SIGALRM, sig_catcher) == SIG_ERR)
    {
	perror("signal");
	exit(1);
    }

#ifdef DOTIMEOUT
    if (!PERM_HIDE(currutmp))
	idle_monitor_time += M_INT;
    if (idle_monitor_time > MONITOR_TIMEOUT)
    {
	clear();
	fprintf(stderr, "timeout\n");
	abort_bbs(0);
    }
#endif
    do_list("追蹤站友");
    alarm(M_INT);
}

int t_monitor() {
    char c;
    int i;

    setutmpmode(MONITOR);
    alarm(0);
    signal(SIGALRM, sig_catcher);
    idle_monitor_time = 0;

    do_list("追蹤站友");
    alarm(M_INT);
    while (YEA)
    {
	i = read(0, &c, 1);
	if (!i || c == Ctrl('D') || c == Ctrl('C'))
	    break;
	else if (i == -1)
	{
	    if (errno != EINTR)
	    {
		perror("read");
		exit(1);
	    }
	}
	else
	    idle_monitor_time = 0;
    }
    return 0;
}
