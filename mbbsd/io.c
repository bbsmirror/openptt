/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <signal.h>
#include "config.h"
#include "pttstruct.h"
#include "perm.h"
#include "modes.h"
#include "common.h"
#include "proto.h"

#if defined(Linux)
#define OBUFSIZE  2048
#define IBUFSIZE  128
#else
#define OBUFSIZE  4096
#define IBUFSIZE  256
#endif

extern int current_font_type;
extern char *fn_proverb;
extern userinfo_t *currutmp;
extern unsigned int currstat;
extern pid_t currpid;
extern int errno;
extern screenline_t *big_picture;
extern int t_lines, t_columns;  /* Screen size / width */
extern int curr_idle_timeout;

static char outbuf[OBUFSIZE], inbuf[IBUFSIZE];
static int obufsize = 0, ibufsize = 0;
static int icurrchar = 0;

/* ----------------------------------------------------- */
/* 定時顯示動態看板                                      */
/* ----------------------------------------------------- */
extern userec_t cuser;

static void hit_alarm_clock() {
    if(HAS_PERM(PERM_NOTIMEOUT) || PERM_HIDE(currutmp) || currstat == MAILALL)
	return;
//    if(time(0) - currutmp->lastact > IDLE_TIMEOUT - 2) {
    if(time(0) - currutmp->lastact > curr_idle_timeout - 2) {
	clear();
	if(currpid > 0) kill(currpid, SIGHUP);
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

int ochar(int c) {
    if(obufsize > OBUFSIZE - 1) {
	write(1, outbuf, obufsize);
	obufsize = 0;
    }
    outbuf[obufsize++] = c;
    return 0;
}

/* ----------------------------------------------------- */
/* input routines                                        */
/* ----------------------------------------------------- */

static int i_newfd = 0;
static struct timeval i_to, *i_top = NULL;
static int (*flushf) () = NULL;

void add_io(int fd, int timeout) {
    i_newfd = fd;
    if(timeout) {
	i_to.tv_sec = timeout;
	i_to.tv_usec = 16384;  /* Ptt: 改成16384 避免不按時for loop吃cpu time
				  16384 約每秒64次 */
	i_top = &i_to;
    } else
	i_top = NULL;
}

int num_in_buf() {
    return icurrchar - ibufsize;
}

char watermode = -1; 
/* Ptt 水球回顧用的參數 */
/* watermode = -1  沒在回水球
             = 0   在回上一顆水球  (Ctrl-R)
	     > 0   在回前 n 顆水球 (Ctrl-R Ctrl-R) */

extern  char no_oldmsg,oldmsg_count;

/*
	dogetch() is not reentrant-safe. SIGUSR[12] might happen at any time,
	and dogetch() might be called again, and then ibufsize/icurrchar/inbuf
	might be inconsistent.
	We try to not segfault here... 
*/

static int dogetch() {
    int len;

    if(ibufsize <= icurrchar) {

	if(flushf)
	    (*flushf)();

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

    if(currutmp) 
	currutmp->lastact = time(0);
    return inbuf[icurrchar++];
}

int igetch() {
    register int ch;
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
		/* 按過兩次 Ctrl-R 後再按 Ctrl-R 
		   適當地將 watermode + 1 */
		watermode = (watermode + oldmsg_count) % oldmsg_count + 1;
		t_display_new();
		continue;
	    } else if (currutmp->mode == 0 && (currutmp->chatid[0] == 2 || 
					       currutmp->chatid[0] == 3) &&
		       oldmsg_count != 0 && watermode == 0) {
		/* 第二次按 Ctrl-R */
		watermode = 1;
		t_display_new();
		continue;
            } else if(currutmp->msgs[0].last_pid) {
		/* 第一次按 Ctrl-R (必須先被丟過水球) */
		screenline_t *screen0 = calloc(t_lines, sizeof(screenline_t));
		int y, x, my_newfd;
		
		getyx(&y, &x);
		memcpy(screen0, big_picture, t_lines * sizeof(screenline_t));
		
		/* 如果正在 talk 的話先不處理對方送過來的封包 (不去select) */
		my_newfd = i_newfd;
		i_newfd = 0;
		show_last_call_in(0);
		watermode = 0;
		my_write(currutmp->msgs[0].last_pid, "水球丟過去 ： ", currutmp->msgs[0].last_userid, 0);
		i_newfd = my_newfd;
		
		/* 還原螢幕 */
		memcpy(big_picture, screen0, t_lines * sizeof(screenline_t));
		move(y, x);
		free(screen0);
		redoscr();
		continue;
            } else
		return ch;
        case '\n':   /* Ptt把 \n拿掉 */
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
	/* 這邊有問題, 先拿掉, 玩牌的時候不准丟水球
	if(i == Ctrl('R')) {
	    if(currutmp->msgs[0].last_pid) {
		show_last_call_in();
		my_write(currutmp->msgs[0].last_pid, "水球丟回去：",
			 currutmp->msgs[0].last_userid);
	    }
	} else
	*/
	    return i;
    }
}                
