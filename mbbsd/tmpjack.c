/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "config.h"
#include "struct.h"
#include "modes.h"
#include "common.h"
#include "proto.h"

extern userec_t cuser;

#define p_MAX 5  /* president«J¿ï¤H¼Æ */
#define j_MAX 4  /* 4ºØ¿ï¶µ */

extern char *BBSName;

#define PD_RECORD "etc/Bet/president.data"
#define PD_USER   "etc/Bet/president.user"

#define TMP_JACK_RECORD  "etc/tmp_jack.data"
#define TMP_JACK_USER    "etc/tmp_jack.user"

#define BARBQ            "etc/barbq.user"
#define BARBQ_PICTURE    "etc/barbq.pic"


static char *p_betname[p_MAX] = {
    "³s¾Ô", "³¯¤ô«ó", "§º·¡·ì", "§õ±Î", "³\\«H¨}"
};
static char *j_betname[j_MAX] = {
    "°¨¨ë", "©Ý¯îªÌ",  "·È°¨",  "¥§§J"
};

#ifndef _BBS_UTIL_C_
static int j_Is_Num(char*s, int n) {
    int i;
    for(i=0;i<n;i++)
	if(s[i]<'0' || s[i]>'9') return 0;
    return 1;
}

static void p_show_bet() {
    FILE *fp = fopen(PD_RECORD,"r");
    int i, total = 0, ticket[p_MAX] = {0, 0, 0, 0, 0};

    if(fp) {
	fscanf(fp,"%9d %9d %9d %9d %9d\n",
	       &ticket[0],&ticket[1],&ticket[2],&ticket[3],&ticket[4]);
	for(i = 0; i < p_MAX; i++)
	    total += ticket[i];
	fclose(fp);
    }

    prints("\033[33m1.%-6s: %-9d2.%-6s: %-9d3.%-6s: %-9d\n4.%-6s: %-9d5.%-6s: %-9d\n\033[m"
           "\033[42m ¤U©ãª÷ÃB:\033[31m %d ¤¸ \033[m",
           p_betname[0], ticket[0], p_betname[1], ticket[1],
           p_betname[2], ticket[2], p_betname[3], ticket[3],
           p_betname[4], ticket[4],
           total);
}

static void j_show_bet() {
    FILE *fp = fopen(TMP_JACK_RECORD,"r");
    int i, total = 0, ticket[j_MAX] = {0, 0, 0, 0};

    if(fp) {
        fscanf(fp,"%9d %9d %9d %9d\n",
               &ticket[0],&ticket[1],&ticket[2],&ticket[3]);
        for(i = 0; i < j_MAX; i++)
            total += ticket[i];
        fclose(fp);
    }

    prints("\033[33m1.%-6s: %-9d2.%-6s: %-9d3.%-6s: %-9d4.%-6s: %-9d\n\033[m\n"
           "\033[42m ¤Uª`Á`ª÷ÃB:\033[31m %d ¤¸ \033[m",
           j_betname[0], ticket[0], j_betname[1], ticket[1],
           j_betname[2], ticket[2], j_betname[3], ticket[3],
           total);
}

static void p_show_ticket_data() {
    clear();
    showtitle("Á`²Î½ä½L[§@ªÌ:Heat]", BBSName);
    move(1, 0);
    /*outs("
      [[1;31m¢z[[37m¢i[[31m    ¢z[[37m¢i[[31m¢z[[37m¢i¢i¢i¢i[[31m    ¢z[[37m¢i¢i¢i
      [[1;31m¢x[[37m¢i¢i[[31m  ¢x[[37m¢i[[31m¢x[[37m¢i[[31m¢w¢w¢{[[37m¢i[[31m¢z[[37m¢i[[31m¢w¢w¢{[[37m
      [[1;31m¢x[[37m¢i[[31m¢{[[37m¢i[[31m¢x[[37m¢i[[31m¢x[[37m¢i¢i¢i¢i[[31m¢}¢x[[37m¢i¢i¢i¢i¢i
      [[1;31m¢x[[37m¢i[[31m¢|¢{[[37m¢i¢i[[31m¢x[[37m¢i[[31m¢w¢w¢{[[37m¢i[[31m¢x[[37m¢i[[31m¢w¢w¢{[[37m
      [[1;31m¢x[[37m¢i[[31m  ¢|¢{[[37m¢i[[31m¢x[[37m¢i¢i¢i¢i[[31m¢}¢x[[37m¢i[[31m    ¢x[[37m¢i
      [[1;31m¢|¢}    ¢|¢}¢|¢w¢w¢w¢}  ¢|¢}    ¢|¢}
      [[1;33m¡»[[37m¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w[[33m¡»[[m
      [[1;35m                        ³W«h: ¥iÁÊ¶R¥|ºØ¤£¦P¿ï¶µ¡C
      µ¥µ²ªG¥X¨Ó´N¶}²¼µ¹¿ú­ò ¼úª÷¥Ñ¨t²Î©â¨ú5%¤§µ|ª÷¡C

      [[36m°¨¨ë[[37m¢w¢w¢w¢w¢w¢{               ¢z¢w¢w¢w¢w¢w[[36m¥§§J
      [[37m ¢u¢w[[33mFinals(2,3,2)[[37m¢t
      [[36m©Ý¯îªÌ[[37m¢w¢w¢w¢w¢}               ¢|¢w¢w¢w¢w¢w[[36m·È°¨[[m");
    */
    move(6,0);
    outs("                          2000Á`²Î¾÷\n");
    outs("        µ¥µ²ªG¥X¨Ó´N¶}²¼µ¹¿ú­ò ¼úª÷¥Ñ¨t²Î©â¨ú5%¤§µ|ª÷¡C");

    move(15,0);
    outs("\033[1;32m¥Ø«e±¡§Î:\033[m\n");
    p_show_bet();
    move(19, 0);
    reload_money();
    prints("\033[44m¿ú: %-10d \033[m\n\033[1m ½Ð¿ï¾Ü­nÁÊ¶RªººØÃþ(1~5)[Q:Â÷¶}]"
	   "\033[m:", cuser.money);
}


void j_show_ticket_data() {
    clear();
    showtitle("NBA½ä½L[§@ªÌ:Heat]", BBSName);
    move(1, 0);
    outs("
[1;31m¢z[37m¢i[31m    ¢z[37m¢i[31m¢z[37m¢i¢i¢i¢i[31m    ¢z[37m¢i¢i¢i
[1;31m¢x[37m¢i¢i[31m  ¢x[37m¢i[31m¢x[37m¢i[31m¢w¢w¢{[37m¢i[31m¢z[37m¢i[31m¢w¢w¢{[37m¢i
[1;31m¢x[37m¢i[31m¢{[37m¢i[31m¢x[37m¢i[31m¢x[37m¢i¢i¢i¢i[31m¢}¢x[37m¢i¢i¢i¢i¢i
[1;31m¢x[37m¢i[31m¢|¢{[37m¢i¢i[31m¢x[37m¢i[31m¢w¢w¢{[37m¢i[31m¢x[37m¢i[31m¢w¢w¢{[37m¢i
[1;31m¢x[37m¢i[31m  ¢|¢{[37m¢i[31m¢x[37m¢i¢i¢i¢i[31m¢}¢x[37m¢i[31m    ¢x[37m¢i
[1;31m¢|¢}    ¢|¢}¢|¢w¢w¢w¢}  ¢|¢}    ¢|¢}
[1;33m¡»[37m¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w[33m¡»[m
[1;35m                        ³W«h: ¥iÁÊ¶R¥|ºØ¤£¦P¿ï¶µ¡C
             µ¥µ²ªG¥X¨Ó´N¶}²¼µ¹¿ú­ò ¼úª÷¥Ñ¨t²Î©â¨ú5%¤§µ|ª÷¡C

             [36m°¨¨ë[37m¢w¢w¢w¢w¢w¢{               ¢z¢w¢w¢w¢w¢w[36m¥§§J
                          [37m ¢u¢w[33mFinals(2,3,2)[37m¢t
             [36m©Ý¯îªÌ[37m¢w¢w¢w¢w¢}               ¢|¢w¢w¢w¢w¢w[36m·È°¨[m");
    move(15,0);
    outs("\033[1;32m¥Ø«e¤Uª`ª¬ªp:\033[m\n");
    j_show_bet();
    move(19, 0);
    reload_money();
    prints("\033[44m¿ú: %-10d \033[m\n\033[1m ½Ð¿ï¾Ü­nÁÊ¶RªººØÃþ(1~4)[Q:Â÷¶}]"
           "\033[m:", cuser.money);
}

static void p_append_ticket_record(int ch, int n) {
    FILE *fp;
    int ticket[p_MAX] = {0,0,0,0,0};

    if((fp = fopen(PD_USER,"a"))) {
        fprintf(fp, "%s %d %d\n", cuser.userid, ch, n);
        fclose(fp);
    }

    if((fp = fopen(PD_RECORD,"r+"))) {
        fscanf(fp,"%9d %9d %9d %9d %9d\n",
	       &ticket[0], &ticket[1], &ticket[2], &ticket[3], &ticket[4]);
        ticket[ch] += n;
        rewind(fp);
        fprintf(fp, "%9d %9d %9d %9d %9d\n",
		ticket[0], ticket[1], ticket[2], ticket[3], ticket[4]);
        fclose(fp);
    } else if((fp = fopen(PD_RECORD,"w"))) {
        ticket[ch] += n;
        fprintf(fp,"%9d %9d %9d %9d %9d\n",
		ticket[0], ticket[1], ticket[2], ticket[3], ticket[4]);
        fclose(fp);
    }
}

static void j_append_ticket_record(int ch, int n) {
    FILE *fp;
    int ticket[j_MAX] = {0,0,0,0};

    if((fp = fopen(TMP_JACK_USER,"a"))) {
        fprintf(fp, "%s %d %d\n", cuser.userid, ch, n);
        fclose(fp);
    }

    if((fp = fopen(TMP_JACK_RECORD,"r+"))) {
        fscanf(fp,"%9d %9d %9d %9d\n",
               &ticket[0], &ticket[1], &ticket[2], &ticket[3]);
        ticket[ch] += n;
        rewind(fp);
        fprintf(fp, "%9d %9d %9d %9d\n",
                ticket[0], ticket[1], ticket[2], ticket[3]);
        fclose(fp);
    } else if((fp = fopen(TMP_JACK_RECORD,"w"))) {
        ticket[ch] += n;
        fprintf(fp,"%9d %9d %9d %9d\n",
                ticket[0], ticket[1], ticket[2], ticket[3]);
        fclose(fp);
    }
}

#define lockreturn0(unmode, state) if(lockutmpmode(unmode, state)) return 0

int p_ticket_main() {
    int ch, n;
    char buf[16];

    lockreturn0(TMPJACK, LOCK_MULTI);

    //more("etc/nba.doc",YEA);
    while(1) {
	p_show_ticket_data();
	ch = igetch();
	if(ch =='q' || ch == 'Q')
	    break;
	reload_money();
	if(cuser.money < 100)
	{
	    move(22,0);
	    prints("¿ú¤£°÷­ò ¦Ü¤Ö­n100¼Ú");
	    unlockutmpmode();
	    pressanykey();
	    return 0;
	}
	ch -= '1';

	if(ch > p_MAX-1 || ch < 0)
	    continue;
	n = 0;
	bzero(buf,sizeof(buf));
	do{
	    getdata(19, 0, "¦h¤Ö¿ú«¨?(100¥H¤W q:¸¨¶])?",
		    buf, 8, LCECHO);
	    if(buf[0] == 'q')
	    {
		unlockutmpmode();
		return 0;
	    }
	}while(!j_Is_Num(buf,strlen(buf)) || (n=atoi(buf)) < 100 ||
	       n > cuser.money);
	demoney(n);
	p_append_ticket_record(ch,n);
    }
    unlockutmpmode();
    return 0;
}

int j_ticket_main() {
    int ch, n;
    char buf[16];

    lockreturn0(TMPJACK, LOCK_MULTI);

    more("etc/nba.doc",YEA);
    while(1) {
        j_show_ticket_data();
        ch = igetch();
        if(ch =='q' || ch == 'Q')
            break;
	reload_money();
        if(cuser.money < 100)
        {
            move(22,0);
            prints("¿ú¤£°÷­ò ¦Ü¤Ö­n100¼Ú");
	    unlockutmpmode();
            pressanykey();
            return 0;
        }
        ch -= '1';
        if(ch > j_MAX-1 || ch < 0)
            continue;
        n = 0;
        bzero(buf,sizeof(buf));
	do{
	    getdata(19, 0, "¦h¤Ö¿ú«¨?(100¥H¤W q:¸¨¶])?",
		    buf, 8, LCECHO);
	    if(buf[0] == 'q')
	    {
		unlockutmpmode();
		return 0;
	    }
	}while(!j_Is_Num(buf,strlen(buf)) || (n=atoi(buf)) < 100 ||
	       n > cuser.money);
	demoney(n);
        j_append_ticket_record(ch,n);
    }
    unlockutmpmode();
    return 0;
}

//¥H¤UÄÝ©ó¯N¦×Âû¤lªºµ{¦¡
static int b_is_in(FILE *fp) {
    char buf[100],*ptr,id[20];

    while( fgets(buf,100,fp) )
    {
        ptr = buf;
        ptr += 5;
        strncpy(id,ptr,16);
        if( (ptr=strchr(id,' ')) ) *ptr = 0;
        if( !strcmp(id,cuser.userid) ) return 1;
    }
    return 0;
}

static void b_list_file() {
    more(BARBQ,YEA);
}

static void b_showfile(FILE *fd) {
    char buf[300];

    fseek(fd,0,SEEK_SET);
    move(1,0);
    while( fgets(buf,300,fd) )
	prints("%s",buf);
}

int reg_barbq() {
    char ch[2],sex[2],trf[2],nick[16];
    FILE *fp,*fd;

    if( !(fp = fopen(BARBQ,"r+")) || !(fd = fopen(BARBQ_PICTURE,"r")) )
    {
	move(22,0);prints("µwºÐ¤Ó¿S¤F µo¥Í¿ù»~!!");
	pressanykey();
	return 0;
    }


    while(1)
    {
	clear();
	b_showfile(fd);
	getdata(13, 0, "­n°Ñ¥[¯N¦×¶Ü(y:¥[¤J n:½Ö³¾§A s:¦C¥X¥Ø«e¥hªº¤H)?",
		ch, 2, LCECHO);
	if( ch[0] != 'y' && ch[0]!='s' )
	{
	    fclose(fp);
	    fclose(fd);
	    return 0;
	}

	if( ch[0] == 's' )
	{
	    b_list_file();
	    continue;
	}

	if( b_is_in(fp) )
	{
	    move(22,0);prints("§A¤§«e´N¥[¤J¤F ¦A¦¸´£¿ô ¤£¨Ó¦©¿ú­ò :)");
	    fclose(fp);
	    fclose(fd);
	    pressanykey();
	    return 0;
	}

	move(13,0);clrtoeol();prints("¶}©l¶ñ¼g¸ê®Æ.........");
	getdata(14, 0, "©Ê§O:[1:¨k 2:¤k]",
		sex, 2, LCECHO);
	getdata(14, 0, "«e©¹¤è¦¡:[1:¦Û¦æ  2:¥ý¶°¦Xµ¹¤u§@¤H­û±a]",
		trf, 2, LCECHO);
	getdata(14, 0, "¬°¦Û¤v¨ú¦U¼ÊºÙ§a:",
		nick, 9, LCECHO);

	if(
	    sex[0]<'1'||sex[0]>'2'||trf[0]<'1'||trf[0]>'2'||
	    strlen(nick)<=0
	    )
	{
	    move(15,0);prints("¿é¤J®æ¦¡¤£¹ï ­«¨Ó¤@¦¸§a!!");
	    pressanykey();
	    continue;
	}

	fseek(fp,0,SEEK_END);

	fprintf(fp, "±b¸¹:%-16s¼ÊºÙ:%-12s©Ê§O:%-6s¥æ³q:%-6s\n",
		cuser.userid, nick, (sex[0]=='1')?"¨k":"¤k", (trf[0]=='1')?"¦Û¦æ":"¶°¦X");

	move(22,0);prints("§¹¦¨¥[¤J¤âÄò °O±o¨Ó¼Ú ¤£µM¦©¿ú­ò ^_^ ");
	pressanykey();
	fclose(fp);
	fclose(fd);
	return 0;
    }
}
#endif
