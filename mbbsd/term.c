/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "config.h"
#include "pttstruct.h"
#include "common.h"
#include "proto.h"

int t_lines = 24;
int b_lines = 23;
int p_lines = 20;
int t_columns = 80;
int automargins = 1;

void init_tty() {
    struct termios tty_state;
    
    if(tcgetattr(1, &tty_state) < 0) {
	syslog(LOG_ERR, "tcgetattr(): %m");
	return;
    }
    cfmakeraw(&tty_state);
    tcsetattr(1, TCSANOW, &tty_state);
}

void do_move(int destcol, int destline) {
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

void change_scroll_range(int top, int bottom) {
    char buf[16], *p;
    
    sprintf(buf, "\33[%d;%dr", top + 1, bottom + 1);
    for(p = buf; *p; p++)
	ochar(*p);
}

void scroll_forward() {
    ochar('\33');
    ochar('D');
}
