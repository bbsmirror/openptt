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

#ifdef HAVE_X_BOARDS
static int x_boards() {
    more("etc/topboard.tmp", YEA);
    return 0;
}
#endif

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

/* ���} BBS �� */
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

    if(cuser.money < 5) {
	outmsg("\033[1;41m �u�r! �n�뤭�Ȥ~��d��...�S���C..\033[m");
	clrtoeol();
	refresh();
	return 0;
    }

    setutmpmode(EDNOTE);
    do {
	myitem.buf[0][0] = myitem.buf[1][0] = myitem.buf[2][0] = '\0';
	move(12, 0);
	clrtobot();
	outs("\n�뤭��... ��... �Яd�� (�ܦh�T��)�A��[Enter]����");
	for(i = 0; (i < 3) && getdata(16 + i, 0, "�G", myitem.buf[i], 78,
				      DOECHO) && *myitem.buf[i]; i++);
	getdata(b_lines - 1, 0, "(S)�x�s (E)���s�ӹL (Q)�����H[S] ",
		buf, 3, LCECHO);

	if(buf[0] == 'q' || (i == 0 && *buf != 'e'))
	    return 0;
    } while(buf[0] == 'e');
    demoney(5);
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

    fputs("\033[1;31;44m��s�w�w�w�w�w�w�w�w�w�w�w�w�w�w�t"
	  "\033[37m�Ĳ��W���O\033[31m�u�w�w�w�w�w�w�w�w�w�w�w�w�w�w�s��"
	  "\033[m\n", fp);
    collect = 1;

    while(total) {
	sprintf(buf, "\033[1;31m�~�t\033[32m %s \033[37m(%s)",
		myitem.userid, myitem.username);
	len = strlen(buf);

	for(i = len ; i < 73; i++)
	    strcat(buf, " ");
	sprintf(buf2, " \033[1;36m%.14s\033[31m   �u��\033[m\n",
		Cdate(&(myitem.date)));
	strcat(buf, buf2);
	fputs(buf, fp);
	if(collect)
	    fputs(buf, foo);
	for(i = 0 ; i < 3 && *myitem.buf[i]; i++) {
            fprintf(fp, "\033[1;31m�x\033[m%-74.74s\033[1;31m�x\033[m\n",
		    myitem.buf[i]);
            if(collect)
		fprintf(foo, "\033[1;31m�x\033[m%-74.74s\033[1;31m�x\033[m\n",
			myitem.buf[i]);
        }
	fputs("\033[1;31m���s�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w"
	      "�w�w�w�w�w�w�w�w�w�w�w�w�s��\033[m\n",fp);

	if(collect) {
	    fputs("\033[1;31m���s�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w"
		  "�w�w�w�w�w�w�w�w�w�w�w�w�w�w�s��\033[m\n", foo);
	    fclose(foo);
	    collect = 0;
	}
	
	write(fx, &myitem, sizeof(myitem));
	
	if(--total)
	    read(fd, (char *) &myitem, sizeof(myitem));
    }
    fputs("\033[1;31;44m��r�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w"
	  "�w�w�w�w�w�w�w�w�w�w�w�w�r��\033[m\n",fp);
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
	prints("%16s   %-18s�v�d����\n\n", "�s��", "���� ID");

	for(i = 0; i < j; i++)
	    prints("%15d.   \033[1;%dm%-16s%s\033[0m\n",
		   i + 1, 31 + i % 7, sysoplist[i].userid, sysoplist[i].duty);
	prints("%-14s0.   \033[1;%dm���}\033[0m", "", 31 + j % 7);
	getdata(b_lines - 1, 0, "                   �п�J�N�X[0]�G",
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

    getdata(b_lines - 1, 0, "�z�T�w�n���}�i " BBSNAME " �j��(Y/N)�H[N] ",
	    genbuf, 3, LCECHO);

    if(*genbuf != 'y')
	return 0;

    movie(999);
    if(cuser.userlevel) {
	getdata(b_lines - 1, 0,
		"(G)�H���ӳu (M)���گ��� (N)�Ĳ��W���y�����H[G] ",
		genbuf, 3, LCECHO);
	if(genbuf[0] == 'm')
	    mail_sysop();
	else if(genbuf[0] == 'n')
	    note();
    }
    
    clear();
    prints("\033[1;36m�˷R�� \033[33m%s(%s)\033[36m�A�O�ѤF�A�ץ��{\033[45;33m"
	   " %s \033[40;36m�I\n�H�U�O�z�b���������U���:\033[0m\n",
	   cuser.userid, cuser.username, BBSName);
    user_display(&cuser, 0);
    pressanykey();
    
    more("etc/Logout",NA);
    pressanykey();
    u_exit("EXIT ");
    
    exit(0);
}

/* �䴩�~���{�� : tin�Bgopher�Bwww�Bbbsnet�Bgame�Bcsh */
#define LOOKFIRST       (0)
#define LOOKLAST        (1)
#define QUOTEMODE       (2)
#define MAXCOMSZ        (1024)
#define MAXARGS         (40)
#define MAXENVS         (20)
#define BINDIR          BBSHOME"/bin/"

#define MAXPATHLEN 256

#ifdef HAVE_TIN
static int x_tin() {
    clear();
    return exec_cmd(NEWS, YEA, "bin/tin.sh", "TIN");
}
#endif

#ifdef HAVE_GOPHER
static int x_gopher() {
    clear();
    return exec_cmd(GOPHER, YEA, "bin/gopher.sh", "GOPHER");
}
#endif

#ifdef HAVE_WWW
static int x_www() {
    return exec_cmd(WWW, NA, "bin/www.sh", "WWW");
}
#endif

#ifdef HAVE_IRC
static int x_irc() {
    return exec_cmd(XMODE, NA, "bin/irc.sh", "IRC");
}
#endif

#ifdef HAVE_ARCHIE
static int x_archie() {
    char buf[STRLEN], ans[4];
    char genbuf1[100], genbuf2[200];
    char *s;

    setutmpmode(ARCHIE);
    clear();
    outs("\n�w����{�i\033[1;33;44m" BBSNAME "\033[m�j�ϥ� "
	 "\033[32mARCHIE\033[m �\\��\n");
    outs("\n���\\��N���z�C�X�b���� FTP ���s���z���M�䪺�ɮ�.\n");
    outs("\n�п�J���j�M���r��, �Ϊ����� <ENTER> �����C\n");
    outs("\n                            coder by Harimau\n");
    outs("                              modified by Leeym\n");
    getdata(13,0,"�j�M�r��G",buf,20,DOECHO,0);
    if(buf[0]=='\0') {
	prints("\n�����j�M.....\n");
	pressanykey();
	return;
    }

    for(s = buf; *s != '\0'; s++) {
        if(isspace(*s)) {
            prints("\n�@���u��j�M�@�Ӧr���, ����ӳg�߳�!!");
            pressanykey();
            return;
	}
    }
    bbssetenv("ARCHIESTRING", buf);
    exec_cmd(ARCHIE, YEA, "bin/archie.sh", ARCHIE);
    log_usies("ARCHIE", "");
    strcpy(genbuf1, buf);
    sprintf(buf, BBSHOME "/tmp/archie.%s", cuser.userid);
    if(dashf(buf)) {
	getdata(0, 0, "�n�N���G�H�^�H�c��(Y/N)�H[N]", ans, 3, DOECHO,0);
	if(*ans == 'y') {
	    fileheader_t mhdr;
	    char title[128], buf1[80];
	    FILE* fp;
	    
	    sethomepath(buf1, cuser.userid);
	    stampfile(buf1, &mhdr);
	    strcpy(mhdr.owner, cuser.userid);
	    sprintf(genbuf2, "Archie �j�M�ɮ�: %s ���G", genbuf1);
	    strcpy(mhdr.title, genbuf2);
	    mhdr.savemode = 0;
	    mhdr.filemode = 0;
	    sethomedir(title, cuser.userid);
	    append_record(title, &mhdr, sizeof(mhdr));
	    Link(buf, buf1);
	}
	more( buf, YEA);
	unlink (buf);
    }
}
#endif                          /* HAVE_ARCHIE */