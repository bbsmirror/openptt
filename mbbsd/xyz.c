/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "config.h"
#include "pttstruct.h"
#include "common.h"
#include "modes.h"
#include "proto.h"

extern char *fn_note_ans;
extern int b_lines;             /* Screen bottom line number: t_lines-1 */
extern char *BBSName;
extern char fromhost[];
extern userinfo_t *currutmp;
extern int curr_idle_timeout;
extern userec_t cuser;

int x_boardman() {
    more("etc/topboardman", YEA);
    return 0;
}

int x_user100() {
    more("etc/topusr100", YEA);
    return 0;
}

int x_history() {
    more("etc/history", YEA);
    return 0;
}

int x_birth() {
    more("etc/birth.today", YEA);
    return 0;
}

int x_weather() {
    more("etc/weather.tmp", YEA);
    return 0;
}

int x_stock() {
    more("etc/stock.tmp", YEA);
    return 0;
}

int x_note() {
    more(fn_note_ans, YEA);
    return 0;
}

int x_issue() {
    more("etc/day", YEA);
    return 0;
}

int x_week() {
    more("etc/week", YEA);
    return 0;
}

int x_today() {
    more("etc/today", YEA);
    return 0;
}

int x_yesterday() {
    more("etc/yesterday", YEA);
    return 0;
}

int x_login() {
    more("etc/Welcome_login", YEA);
    return 0;
}

#ifdef HAVE_INFO
static int x_program() {
    more("etc/version", YEA);
    return 0;
}
#endif

#ifdef HAVE_LICENSE
static int x_gpl() {
    more("etc/GPL", YEA);
    return 0;
}
#endif

/* 離開 BBS 站 */
int note() {
    static char *fn_note_tmp = "note.tmp";
    static char *fn_note_dat = "note.dat";
    int total = 0, i, collect, len;
    struct stat st;
    char buf[256], buf2[80];
    int fd, fx;
    FILE *fp, *foo;

    typedef struct notedata_t {
	time_t date;
	char userid[IDLEN + 1];
	char username[19];
	char buf[3][80];
    } notedata_t;
    notedata_t myitem;

    setutmpmode(EDNOTE);
    do {
	myitem.buf[0][0] = myitem.buf[1][0] = myitem.buf[2][0] = '\0';
	move(12, 0);
	clrtobot();
	outs("\n投五銀... 嗶... 請留言 (至多三行)，按[Enter]結束");
	for(i = 0; (i < 3) && getdata(16 + i, 0, "：", myitem.buf[i], 78,
				      DOECHO) && *myitem.buf[i]; i++);
	getdata(b_lines - 1, 0, "(S)儲存 (E)重新來過 (Q)取消？[S] ",
		buf, 3, LCECHO);

	if(buf[0] == 'q' || (i == 0 && *buf != 'e'))
	    return 0;
    } while(buf[0] == 'e');
    strcpy(myitem.userid, cuser.userid);
    strncpy(myitem.username, cuser.username, 18);
    myitem.username[18] = '\0';
    time(&(myitem.date));

    /* begin load file */
    if((foo = fopen(".note", "a")) == NULL)
	return 0;

    if((fp = fopen(fn_note_ans, "w")) == NULL)
	return 0;

    if((fx = open(fn_note_tmp, O_WRONLY | O_CREAT, 0644)) <= 0)
	return 0;

    if((fd = open(fn_note_dat, O_RDONLY)) == -1)
	total = 1;
    else if(fstat(fd, &st) != -1) {
	total = st.st_size / sizeof(notedata_t) + 1;
	if (total > MAX_NOTE)
	    total = MAX_NOTE;
    }

    fputs("\033[1;31;44m☉┬──────────────┤"
	  "\033[37m酸甜苦辣板\033[31m├──────────────┬☉"
	  "\033[m\n", fp);
    collect = 1;

    while(total) {
	sprintf(buf, "\033[1;31m╭┤\033[32m %s \033[37m(%s)",
		myitem.userid, myitem.username);
	len = strlen(buf);

	for(i = len ; i < 73; i++)
	    strcat(buf, " ");
	sprintf(buf2, " \033[1;36m%.14s\033[31m   ├╮\033[m\n",
		Cdate(&(myitem.date)));
	strcat(buf, buf2);
	fputs(buf, fp);
	if(collect)
	    fputs(buf, foo);
	for(i = 0 ; i < 3 && *myitem.buf[i]; i++) {
            fprintf(fp, "\033[1;31m│\033[m%-74.74s\033[1;31m│\033[m\n",
		    myitem.buf[i]);
            if(collect)
		fprintf(foo, "\033[1;31m│\033[m%-74.74s\033[1;31m│\033[m\n",
			myitem.buf[i]);
        }
	fputs("\033[1;31m╰┬───────────────────────"
	      "────────────┬╯\033[m\n",fp);

	if(collect) {
	    fputs("\033[1;31m╰┬─────────────────────"
		  "──────────────┬╯\033[m\n", foo);
	    fclose(foo);
	    collect = 0;
	}
	
	write(fx, &myitem, sizeof(myitem));
	
	if(--total)
	    read(fd, (char *) &myitem, sizeof(myitem));
    }
    fputs("\033[1;31;44m☉┴───────────────────────"
	  "────────────┴☉\033[m\n",fp);
    fclose(fp);
    close(fd);
    close(fx);
    Rename(fn_note_tmp, fn_note_dat);
    more(fn_note_ans, YEA);
    return 0;
}

static void mail_sysop() {
    FILE *fp;
    char genbuf[200];
    
    if((fp = fopen("etc/sysop", "r"))) {
	int i, j;
	char *ptr;
	
	typedef struct sysoplist_t {
	    char userid[IDLEN + 1];
	    char duty[40];
	} sysoplist_t;
	sysoplist_t sysoplist[9];
	
	j = 0;
	while(fgets(genbuf, 128, fp)) {
	    if((ptr = strchr(genbuf, '\n'))) {
		*ptr = '\0';
		if((ptr = strchr(genbuf, ':'))) {
		    *ptr = '\0';
		    do {
			i = *++ptr;
		    } while(i == ' ' || i == '\t');
		    if(i) {
			strcpy(sysoplist[j].userid, genbuf);
			strcpy(sysoplist[j++].duty, ptr);
		    }
		}
	    }
	}
	
	move(12, 0);
	clrtobot();
	prints("%16s   %-18s權責劃分\n\n", "編號", "站長 ID");

	for(i = 0; i < j; i++)
	    prints("%15d.   \033[1;%dm%-16s%s\033[0m\n",
		   i + 1, 31 + i % 7, sysoplist[i].userid, sysoplist[i].duty);
	prints("%-14s0.   \033[1;%dm離開\033[0m", "", 31 + j % 7);
	getdata(b_lines - 1, 0, "                   請輸入代碼[0]：",
		genbuf, 4, DOECHO);
	i = genbuf[0] - '0' - 1;
	if(i >= 0 && i < j) {
	    clear();
	    do_send(sysoplist[i].userid, NULL);
	}
    }
}

int m_sysop() {
    setutmpmode(MSYSOP);
    mail_sysop();
    return 0;
}

int Goodbye() {
    extern void movie();
    char genbuf[100];

    getdata(b_lines - 1, 0, "您確定要離開【 " BBSNAME " 】嗎(Y/N)？[N] ",
	    genbuf, 3, LCECHO);

    if(*genbuf != 'y')
	return 0;

    movie(999);
    if(cuser.userlevel) {
	getdata(b_lines - 1, 0,
		"(G)隨風而逝 (M)托夢站長 (N)酸甜苦辣流言版？[G] ",
		genbuf, 3, LCECHO);
	if(genbuf[0] == 'm')
	    mail_sysop();
	else if(genbuf[0] == 'n')
	    note();
    }
    
    clear();
    prints("\033[1;36m親愛的 \033[33m%s(%s)\033[36m，別忘了再度光臨\033[45;33m"
	   " %s \033[40;36m！\n以下是您在站內的註冊資料:\033[0m\n",
	   cuser.userid, cuser.username, BBSName);
    user_display(&cuser, 0);
    pressanykey();
    
    more("etc/Logout",NA);
    pressanykey();
    u_exit("EXIT ");
    
    exit(0);
}

