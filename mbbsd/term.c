/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>
#include "config.h"
#include "struct.h"
#include "common.h"
#include "proto.h"

int tgetent(const char *bp, char *name);
char *tgetstr(const char *id, char **area);
int tgetflag(const char *id);
int tgetnum(const char *id);
int tputs(const char *str, int affcnt, int (*putc)(int));
char *tparm(const char *str, ...);
char *tgoto(const char *cap, int col, int row);

static struct termios tty_state, tty_new;

/* ----------------------------------------------------- */
/* basic tty control                                     */
/* ----------------------------------------------------- */
void init_tty() {
    if(tcgetattr(1, &tty_state) < 0) {
	syslog(LOG_ERR, "tcgetattr(): %m");
	return;
    }
    memcpy(&tty_new, &tty_state, sizeof(tty_new));
    tty_new.c_lflag &= ~(ICANON | ECHO | ISIG);
/*    tty_new.c_cc[VTIME] = 0;
    tty_new.c_cc[VMIN] = 1; */
    tcsetattr(1, TCSANOW, &tty_new);
    system("stty raw -echo");
}

/* ----------------------------------------------------- */
/* init tty control code                                 */
/* ----------------------------------------------------- */


#define TERMCOMSIZE (40)

char clearbuf[TERMCOMSIZE];
int clearbuflen;

char cleolbuf[TERMCOMSIZE];
int cleolbuflen;

static char cursorm[TERMCOMSIZE];
static char *cm;

static char changescroll[TERMCOMSIZE];
static char *cs;

static char savecursor[TERMCOMSIZE];
static char *sc;

static char restorecursor[TERMCOMSIZE];
static char *rc;

static char scrollforward[TERMCOMSIZE];
static char *sf;

static char scrollreverse[TERMCOMSIZE];
static char *sr;

char scrollrev[TERMCOMSIZE];
int scrollrevlen;

char strtstandout[TERMCOMSIZE];
int strtstandoutlen;

char endstandout[TERMCOMSIZE];
int endstandoutlen;

int t_lines = 24;
int b_lines = 23;
int p_lines = 20;
int t_columns = 80;

int automargins;

static char *outp;
static int *outlp;


static int outcf(int ch) {
    if(*outlp < TERMCOMSIZE) {
	(*outlp)++;
	*outp++ = ch;
    }
    return 0;
}

extern screenline_t *big_picture;

static void term_resize(int sig) {
    struct winsize newsize;
    screenline_t *new_picture;

    signal(SIGWINCH, SIG_IGN);	/* Don't bother me! */
    ioctl(0, TIOCGWINSZ, &newsize);
    if(newsize.ws_row > t_lines) {
	new_picture = (screenline_t *)calloc(newsize.ws_row,
					     sizeof(screenline_t));
	if(new_picture == NULL) {
	    syslog(LOG_ERR, "calloc(): %m");
	    return;
	}
	free(big_picture);
	big_picture = new_picture;
    }

    t_lines=newsize.ws_row;
    b_lines=t_lines-1;
    p_lines=t_lines-4;

    signal(SIGWINCH, term_resize);
}

extern speed_t ospeed;

int term_init() {
    extern char PC, *UP, *BC;
    static char UPbuf[TERMCOMSIZE];
    static char BCbuf[TERMCOMSIZE];
    static char buf[1024];
    char sbuf[2048];
    char *sbp, *s;
    char *term = "vt100";
    
    ospeed = cfgetospeed(&tty_state);
    if(tgetent(buf, term) != 1)
	return NA;

    sbp = sbuf;
    s = tgetstr("pc", &sbp);      /* get pad character */
    if(s) PC = *s;

    automargins = tgetflag("am");

    outp = clearbuf;              /* fill clearbuf with clear screen command */
    outlp = &clearbuflen;
    clearbuflen = 0;
    sbp = sbuf;
    s = tgetstr("cl", &sbp);
    if(s) tputs(s, t_lines, outcf);

    outp = cleolbuf;              /* fill cleolbuf with clear to eol command */
    outlp = &cleolbuflen;
    cleolbuflen = 0;
    sbp = sbuf;
    s = tgetstr("ce", &sbp);
    if(s) tputs(s, 1, outcf);

    outp = scrollrev;
    outlp = &scrollrevlen;
    scrollrevlen = 0;
    sbp = sbuf;
    s = tgetstr("sr", &sbp);
    if(s) tputs(s, 1, outcf);

    outp = strtstandout;
    outlp = &strtstandoutlen;
    strtstandoutlen = 0;
    sbp = sbuf;
    s = tgetstr("so", &sbp);
    if(s) tputs(s, 1, outcf);

    outp = endstandout;
    outlp = &endstandoutlen;
    endstandoutlen = 0;
    sbp = sbuf;
    s = tgetstr("se", &sbp);
    if(s) tputs(s, 1, outcf);

    sbp = cursorm;
    cm = tgetstr("cm", &sbp);

    sbp = changescroll;
    cs = tgetstr("cs", &sbp);

    sbp = scrollforward;
    sf = tgetstr("sf", &sbp);

    sbp = scrollreverse;
    sr = tgetstr("sr", &sbp);

    sbp = savecursor;
    sc = tgetstr("sc", &sbp);

    sbp = restorecursor;
    rc = tgetstr("rc", &sbp);

    sbp = UPbuf;
    UP = tgetstr("up", &sbp);
    sbp = BCbuf;
    BC = tgetstr("bc", &sbp);

    b_lines = t_lines - 1;
    p_lines = t_lines - 4;

    signal(SIGWINCH, term_resize);
    return YEA;
}

char term_buf[32];

void do_move(int destcol, int destline) {
    tputs(tgoto(cm, destcol, destline), 0, ochar);
}

void save_cursor() {
    tputs(sc, 0, ochar);
}

void restore_cursor() {
    tputs(rc, 0, ochar);
}

void change_scroll_range(int top, int bottom) {
    tputs(tparm(cs, top, bottom), 0, ochar);
}

void scroll_forward() {
    tputs(sf, 0, ochar);
}
