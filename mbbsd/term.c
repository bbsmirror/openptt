/* $Id$ */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "config.h"
#include "pttstruct.h"
#include "proto.h"
#include "perm.h"
#include "modes.h"
#include "common.h"

extern struct utmpfile_t *utmpshm;

#if defined(linux)
#define OBUFSIZE  2048
#define IBUFSIZE  128
#else
#define OBUFSIZE  4096
#define IBUFSIZE  256
#endif

int t_lines = 24;
int b_lines = 23;
int p_lines = 20;
int t_columns = 80;
int automargins = 1;

screenline_t *big_picture = NULL;

/* initialization */
void InitTerminal() {
    struct termios tty_state;
    
    if(tcgetattr(1, &tty_state) < 0) {
	syslog(LOG_ERR, "tcgetattr(): %m");
	return;
    }
    cfmakeraw(&tty_state);
    tcsetattr(1, TCSANOW, &tty_state);
}

/* input */
static int i_newfd = 0;
static struct timeval i_to, *i_top = NULL;

static char inbuf[IBUFSIZE];

void add_io(int fd, int timeout) {
    i_newfd = fd;
    if(timeout) {
	i_to.tv_sec = timeout;
	i_to.tv_usec = 0;
	i_top = &i_to;
    } else
	i_top = NULL;
}

static int icurrchar = 0, ibufsize = 0;

/* dogetch() is not reentrant-safe. SIGUSR[12] might happen at any time,
   and dogetch() might be called again, and then ibufsize/icurrchar/inbuf
   might be inconsistent.
   We try to not segfault here... */
static int dogetch() {
    int len;
    extern userinfo_t *currutmp;
    
    if(ibufsize <= icurrchar) {
	refresh();
	
	if(i_newfd) {
	
	    struct timeval timeout;
	    fd_set readfds;
			
	    if(i_top) timeout=*i_top; /* copy it because select() might change it */
			
	    FD_ZERO(&readfds);
	    FD_SET(0, &readfds);
	    FD_SET(i_newfd, &readfds);

	    /* jochang: modify first argument of select from FD_SETSIZE */
	    /* since we are only waiting input from fd 0 and i_newfd(>0) */
		
	    while((len = select(i_newfd+1, &readfds, NULL, NULL, i_top?&timeout:NULL))<0)
	    {
		if(errno != EINTR)
		    abort_bbs(0);
		    /* raise(SIGHUP); */
	    }

	    if(len == 0)
		return I_TIMEOUT;

	    if(i_newfd && FD_ISSET(i_newfd, &readfds))
		return I_OTHERDATA;
	}

	while((len = read(0, inbuf, IBUFSIZE)) <= 0) {
	    if(len == 0 || errno != EINTR)
	    	abort_bbs(0);
		/* raise(SIGHUP); */
	}
	ibufsize = len;
	icurrchar = 0;
    }

    if(currutmp) {
	MPROTECT_UTMP_RW;
	currutmp->lastact = time(0);
	MPROTECT_UTMP_R;
    }
    return inbuf[icurrchar++];
}

char watermode = -1; 
/* watermode = -1  ¨S¦b¦^¤ô²y
             = 0   ¦b¦^¤W¤@Áû¤ô²y  (Ctrl-R)
	     > 0   ¦b¦^«e n Áû¤ô²y (Ctrl-R Ctrl-R) */

int igetch() {
    register int ch;
    extern userinfo_t *currutmp;
extern char oldmsg_count;
    
    while((ch = dogetch())) {
	switch(ch) {
	case Ctrl('L'):
	    redoscr();
	    continue;
	case Ctrl('U'):
	    if(currutmp != NULL &&  currutmp->mode != EDITING
	       && currutmp->mode !=  LUSERS && currutmp->mode) {

		screenline_t *screen0 = calloc(t_lines, sizeof(screenline_t));
		int y, x, my_newfd;

		getyx(&y, &x);
		memcpy(screen0, big_picture, t_lines * sizeof(screenline_t));
		my_newfd = i_newfd;
		i_newfd = 0;
		t_users();
		i_newfd = my_newfd;
		memcpy(big_picture, screen0, t_lines * sizeof(screenline_t));
		move(y, x);
		free(screen0);
		redoscr();
		continue;
	    } else
		return (ch);
        case Ctrl('R'):
	    if(currutmp == NULL)
		return (ch);
	    else if(watermode > 0) {
		/* «ö¹L¨â¦¸ Ctrl-R «á¦A«ö Ctrl-R 
		   ¾A·í¦a±N watermode + 1 */
		watermode = (watermode + oldmsg_count) % oldmsg_count + 1;
		t_display_new();
		continue;
	    } else if (currutmp->mode == 0 && (currutmp->chatid[0] == 2 || 
					       currutmp->chatid[0] == 3) &&
		       oldmsg_count != 0 && watermode == 0) {
		/* ²Ä¤G¦¸«ö Ctrl-R */
		watermode = 1;
		t_display_new();
		continue;
            } else if(currutmp->msgs[0].last_pid) {
		/* ²Ä¤@¦¸«ö Ctrl-R (¥²¶·¥ý³Q¥á¹L¤ô²y) */
		screenline_t *screen0 = calloc(t_lines, sizeof(screenline_t));
		int y, x, my_newfd;
		
		getyx(&y, &x);
		memcpy(screen0, big_picture, t_lines * sizeof(screenline_t));
		
		/* ¦pªG¥¿¦b talk ªº¸Ü¥ý¤£³B²z¹ï¤è°e¹L¨Óªº«Ê¥] (¤£¥hselect) */
		my_newfd = i_newfd;
		i_newfd = 0;
		show_last_call_in(0);
		watermode = 0;
		my_write(currutmp->msgs[0].last_pid, "¤ô²y¥á¹L¥h ¡G ", currutmp->msgs[0].last_userid, 0);
		i_newfd = my_newfd;
		
		/* ÁÙ­ì¿Ã¹õ */
		memcpy(big_picture, screen0, t_lines * sizeof(screenline_t));
		move(y, x);
		free(screen0);
		redoscr();
		continue;
            } else
		return ch;
        case '\n':   /* Ptt§â \n®³±¼ */
	    continue;
        case KEY_TAB:
	    if(watermode > 0) {
		watermode = (watermode + oldmsg_count) % oldmsg_count + 1;
		t_display_new();
		continue;
	    } else
		return (ch);
        case Ctrl('T'):
	    if(watermode > 0) {
		watermode = (watermode + oldmsg_count - 2 ) % oldmsg_count + 1;
		t_display_new();
		continue;
	    }
        default:
	    return ch;
	}
    }
    return 0;
}

int oldgetdata(int line, int col, char *prompt, char *buf, int len, int echo) {
    register int ch, i;
    int clen;
    int x = col, y = line;
    extern unsigned char scr_cols;
#define MAXLASTCMD 12
    static char lastcmd[MAXLASTCMD][80];

    strip_ansi(buf, buf, STRIP_ALL);

    if(prompt) {

	move(line, col);

	clrtoeol();

	outs(prompt);

	x += strip_ansi(NULL,prompt,0);
    }

    if(!echo) {
	len--;
	clen = 0;
	while((ch = igetch()) != '\r') {
	    if(ch == '\177' || ch == Ctrl('H')) {
		if(!clen) {
		    bell();
		    continue;
		}
		clen--;
		if(echo) {
		    ochar(Ctrl('H'));
		    ochar(' ');
		    ochar(Ctrl('H'));
		}
		continue;
	    }
// Ptt
#ifdef BIT8
	    if(!isprint2(ch))
#else
		if(!isprint(ch))
#endif
		{
		    if(echo)
			bell();
		    continue;
		}

	    if(clen >= len)  {
		if(echo)
		    bell();
		continue;
	    }
	    buf[clen++] = ch;
	    if(echo)
		ochar(ch);
	}
	buf[clen] = '\0';
	outc('\n');
	oflush();
    } else {
	int cmdpos = -1;
	int currchar = 0;

	standout();
	for(clen = len--; clen; clen--)
	    outc(' ');
	standend();
	buf[len] = 0;
	move(y, x);
	edit_outs(buf);
	clen = currchar = strlen(buf);

	while(move(y, x + currchar), (ch = igetkey()) != '\r') {
	    switch(ch) {
	    case KEY_DOWN:
	    case Ctrl('N'):
                buf[clen] = '\0';  /* Ptt */
                strncpy(lastcmd[cmdpos], buf, 79);
                cmdpos += MAXLASTCMD - 2;
	    case Ctrl('P'):
	    case KEY_UP:
                if(ch == KEY_UP || ch == Ctrl('P')) {
		    buf[clen] = '\0';  /* Ptt */
		    strncpy(lastcmd[cmdpos], buf, 79);
                }
                cmdpos++;
                cmdpos %= MAXLASTCMD;
                strncpy(buf, lastcmd[cmdpos], len);
                buf[len] = 0;

                move(y, x);                   /* clrtoeof */
                for(i = 0; i <= clen; i++)
		    outc(' ');
                move(y, x);
                edit_outs(buf);
                clen = currchar = strlen(buf);
                break;
	    case KEY_LEFT:
                if(currchar)
		    --currchar;
                break;
	    case KEY_RIGHT:
                if(buf[currchar])
		    ++currchar;
                break;
	    case '\177':
	    case Ctrl('H'):
                if(currchar) {
		    currchar--;
		    clen--;
		    for(i = currchar; i <= clen; i++)
			buf[i] = buf[i + 1];
		    move(y, x + clen);
		    outc(' ');
		    move(y, x);
		    edit_outs(buf);
                }
                break;
	    case Ctrl('Y'):
                currchar = 0;
	    case Ctrl('K'):
                buf[currchar] = 0;
                move(y, x + currchar);
                for(i = currchar; i < clen; i++)
		    outc(' ');
                clen = currchar;
                break;
	    case Ctrl('D'):
                if(buf[currchar]) {
		    clen--;
		    for(i = currchar; i <= clen; i++)
			buf[i] = buf[i + 1];
		    move(y, x + clen);
		    outc(' ');
		    move(y, x);
		    edit_outs(buf);
                }
                break;
	    case Ctrl('A'):
                currchar = 0;
                break;
	    case Ctrl('E'):
                currchar = clen;
                break;
	    default:
                if(isprint2(ch) && clen < len && x + clen < scr_cols) {
		    for(i = clen + 1; i > currchar;i--)
			buf[i] = buf[i - 1];
		    buf[currchar] = ch;
		    move(y, x + currchar);
		    edit_outs(buf + currchar);
		    currchar++;
		    clen++;
		}
                break;
	    }/* end case */
	}  /* end while */

	if(clen > 1)
	    for(cmdpos = MAXLASTCMD - 1; cmdpos; cmdpos--) {
                strcpy(lastcmd[cmdpos], lastcmd[cmdpos - 1]);
                strncpy(lastcmd[0], buf, len);
	    }
	if(echo)
	    outc('\n');
	refresh();
    }
    if((echo == LCECHO) && ((ch = buf[0]) >= 'A') && (ch <= 'Z'))
	buf[0] = ch | 32;
#ifdef SUPPORT_GB    
    if(echo == DOECHO &&  current_font_type == TYPE_GB)
    {
	strcpy(buf,hc_convert_str(buf, HC_GBtoBIG, HC_DO_SINGLE));
    }
#endif
    return clen;
}

/* Ptt */
int getdata_buf(int line, int col, char *prompt, char *buf, int len, int echo) {
    return oldgetdata(line, col, prompt, buf, len, echo);
}

int getdata_str(int line, int col, char *prompt, char *buf, int len, int echo, char *defaultstr) {
    strncpy(buf, defaultstr, len);
    
    buf[len] = 0;
    return oldgetdata(line, col, prompt, buf, len, echo);
}

int getdata(int line, int col, char *prompt, char *buf, int len, int echo) {
    buf[0] = 0;
    return oldgetdata(line, col, prompt, buf, len, echo);
}

int KEY_ESC_arg;

int igetkey() {
    int mode;
    int ch, last;

    mode = last = 0;
    while(1) {
	ch = igetch();
	if(mode == 0) {
	    if(ch == KEY_ESC)
		mode = 1;
	    else
		return ch;              /* Normal Key */
	} else if (mode == 1) {         /* Escape sequence */
	    if(ch == '[' || ch == 'O')
		mode = 2;
	    else if(ch == '1' || ch == '4')
		mode = 3;
	    else {
		KEY_ESC_arg = ch;
		return KEY_ESC;
	    }
	} else if(mode == 2) {          /* Cursor key */
	    if(ch >= 'A' && ch <= 'D')
		return KEY_UP + (ch - 'A');
	    else if(ch >= '1' && ch <= '6')
		mode = 3;
	    else
		return ch;
	} else if (mode == 3) {         /* Ins Del Home End PgUp PgDn */
	    if(ch == '~')
		return KEY_HOME + (last - '1');
	    else
		return ch;
	}
	last = ch;
    }
}

int i_get_key() {
    int i;
    while(1) {
	i = egetch();
	/* ³oÃä¦³°ÝÃD, ¥ý®³±¼, ª±µPªº®É­Ô¤£­ã¥á¤ô²y
	if(i == Ctrl('R')) {
	    if(currutmp->msgs[0].last_pid) {
		show_last_call_in();
		my_write(currutmp->msgs[0].last_pid, "¤ô²y¥á¦^¥h¡G",
			 currutmp->msgs[0].last_userid);
	    }
	} else
	*/
	    return i;
    }
}                

/* output */


/* ------------ */

void DoMove(int destcol, int destline) {
    char buf[16], *p;
    
    sprintf(buf, "\33[%d;%dH", destline + 1, destcol + 1);
    for(p = buf; *p; p++)
	ochar(*p);
}

void save_cursor() {
    ochar('\33');
    ochar('7');
}

void restore_cursor() {
    ochar('\33');
    ochar('8');
}

static void change_scroll_range(int top, int bottom) {
    char buf[16], *p;
    
    sprintf(buf, "\33[%d;%dr", top + 1, bottom + 1);
    for(p = buf; *p; p++)
	ochar(*p);
}

static void scroll_forward() {
    ochar('\33');
    ochar('D');
}

extern int current_font_type;
extern char *fn_proverb;
extern unsigned int currstat;
extern pid_t currpid;
extern int errno;
extern int curr_idle_timeout;

static char outbuf[OBUFSIZE];
static int obufsize = 0;

int ochar(int c) {
    if(obufsize > OBUFSIZE - 1) {
	write(1, outbuf, obufsize);
	obufsize = 0;
    }
    outbuf[obufsize++] = c;
    return 0;
}

/* ----------------------------------------------------- */
/* ©w®ÉÅã¥Ü°ÊºA¬ÝªO                                      */
/* ----------------------------------------------------- */
extern userec_t cuser;

static void hit_alarm_clock() {
    extern userinfo_t *currutmp;
    
    if(HAS_PERM(PERM_NOTIMEOUT) || PERM_HIDE(currutmp) || currstat == MAILALL)
	return;
//    if(time(0) - currutmp->lastact > IDLE_TIMEOUT - 2) {
    if(time(0) - currutmp->lastact > curr_idle_timeout - 2) {
	clear();
	kill(currpid, SIGHUP);
    }
//    alarm(IDLE_TIMEOUT);
    alarm(curr_idle_timeout);
}

void init_alarm() {
    signal(SIGALRM, (void (*)(int))hit_alarm_clock);
//    alarm(IDLE_TIMEOUT);
    alarm(curr_idle_timeout);
}

/* ----------------------------------------------------- */
/* output routines                                       */
/* ----------------------------------------------------- */

void oflush() {
    if(obufsize) {
	write(1, outbuf, obufsize);
	obufsize = 0;
    }
}

void output(char *s, int len) {
    /* Invalid if len >= OBUFSIZE */

    if(obufsize + len > OBUFSIZE) {
	write(1, outbuf, obufsize);
	obufsize = 0;
    }
    memcpy(outbuf + obufsize, s, len);
    obufsize += len;
}

extern int showansi;

#ifdef SUPPORT_GB    
static int current_font_type=TYPE_BIG5;
static int gbinited=0;
#endif
#define SCR_WIDTH       80 
#define o_clear()     output("\33[H\33[J", 6)
#define o_cleol()     output("\33[K", 3)
#define o_scrollrev() output("\33M", 2)
#define o_standup()   output("\33[7m", 4)
#define o_standdown() output("\33[m", 3)

unsigned char scr_lns, scr_cols;
static unsigned char cur_ln = 0, cur_col = 0;
static unsigned char docls, downfrom = 0;
static unsigned char standing = NA;
static char roll = 0;
static int scrollcnt, tc_col, tc_line;

#define MODIFIED (1)            /* if line has been modifed, screen output */
#define STANDOUT (2)            /* if this line has a standout region */

void initscr() {
    if(!big_picture) {
	scr_lns = t_lines;
	scr_cols = t_columns = ANSILINELEN;
	/* scr_cols = MIN(t_columns, ANSILINELEN); */
	big_picture = (screenline_t *) calloc(scr_lns, sizeof(screenline_t));
	docls = YEA;
    }
}

void move(int y, int x) {
    cur_col = x;
    cur_ln = y;
}

void getyx(int *y, int *x) {
    *y = cur_ln;
    *x = cur_col;
}

static void rel_move(int was_col, int was_ln, int new_col, int new_ln) {
    if(new_ln >= t_lines || new_col >= t_columns)
	return;

    tc_col = new_col;
    tc_line = new_ln;
    if(new_col == 0) {
	if(new_ln == was_ln) {
	    if(was_col)
		ochar('\r');
	    return;
	} else if(new_ln == was_ln + 1) {
	    ochar('\n');
	    if(was_col)
		ochar('\r');
	    return;
	}
    }

    if(new_ln == was_ln) {
	if(was_col == new_col)
	    return;

	if(new_col == was_col - 1) {
	    ochar(Ctrl('H'));
	    return;
	}
    }
    DoMove(new_col, new_ln);
}

static void standoutput(char *buf, int ds, int de, int sso, int eso) {
    int st_start, st_end;
    
    if(eso <= ds || sso >= de) {
	output(buf + ds, de - ds);
    } else {
	st_start = MAX(sso, ds);
	st_end = MIN(eso, de);
	if(sso > ds)
	    output(buf + ds, sso - ds);
	o_standup();
	output(buf + st_start, st_end - st_start);
	o_standdown();
	if(de > eso)
	    output(buf + eso, de - eso);
    }
}

void redoscr() {
    register screenline_t *bp;
    register int i, j, len;

    o_clear();
    for(tc_col = tc_line = i = 0, j = roll; i < scr_lns; i++, j++) {
	if(j >= scr_lns)
	    j = 0;
	bp = &big_picture[j];
	if((len = bp->len)) {
	    rel_move(tc_col, tc_line, 0, i);
	    if(bp->mode & STANDOUT)
		standoutput(bp->data, 0, len, bp->sso, bp->eso);
	    else
		output(bp->data, len);
	    tc_col += len;
	    if(tc_col >= t_columns) {
		if (automargins)
		    tc_col = t_columns - 1;
		else {
		    tc_col -= t_columns;
		    tc_line++;
		    if(tc_line >= t_lines)
			tc_line = b_lines;
		}
	    }
	    bp->mode &= ~(MODIFIED);
	    bp->oldlen = len;
	}
    }
    rel_move(tc_col, tc_line, cur_col, cur_ln);
    docls = scrollcnt = 0;
    oflush();
}

void refresh() {
    register screenline_t *bp = big_picture;
    register int i, j, len;
    if(icurrchar != ibufsize)
	return;

    if((docls) || (abs(scrollcnt) >= (scr_lns - 3))) {
	redoscr();
	return;
    }

    if(scrollcnt < 0) {
	redoscr();
	return;
    } else if (scrollcnt > 0) {
	rel_move(tc_col, tc_line, 0, b_lines);
	do {
	    ochar('\n');
	} while(--scrollcnt);
    }

    for(i = 0, j = roll; i < scr_lns; i++, j++) {
	if(j >= scr_lns)
	    j = 0;
	bp = &big_picture[j];
	len = bp->len;
	if(bp->mode & MODIFIED && bp->smod < len) {
	    bp->mode &= ~(MODIFIED);
	    if(bp->emod >= len)
		bp->emod = len - 1;
	    rel_move(tc_col, tc_line, bp->smod, i);

	    if(bp->mode & STANDOUT)
		standoutput(bp->data, bp->smod, bp->emod + 1,
			    bp->sso, bp->eso);
	    else
		output(&bp->data[bp->smod], bp->emod - bp->smod + 1);
	    tc_col = bp->emod + 1;
	    if(tc_col >= t_columns) {
		if(automargins)	{
		    tc_col -= t_columns;
		    if(++tc_line >= t_lines)
			tc_line = b_lines;
		} else
		    tc_col = t_columns - 1;
	    }
	}
	
	if(bp->oldlen > len) {
	    rel_move(tc_col, tc_line, len, i);
	    o_cleol();
	}
	bp->oldlen = len;

    }

    rel_move(tc_col, tc_line, cur_col, cur_ln);

    oflush();
}

void clear() {
    register screenline_t *slp;

    register int i;

    docls = YEA;
    cur_col = cur_ln = roll = downfrom = i = 0;
    do {
	slp = &big_picture[i];
	slp->mode = slp->len = slp->oldlen = 0;
    } while(++i < scr_lns);
}

void clrtoeol() {
    register screenline_t *slp;
    register int ln;

    standing = NA;
    if((ln = cur_ln + roll) >= scr_lns)
	ln -= scr_lns;
    slp = &big_picture[ln];
    if(cur_col <= slp->sso)
	slp->mode &= ~STANDOUT;

    if(cur_col > slp->oldlen) {
	for(ln = slp->len; ln <= cur_col; ln++)
	    slp->data[ln] = ' ';
    }

    if(cur_col < slp->oldlen) {
	for(ln = slp->len; ln >= cur_col; ln--)
	    slp->data[ln] = ' ';
    }
	
    slp->len = cur_col;
}

void clrtoline(int line) {
    register screenline_t *slp;
    register int i, j;

    for(i = cur_ln, j = i + roll; i < line; i++, j++) {
	if(j >= scr_lns)
	    j -= scr_lns;
	slp = &big_picture[j];
	slp->mode = slp->len = 0;
	if(slp->oldlen)
	    slp->oldlen = 255;
    }
}     

void clrtobot() {
    clrtoline(scr_lns);
}

void outch(unsigned char c) {
    register screenline_t *slp;
    register int i;

    if((i = cur_ln + roll) >= scr_lns)
	i -= scr_lns;
    slp = &big_picture[i];
 
    if(c == '\n' || c == '\r') {
	if(standing) {
	    slp->eso = MAX(slp->eso, cur_col);
	    standing = NA;
	}
	if((i = cur_col - slp->len) > 0)
	    memset(&slp->data[slp->len], ' ', i + 1);
	slp->len = cur_col;
	cur_col = 0;
	if(cur_ln < scr_lns)
	    cur_ln++;
	return;
    }
/*
  else if(c != '\033' && !isprint2(c))
  {
  c = '*'; //substitute a '*' for non-printable 
  }
*/
    if(cur_col >= slp->len) {
	for(i = slp->len; i < cur_col; i++)
	    slp->data[i] = ' ';
	slp->data[cur_col] = '\0';
	slp->len = cur_col + 1;
    }

    if(slp->data[cur_col] != c) {
	slp->data[cur_col] = c;
	if((slp->mode & MODIFIED) != MODIFIED)
	    slp->smod = slp->emod = cur_col;
	slp->mode |= MODIFIED;
	if(cur_col > slp->emod)
	    slp->emod = cur_col;
	if(cur_col < slp->smod)
	    slp->smod = cur_col;
    }

    if (++cur_col >= scr_cols)
    {
	if (standing && (slp->mode & STANDOUT))
	{
	    standing = 0;
	    slp->eso = MAX(slp->eso, cur_col);
	}
	cur_col = 0;
	if (cur_ln < scr_lns)
	    cur_ln++;
    }             

}

static void parsecolor(char *buf) {
    char *val;
    char data[24];
    
    data[0] = '\0';
    val = (char *)strtok(buf, ";");

    while(val) {
	if(atoi(val) < 30) {
	    if(data[0])
		strcat(data, ";");
	    strcat(data, val);
	}
	val = (char *) strtok(NULL, ";");
    }
    strcpy(buf, data);
}

#define NORMAL (00)
#define ESCAPE (01)
#define VTKEYS (02)

void outc(unsigned char ch) {
    if(showansi)
	outch(ch);
    else {
	static char buf[24];
	static int p = 0;
	static int mode = NORMAL;
	int i;

	switch(mode) {
	case NORMAL:
	    if(ch == '\033')
		mode = ESCAPE;
	    else
		outch(ch);
	    return;
	case ESCAPE:
	    if(ch == '[')
		mode = VTKEYS;
	    else {
		mode = NORMAL;
		outch('');
		outch(ch);
	    }
	    return;
	case VTKEYS:
	    if(ch == 'm') {
		buf[p++] = '\0';
		parsecolor(buf);
	    } else if((p < 24) && (not_alpha(ch))) {
		buf[p++] = ch;
		return;
	    }
	    if(buf[0]) {
		outch('');
		outch('[');

		for(i = 0; (p = buf[i]); i++)
		    outch(p);
		outch(ch);
	    }
	    p = 0;
	    mode = NORMAL;
	}
    }
}

static void do_outs(char *str) {
    while(*str)
    {
        outc(*str++);
    }
}          
#ifdef SUPPORT_GB    
static void gb_init()
{
    if(current_font_type == TYPE_GB)
    {
	hc_readtab(BBSHOME"/etc/hc.tab");
    }
    gbinited = 1;
}

static void gb_outs(char *str)
{
    do_outs(hc_convert_str(str, HC_BIGtoGB, HC_DO_SINGLE));
}             
#endif
int edit_outs(char *text) {
    register int column = 0;
    register char ch;
#ifdef SUPPORT_GB    
    if(current_font_type == TYPE_GB) 
	text = hc_convert_str(text, HC_BIGtoGB, HC_DO_SINGLE);
#endif
    while((ch = *text++) && (++column < SCR_WIDTH))
        outch(ch == 27 ? '*' : ch);

    return 0;
}                  

void outs(char *str) {
#ifdef SUPPORT_GB    
    if(current_font_type == TYPE_BIG5)
#endif
	do_outs(str);
#ifdef SUPPORT_GB    
    else
    {
	if(!gbinited) gb_init();
	gb_outs(str);
    }
#endif
}


/* Jaky */
void  Jaky_outs(char *str, int line) {
#ifdef SUPPORT_GB    
    if(current_font_type == TYPE_GB)
	str = hc_convert_str(str, HC_BIGtoGB, HC_DO_SINGLE);    
#endif
    while(*str && line) {
	outc(*str);
	if(*str=='\n')
	    line--;
	str++;
    }
}

void outmsg(char *msg) {
    move(b_lines, 0);
    clrtoeol();
#ifdef SUPPORT_GB    
    if(current_font_type == TYPE_GB)
	msg = hc_convert_str(msg, HC_BIGtoGB, HC_DO_SINGLE);    
#endif
    while(*msg)
	outc(*msg++);
}

void prints(char *fmt, ...) {
    va_list args;
    char buff[1024];

    va_start(args, fmt);
    vsprintf(buff, fmt, args);
    va_end(args);
    outs(buff);
}

void mprints(int y, int x, char *str) {
    move(y, x);
    prints(str);
} 

void scroll() {
    scrollcnt++;
    if(++roll >= scr_lns)
	roll = 0;
    move(b_lines, 0);
    clrtoeol();
}

void rscroll() {
    scrollcnt--;
    if(--roll < 0)
	roll = b_lines;
    move(0, 0);
    clrtoeol();
}

void region_scroll_up(int top, int bottom) {
    int i;

    if(top > bottom) {
	i = top; 
	top = bottom;
	bottom = i;
    }

    if(top < 0 || bottom >= scr_lns)
	return;

    for(i = top; i < bottom; i++)
	big_picture[i] = big_picture[i + 1];
    memset(big_picture + i, 0, sizeof(*big_picture));
    memset(big_picture[i].data, ' ', scr_cols);
    save_cursor();
    change_scroll_range(top, bottom);
    DoMove(0, bottom);
    scroll_forward();
    change_scroll_range(0, scr_lns - 1);
    restore_cursor();
    refresh();
}

void standout() {
    if(!standing) {
	register screenline_t *slp;

	slp = &big_picture[((cur_ln + roll) % scr_lns)];
	standing = YEA;
	slp->sso = slp->eso = cur_col;
	slp->mode |= STANDOUT;
    }
}

void standend() {
    if(standing) {
	register screenline_t *slp;

	slp = &big_picture[((cur_ln + roll) % scr_lns)];
	standing = NA;
	slp->eso = MAX(slp->eso, cur_col);
    }
}
