/* $Id$ */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "config.h"
#include "struct.h"
#include "common.h"
#include "modes.h"
#include "proto.h"

extern char *BBSName;

#define VICE_PLAY   BBSHOME "/etc/vice/vice.play"
#define VICE_JAKPOT BBSHOME "/etc/vice/vice.jakpot"
#define VICE_DATA   "vice.data"
#define VICE_BINGO  BBSHOME "/etc/vice.bingo"
#define VICE_SHOW   BBSHOME "/etc/vice.show"
/*
#define VICE_TOSEE  "vice.tosee"
*/
#define VICE_LOST   BBSHOME "/etc/vice/vice.lost"
#define VICE_WIN    BBSHOME "/etc/vice/vice.win"
#define VICE_END    BBSHOME "/etc/vice/vice.end"
#define VICE_NO     BBSHOME "/etc/vice/vice.no"
#define MAX_NO_PICTURE   2
#define MAX_WIN_PICTURE  2
#define MAX_LOST_PICTURE 3
#define MAX_END_PICTURE  5

static char tbingo[6][9];

static char *substring(const char* str, int begin, int end) {
    char *buf = (char*)malloc(9);
    int i;

    for(i = 0; begin <= end; i++, begin++)
        buf[i] = str[begin];
    buf[i] = '\0';
    return buf;
}

static int vice_load(FILE *fb) {
    char buf[16], *ptr;
    int i = 0;

    bzero((char*)tbingo, sizeof(tbingo));
    while(i < 6 && fgets(buf, 15, fb)) {
        if((ptr = strchr(buf, '\n')))
            *ptr = 0;
        strcpy(tbingo[i++], buf);
    }
    fclose(fb);
    return 0;
}

static int check(char *data) {
    int i = 0, j;

    if(!strcmp(data, tbingo[0]))
        return 8;

    for(j = 8; j > 0; j--)
        for(i = 1; i < 6; i++)
            if(!strcmp(substring(data, 8 - j, 7),
                       substring(tbingo[i], 8-j, 7)))
                return j - 1;
    return 0;
}

static int ran_showfile(int y, int x, char *filename, int maxnum) {
    FILE *fs;
    char buf[512];

    bzero(buf, sizeof(char) * 512);
    sprintf(buf, "%s%d", filename, rand() % maxnum + 1);
    if(!(fs = fopen(buf, "r"))) {
        move(10,10);
        prints("can't open file: %s", buf);
        return 0;
    }

    move(y, x);

    while(fgets(buf, 511, fs))
        prints("%s", buf);

    fclose(fs);
    return 1;
}

static int ran_showmfile(char *filename, int maxnum) {
    char buf[256];

    sprintf(buf, "%s%d", filename, rand() % maxnum + 1);
    return more(buf, NA);
}

extern userec_t cuser;
#define lockreturn0(unmode, state) if(lockutmpmode(unmode, state)) return 0

int vice_main() {
    FILE *fb = fopen(VICE_BINGO, "r"), *fd, *fj;
    char buf_data[256]
	, serial[16], ch[2], *ptr;
    int TABLE[] = {0,10,200,1000,4000,10000,40000,100000,200000};
    int total = 0, money, i = 4, j = 0;

    sprintf(buf_data, BBSHOME "/home/%c/%s/%s",
            cuser.userid[0], cuser.userid, VICE_DATA);
    fd = fopen(buf_data, "r");

    if(!fd) {
        ran_showmfile(VICE_NO, MAX_NO_PICTURE);
        return 0;
    }
    clear();
    ran_showfile(0, 0, VICE_PLAY, 1);
    ran_showfile(10, 0, VICE_SHOW, 1);

    getdata(8, 0, "任一鍵:對獎 s:查票 q:離開): ", ch, 2, LCECHO);

    if(ch[0] == 'q' || ch[0] == 'Q'){
	if(fd) fclose(fd);
	return 0;
    }

    if(ch[0] == 's' || ch[0]=='S') {
        j = 0;
        i = 0;
        move(10,22);
        clrtoeol();
        prints("這一期的發票號碼");
        while(fgets(serial, 15, fd)) {
            if((ptr = strchr(serial,'\r')))
                *ptr = 0;
            if(j == 0)
                i++;
            move(10 + i, 20 + j);
            prints("%s", serial);
            j += 9;
            j %= 45;
        }
        getdata(8, 0, "妳要開始對獎了嗎？(或是按q離開)): ", ch, 2, LCECHO);
        if(ch[0] == 'q'){
            if(fd) fclose(fd);
            return 0;
        }
    }

    lockreturn0(VICE, LOCK_MULTI);

    if(!fd) {
        ran_showmfile(VICE_END, MAX_END_PICTURE);
        unlockutmpmode();
        return 0;
    }

    showtitle("發票對獎", BBSName);
    rewind(fd);
    vice_load(fb);
    while(fgets(serial, 15, fd)) {
        if((ptr = strchr(serial,'\n')))
            *ptr = 0;
        money = TABLE[check(serial)];
        total += money;
        prints("%s 中了 %d\n", serial, money);
    }
    pressanykey();
    if(total > 0) {
        if(!(fj = fopen(VICE_JAKPOT, "a")))
            perror("can't open jakpot file");
        ran_showmfile(VICE_WIN, MAX_WIN_PICTURE);
        move(22,0);
        clrtoeol();
        prints("全部的發票中了 %d 塊錢\n", total);
        inmoney(total);
        fprintf(fj, "%-15s 中了$%d\n", cuser.userid, total);
        fclose(fj);
    } else
        ran_showmfile(VICE_LOST, MAX_LOST_PICTURE);

    fclose(fd);

    unlink(buf_data);

    pressanykey();
    unlockutmpmode();
    return 0;
}
