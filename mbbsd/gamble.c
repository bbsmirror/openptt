/* $Id$ */
#include <stdio.h>
#include <sys/types.h>
#include "config.h"
#include "pttstruct.h"
#include "common.h"
#include "modes.h"
#include "proto.h"

extern char *BBSName;

#define FN_TICKET_RECORD "etc/ticket.data"
#define FN_TICKET_USER   "etc/ticket.user"

static char *betname[8] = {"Ptt", "Jaky",  "Action",  "Heat",
			   "DUNK", "Jungo", "waiting", "wofe"};
#ifndef _BBS_UTIL_C_
static void show_bet() {
    FILE *fp = fopen(FN_TICKET_RECORD,"r");
    int i, total = 0, ticket[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    if(fp) {
	fscanf(fp,"%9d %9d %9d %9d %9d %9d %9d %9d\n",
	       &ticket[0],&ticket[1],&ticket[2],&ticket[3],
	       &ticket[4],&ticket[5],&ticket[6],&ticket[7]);
	for(i = 0; i < 8; i++)
	    total += ticket[i];
	fclose(fp);
    }
    
    prints("\033[33m1.%-6s: %-9d2.%-6s: %-9d3.%-6s: %-9d4.%-6s: %-9d\n"
	   "5.%-6s: %-9d6.%-6s: %-9d7.%-6s: %-9d8.%-6s: %-9d\033[m\n"
	   "\033[42m �U�`�`���B:\033[31m %d00 �� \033[m",
	   betname[0], ticket[0], betname[1], ticket[1],
	   betname[2], ticket[2], betname[3], ticket[3],
	   betname[4], ticket[4], betname[5], ticket[5],
	   betname[6], ticket[6], betname[7], ticket[7], total);
}

extern userec_t cuser;

static void show_ticket_data() {
    clear();
    showtitle("��tt��L", BBSName);
    move(2, 0);
    outs("\033[32m�W�h:\033[m 1.�i�ʶR�K�ؤ��P�������m���C�C�i�n��100���C\n"
	 "      2.�C�� 2:00 11:00 16:00 21:00 �}���C\n"
	 "      3.�}���ɷ|��X�@�رm��, ���ʶR�ӱm����, �h�i���ʶR���i�Ƨ���"
	 "�`����C\n"
	 "      4.�C�������Ѩt�Ω��5%���|���C\n\n"
	 "\033[32m�e�X���}�����G:\033[m" );
    show_file(FN_TICKET, 8, -1, NO_RELOAD);
    move(15,0);
    outs("\033[1;32m�ثe�U�`���p:\033[m\n");
    show_bet();
    move(20, 0);
    reload_money();
    prints("\033[44m��: %-10d \033[m\n\033[1m �п�ܭn�ʶR������(1~8)[Q:���}]"
	   "\033[m:", cuser.money);
}

static void append_ticket_record(int ch, int n) {
    FILE *fp;
    int ticket[8] = {0,0,0,0,0,0,0,0};
    
    if((fp = fopen(FN_TICKET_USER,"a"))) {
	fprintf(fp, "%s %d %d\n", cuser.userid, ch, n);
	fclose(fp);
    }
    
    if((fp = fopen(FN_TICKET_RECORD,"r+"))) {
	fscanf(fp,"%9d %9d %9d %9d %9d %9d %9d %9d\n",
	       &ticket[0], &ticket[1], &ticket[2], &ticket[3],
	       &ticket[4], &ticket[5], &ticket[6], &ticket[7]);
	ticket[ch] += n;
	rewind(fp);
	fprintf(fp, "%9d %9d %9d %9d %9d %9d %9d %9d\n",
		ticket[0], ticket[1], ticket[2], ticket[3],
		ticket[4], ticket[5], ticket[6], ticket[7]);
	fclose(fp);
    } else if((fp = fopen(FN_TICKET_RECORD,"w"))) {
	ticket[ch] += n;
	fprintf(fp,"%9d %9d %9d %9d %9d %9d %9d %9d\n",
		ticket[0], ticket[1], ticket[2], ticket[3],
		ticket[4], ticket[5], ticket[6], ticket[7]);
	fclose(fp);
    }
}

#define lockreturn0(unmode, state) if(lockutmpmode(unmode, state)) return 0

int ticket_main() {
    int ch, n;
    
    lockreturn0(TICKET, LOCK_MULTI);
    
    while(1) {
	show_ticket_data();
	ch = igetch();
	if(ch=='q' || ch == 'Q')
	    break;
	ch -= '1';
	if(ch > 7 || ch < 0)
	    continue;
	n = 0;
	ch_buyitem(100, "etc/buyticket", &n);
	if(n > 0)
	    append_ticket_record(ch,n);
    }
    unlockutmpmode();
    return 0;
}
#endif
