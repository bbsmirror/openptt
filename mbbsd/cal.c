/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "config.h"
#include "pttstruct.h"
#include "common.h"
#include "modes.h"
#include "perm.h"
#include "proto.h"

extern struct utmpfile_t *utmpshm;
extern int usernum;

/* ���� Multi play */
static int count_multiplay(int unmode) {
    register int i, j;
    register userinfo_t *uentp;
    
    for(i = j = 0; i < USHM_SIZE; i++) {
	uentp = &(utmpshm->uinfo[i]);
	if(uentp->uid == usernum)
	    if(uentp->lockmode == unmode)
		j++;
    }
    return j;
}

extern userinfo_t *currutmp;
extern char *ModeTypeTable[];

int lockutmpmode(int unmode, int state) {
    int errorno = 0;
    
    if(currutmp->lockmode)
	errorno = 1;
    else if(count_multiplay(unmode))
	errorno = 2;
    
    if(errorno && !(state == LOCK_THIS && errorno == LOCK_MULTI)) {
	clear();
	move(10,20);
	if(errorno == 1)
	    prints("�Х����} %s �~��A %s ",
		   ModeTypeTable[currutmp->lockmode],
		   ModeTypeTable[unmode]);
	else 
	    prints("��p! �z�w����L�u�ۦP��ID���b%s",
		   ModeTypeTable[unmode]);
	pressanykey();
	return errorno;
    }
    
    setutmpmode(unmode);
    MPROTECT_UTMP_RW;
    currutmp->lockmode = unmode;
    MPROTECT_UTMP_R;
    return 0;
}

int unlockutmpmode() {
    MPROTECT_UTMP_RW;
    currutmp->lockmode = 0;
    MPROTECT_UTMP_R;
    return 0;
}

extern userec_t cuser;
extern userec_t xuser;

/* �ϥο������ */
int reload_money() {
    passwd_query(usernum, &xuser);
    cuser.money = xuser.money;
    cuser.lastsong = xuser.lastsong;
    return 0;
}

#define VICE_NEW   "vice.new"
#define VICE_BASE  "etc/vice.base"
#define VICE_COUNT "etc/vice.count"

/* Heat:�o�� */
static int vice(int money, char* item) {
    char genbuf[200], buf[128];
    fileheader_t mymail;
    int viceserial, base = 0;
    /* bfp�N�O�n��shm�Ө��N���ɮ׭� */
    FILE *bfp = fopen(VICE_BASE, "r+"),*cfp=fopen(VICE_COUNT,"a"),
        *nfp;
    
    sprintf(buf, BBSHOME"/home/%c/%s/%s",
	    cuser.userid[0], cuser.userid, VICE_NEW);
    nfp = fopen(buf, "a");
    if(!bfp || !nfp || !cfp)
	return 0;
   
    if(fgets(buf, 9, bfp))
	base = atoi(buf);
    else
	perror("can't open bingo file of vice file");
    viceserial = ++base;
    
    fprintf(cfp,"%s\n",cuser.userid);
    fprintf(nfp, "%08d\n", viceserial);
    rewind(bfp);
    fprintf(bfp, "%08d", base);
    fclose(bfp);
    fclose(nfp);
    fclose(cfp);
   
    sprintf(genbuf, BBSHOME "/home/%c/%s", cuser.userid[0], cuser.userid);
    stampfile(genbuf, &mymail);
    strcpy(mymail.owner, BBSNAME "�g�ٳ�");
    sprintf(mymail.title, "%s ��F%d$ \033[1;37m�s��[\033[1;33m%08d\033[m]",
	    item, money, viceserial);
    mymail.savemode = 0;
    unlink(genbuf);
    Link(BBSHOME "/etc/vice.txt", genbuf);   
    sprintf(genbuf,BBSHOME"/home/%c/%s/.DIR", cuser.userid[0], cuser.userid);
    
    append_record(genbuf, &mymail, sizeof(mymail));
    return 0;
}

#define lockreturn(unmode, state) if(lockutmpmode(unmode, state)) return 
#define lockreturn0(unmode, state) if(lockutmpmode(unmode, state)) return 0
#define lockbreak(unmode, state) if(lockutmpmode(unmode, state)) break
#define SONGBOOK  "etc/SONGBOOK"
#define OSONGPATH "etc/SONGO"
extern char trans_buffer[];
extern char save_title[];

static int osong(char *defaultid) {
    char destid[IDLEN + 1],buf[200],genbuf[200],filename[256],say[51];
    char receiver[60],ano[2];
    FILE *fp,*fp1;// *fp2;
    fileheader_t mail;
//, mail2;
    time_t now;
    
    now = time(NULL);
    strcpy(buf, Cdatedate(&now));
    reload_money();
    
    lockreturn0(OSONG, LOCK_MULTI);
    
    reload_money();
    /* Jaky �@�H�@���I�@�� */
    if(!strcmp(buf, Cdatedate(&cuser.lastsong)) && !HAS_PERM(PERM_SYSOP)) {
	move(22,0);
	outs("�A���Ѥw�g�I�L�o�A���ѦA�I�a....");
	refresh();
	pressanykey();

	unlockutmpmode();
	return 0;
    }

    if(cuser.money < 200) {
	move(22, 0);
	outs("�I�q�n200�ȭ�!....");
	refresh();
	pressanykey();
	unlockutmpmode();
	return 0;
    }
    move(12, 0);
    clrtobot();
    sprintf(buf, "�˷R�� %s �w��Ө�ڮ�۰��I�q�t��\n", cuser.userid);
    outs(buf);
    trans_buffer[0] = 0;
    if(!defaultid){
	  getdata(13, 0, "�n�I���֩O:[�i������ Enter ����q]", destid, IDLEN + 1, DOECHO);
	  while (!destid[0]){
	     a_menu("�I�q�q��", SONGBOOK,0 );
	     clear();
	     getdata(13, 0, "�n�I���֩O:[�i�� Enter ���s��q]", destid, IDLEN + 1, DOECHO);
	  }
	}
    else
	strcpy(destid,defaultid);

    /* Heat:�I�q�̰ΦW�\�� */    
    getdata(14,0, "�n�ΦW��?[y/n]:", ano, 2, DOECHO);
     
    if(!destid[0]) {
	unlockutmpmode();
	return 0;
    }
    
    getdata_str(14, 0, "�Q�n�n��L(�o)��..:", say, 51, DOECHO, "�ڷR�p..");
    sprintf(save_title, "%s:%s", (ano[0]=='y')?"�ΦW��":cuser.userid, say);
    getdata_str(16, 0, "�H��֪��H�c(�i��E-mail)?", receiver, 45,
		LCECHO, destid);
    

    
    if (!trans_buffer[0]){
       outs("\n���ۭn��q�o..�i�J�q���n�n����@���q�a..^o^");
       pressanykey();    
       a_menu("�I�q�q��", SONGBOOK,0 );
    }
    if(!trans_buffer[0] ||  strstr(trans_buffer, "home") ||
       strstr(trans_buffer, "boards") || !(fp = fopen(trans_buffer, "r"))) {
	unlockutmpmode();
	return 0;
    }
    
    strcpy(filename, OSONGPATH);
    
    stampfile(filename, &mail);
    
    unlink(filename);
    
    if(!(fp1 = fopen(filename, "w"))) {
	fclose(fp);
	unlockutmpmode();
	return 0;
    }
    
    strcpy(mail.owner, "�I�q��");
    sprintf(mail.title, "�� %s �I�� %s ", (ano[0]=='y')?"�ΦW��":cuser.userid, destid);
    mail.savemode = 0;
    
    while(fgets(buf, 200, fp)) {
	char *po;
	if(!strncmp(buf, "���D: ", 6)) {
	    clear();
	    move(10,10);prints("%s", buf);
	    pressanykey();
	    fclose(fp);
	    unlockutmpmode();
	    return 0;
	}
	while((po = strstr(buf, "<~Src~>"))) {
	    po[0] = 0;
	    sprintf(genbuf,"%s%s%s",buf,(ano[0]=='y')?"�ΦW��":cuser.userid,po+7);
	    strcpy(buf,genbuf);
        }
	while((po = strstr(buf, "<~Des~>"))) {
	    po[0] = 0;
	    sprintf(genbuf,"%s%s%s",buf,destid,po+7);
	    strcpy(buf,genbuf);
        }
	while((po = strstr(buf, "<~Say~>"))) {
	    po[0] = 0;
	    sprintf(genbuf,"%s%s%s",buf,say,po+7);
	    strcpy(buf,genbuf);
        }
	fputs(buf,fp1);
    }
    fclose(fp1);
    fclose(fp);

//    do_append(OSONGMAIL "/.DIR", &mail2, sizeof(mail2));
    
    if(do_append(OSONGPATH "/.DIR", &mail, sizeof(mail)) != -1) {
	  cuser.lastsong = time(NULL);
	/* Jaky �W�L 500 ���q�N�}�l�� */
	  if (get_num_records(OSONGPATH "/.DIR", sizeof(mail)) > 500){
	    delete_record(OSONGPATH "/.DIR", sizeof(mail), 1);
	  }
	/* ��Ĥ@������ */
	  demoney(200);
    }
    sprintf(save_title, "%s:%s", (ano[0]=='y')?"�ΦW��":cuser.userid, say);
    hold_mail(filename, destid);

    if(receiver[0]) {
#ifndef USE_BSMTP
	bbs_sendmail(filename, save_title, receiver);
#else
	bsmtp(filename, save_title, receiver,0);
#endif
    }
    clear();
    outs(
	"\n\n  ���߱z�I�q�����o..\n"
	"  �@�p�ɤ��ʺA�ݪO�|�۰ʭ��s��s\n"
	"  �j�a�N�i�H�ݨ�z�I���q�o\n\n"
	"  �I�q��������D�i�H��Note�O����ذϧ䵪��\n"
	"  �]�i�bNote�O��ذϬݨ�ۤv���I�q�O��\n"
	"  ������O�Q���N���]�w���Note�O�d��\n"
	"  ���ˤ����O�D���z�A��\n");
    pressanykey();
    vice(200, "�I�q");
    sortsong();
    topsong();

    unlockutmpmode();
    return 1;
}

int ordersong() {
    osong(NULL);
    return 0;
}

/* �ϥο������ */
int inumoney(char *tuser, int money) {
    int unum;
    
    if((unum = getuser(tuser))) {
	xuser.money += money;
	passwd_update(unum, &xuser);
	return xuser.money;
    } else
	return -1;
}

int inmoney(int money) {
    passwd_query(usernum, &xuser);
    cuser.money = xuser.money + money;
    passwd_update(usernum, &cuser);
    return cuser.money;
}

static int inmailbox(int m) {
    passwd_query(usernum, &xuser);
    cuser.exmailbox = xuser.exmailbox + m;
    passwd_update(usernum, &cuser);
    return cuser.exmailbox;
}

int deumoney(char *tuser, int money) {
    int unum;
    if((unum = getuser(tuser))) {
	if((unsigned long int)xuser.money <= (unsigned long int)money)
	    xuser.money=0;
	else
	    xuser.money -= money;
	passwd_update(unum, &xuser);
	return xuser.money;
    } else
	return -1;
}

int demoney(int money) {
    passwd_query(usernum, &xuser);
    if((unsigned long int)xuser.money <= (unsigned long int)money)
	cuser.money=0;
    else
	cuser.money = xuser.money - money;
    passwd_update(usernum, &cuser);
    return cuser.money;
}

extern int b_lines;

#if !HAVE_FREECLOAK
/* ������ */
int p_cloak() {
    char buf[4];
    getdata(b_lines-1, 0,
	    currutmp->invisible ? "�T�w�n�{��?[y/N]" : "�T�w�n����?[y/N]",
	    buf, 3, LCECHO);
    if(buf[0] != 'y')
	return 0;
    if(cuser.money >= 19) {
	demoney(19);
	vice(19, "cloak");
	MPROTECT_UTMP_RW;
	currutmp->invisible %= 2;
	outs((currutmp->invisible ^= 1) ? MSG_CLOAKED : MSG_UNCLOAK);
	MPROTECT_UTMP_R;
	refresh();
	safe_sleep(1);
    }
    return 0;
}
#endif

int p_from() {
    char ans[4];

    getdata(b_lines-2, 0, "�T�w�n��G�m?[y/N]", ans, 3, LCECHO);
    if(ans[0] != 'y')
	return 0;
    reload_money();
    if(cuser.money < 49)
	return 0;
    if(getdata_buf(b_lines-1, 0, "�п�J�s�G�m:",
		   currutmp->from, 17, DOECHO)) {
	demoney(49);
	vice(49,"home");
	MPROTECT_UTMP_RW;
	currutmp->from_alias=0;
	MPROTECT_UTMP_R;
    }
    return 0;
}

int p_exmail() {
    char ans[4],buf[200];
    int n;

    if(cuser.exmailbox >= MAX_EXKEEPMAIL) {
	sprintf(buf,"�e�q�̦h�W�[ %d �ʡA����A�R�F�C", MAX_EXKEEPMAIL);
	outs(buf);
	refresh();
	return 0;
    }
    sprintf(buf,"�z���W�� %d �ʮe�q�A�٭n�A�R�h��?",
	    cuser.exmailbox);
    
    getdata_str(b_lines-2, 0, buf, ans, 3, LCECHO, "10");
    
    n = atoi(ans);
    if(!ans[0] || !n)
	return 0;
    if(n + cuser.exmailbox > MAX_EXKEEPMAIL)
	n = MAX_EXKEEPMAIL - cuser.exmailbox;
    reload_money();
    if(cuser.money < n * 1000)
	return 0;
    demoney(n * 1000);
    vice(n * 1000, "mail");
    inmailbox(n);
    return 0;
}

void mail_redenvelop(char* from, char* to, int money, char mode){
    char genbuf[200];
    fileheader_t fhdr;
    time_t now;
    FILE* fp;
    sprintf(genbuf, "home/%c/%s", to[0], to);
    stampfile(genbuf, &fhdr);
    if (!(fp = fopen(genbuf, "w")))
        return;
    now = time(NULL);
    fprintf(fp, "�@��: %s\n"
                "���D: �۰]�i�_\n"
                "�ɶ�: %s\n"
                "\033[1;33m�˷R�� %s �G\n\n\033[m"
                "\033[1;31m    �ڥ]���A�@�� %d �����j���]�� ^_^\n\n"
                "    §�����N���A�Я���...... ^_^\033[m\n"
                , from, ctime(&now), to, money);
    fclose(fp);
    sprintf(fhdr.title, "�۰]�i�_");
    strcpy(fhdr.owner, from);

    if (mode == 'y')
       vedit(genbuf, NA, NULL);
    sprintf(genbuf, "home/%c/%s/.DIR", to[0], to);       
    append_record(genbuf, &fhdr, sizeof(fhdr));
}

int p_give() {
    int money;
    char id[IDLEN + 1], genbuf[90];
    time_t now = time(0);
    
    move(1,0);
    usercomplete("�o�쩯�B�઺id:", id);
    if(!id[0] || !strcmp(cuser.userid,id) ||
       !getdata(2, 0, "�n���h�ֿ�:", genbuf, 7, LCECHO))
	return 0;
    money = atoi(genbuf);
    reload_money();
    if(money > 0 && cuser.money >= money && (inumoney(id, money) != -1)) {
	demoney(money);
	now = time(NULL);
	sprintf(genbuf,"%s\t��%s\t%d\t%s", cuser.userid, id, money,
		ctime(&now));
	log_file(FN_MONEY, genbuf);
	genbuf[0] = 'n';
	getdata(3, 0, "�n�ۦ�Ѽg���]�U�ܡH[y/N]", genbuf, 2, LCECHO);
	mail_redenvelop(cuser.userid, id, money, genbuf[0]);
    }
    return 0;
}

int p_touch_boards() {
    touch_boards();
    move(b_lines - 1, 0);
    outs("BCACHE ��s����\n");
    pressanykey();
    return 0;
}

int p_sysinfo() {
    char buf[100];
    long int total,used;
    float p;
    
    move(b_lines-1,0);
    clrtoeol();
    cpuload(buf);
    outs("CPU �t�� : ");
    outs(buf);
    
    p = swapused(&total,&used);
    sprintf(buf, " �����O����ϥβv: %.3f  (����:%ldMB �α�:%ldMB)\n",
	    p, total >> 20, used >> 20);
    outs(buf);
    pressanykey();
    return 0;
}

/* �p�p��� */
static void ccount(float *a, float b, int cmode) {
    switch(cmode) {
    case 0:
    case 1:
    case 2:
        *a += b;
        break;
    case 3:
        *a -= b;
        break;
    case 4:
        *a *= b;
        break;
    case 5:
        *a /= b;
        break;
    }
}

int cal() {
    float a = 0;
    char flo = 0, ch = 0;
    char mode[6] = {' ','=','+','-','*','/'} , cmode = 0;
    char buf[100] = "[            0] [ ] ", b[20] = "0";
    
    move(b_lines - 1, 0);
    clrtoeol();
    outs(buf);
    move(b_lines, 0);
    clrtoeol();
    outs("\033[44m �p�p���  \033[31;47m      (0123456789+-*/=) "
	 "\033[30m��J     \033[31m  "
	 "(Q)\033[30m ���}                   \033[m");
    while(1) {
	ch = igetch();
	switch(ch) {
	case '\r':
            ch = '=';
	case '=':
	case '+':
	case '-':
	case '*':
	case '/':
            ccount(&a, atof(b), cmode);
            flo = 0;
            b[0] = '0';
            b[1] = 0;
            move(b_lines - 1, 0);
            sprintf(buf, "[%13.2f] [%c] ", a, ch);
            outs(buf);
            break;
	case '.':
            if(!flo)
		flo = 1;
            else
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
	case '0':
	    if(strlen(b) > 13)
		break;
	    if(flo || b[0] != '0')
		sprintf(b,"%s%c",b,ch);
	    else
		b[0]=ch;
	    move(b_lines - 1, 0);
	    sprintf(buf, "[%13s] [%c]", b, mode[(int)cmode]);
	    outs(buf);
	    break;
	case 'q':
	    return 0;
	}
	
	switch(ch) {
	case '=':
	    a = 0;
	    cmode = 0;
	    break;
	case '+':
	    cmode = 2;
	    break;
	case '-':
	    cmode = 3;
	    break;
	case '*':
	    cmode = 4;
	    break;
	case '/':
	    cmode = 5;
	    break;
	}
    }
}
