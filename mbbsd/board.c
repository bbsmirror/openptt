/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"
#include "struct.h"
#include "perm.h"
#include "modes.h"
#include "common.h"
#include "proto.h"

#define BRC_STRLEN 15             /* Length of board name */
#define BRC_MAXSIZE     24576
#define BRC_ITEMSIZE    (BRC_STRLEN + 1 + BRC_MAXNUM * sizeof( int ))
#define BRC_MAXNUM      80

static char *brc_getrecord(char *ptr, char *name, int *pnum, int *list) {
    int num;
    char *tmp;

    strncpy(name, ptr, BRC_STRLEN);
    ptr += BRC_STRLEN;
    num = (*ptr++) & 0xff;
    tmp = ptr + num * sizeof(int);
    if (num > BRC_MAXNUM)
	num = BRC_MAXNUM;
    *pnum = num;
    memcpy(list, ptr, num * sizeof(int));
    return tmp;
}

static time_t brc_expire_time;

static char *brc_putrecord(char *ptr, char *name, int num, int *list) {
    if(num > 0 && list[0] > brc_expire_time) {
	if (num > BRC_MAXNUM)
	    num = BRC_MAXNUM;

	while(num > 1 && list[num - 1] < brc_expire_time)
	    num--;

	strncpy(ptr, name, BRC_STRLEN);
	ptr += BRC_STRLEN;
	*ptr++ = num;
	memcpy(ptr, list, num * sizeof(int));
	ptr += num * sizeof(int);
    }
    return ptr;
}

extern userec_t cuser;
extern char currboard[];        /* name of currently selected board */
static int brc_changed = 0;
static char brc_buf[BRC_MAXSIZE];
int brc_num;
static char brc_name[BRC_STRLEN];
int brc_list[BRC_MAXNUM];
static char *fn_boardrc = ".boardrc";
static int brc_size;

void brc_update() {
    if(brc_changed && cuser.userlevel) {
	char dirfile[STRLEN], *ptr;
	char tmp_buf[BRC_MAXSIZE - BRC_ITEMSIZE], *tmp;
	char tmp_name[BRC_STRLEN];
	int tmp_list[BRC_MAXNUM], tmp_num;
	int fd, tmp_size;

	ptr = brc_buf;
	if(brc_num > 0)
	    ptr = brc_putrecord(ptr, brc_name, brc_num, brc_list);

	setuserfile(dirfile, fn_boardrc);
	if((fd = open(dirfile, O_RDONLY)) != -1) {
	    tmp_size = read(fd, tmp_buf, sizeof(tmp_buf));
	    close(fd);
	} else {
	    tmp_size = 0;
	}

	tmp = tmp_buf;
	while(tmp < &tmp_buf[tmp_size] && (*tmp >= ' ' && *tmp <= 'z')) {
	    tmp = brc_getrecord(tmp, tmp_name, &tmp_num, tmp_list);
	    if(strncmp(tmp_name, currboard, BRC_STRLEN))
		ptr = brc_putrecord(ptr, tmp_name, tmp_num, tmp_list);
	}
	brc_size = (int)(ptr - brc_buf);

	if((fd = open(dirfile, O_WRONLY | O_CREAT, 0644)) != -1) {
	    ftruncate(fd, 0);
	    write(fd, brc_buf, brc_size);
	    close(fd);
	}
	brc_changed = 0;
    }
}

static void read_brc_buf() {
    char dirfile[STRLEN];
    int fd;

    if(brc_buf[0] == '\0') {
	setuserfile(dirfile, fn_boardrc);
	if((fd = open(dirfile, O_RDONLY)) != -1) {
	    brc_size = read(fd, brc_buf, sizeof(brc_buf));
	    close(fd);
	} else {
	    brc_size = 0;
	}
    }
}

extern int currbid;
extern unsigned int currbrdattr;
extern boardheader_t *bcache;

int brc_initial(char *boardname) {
    char *ptr;

    if(strcmp(currboard, boardname) == 0) {
	return brc_num;
    }

    brc_update();
    strcpy(currboard, boardname);
    currbid = getbnum(currboard);
    currbrdattr = bcache[currbid - 1].brdattr;
    read_brc_buf();

    ptr = brc_buf;
    while(ptr < &brc_buf[brc_size] && (*ptr >= ' ' && *ptr <= 'z')) {
	ptr = brc_getrecord(ptr, brc_name, &brc_num, brc_list);
	if (strncmp(brc_name, currboard, BRC_STRLEN) == 0)
	    return brc_num;
    }
    strncpy(brc_name, boardname, BRC_STRLEN);
    brc_num = brc_list[0] = 1;
    return 0;
}

void brc_addlist(char *fname) {
    int ftime, n, i;

    if(!cuser.userlevel)
	return;

    ftime = atoi(&fname[2]);
    if(ftime <= brc_expire_time
	/* || fname[0] != 'M' || fname[1] != '.' */ ) {
	return;
    }
    if(brc_num <= 0) {
	brc_list[brc_num++] = ftime;
	brc_changed = 1;
	return;
    }
    if((brc_num == 1) && (ftime < brc_list[0]))
	return;
    for(n = 0; n < brc_num; n++) {
	if(ftime == brc_list[n]) {
	    return;
	} else if(ftime > brc_list[n]) {
	    if(brc_num < BRC_MAXNUM)
		brc_num++;
	    for(i = brc_num - 1; --i >= n; brc_list[i + 1] = brc_list[i]);
	    brc_list[n] = ftime;
	    brc_changed = 1;
	    return;
	}
    }
    if(brc_num < BRC_MAXNUM) {
	brc_list[brc_num++] = ftime;
	brc_changed = 1;
    }
}

static int brc_unread_time(time_t ftime, int bnum, int *blist) {
    int n;

    if(ftime <= brc_expire_time )
	return 0;

    if(brc_num <= 0)
	return 1;
    for(n = 0; n < bnum; n++) {
	if(ftime > blist[n])
	    return 1;
	else if(ftime == blist[n])
	    return 0;
    }
    return 0;
}

int brc_unread(char *fname, int bnum, int *blist) {
    int ftime, n;

    ftime = atoi(&fname[2]);

    if(ftime <= brc_expire_time )
	return 0;

    if(brc_num <= 0)
	return 1;
    for(n = 0; n < bnum; n++) {
	if(ftime > blist[n])
	    return 1;
	else if(ftime == blist[n])
	    return 0;
    }
    return 0;
}

typedef struct {
    int pos, *total;
    time_t *lastposttime, bupdate;
    char *name, *title, *BM;
    unsigned char unread, zap, bvote;
    unsigned int brdattr;
    int uid, gid;
} boardstat_t;

extern time_t login_start_time;
extern int numboards;
static char *str_bbsrc = ".bbsrc";
static int *zapbuf;
static boardstat_t *nbrd;

static void load_zapbuf() {
    register int n, size;
    char fname[60];

    /* MAXBOARDS ==> ¦Ü¦h¬Ý±o¨£ 4 ­Ó·sªO */
    n = numboards + 4;
    size = n * sizeof(int);
    zapbuf = (int *) malloc(size);
    while(n)
	zapbuf[--n] = login_start_time;
    setuserfile(fname, str_bbsrc);
    if((n = open(fname, O_RDONLY, 0600)) != -1) {
	read(n, zapbuf, size);
	close(n);
    }
    if(!nbrd)
	nbrd = (boardstat_t *)malloc(MAX_BOARD * sizeof(boardstat_t));
    brc_expire_time = login_start_time - 365 * 86400;
}

static void save_zapbuf() {
    register int fd, size;
    char fname[60];

    setuserfile(fname, str_bbsrc);
    if((fd = open(fname, O_WRONLY | O_CREAT, 0600)) != -1) {
	size = numboards * sizeof(int);
	write(fd, zapbuf, size);
	close(fd);
    }
}

extern char *fn_visable;

int Ben_Perm(boardheader_t *bptr) {
    register int level,brdattr;
    register char *ptr;
    char buf[64];

    level = bptr->level;
    brdattr = bptr->brdattr;

    if(HAS_PERM(PERM_SYSOP))
	return 1;

    ptr = bptr->BM;
    if(is_BM(ptr))
	return 1;

    /* ¯¦±K¬ÝªO¡G®Ö¹ï­º®uªO¥Dªº¦n¤Í¦W³æ */

    if(brdattr & BRD_HIDE) { /* ÁôÂÃ */
	setbfile(buf, bptr->brdname, fn_visable);
	if(!belong(buf, cuser.userid)) {
	    if(brdattr & BRD_POSTMASK)
		return 0;
	    else
		return 2;
	} else
	    return 1;
    }
    /* ­­¨î¾\ÅªÅv­­ */
    if(level && !(brdattr & BRD_POSTMASK) && !HAS_PERM(level))
	return 0;

    return 1;
}

extern char currauthor[];
extern int b_lines;
extern char currowner[];

static int have_author(char* brdname) {
    char dirname[100];

    sprintf(dirname, "¥¿¦b·j´M§@ªÌ[33m%s[m ¬ÝªO:[1;33m%s[0m.....",
	    currauthor,brdname);
    move(b_lines, 0);
    clrtoeol();
    outs(dirname);
    refresh();

    setbdir(dirname, brdname);
    str_lower(currowner, currauthor);

    return search_rec(dirname, cmpfowner);
}

static int check_newpost(boardstat_t *ptr) { /* Ptt §ï */
    fileheader_t fh;
    int tbrc_list[BRC_MAXNUM], tbrc_num;
    char bname[BRC_STRLEN];
    struct stat st;
    char fname[FNLEN];
    char genbuf[200], *po;
    int fd, total;
    time_t ftime;

    ptr->unread = 0;
    if(ptr->brdattr & BRD_GROUPBOARD)
	return 0;

    if(*(ptr->total) == 0) {
	setbdir(genbuf, ptr->name);
	if((fd = open(genbuf, O_RDWR)) < 0)
	    return 0;

	fstat(fd, &st);                /* ¥Î¨ì³o­Ófstat «Ü¤£¦n */
	total = st.st_size / sizeof(fh);  /* ºâ¥þ³¡¦³´X½g */
	if(total <= 0) {
	    close(fd);
	    return 0;
	}
	close(fd);
	*(ptr->total) = total;
    } else
	total = *(ptr->total) ;

    if(*(ptr->lastposttime) == 0) {
	setbdir(genbuf, ptr->name);
	if((fd = open(genbuf, O_RDWR)) < 0)
	    return 0;
	lseek(fd, (off_t) (total - 1) * sizeof(fh), SEEK_SET);
	if(read(fd, fname, FNLEN) <= 0) {
	    close(fd);
	    return 1;
        }
	*(ptr->lastposttime) = ftime = (time_t) atoi(&fname[2]);
	close(fd);
    } else {
	ftime = *(ptr->lastposttime) ;
    }

    read_brc_buf();
    po = brc_buf;
    while(po < &brc_buf[brc_size] && (*po >= ' ' && *po <= 'z')) {
	po = brc_getrecord(po, bname, &tbrc_num, tbrc_list);
	if(strncmp(bname, ptr->name, BRC_STRLEN) == 0) {
	    if(brc_unread_time(ftime,tbrc_num,tbrc_list)) {
		ptr->unread = 1;
            }
	    return 1;
	}
    }
    ptr->unread = 1;
    return 1;
}

extern int currmode;
extern struct bcache_t *brdshm;
static int brdnum;
int class_uid = -1;
static int yank_flag = 0;

static void load_boards() {
    boardheader_t *bptr;
    boardstat_t *ptr;
/*
    char brdclass[5];
*/
    register int n;
    register char state ;

    if(!zapbuf)
	load_zapbuf();

    brdnum = 0;
    for(n = 0; n < numboards; n++) {
	bptr = &bcache[n];
	if(bptr->brdname[0] == '\0')
	    continue;
	if(class_uid != -1) {
	    if(bptr->gid != class_uid)
		continue;
	} else {
	    if(bptr->brdattr & BRD_GROUPBOARD) continue;
	}
	if(((state = Ben_Perm(bptr)) || (currmode & MODE_MENU)) &&
	   (yank_flag == 1 || (yank_flag == 2 &&
			       ((bptr->brdattr & BRD_GROUPBOARD ||
				 have_author(bptr->brdname)))) ||
	    (yank_flag != 2 && zapbuf[n]))) {

	    ptr = &nbrd[brdnum++];
	    ptr->total = &(brdshm->total[n]);
	    ptr->lastposttime = &(brdshm->lastposttime[n]);
	    ptr->name = bptr->brdname;
	    ptr->title = bptr->title;
	    ptr->BM = bptr->BM;
	    ptr->pos = n;
	    ptr->bvote = bptr->bvote;
	    ptr->zap = (zapbuf[n] == 0);
	    ptr->brdattr =  bptr->brdattr;
	    ptr->uid = bptr->uid;
	    ptr->gid = bptr->gid;
	    ptr->bupdate = bptr->bupdate;
/*
  if(bptr->level && !(bptr->brdattr & BRD_POSTMASK)
  && !(bptr->brdattr & BRD_GROUPBOARD))
  ptr->brdattr |= BRD_HIDE;
*/
	    if((bptr->brdattr & BRD_HIDE) && state == 1)
		ptr->brdattr |= BRD_POSTMASK;
	    check_newpost(ptr);
	}
    }

    /* ¦pªG user ±N©Ò¦³ boards ³£ zap ±¼¤F */
    if(!brdnum && class_uid == -1)
      {
	if(yank_flag == 0)
	    yank_flag = 1;
	else if (yank_flag == 2)
	    yank_flag = 0;
      }
}

static int search_board(int num) {
    char genbuf[IDLEN + 2];
    move(0, 0);
    clrtoeol();
    CreateNameList();
    for(num = 0; num < brdnum; num++)
	AddNameList(nbrd[num].name);
    namecomplete(MSG_SELECT_BOARD, genbuf);
    for (num = 0; num < brdnum; num++)
        if (!strcmp(nbrd[num].name, genbuf))
	    return num;
    return -1;
}

static int unread_position(char *dirfile, boardstat_t *ptr) {
    fileheader_t fh;
    char fname[FNLEN];
    register int num, fd, step, total;

    total = *(ptr->total);
    num = total + 1;
    if(ptr->unread && (fd = open(dirfile, O_RDWR)) > 0) {
	if(!brc_initial(ptr->name)) {
	    num = 1;
	} else {
	    num = total - 1;
	    step = 4;
	    while(num > 0) {
		lseek(fd, (off_t)(num * sizeof(fh)), SEEK_SET);
		if(read(fd, fname, FNLEN) <= 0 ||
		   !brc_unread(fname,brc_num,brc_list))
		    break;
		num -= step;
		if(step < 32)
		    step += step >> 1;
	    }
	    if(num < 0)
		num = 0;
	    while(num < total) {
		lseek(fd, (off_t)(num * sizeof(fh)), SEEK_SET);
		if(read(fd, fname, FNLEN) <= 0 ||
		   brc_unread(fname,brc_num,brc_list))
		    break;
		num++;
	    }
	}
	close(fd);
    }
    if(num < 0)
	num = 0;
    return num;
}

static void brdlist_foot() {
    prints("\033[34;46m  ¿ï¾Ü¬ÝªO  \033[31;47m  (c)\033[30m·s¤å³¹¼Ò¦¡  "
	   "\033[31m(v/V)\033[30m¼Ð°O¤wÅª/¥¼Åª  \033[31m(y)\033[30m¿z¿ï%s"
	   "  \033[31m(z)\033[30m¤Á´«¿ï¾Ü  \033[m",
	   yank_flag ? "³¡¥÷" : "¥þ³¡");
}

extern unsigned int currstat;
extern char *BBSName;

static void show_brdlist(int head, int clsflag, int newflag) {
    int myrow = 2;
    if(class_uid == 0) {
	currstat = CLASS;
	myrow = 6;
	showtitle("¤ÀÃþ¬ÝªO", BBSName);
	movie(0);
	move(1, 0);
	prints(
	    "                                                              "
	    "¢©  ¢~¡X\033[33m¡´\n"
	    "                                                    ùá¡X  \033[m "
	    "¢¨¢i\033[47m¡ó\033[40m¢i¢i¢©ùç\n");
	prints(
	    "  \033[44m   ¡s¡s¡s¡s¡s¡s¡s¡s                               "
	    "\033[33mùø\033[m\033[44m ¢©¢¨¢i¢i¢i¡¿¡¿¡¿ùø \033[m\n"
	    "  \033[44m                                                  "
	    "\033[33m  \033[m\033[44m ¢«¢ª¢i¢i¢i¡¶¡¶¡¶ ùø\033[m\n"
	    "                                  ¡s¡s¡s¡s¡s¡s¡s¡s    \033[33m"
	    "¢x\033[m   ¢ª¢i¢i¢i¢i¢« ùø\n"
	    "                                                      \033[33mùó"
	    "¡X¡X\033[m  ¢«      ¡X¡Ï\033[m");
    } else if (clsflag) {
	showtitle("¬ÝªO¦Cªí", BBSName);
	prints("[¡ö]¥D¿ï³æ [¡÷]¾\\Åª [¡ô¡õ]¿ï¾Ü [y]¸ü¤J [S]±Æ§Ç [/]·j´M "
	       "[TAB]¤åºK¡E¬ÝªO [h]¨D§U\n"
	       "\033[7m%-20s Ãþ§O Âà«H%-31s§ë²¼ ªO    ¥D     \033[m",
	       newflag ? "Á`¼Æ ¥¼Åª ¬Ý  ªO" : "  ½s¸¹  ¬Ý  ªO",
	       "  ¤¤   ¤å   ±Ô   ­z");
	move(b_lines, 0);
	brdlist_foot();
    }

    if(brdnum > 0) {
	boardstat_t *ptr;
	static char *color[8]={"","\033[32m",
			       "\033[33m","\033[36m","\033[34m","\033[1m",
			       "\033[1;32m","\033[1;33m"};
	static char *unread[2]={"\033[37m  \033[m","\033[1;31m£¾\033[m"};
	
	while(++myrow < b_lines) {
	    move(myrow, 0);
	    clrtoeol();
	    if(head < brdnum) {
		ptr = &nbrd[head++];
		if(class_uid == 0)
		    prints("          ");
		if(yank_flag == 2)
		    prints("%5d%c%c ", head,ptr->brdattr & BRD_HIDE ? ')':' ',
			   ptr->brdattr & BRD_GROUPBOARD ? ' ':'A');
		else if(!newflag) {
		    prints("%5d%c%s", head,
			   !(ptr->brdattr & BRD_HIDE) ? ' ':
			   (ptr->brdattr & BRD_POSTMASK) ? ')' : '-',
			   ptr->zap ? "- " :
			   (ptr->brdattr & BRD_GROUPBOARD) ? "  " :
			   unread[ptr->unread]);
		} else if(ptr->zap) {
		    ptr->unread = 0;
		    outs("   ¡ß ¡ß");
		} else {
		    if(newflag) {
		    	if((ptr->brdattr & BRD_GROUPBOARD))
		    	    prints("        ");
		    	else
			    prints("%6d%s", (int)(*(ptr->total)), unread[ptr->unread]);
		    }
		}
		if(class_uid != 0) {
		    prints("%-13s%s%5.5s\033[37m%2.2s\033[m%-35.35s%c %.13s",
			   ptr->name,
			   color[(unsigned int)
				(ptr->title[1] + ptr->title[2] +
				 ptr->title[3] + ptr->title[0]) & 07],
			   ptr->title, ptr->title+5, ptr->title+7,
			   (ptr->brdattr & BRD_BAD) ? 'X' : " ARBCDEFGHI"[ptr->bvote],
			   ptr->BM);
		    refresh();
		} else {
		    prints("%-40.40s %.13s", ptr->title + 7, ptr->BM);
		}
	    }
	    clrtoeol();
	}
    }
}

static char *choosebrdhelp[] = {
    "\0¬ÝªO¿ï³æ»²§U»¡©ú",
    "\01°ò¥»«ü¥O",
    "(p)(¡ô)        ¤W¤@­Ó¬ÝªO",
    "(n)(¡õ)        ¤U¤@­Ó¬ÝªO",
    "(P)(^B)(PgUp)  ¤W¤@­¶¬ÝªO",
    "(N)(^F)(PgDn)  ¤U¤@­¶¬ÝªO",
    "($)            ³Ì«á¤@­Ó¬ÝªO",
    "(¼Æ¦r)         ¸õ¦Ü¸Ó¶µ¥Ø",
    "\01¶i¶¥«ü¥O",
    "(TAB)          ¬ÝªO/¤åºK ¼Ò¦¡¤Á´«",
    "(r)(¡÷)(Rtn)   ¶i¤J¦h¥\\¯à¾\\Åª¿ï³æ",
    "(q)(¡ö)        ¦^¨ì¥D¿ï³æ",
    "(z)(Z)         ­q¾\\/¤Ï­q¾\\¬ÝªO ­q¾\\/¤Ï­q¾\\©Ò¦³¬ÝªO",
    "(y)            ¦C¥X/¤£¦C¥X©Ò¦³¬ÝªO",
    "(v/V)          ³q³q¬Ý§¹/¥þ³¡¥¼Åª",
    "(S)            «ö·Ó¦r¥À/¤ÀÃþ±Æ§Ç",
    "(/)            ·j´M¬ÝªO",
    "\01¤p²Õªø«ü¥O",
    "(E/W)          ³]©w¬ÝªO/³]©w¤p²Õ³Æ§Ñ",
    "(B)            ¶}·s¬ÝªO",
    NULL
};

/* ®Ú¾Ú title ©Î name °µ sort */
static int cmpboard(boardstat_t *brd, boardstat_t *tmp) {
    register int type = 0;

    if(!type) {
	type = strncmp(brd->title, tmp->title, 4);
	type *= 256;
	type += strcasecmp(brd->name, tmp->name);
    }
    if(!(cuser.uflag & BRDSORT_FLAG))
	type = strcasecmp(brd->name, tmp->name);
    return type;
}

static void set_menu_BM(char *BM) {
    if(HAS_PERM(PERM_ALLBOARD) || is_BM(BM)) {
	currmode |= MODE_MENU;
	cuser.userlevel |= PERM_SYSSUBOP;
    }
}

extern int p_lines;             /* a Page of Screen line numbers: tlines-4 */
extern int t_lines;
extern char *fn_notes;
static char *privateboard =
"\n\n\n\n         ¹ï¤£°_ ¦¹ªO¥Ø«e¥u­ã¬ÝªO¦n¤Í¶i¤J  ½Ð¥ý¦VªO¥D¥Ó½Ð¤J¹Ò³\\¥i";
static int brd_class=-1;

static void choose_board(int newflag) {
    static int num = 0;
    boardstat_t *ptr;
    int head = -1, ch = 0, tmp,tmp1, uidtmp, classtmp;
#if HAVE_SEARCH_ALL
    char genbuf[200];
#endif
    extern time_t board_visit_time;
    
    setutmpmode(newflag ? READNEW : READBRD);
    brdnum = 0;
    if(!cuser.userlevel)         /* guest yank all boards */
	yank_flag = 1;
    
    do {
	if(brdnum <= 0) {
	    load_boards();
	    if(brdnum <= 0) {
		if(HAS_PERM(PERM_SYSOP) || (currmode & MODE_MENU)) {
		    if(m_newbrd() == -1)
			break;
		    brdnum = -1;
		    continue;
		} else
		    break;
	    }
	    qsort(nbrd, brdnum, sizeof(boardstat_t), (QCAST)cmpboard);
	    head = -1;
	}
	
	if(num < 0)
	    num = 0;
	else if(num >= brdnum)
	    num = brdnum - 1;
	
	if(head < 0) {
	    if(newflag) {
		tmp = num;
		while(num < brdnum) {
		    ptr = &nbrd[num];
		    if(ptr->unread)
			break;
		    num++;
		}
		if(num >= brdnum)
		    num = tmp;
	    }
	    head = (num / p_lines) * p_lines;
	    show_brdlist(head, 1, newflag);
	} else if(num < head || num >= head + p_lines) {
	    head = (num / p_lines) * p_lines;
	    show_brdlist(head, 0, newflag);
	}
	if(class_uid == 0)
	    ch = cursor_key(7 + num - head, 10);
	else
	    ch = cursor_key(3 + num - head, 0);
	
	switch(ch) {
	case 'e':
	case KEY_LEFT:
	case EOF:
	    ch = 'q';
	case 'q':
	    break;
	case 'c':
	    if(yank_flag == 2) {
		newflag = yank_flag = 0;
		brdnum = -1;
	    }
	    show_brdlist(head, 1, newflag ^= 1);
	    break;
#if HAVE_SEARCH_ALL
	case 'a': {
	    if(yank_flag != 2 ) {
		if(getdata_str(1, 0, "§@ªÌ ", genbuf, IDLEN + 2, DOECHO,
			   currauthor))
		    strncpy(currauthor, genbuf, IDLEN + 2);
		if(*currauthor)
		    yank_flag = 2;
		else
		    yank_flag= 0;
	    } else
		yank_flag = 0;
	    brdnum = -1;
	    show_brdlist(head, 1, newflag);
	    break;
	}
#endif /* HAVE_SEARCH_ALL */
	case KEY_PGUP:
	case 'P':
	case 'b':
	case Ctrl('B'):
	    if(num) {
		num -= p_lines;
		break;
	    }
	case KEY_END:
	case '$':
	    num = brdnum - 1;
	    break;
	case ' ':
	case KEY_PGDN:
	case 'N':
	case Ctrl('F'):
	    if(num == brdnum - 1)
		num = 0;
	    else
		num += p_lines;
	    break;
	case Ctrl('C'):
	    cal();
	    show_brdlist(head, 1, newflag);
	    break;
	case Ctrl('I'):
	    t_idle();
	    show_brdlist(head, 1, newflag);
	    break;
	case KEY_UP:
	case 'p':
	case 'k':
	    if (num-- <= 0)
		num = brdnum - 1;
	    break;
	case KEY_DOWN:
	case 'n':
	case 'j':
	    if (++num < brdnum)
		break;
	case '0':
	case KEY_HOME:
	    num = 0;
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
	    if((tmp = search_num(ch, brdnum)) >= 0)
		num = tmp;
	    brdlist_foot();
	    break;
	case 'h':
	    show_help(choosebrdhelp);
	    show_brdlist(head, 1, newflag);
	    break;
	case Ctrl('A'):
	    Announce();
	    show_brdlist(head, 1, newflag);
	    break;
	case '/':
	    if((tmp = search_board(num)) >= 0)
		num = tmp;
	    show_brdlist(head, 1, newflag);
	    break;
	case 'S':
	    cuser.uflag ^= BRDSORT_FLAG;
	    qsort(nbrd, brdnum, sizeof(boardstat_t), (QCAST)cmpboard);
	    brdnum = -1;
	    break;
	case 'y':
	    if(yank_flag == 2)
		yank_flag = 0;
	    else
		yank_flag ^= 1;
	    brdnum = -1;
	    break;
	case 'z':
	    if(HAS_PERM(PERM_BASIC)) {
		ptr = &nbrd[num];
		ptr->zap = !ptr->zap;
		if(ptr->brdattr & BRD_NOZAP) ptr->zap = 0;
		if(!ptr->zap) check_newpost(ptr);
		zapbuf[ptr->pos] = (ptr->zap ? 0 : login_start_time);
		head = 9999;
	    }
	    break;
	case 'Z':                   /* Ptt */
	    if(HAS_PERM(PERM_BASIC)) {
		for(tmp = 0; tmp < brdnum; tmp++) {
		    ptr = &nbrd[tmp];
		    ptr->zap = !ptr->zap;
		    if(ptr->brdattr & BRD_NOZAP)
			ptr->zap = 0;
		    if(!ptr->zap)
			check_newpost(ptr);
		    zapbuf[ptr->pos] = (ptr->zap ? 0 : login_start_time);
		}
		head = 9999;
	    }
	    break;
	case 'v':
	case 'V':
	    ptr = &nbrd[num];
	    brc_initial(ptr->name);
	    if(ch == 'v') {
		ptr->unread = 0;
		zapbuf[ptr->pos] = time((time_t *) &brc_list[0]);
	    } else
		zapbuf[ptr->pos] = brc_list[0] = ptr->unread = 1;
	    brc_num = brc_changed = 1;
	    brc_update();
	    show_brdlist(head, 0, newflag);
	    break;
	case 's':
	    if((tmp = search_board(-1)) < 0) {
		show_brdlist(head, 1, newflag);
		break;
	    }
	    num = tmp;
	case 'E':
	    if(HAS_PERM(PERM_SYSOP) || (currmode & MODE_MENU)) {
		ptr = &nbrd[num];
		move(1,1);
		clrtobot();
		m_mod_board(ptr->name);
		brdnum = -1;
	    }
	    break;
	case 'B':
	    if(HAS_PERM(PERM_SYSOP) || (currmode & MODE_MENU)) {
		m_newbrd();
		brdnum = -1;
	    }
	    break;
	case 'W':
	    if(brd_class >= 0 && 
	       (HAS_PERM(PERM_SYSOP) || (currmode & MODE_MENU))) {
		b_note_edit_bname(brd_class);
		brdnum = -1;
	    }
	    break;
	case KEY_RIGHT:
	case '\n':
	case '\r':
	case 'r':
	{
	    char buf[STRLEN];
	    
	    ptr = &nbrd[num];
	    
	    if(!(ptr->brdattr & BRD_GROUPBOARD)) {    /* «Dsub class */
		if(!(ptr->brdattr & BRD_HIDE) ||
		   (ptr->brdattr & BRD_POSTMASK)) {
		    brc_initial(ptr->name);

		    if(yank_flag == 2) {
			setbdir(buf, currboard);
			tmp = have_author(currboard) - 1;
			head = tmp - t_lines / 2;
			getkeep(buf, head > 1 ? head : 1, -(tmp + 1));
		    } else if(newflag) {
			setbdir(buf, currboard);
			tmp = unread_position(buf, ptr);
			head = tmp - t_lines / 2;
			getkeep(buf, head > 1 ? head : 1, tmp + 1);
		    }
		    board_visit_time = zapbuf[ptr->pos];
		    if(!ptr->zap)
			time((time_t *) &zapbuf[ptr->pos]);
		    Read();
		    check_newpost(ptr);
		    head = -1;
		    setutmpmode(newflag ? READNEW : READBRD);
		} else {
		    setbfile(buf, ptr->name, FN_APPLICATION);
		    if(more(buf,YEA)==-1) {
			move(1,0);
			clrtobot();
			outs(privateboard);
			pressanykey();
		    }
		    head = -1;
		}
	    } else {                                  /* sub class */
		move(12,1);
		uidtmp = class_uid;
		tmp1=num; 
		num=0;
		class_uid = ptr->uid;

		if (!(currmode & MODE_MENU))	/* ¦pªGÁÙ¨S¦³¤p²ÕªøÅv­­ */
		   set_menu_BM(ptr->BM);

		if(time(NULL) < ptr->bupdate) {
			setbfile(buf, ptr->name, fn_notes);
			if(more(buf, NA) != -1)
 	 		   pressanykey();
		}              
		classtmp = brd_class;
		brd_class = getbnum(ptr->name);
		
		choose_board(0);
		
		brd_class = classtmp;
		currmode &= ~MODE_MENU;	/* Â÷¶}ª©ª©«á´N§âÅv­­®³±¼³á */
		num=tmp1;
		class_uid = uidtmp;
		brdnum = -1;
	    }
	}
	}
    } while(ch != 'q');
    save_zapbuf();
}

int root_board() {
    class_uid = 0;
    brd_class=-1;
    choose_board(0);
    return 0;
}

int Boards() {
    class_uid = -1;
    choose_board(0);
    return 0;
}

extern userinfo_t *currutmp;

int New() {
    int mode0 = currutmp->mode;
    int stat0 = currstat;

    class_uid = -1;
    choose_board(1);
    currutmp->mode = mode0;
    currstat = stat0;
    return 0;
}

/*
int v_favorite(){
    char fname[256];
    char inbuf[2048];
    FILE* fp;
    int nGroup;
    char* strtmp;
    
    setuserfile(fname,str_favorite);
    
    if (!(fp=fopen(fname,"r")))
        return -1;
    move(0,0);
    clrtobot();
    fgets(inbuf,sizeof(inbuf),fp);
    nGroup=atoi(inbuf);
    
    currutmp->nGroup=0;
    currutmp->ninRoot=0;
    
    while(nGroup!=currutmp->nGroup+1){
        fgets(inbuf,sizeof(inbuf),fp);
        prints("%s\n",strtmp=strtok(inbuf," \n"));
        strcpy(currutmp->gfavorite[currutmp->nGroup++],strtmp);
        while((strtmp=strtok(NULL, " \n"))){
            prints("     %s %d\n",strtmp,getbnum(strtmp));
        }
        currutmp->nGroup++;
    }
    prints("+++%d+++\n",currutmp->nGroup);
    
    fgets(inbuf,sizeof(inbuf),fp);
    
    for(strtmp=strtok(inbuf, " \n");strtmp;strtmp=strtok(NULL, " \n")){
        if (strtmp[0]!='#')
            prints("*** %s %d\n",strtmp, getbnum(strtmp));
        else
            prints("*** %s %d\n",strtmp+1, -1);
        currutmp->ninRoot++;
    }
    
    fclose(fp);
    pressanykey();
    return 0;
} 
*/
