/* $Id$ */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include "config.h"
#include "pttstruct.h"
#include "common.h"
#include "perm.h"
#include "modes.h"
#include "proto.h"

#define SOCKET_QLEN 4
#define TH_LOW 100
#define TH_HIGH 120

extern struct utmpfile_t *utmpshm;

extern int t_lines, t_columns;  /* Screen size / width */
extern int b_lines;             /* Screen bottom line number: t_lines-1 */
extern userinfo_t *currutmp;
extern int curr_idle_timeout;

static void do_aloha(char *hello);

static jmp_buf byebye;

int talkrequest = NA;

static char remoteusername[40] = "?";

extern struct fromcache_t *fcache;
extern struct utmpfile_t *utmpshm;
extern int fcache_semid;

static unsigned char enter_uflag;
static int use_shell_login_mode=0;

char fromhost[STRLEN] = "\0";

static struct sockaddr_in xsin;

/* set signal handler, which won't be reset once signal comes */
static void signal_restart(int signum, void (*handler)(int)) {
    struct sigaction act;
    act.sa_handler=handler;
    memset(&(act.sa_mask), 0, sizeof(sigset_t));
    act.sa_flags=0;
    sigaction(signum,&act,NULL);
}

static void start_daemon() {
    int n;
    char buf[80];
    
    /*
     * More idiot speed-hacking --- the first time conversion makes the C
     * library open the files containing the locale definition and time zone.
     * If this hasn't happened in the parent process, it happens in the
     * children, once per connection --- and it does add up.
     */
    time_t dummy = time(NULL);
    struct tm *dummy_time = localtime(&dummy);

    strftime(buf, 80, "%d/%b/%Y:%H:%M:%S", dummy_time);

    if((n = fork())) {
	fprintf(stdout, "pid[%d]\n", n);
	exit(0);
    }
    n = getdtablesize();
    /*
      while(n)
      close(--n);*/
}

static void reapchild(int sig) {
    int state, pid;
    
    while((pid = waitpid(-1, &state, WNOHANG | WUNTRACED)) > 0)
	;
}

#define BANNER \
"【" BBSNAME "】◎ 台大流行網 ◎(" MYHOSTNAME ")\r\n"\
"  調幅(" MYIP ") "

/* check load and print approriate banner string in buf */
static int chkload(char *buf) {
    char cpu_load[30];
    int i;
    
    i = cpuload(cpu_load);
    
    sprintf(buf, "%s 系統負荷 %s %s \r\n", BANNER, cpu_load,
	    (i > MAX_CPULOAD ? "，高負荷量，請稍後再來 "
	     "(請利用port 3000~3010連線)" : ""));

    if(i > MAX_CPULOAD)
	return 1;
    else if(i > MAX_CPULOAD/2)
    	curr_idle_timeout = 10*60;
    else
    	curr_idle_timeout = 30*60;

    return 0;
}

extern userec_t cuser;

void log_user(char *msg) {
    char filename[200];
    
    sprintf(filename, BBSHOME "/home/%c/%s/USERLOG",
	    cuser.userid[0], cuser.userid);
    log_file(filename, msg);
}

extern time_t login_start_time;

void log_usies(char *mode, char *mesg) {
    char genbuf[200];
    time_t now = time(0);
    
    if (!mesg)
	sprintf(genbuf, cuser.userid[0] ? "%s %s %-12s Stay:%d (%s)" :
		"%s %s %s Stay:%d (%s)",
		Cdate(&now), mode, cuser.userid,
		(now - login_start_time) / 60, cuser.username);
    else
	sprintf(genbuf, cuser.userid[0] ? "%s %s %-12s %s" : "%s %s %s%s",
		Cdate(&now), mode, cuser.userid, mesg);
    log_file(FN_USIES, genbuf);

    /* 追蹤使用者 */
    if(HAS_PERM(PERM_LOGUSER))
	log_user(genbuf);
}

static void setflags(int mask, int value) {
    if(value)
	cuser.uflag |= mask;
    else
	cuser.uflag &= ~mask;
}

extern int usernum;
extern int currmode;

void u_exit(char *mode) {
    userec_t xuser;
    int diff = (time(0) - login_start_time) / 60;

    passwd_query(usernum, &xuser);
    
    auto_backup();
    
    setflags(PAGER_FLAG, currutmp->pager != 1);
    setflags(CLOAK_FLAG, currutmp->invisible);
    
    xuser.invisible = currutmp->invisible % 2;
    xuser.pager = currutmp->pager % 5;
    
    if(!(HAS_PERM(PERM_SYSOP) && HAS_PERM(PERM_DENYPOST)))
	do_aloha("<<下站通知>> -- 我走囉！");
    
    purge_utmp(currutmp);
    if((cuser.uflag != enter_uflag) || (currmode & MODE_DIRTY) || !diff) {
	xuser.uflag = cuser.uflag;
	xuser.numposts = cuser.numposts;
	if(!diff && cuser.numlogins)
	    xuser.numlogins = --cuser.numlogins; /* Leeym 上站停留時間限制式 */
	reload_money();
	passwd_update(usernum, &xuser);
    }
    log_usies(mode, NULL);
}

static void system_abort() {
    if(currmode)
	u_exit("ABORT");
    
    clear();
    refresh();
    fprintf(stdout, "謝謝光臨, 記得常來喔 !\n");
    exit(0);
}

void abort_bbs(int sig) {
    if(currmode)
	u_exit("AXXED");
    exit(0);
}

static void abort_bbs_debug(int sig) {
    if(currmode)
	u_exit("AXXED");
    // printpt("debug me!(%d)",sig);
    // sleep(3600);	/* wait 60 mins for debug */
    exit(0);
}

/* 登錄 BBS 程式 */
static void mysrand() {
    register int i = utmpshm->number;	/* 站上的人數當洗牌次數 */
    
    srand(time(NULL) + currutmp->pid);	/* 時間跟 pid 當 rand 的 seed */
    
    while(i--)
	rand();
}

extern userec_t xuser;

int dosearchuser(char *userid) {
    if((usernum = getuser(userid)))
	memcpy(&cuser, &xuser, sizeof(cuser));
    else
	memset(&cuser, 0, sizeof(cuser));
    return usernum;
}

static void talk_request() {
    bell();
    bell();
    if(currutmp->msgcount) {
	char buf[200];
	time_t now = time(0);
	
	sprintf(buf, "\033[33;41m★%s\033[34;47m [%s] %s \033[0m",
		utmpshm->uinfo[currutmp->destuip].userid, my_ctime(&now),
		(currutmp->sig == 2) ? "重要消息廣播！(請Ctrl-U,l查看熱訊記錄)"
		: "呼叫、呼叫，聽到請回答");
	move(0, 0);
	clrtoeol();
	outs(buf);
	refresh();
    } else {
	unsigned char mode0 = currutmp->mode;
	char c0 = currutmp->chatid[0];
	screenline_t *screen0 = calloc(t_lines, sizeof(screenline_t));
	extern screenline_t *big_picture;
	
	MPROTECT_UTMP_RW;
	currutmp->mode = 0;
	currutmp->chatid[0] = 1;
	MPROTECT_UTMP_R;
	memcpy(screen0, big_picture, t_lines * sizeof(screenline_t));
 	talkreply();
	MPROTECT_UTMP_RW;
	currutmp->mode = mode0;
	currutmp->chatid[0] = c0;
	MPROTECT_UTMP_R;
	memcpy(big_picture, screen0, t_lines * sizeof(screenline_t));
	free(screen0);
	redoscr();
    }
}

extern char *fn_writelog;

void show_last_call_in(int save) {
    char buf[200];

    sprintf(buf, "\033[1;33;46m★%s\033[37;45m %s \033[m",
	    currutmp->msgs[0].last_userid,
	    currutmp->msgs[0].last_call_in);
    move(b_lines, 0);
    clrtoeol();
    refresh();
    outmsg(buf);
    
    if(save) {
	char genbuf[200];
	time_t now;
	extern FILE *fp_writelog;
	
	if(fp_writelog == NULL) {
	    sethomefile(genbuf, cuser.userid, fn_writelog);
	    fp_writelog = fopen(genbuf, "a");
	}
	if(fp_writelog) {
	    time(&now);
	    fprintf(fp_writelog, 
		    "%s \033[0m[%s]\n",
		    buf, Cdatelite(&now));
	}
    }
}

extern unsigned int currstat;
msgque_t oldmsg[MAX_REVIEW];	/* 丟過去的水球 */
char no_oldmsg = 0, oldmsg_count = 0;	/* pointer */

static void write_request(int sig) {
    struct tm *ptime;
    time_t now;
    extern char watermode;
    
    time(&now);
    ptime = localtime(&now);
    
    if(currutmp->pager != 0 &&
       cuser.userlevel != 0 &&
       currutmp->msgcount != 0 &&
       currutmp->mode != TALK &&
       currutmp->mode != EDITING &&
       currutmp->mode != CHATING &&
       currutmp->mode != PAGE &&
       currutmp->mode != IDLE &&
       currutmp->mode != MAILALL &&
       currutmp->mode != MONITOR) {
	int i;
	char c0 = currutmp->chatid[0];
	int currstat0 = currstat;
	unsigned char mode0 = currutmp->mode;
	
	MPROTECT_UTMP_RW;
	currutmp->mode = 0;
	currutmp->chatid[0] = 2;
	MPROTECT_UTMP_R;
	currstat = XMODE;
	
	do {
	    bell();
	    show_last_call_in(1);
	    igetch();
	    MPROTECT_UTMP_RW;
	    currutmp->msgcount--;
	    MPROTECT_UTMP_R;
	    if(currutmp->msgcount>=MAX_MSGS)
	    {
		/* this causes chaos... jochang */
		raise(SIGFPE);
	    }
	    memcpy(&oldmsg[(int)no_oldmsg],
		   &currutmp->msgs[0], sizeof(msgque_t));
	    no_oldmsg++;
	    no_oldmsg %= MAX_REVIEW;
	    if(oldmsg_count < MAX_REVIEW)
		oldmsg_count++;
	    
	    MPROTECT_UTMP_RW;
	    for(i = 0; i < currutmp->msgcount && i < MAX_MSGS; i++)
		currutmp->msgs[i] = currutmp->msgs[i + 1];
	    MPROTECT_UTMP_R;
	} while(currutmp->msgcount);
	MPROTECT_UTMP_RW;
	currutmp->chatid[0] = c0;
	currutmp->mode = mode0;
	MPROTECT_UTMP_R;
	currstat = currstat0;
    } else {
	bell();
	show_last_call_in(1);
	memcpy(&oldmsg[(int)no_oldmsg], &currutmp->msgs[0], sizeof(msgque_t));
	no_oldmsg++;
	no_oldmsg %= MAX_REVIEW;
	if(oldmsg_count < MAX_REVIEW)
	    oldmsg_count++;
	if(watermode > 0) {
	    if(watermode < oldmsg_count)
		watermode++;
	    t_display_new();
	}
	refresh();
	MPROTECT_UTMP_RW;
	currutmp->msgcount = 0;
	MPROTECT_UTMP_R;
    }
}

static void multi_user_check() {
    register userinfo_t *ui;
    register pid_t pid;
    char genbuf[3];

    if(HAS_PERM(PERM_SYSOP))
	return;		/* don't check sysops */
    
    if(cuser.userlevel) {
	if(!(ui = search_ulist(cmpuids, usernum)))
	    return;	/* user isn't logged in */
	
	pid = ui->pid;
	if(!pid /*|| (kill(pid, 0) == -1)*/)
	    return;	/* stale entry in utmp file */
	
	getdata(b_lines - 1, 0, "您想刪除其他重複的 login (Y/N)嗎？[Y] ",
		genbuf, 3, LCECHO);

	if(genbuf[0] != 'n') {
	    kill(pid, SIGHUP);
	    log_usies("KICK ", cuser.username);
	} else {
	    if(count_multi() >= 3)
		system_abort();		/* Goodbye(); */
	}
    } else {
	/* allow multiple guest user */
	if(count_multi() > 32) {
	    outs("\n抱歉，目前已有太多 guest, 請稍後再試。\n");
	    pressanykey();
	    oflush();
	    exit(1);
	}
    }
}

/* bad login */
static char str_badlogin[] = "logins.bad";

static void logattempt(char *uid, char type) {
    char fname[40];
    int fd, len;
    char genbuf[200];
    
    sprintf(genbuf, "%c%-12s[%s] %s@%s\n", type, uid,
	    Cdate(&login_start_time), remoteusername, fromhost);
    len = strlen(genbuf);
    if((fd = open(str_badlogin, O_WRONLY | O_CREAT | O_APPEND, 0644)) > 0) {
	write(fd, genbuf, len);
	close(fd);
    }
    if(type == '-') {
	sprintf(genbuf, "[%s] %s\n", Cdate(&login_start_time), fromhost);
	len = strlen(genbuf);
	sethomefile(fname, uid, str_badlogin);
	if((fd = open(fname, O_WRONLY | O_CREAT | O_APPEND, 0644)) > 0) {
	    write(fd, genbuf, len);
	    close(fd);
	}
    }
}

extern char *str_new;
extern char *err_uid;

static void login_query() {
    char uid[IDLEN + 1], passbuf[PASSLEN];
    int attempts;
    char genbuf[200];
    resolve_utmp();
    attach_uhash();
    attempts = utmpshm->number;
    show_file("etc/Welcome", 1, -1, NO_RELOAD);
    output("1",1);
    if(attempts >= MAX_ACTIVE) {
	outs("由於人數太多，請您稍後再來。\n");
	refresh();
	exit(1);
    }

    /* hint */
    
    attempts = 0;
    while(1) {

	if(attempts++ >= LOGINATTEMPTS) {
	    more("etc/goodbye", NA);
	    pressanykey();
	    exit(1);
	}

	getdata(20, 0, "請輸入代號，或以[guest]參觀，以[new]註冊：",
		uid, IDLEN + 1, DOECHO);
	if(strcasecmp(uid, str_new) == 0) {
#ifdef LOGINASNEW
	    new_register();
	    break;
#else
	    outs("本系統目前無法以 new 註冊, 請用 guest 進入\n");
	    continue;
#endif
	} else if(uid[0] == '\0' || !dosearchuser(uid)) {
	    outs(err_uid);
	} else if(strcmp(uid, STR_GUEST)) {
	    getdata(21, 0, MSG_PASSWD, passbuf, PASSLEN, NOECHO);
	    passbuf[8] = '\0';
	    
	    if(!checkpasswd(cuser.passwd, passbuf) /* ||
	       (HAS_PERM(PERM_SYSOP) && !use_shell_login_mode)*/) {
		logattempt(cuser.userid, '-');
		outs(ERR_PASSWD);
	    } else {
		logattempt(cuser.userid, ' ');
		if(strcasecmp("SYSOP", cuser.userid) == 0)
		    cuser.userlevel = PERM_BASIC | PERM_CHAT | PERM_PAGE |
			PERM_POST | PERM_LOGINOK | PERM_MAILLIMIT |
			PERM_CLOAK | PERM_SEECLOAK | PERM_XEMPT |
			PERM_DENYPOST | PERM_BM | PERM_ACCOUNTS |
			PERM_CHATROOM | PERM_BOARD | PERM_SYSOP |
			PERM_BBSADM;
		break;
	    }
	} else {	/* guest */
	    cuser.userlevel = 0;
	    cuser.uflag = COLOR_FLAG | PAGER_FLAG | BRDSORT_FLAG | MOVIE_FLAG;
	    break;
	}
    }
    multi_user_check();
    sethomepath(genbuf, cuser.userid);
    mkdir(genbuf, 0755);
}

void add_distinct(char *fname, char *line) {
    FILE *fp;
    int n = 0;
    
    if((fp = fopen(fname, "a+"))) {
	char buffer[80];
	char tmpname[100];
	FILE *fptmp;
	
	strcpy(tmpname, fname);
	strcat(tmpname, "_tmp");
	if(!(fptmp = fopen(tmpname, "w"))) {
	    fclose(fp);
	    return;
	}
	rewind(fp);
	while(fgets(buffer, 80, fp)) {
	    char *p = buffer + strlen(buffer) - 1;

	    if(p[-1] == '\n' || p[-1] == '\r')
		p[-1] = 0;
	    if(!strcmp(buffer, line))
		break;
	    sscanf(buffer + strlen(buffer) + 2, "%d", &n);
	    fprintf(fptmp, "%s%c#%d\n", buffer, 0, n);
	}

	if(feof(fp))
	    fprintf(fptmp, "%s%c#1\n", line, 0);
	else {
	    sscanf(buffer + strlen(buffer) + 2, "%d", &n);
	    fprintf(fptmp, "%s%c#%d\n", buffer, 0, n + 1);
	    while(fgets(buffer, 80, fp)) {
		sscanf(buffer + strlen(buffer) + 2, "%d", &n);
		fprintf(fptmp, "%s%c#%d\n", buffer, 0, n);
	    }
	}
	fclose(fp);
	fclose(fptmp);
	unlink(fname);
	rename(tmpname, fname);
    }
}

void del_distinct(char *fname, char *line) {
    FILE *fp;
    int n = 0;
    
    if((fp = fopen(fname, "r"))) {
	char buffer[80];
	char tmpname[100];
	FILE *fptmp;
	
	strcpy(tmpname, fname);
	strcat(tmpname, "_tmp");
	if(!(fptmp = fopen(tmpname, "w"))) {
	    fclose(fp);
	    return;
	}
	rewind(fp);
	while(fgets(buffer, 80, fp)) {
	    char *p = buffer + strlen(buffer) - 1;
	    
	    if(p[-1] == '\n' || p[-1] == '\r')
		p[-1] = 0;
	    if(!strcmp(buffer, line))
		break;
	    sscanf(buffer + strlen(buffer) + 2, "%d", &n);
	    fprintf(fptmp, "%s%c#%d\n", buffer, 0, n);
	}

	if(!feof(fp))
	    while(fgets(buffer, 80, fp)) {
		sscanf(buffer + strlen(buffer) + 2, "%d", &n);
		fprintf(fptmp, "%s%c#%d\n", buffer, 0, n);
	    }
	fclose(fp);
	fclose(fptmp);
	unlink(fname);
	rename(tmpname, fname);
    }
}

#ifdef WHERE
static int where(char *from) {
    register int i = 0, count = 0, j;
    
    for (j = 0; j < fcache->top; j++) {
	char *token = strtok(fcache->domain[j], "&");
	
	i = 0;
	count = 0;
	while(token) {
	    if(strstr(from, token))
		count++;
	    token = strtok(NULL, "&");
	    i++;
	}
	if(i == count)
	    break;
    }
    if(i != count)
	return 0;
    return j;
}
#endif

static void check_BM() {
    int i;
    boardheader_t *bhdr;
    extern boardheader_t *bcache;
    extern int numboards;
    
    cuser.userlevel &= ~PERM_BM;
    for(i = 0, bhdr = bcache; i < numboards && !is_BM(bhdr->BM); i++, bhdr++);
}

extern pid_t currpid;
extern crosspost_t postrecord;

static void setup_utmp(int mode) {
    userinfo_t uinfo;
    char buf[80];
    char remotebuf[1024];
    time_t now = time(NULL);
    
    memset(&uinfo, 0, sizeof(uinfo));
    uinfo.pid = currpid = getpid();
    uinfo.uid = usernum;
    uinfo.mode = currstat = mode;
    uinfo.msgcount = 0;
    if(!(cuser.numlogins % 20) &&
       cuser.userlevel & PERM_BM)
	check_BM();	/* Ptt 自動取下離職板主權力 */
    
    uinfo.userlevel = cuser.userlevel;
    uinfo.lastact = time(NULL);
    
    postrecord.times = 0;	/* 計算crosspost數 */
    
    strcpy(uinfo.userid, cuser.userid);
    strcpy(uinfo.realname, cuser.realname);
    strcpy(uinfo.username, cuser.username);
    strncpy(uinfo.from, fromhost, 23);
    
    uinfo.five_win = cuser.five_win;
    uinfo.five_lose = cuser.five_lose;
    uinfo.five_tie = cuser.five_tie;
    
    uinfo.invisible = cuser.invisible % 2;
    uinfo.pager = cuser.pager % 5;
    
    uinfo.brc_id = 0;
#ifdef WHERE
    uinfo.from_alias = where(fromhost);
#else
    uinfo.from_alias = 0;
#endif
    setuserfile(buf, "remoteuser");
    
    strcpy(remotebuf, getenv("RFC931"));
    strcat(remotebuf, ctime(&now));
    remotebuf[strlen(remotebuf)-1] = 0;
    add_distinct(buf, remotebuf);
    
    if(enter_uflag & CLOAK_FLAG)
	uinfo.invisible = YEA;
    getnewutmpent(&uinfo);
#ifndef _BBS_UTIL_C_
    friend_load();
#endif
}

extern char margs[];
extern char *str_sysop;
extern char *loginview_file[NUMVIEWFILE][2];

static void user_login() {
    char ans[4], i;
    char genbuf[200];
    struct tm *ptime, *tmp;
    time_t now;
    int a;
    /*** Heat:廣告詞
	 char *ADV[17] = {
	 "記得唷!! 5/12在台大二活地下室見~~~",
	 "你知道Ptt之夜是什麼嗎? 5/12號就要上演耶 快去問吧!",
	 "5/12 Ptt之夜即將引爆 能不去嗎? 在台大二活地下室咩",
	 "不來就落伍了 啥? 就Ptt之夜啊 很棒的晚會唷 時間:5/12",
	 "差點忘了提醒你 5/12我們有約 就台大二活地下室咩!!",
	 "Ptt是啥 想知嗎? 5/12在台大二活地下室告訴你唷",
	 "來來來....5/12快到台大二活地下室去拿獎品吧~~",
	 "去去去...到台大二活地下室去 就5/12麻 有粉多獎品耶",
	 "喂喂喂 怎還楞在這!!快呼朋引伴大鬧ptt",
	 "Ptt最佳豬腳 換你幹幹看 5/12來吧....*^_^*",
	 "幹什麼幹什麼?? 你怎麼不曉得啥是Ptt之夜..老土唷",
	 "累了嗎? 讓我們來為你來一段精采表演吧.. 5/12 Ptt之夜",
	 "世紀末最屁力的晚會 就在台大二活地下室 5/12不見不散 gogo",
	 "到底誰比較帥(美) 來比比吧 5/12Ptt之夜 一較高下",       
	 "台大二活地下室 5/12 聽說會有一場很棒的晚會唷 Ptt之夜",
	 "台大二活地下室 5/12 你能不來嗎?粉多網友等著你耶",
	 "5/12 台大二活地下室 是各約網友見面的好地方呢",
	 }; 
	 char *ADV[] = {
	 "7/17 @LIVE 亂彈, 何欣穗 的 入場卷要送給 ptt 的愛用者!",
	 "欲知詳情請看 PttAct 板!!",
	 }; ***/

    log_usies("ENTER", getenv("RFC931") /* fromhost */ );
    setproctitle("%s: %s", margs, cuser.userid);
    
    /* resolve all cache */
    resolve_garbage();	/* get ptt cache */
    resolve_fcache();
    resolve_boards();
    
    /* 初始化 uinfo、flag、mode */
    setup_utmp(LOGIN);
    mysrand();		/* 初始化: random number 增加user跟時間的差異 */
    currmode = MODE_STARTED;
    enter_uflag = cuser.uflag;

    /* get local time */
    time(&now);
    ptime = localtime(&now);
    tmp = localtime(&cuser.lastlogin);
    
    if((a = utmpshm->number) > fcache->max_user) {
	sem_init(FROMSEM_KEY, &fcache_semid);
	sem_lock(SEM_ENTER, fcache_semid);
	fcache->max_user = a;
	fcache->max_time = now;
	sem_lock(SEM_LEAVE, fcache_semid);
    }
#ifdef INITIAL_SETUP
    if(!getbnum(DEFAULT_BOARD)) {
	strcpy(currboard, "尚未選定");
    } else
#endif
    {
	brc_initial(DEFAULT_BOARD);
	set_board();
    }

    /* 畫面處理開始 */
    if(!(HAS_PERM(PERM_SYSOP) && HAS_PERM(PERM_DENYPOST)))
	do_aloha("<<上站通知>> -- 我來啦！");
    if(ptime->tm_mday == cuser.day && ptime->tm_mon + 1 == cuser.month) {
	more("etc/Welcome_birth", NA);
	MPROTECT_UTMP_RW;
	currutmp->birth = 1;
	MPROTECT_UTMP_R;
    } else {
	more("etc/Welcome_login", NA);
//	pressanykey();
//    more("etc/CSIE_Week", NA);
	MPROTECT_UTMP_RW;
	currutmp->birth = 0;
	MPROTECT_UTMP_R;
    }
    
    if(cuser.userlevel) {	/* not guest */
	move(t_lines - 4, 0);
	prints("      歡迎您第 \033[1;33m%d\033[0;37m 度拜訪本站，"
	       "上次您是從 \033[1;33m%s\033[0;37m 連往本站，\n"
	       "     我記得那天是 \033[1;33m%s\033[0;37m。\n",
	       ++cuser.numlogins, cuser.lasthost, Cdate(&cuser.lastlogin));
	MPROTECT_UTMP_RW;
	currutmp->mind=rand()%8;  /* 初始心情 */
	MPROTECT_UTMP_R;
	pressanykey();
 	
	if(currutmp->birth && tmp->tm_mday != ptime->tm_mday) {
	    more("etc/birth.post", YEA);
	    brc_initial("WhoAmI");
	    set_board();
	    do_post();
	}
	setuserfile(genbuf, str_badlogin);
	if(more(genbuf, NA) != -1) {
	    getdata(b_lines - 1, 0, "您要刪除以上錯誤嘗試的記錄嗎(Y/N)?[Y]",
		    ans, 3, LCECHO);
	    if(*ans != 'n')
		unlink(genbuf);
	}
	check_register();
	strncpy(cuser.lasthost, fromhost, 16);
	cuser.lasthost[15] = '\0';
	restore_backup();
    } else if(!strcmp(cuser.userid, STR_GUEST)) {
	char *nick[13] = {
	    "椰子", "貝殼", "內衣", "寶特瓶", "翻車魚",
	    "樹葉", "浮萍", "鞋子", "潛水艇", "魔王",
	    "鐵罐", "考卷", "大美女"};
	char *name[13] = {
	    "大王椰子", "鸚鵡螺", "比基尼", "可口可樂", "仰泳的魚",
	    "憶", "高岡屋", "AIR Jordon", "紅色十月號", "批踢踢",
	    "SASAYA椰奶", "鴨蛋", "布魯克鱈魚香絲"};
	char *addr[13] = {
	    "天堂樂園", "大海", "綠島小夜曲", "美國", "綠色珊瑚礁",
	    "遠方", "原本海", "NIKE", "蘇聯", "男八618室",
	    "愛之味", "天上", "藍色珊瑚礁"};
	i = login_start_time % 13;
	sprintf(cuser.username, "海邊漂來的%s", nick[(int)i]);
	sprintf(cuser.realname, name[(int)i]);
	MPROTECT_UTMP_RW;
	sprintf(currutmp->username, cuser.username);
	sprintf(currutmp->realname, cuser.realname);
	currutmp->pager = 2;
	MPROTECT_UTMP_R;
	sprintf(cuser.address, addr[(int)i]);
	cuser.sex = i % 8;
	pressanykey();
    } else
	pressanykey();
    
    if(!PERM_HIDE(currutmp))
	cuser.lastlogin = login_start_time;
    
    reload_money();
    passwd_update(usernum, &cuser);
    
    for(i = 0; i < NUMVIEWFILE; i++)
	if((cuser.loginview >> i) & 1)
	    more(loginview_file[(int)i][0], YEA);
	
	
}

static void do_aloha(char *hello) {
    FILE *fp;
    char userid[80];
    char genbuf[200];
    
    setuserfile(genbuf, "aloha");
    if((fp = fopen(genbuf, "r"))) {
	sprintf(genbuf, hello);
	while(fgets(userid, 80, fp)) {
	    userinfo_t *uentp;
	    int tuid;
	    
	    if((tuid = searchuser(userid)) && tuid != usernum &&
	       (uentp = search_ulistn(cmpuids, tuid, 1)) &&
	       ((uentp->userlevel & PERM_SYSOP) ||
		((!currutmp->invisible ||
		  uentp->userlevel & PERM_SEECLOAK) &&
		 !(is_rejected(uentp) & 1)))) {
		my_write(uentp->pid, genbuf, uentp->userid, 2);
	    }
	}
	fclose(fp);
    }
}

static void do_term_init() {
    initscr();
}

extern char* fn_register;
extern int showansi;

static void start_client() {
    extern struct commands_t cmdlist[];
    int nreg;
    
    /* system init */
    currmode = 0;
    
    signal(SIGHUP, abort_bbs);
    signal(SIGTERM, abort_bbs);
    signal(SIGPIPE, abort_bbs);

    signal(SIGINT, abort_bbs_debug);
    signal(SIGQUIT, abort_bbs_debug);
    signal(SIGILL, abort_bbs_debug);
    signal(SIGABRT, abort_bbs_debug);
    signal(SIGFPE, abort_bbs_debug);
    signal(SIGBUS, abort_bbs_debug);
    signal(SIGSEGV, abort_bbs_debug);
    
    signal_restart(SIGUSR1, talk_request);
    signal_restart(SIGUSR2, write_request);
    
    dup2(0, 1);
    
    do_term_init();
    signal(SIGALRM, abort_bbs);
    alarm(600);
    login_query();		/* Ptt 加上login time out */
    user_login();
    m_init();
    
    if (HAS_PERM(PERM_SYSOP) && (nreg = dashs(fn_register)/163) > 100)
    {
    	char cpu_load[30];
    	if(cpuload(cpu_load) > MAX_CPULOAD*2/3)	/* DickG: 根據目前的 load 來 */
	    scan_register_form(fn_register, 1, nreg/20); /* 決定要審核的數目 */
	else
	    scan_register_form(fn_register, 1, nreg/10);
    }
    
    if(HAVE_PERM(PERM_SYSOP | PERM_BM))
	b_closepolls();
    if(!(cuser.uflag & COLOR_FLAG))
	showansi = 0;
#ifdef DOTIMEOUT
    /* init_alarm();*/ // cause strange logout with saving post.
    signal(SIGALRM, SIG_IGN);
#else
    signal(SIGALRM, SIG_IGN);
#endif
    if(chkmailbox())
	m_read();
    
    domenu(MMENU, "主功\能表", (chkmail(0) ? 'M' : 'C'), cmdlist);
}

/* FSA (finite state automata) for telnet protocol */
static void telnet_init() {
    static char svr[] = {
	IAC, DO, TELOPT_TTYPE,
	IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE,
	IAC, WILL, TELOPT_ECHO,
	IAC, WILL, TELOPT_SGA,
	IAC, DO, TELOPT_BINARY
    };
    
    register int n, len;
    register char *cmd, *data;
    int rset, oset;
    struct timeval to;
    char buf[256];
    
    data = buf;
    
    to.tv_sec = 1;
    rset = to.tv_usec = 0;
    FD_SET(0, (fd_set *) & rset);
    oset = rset;
    for(n = 0, cmd = svr; n < 5; n++) {
	len = (n == 1 ? 6 : 3);
	write(0, cmd, len);
	cmd += len;
	
	if(select(1, (fd_set *) & rset, NULL, NULL, &to) > 0)
	    read(0, data, sizeof(buf));
	rset = oset;
    }
}

/* 取得 remote user name 以判定身份                */
/*
 * rfc931() speaks a common subset of the RFC 931, AUTH, TAP, IDENT and RFC
 * 1413 protocols. It queries an RFC 931 etc. compatible daemon on a remote
 * host to look up the owner of a connection. The information should not be
 * used for authentication purposes. This routine intercepts alarm signals.
 *
 * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
 */

#define STRN_CPY(d,s,l) { strncpy((d),(s),(l)); (d)[(l)-1] = 0; }
#define RFC931_TIMEOUT   10
#define RFC931_PORT     113	/* Semi-well-known port */
#define ANY_PORT        0	/* Any old port will do */

/* timeout - handle timeouts */
static void timeout(int sig) {
    longjmp(byebye, sig);
}

static void getremotename(struct sockaddr_in *from, char *rhost, char *rname) {
    struct sockaddr_in our_sin;
    struct sockaddr_in rmt_sin;
    unsigned rmt_port, rmt_pt;
    unsigned our_port, our_pt;
    FILE *fp;
    char buffer[512], user[80], *cp;
    int s;
    static struct hostent *hp;
    
/* get remote host name */
    
    hp = NULL;
    if(setjmp(byebye) == 0) {
	signal(SIGALRM, timeout);
	alarm(3);
	hp = gethostbyaddr((char *) &from->sin_addr, sizeof(struct in_addr),
			   from->sin_family);
	alarm(0);
    }
    strcpy(rhost, hp ? hp->h_name : (char *) inet_ntoa(from->sin_addr));
    
/*
 * Use one unbuffered stdio stream for writing to and for reading from the
 * RFC931 etc. server. This is done because of a bug in the SunOS 4.1.x
 * stdio library. The bug may live in other stdio implementations, too.
 * When we use a single, buffered, bidirectional stdio stream ("r+" or "w+"
 * mode) we read our own output. Such behaviour would make sense with
 * resources that support random-access operations, but not with sockets.
 */
    
    s = sizeof(our_sin);
    if(getsockname(0, (struct sockaddr *) &our_sin, &s) < 0)
	return;
    
    if((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	return;

    if(!(fp = fdopen(s, "r+"))) {
	close(s);
	return;
    }
/* Set up a timer so we won't get stuck while waiting for the server. */
    if(setjmp(byebye) == 0) {
	signal(SIGALRM, timeout);
	alarm(RFC931_TIMEOUT);
	
/*
 * Bind the local and remote ends of the query socket to the same IP
 * addresses as the connection under investigation. We go through all
 * this trouble because the local or remote system might have more than
 * one network address. The RFC931 etc. client sends only port numbers;
 * the server takes the IP addresses from the query socket.
 */
       	our_pt = ntohs(our_sin.sin_port);
	our_sin.sin_port = htons(ANY_PORT);
	
	rmt_sin = *from;
	rmt_pt = ntohs(rmt_sin.sin_port);
	rmt_sin.sin_port = htons(RFC931_PORT);
	
	setbuf(fp, (char *) 0);
	s = fileno(fp);
	
	if(bind(s, (struct sockaddr *) &our_sin, sizeof(our_sin)) >= 0 &&
	   connect(s, (struct sockaddr *) &rmt_sin, sizeof(rmt_sin)) >= 0) {
/*
 * Send query to server. Neglect the risk that a 13-byte write would
 * have to be fragmented by the local system and cause trouble with
 * buggy System V stdio libraries.
 */
	    fprintf(fp, "%u,%u\r\n", rmt_pt, our_pt);
	    fflush(fp);
/*
 * Read response from server. Use fgets()/sscanf() so we can work
 * around System V stdio libraries that incorrectly assume EOF when a
 * read from a socket returns less than requested.
 */
	    if(fgets(buffer, sizeof(buffer), fp) && !ferror(fp) && !feof(fp) &&
	       sscanf(buffer, "%u , %u : USERID :%*[^:]:%79s",
		      &rmt_port, &our_port, user) == 3 &&
	       rmt_pt == rmt_port && our_pt == our_port) {
		
/*
 * Strip trailing carriage return. It is part of the protocol, not
 * part of the data.
 */
		if((cp = (char *)strchr(user, '\r')))
		    *cp = 0;
		strcpy(rname, user);
	    }
	}
	alarm(0);
    }
    fclose(fp);
}

static int bind_port(int port) {
    int sock, on;
    
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    on = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof(on));
    
    on = 0;
    setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *)&on, sizeof(on));
    
    xsin.sin_port = htons(port);
    if(bind(sock, (struct sockaddr *) &xsin, sizeof xsin) < 0) {
	syslog(LOG_INFO, "bbsd bind_port can't bind to %d", port);
	exit(1);
    }
    if(listen(sock, SOCKET_QLEN) < 0) {
	syslog(LOG_INFO, "bbsd bind_port can't listen to %d", port);
	exit(1);
    }
    return sock;
}

static int bad_host(char *name) {
    FILE *list;
    char buf[40];
    
    if((list = fopen("etc/bad_host", "r"))) {
	while(fgets(buf, 40, list)) {
	    buf[strlen(buf) - 1] = '\0';
	    if(!strcmp(buf, name))
		return 1;
	    if(buf[strlen(buf) - 1] == '.' && !strncmp(buf, name, strlen(buf)))
		return 1;
	    if(*buf == '.' && strlen(buf) < strlen(name) &&
	       !strcmp(buf, name + strlen(name) - strlen(buf)))
		return 1;
	}
	fclose(list);
    }
    return 0;
}


/*******************************************************/


static void shell_login(int argc, char *argv[], char *envp[]);
static void daemon_login(int argc, char *argv[], char *envp[]);
static int check_ban_and_load(int fd);
#ifdef SUPPORT_GB    
extern int current_font_type;
#endif

int main(int argc, char *argv[], char *envp[]) {
    /* avoid SIGPIPE */
    signal(SIGPIPE,SIG_IGN);
    
    /* avoid erroneous signal from other mbbsd */
    signal(SIGUSR1,SIG_IGN);
    signal(SIGUSR2,SIG_IGN);
    
    /* check if invoked as "bbs" */
    if(argc == 3)
	shell_login(argc,argv,envp);
    else
    	daemon_login(argc,argv,envp);
    
    nice(2);	/*  Ptt: lower priority */
    login_start_time = time(0);
    start_client();
    return 0;
}

static void shell_login(int argc, char *argv[], char *envp[]) {

    /* Give up root privileges: no way back from here */
    setgid(BBSGID);
    setuid(BBSUID);
    chdir(BBSHOME);

    /* mmap passwd file */
    if(passwd_mmap())
	exit(1);
    
    use_shell_login_mode=1;
    initsetproctitle(argc, argv, envp);
	
    /* copy from the original "bbs" */
    if(argc > 1) {
	strcpy(fromhost, argv[1]);

#if 0
	if(argc > 2)
	    strcpy(tty_name, argv[2]);
#endif
	if(argc > 3)
	    strcpy(remoteusername, argv[3]);
    }

    {
	char cmd[80] = "??@";

	if(!getenv("RFC931"))
	    setenv("RFC931", strcat(cmd, fromhost), 1);
    }
	
    close(2);
    InitTerminal();
    if(check_ban_and_load(0)) exit(0);
}

static void daemon_login(int argc, char *argv[], char *envp[]) {
    int msock, csock;	/* socket for Master and Child */
    FILE *fp;
    int listen_port = 23;
    int len_of_sock_addr;
    char buf[256];

    /* setup standalone */

    start_daemon();

    signal_restart(SIGCHLD, reapchild);

    /* choose port */
    if(argc == 1)
	listen_port = 3006;
    else if(argc >= 2)
	listen_port = atoi(argv[1]);

    sprintf(margs, "%s %d ", argv[0],listen_port);

    /* port binding */
    xsin.sin_family = AF_INET;
    msock = bind_port(listen_port);
    if(msock<0) {
	syslog(LOG_INFO, "mbbsd bind_port failed.\n");
	exit(1);
    }
    
    initsetproctitle(argc, argv, envp);
    setproctitle("%s: listening ", margs);
    
    /* Give up root privileges: no way back from here */
    setgid(BBSGID);
    setuid(BBSUID);
    chdir(BBSHOME);
    
    /* mmap passwd file */
    if(passwd_mmap())
	exit(1);
    
    sprintf(buf, "/var/run/mbbsd.%d.pid", listen_port);
    if((fp = fopen(buf, "w"))) {
	fprintf(fp, "%d\n", getpid());
	fclose(fp);
    }
    
    /* main loop */
    for(;;) {

	len_of_sock_addr = sizeof(xsin);
	csock = accept(msock, (struct sockaddr *)&xsin, &len_of_sock_addr);

	if(csock < 0) {
	    if(errno!=EINTR) sleep(1);
	    continue;
	}
	
	if(check_ban_and_load(csock))
	{
	    close(csock);
	    continue;
	}
	
	if(fork()==0)
	    break;
	else
	    close(csock);
    }	

    /* here is only child running */
	
    close(msock);
    dup2(csock, 0);
    close(csock);

    getremotename(&xsin, fromhost, remoteusername);		/* RFC931 */

    /* ban 掉 bad host / bad user  shiuan88: 賣補帖的 */
    if(bad_host(fromhost) || strstr(fromhost, "bbs.") ||
       strstr(fromhost, "ccsun53.cc.ntu") ||
       !strcmp(remoteusername, "shiuan88")) {
	outs("\n     抱歉, 本站謝絕由此處來的user...\n");
	refresh();
	exit(1);
    }
    {
	char RFC931[80];
	    
	sprintf(RFC931, "%s@%s", remoteusername, fromhost);
	setenv("RFC931", RFC931, 1);
    }
    telnet_init();
	
    setproctitle("%s: ..login..", margs);
}

/* check if we're banning login and if the load is too high.
   if login is permitted, return 0;
   else return -1;
   approriate message is output to fd.
*/
static int check_ban_and_load(int fd)
{
    FILE *fp;
    static char buf[256];
    static time_t chkload_time;
    static int overload;	/* overload or banned, update every 1 sec  */
    static int banned;
    
    if((time(0) - chkload_time) > 1) {
	overload = chkload(buf);
	banned = !access(BBSHOME "/BAN",R_OK) &&
	    strcmp(fromhost, "localhost") != 0;
	chkload_time = time(0);
    }

    write(fd, buf, strlen(buf));

    if(banned && (fp = fopen(BBSHOME "/BAN", "r"))) {
	while(fgets(buf, 256, fp))
	    write(fd, buf, strlen(buf));
	fclose(fp);
    }

    if(banned || overload)
	return -1;
    return 0;
}
