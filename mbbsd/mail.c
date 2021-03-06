/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"
#include "pttstruct.h"
#include "common.h"
#include "perm.h"
#include "modes.h"
#include "proto.h"

extern int b_lines;               /* Screen bottom line number: t_lines-1 */
extern char save_title[];         /* used by editor when inserting */
extern int curredit;
extern char *err_uid;
extern char *msg_cancel;
extern char *msg_uid;
extern char *fn_overrides;
extern char quote_file[80];
extern char quote_user[80];
extern char *fn_notes;
extern char *msg_mailer;
extern char *msg_sure_ny;
extern char *BBSName;
extern char currtitle[44];
extern unsigned char currfmode;               /* current file mode */
extern char *msg_del_ny;
extern char currfile[FNLEN];
extern int currmode;
extern char currboard[];        /* name of currently selected board */
extern char *str_space;
extern char *str_author1;
extern char *str_author2;
extern userinfo_t *currutmp;
extern unsigned int currstat;
extern pid_t currpid;
extern int usernum;
extern char *str_mail_address;
extern userec_t cuser;

char currmaildir[32];
static char msg_cc[] = "\033[32m[群組名單]\033[m\n";
static char listfile[] = "list.0";
static int mailkeep = 0, mailsum = 0;
static int mailsumlimit = 0,mailmaxkeep = 0;

int setforward() {
    char buf[80], ip[50] = "", yn[4];
    FILE *fp;
    
    sethomepath(buf, cuser.userid);
    strcat(buf,"/.forward");
    if((fp = fopen(buf,"r"))) {
	fscanf(fp,"%s",ip);
	fclose(fp);
    }
    getdata_buf(b_lines - 1, 0, "請輸入信箱自動轉寄的email地址:",
		ip, 41, DOECHO);
    if(ip[0] && ip[0] != ' ') {
	getdata(b_lines, 0, "確定開啟自動轉信功\能?(Y/n)", yn, 3,
		LCECHO);
	if(yn[0] != 'n' && (fp = fopen(buf, "w"))) {
	    move(b_lines,0);
	    clrtoeol();
	    fprintf(fp,"%s",ip);
	    fclose(fp);
	    outs("設定完成!");
	    refresh();
	    return 0;
	}
    }
    move(b_lines,0);
    clrtoeol();
    outs("取消自動轉信!");
    unlink(buf);
    refresh();
    return 0;
}

int built_mail_index() {
    char genbuf[128];
    
    sprintf(genbuf, BBSHOME "/bin/buildir " BBSHOME "/home/%c/%s",
	    cuser.userid[0], cuser.userid);
    move(22,0);
    prints("\033[1;31m已經處理完畢!! 諸多不便 敬請原諒~\033[m");pressanykey();
    system(genbuf);
    return 0;
}

int mail_muser(userec_t muser, char *title, char *filename) {
    fileheader_t mhdr;
    char temp[128], buf[80];
	  
    sethomepath(buf, muser.userid);
    if(stampfile(buf, &mhdr))
    	return 0;
    strcpy(mhdr.owner, cuser.userid);
    strncpy(mhdr.title, title, TTLEN);
    mhdr.savemode = 0;
    mhdr.filemode = 0;
    sethomedir(temp, muser.userid);
    Link(filename, buf);
    append_record(temp, &mhdr, sizeof(mhdr));
    return 0;
}

/* Heat: 用id來寄信,內容則link準備好的檔案 */
int mail_id(char* id, char *title, char *filename, char *owner) {
    fileheader_t mhdr;
    char genbuf[200];
    sprintf(genbuf, BBSHOME "/home/%c/%s", id[0], id);     
    stampfile(genbuf, &mhdr);
    strcpy(mhdr.owner, owner);
    strncpy(mhdr.title, title, TTLEN);
    mhdr.savemode = 0;
    mhdr.filemode = 0;
    Link(filename, genbuf);
    sprintf(genbuf, BBSHOME "/home/%c/%s/.DIR", id[0], id);     
    append_record(genbuf, &mhdr, sizeof(mhdr));
    return 0;
}    
          
int invalidaddr(char *addr) {
    if(*addr == '\0')
	return 1;                   /* blank */
    while(*addr) {
	if(not_alnum(*addr) && !strchr("[].%!@:-_;", *addr))
	    return 1;
	addr++;
    }
    return 0;
}

int m_internet() {
    char receiver[60];
    
    getdata(20, 0, "收信人：", receiver, 60, DOECHO);
    if(strchr(receiver, '@') && !invalidaddr(receiver) &&
       getdata(21, 0, "主  題：", save_title, TTLEN, DOECHO))
	do_send(receiver, save_title);
    else {
	move(22, 0);
	outs("收信人或主題不正確, 請重新選取指令");
	pressanykey();
    }
    return 0;
}

void m_init() {
    sethomedir(currmaildir, cuser.userid);
}

int chkmailbox() {
    if(!HAVE_PERM(PERM_SYSOP) && !HAVE_PERM(PERM_MAILLIMIT)) {
	if(HAS_PERM(PERM_BM))
	    mailsumlimit = 300;
	else if(HAS_PERM(PERM_LOGINOK))
	    mailsumlimit = 150;
	else
	    mailsumlimit = 100;
	mailsumlimit += cuser.exmailbox * 10;
	mailmaxkeep = MAX_KEEPMAIL + cuser.exmailbox;
	m_init();
	if((mailkeep = get_num_records(currmaildir, sizeof(fileheader_t))) >
	   mailmaxkeep) {
	    move(b_lines, 0);
	    clrtoeol();
	    bell();
	    prints("您保存信件數目 %d 超出上限 %d, 請整理",
		   mailkeep, mailmaxkeep);
	    bell();
	    refresh();
	    igetch();
	    return mailkeep;
	}
	if((mailsum = get_sum_records(currmaildir, sizeof(fileheader_t))) >
	   mailsumlimit) {
	    move(b_lines, 0);
	    clrtoeol();
	    bell();
	    prints("您保存信件容量 %d(k)超出上限 %d(k), 請整理",
		   mailsum, mailsumlimit);
	    bell();
	    refresh();
	    igetch();
	    return mailkeep;
	}
    }
    return 0;
}

static void do_hold_mail(char *fpath, char *receiver, char *holder) {
    char buf[80], title[128];
    
    fileheader_t mymail;
    
    sethomepath(buf, holder);
    stampfile(buf, &mymail);
    
    mymail.savemode = 'H';        /* hold-mail flag */
    mymail.filemode = FILE_READ;
    strcpy(mymail.owner, "[備.忘.錄]");
    if(receiver) {
	sprintf(title, "(%s) %s", receiver, save_title);
	strncpy(mymail.title, title, TTLEN);
    } else
	strcpy(mymail.title, save_title);
    
    sethomedir(title, holder);
    
    unlink(buf);
    Link(fpath, buf);
    /* Ptt: append_record->do_append */
    do_append(title, &mymail, sizeof(mymail));
}

extern userec_t xuser;

void hold_mail(char *fpath, char *receiver) {
    char buf[4];
    
    getdata(b_lines - 1, 0, "已順利寄出，是否自存底稿(Y/N)？[N] ",
	    buf, 4, LCECHO);
    
    if(buf[0] == 'y')
	do_hold_mail(fpath, receiver, cuser.userid);
}

int do_send(char *userid, char *title) {
    fileheader_t mhdr;
    char fpath[STRLEN];
    char receiver[IDLEN];
    char genbuf[200];
    int internet_mail;
    
    if(strchr(userid, '@'))
	internet_mail = 1;
    else {
	internet_mail = 0;
	if(!getuser(userid))
	    return -1;
	if(!(xuser.userlevel & PERM_READMAIL))
	    return -3;
	
	if(!title)
	    getdata(2, 0, "主題：", save_title, TTLEN, DOECHO);
	curredit |= EDIT_MAIL;
	curredit &= ~EDIT_ITEM;
    }
    
    setutmpmode(SMAIL);
    
    fpath[0] = '\0';
    
    if(internet_mail) {
	int res, ch;

	if(vedit(fpath, NA, NULL) == -1) {
	    unlink(fpath);
	    clear();
	    return -2;
	}
	clear();
	prints("信件即將寄給 %s\n標題為：%s\n確定要寄出嗎? (Y/N) [Y]",
	       userid, title);
	ch = igetch();
	switch(ch) {
	case 'N':
	case 'n':
	    outs("N\n信件已取消");
	    res = -2;
	    break;
	default:
	    outs("Y\n請稍候, 信件傳遞中...\n");
	    res =
#ifndef USE_BSMTP
		bbs_sendmail(fpath, title, userid);
#else
            bsmtp(fpath, title, userid,0);
#endif
	    hold_mail(fpath, userid);
	}
	unlink(fpath);
	return res;
    } else {
	strcpy(receiver, userid);
	sethomepath(genbuf, userid);
	stampfile(genbuf, &mhdr);
	strcpy(mhdr.owner, cuser.userid);
	strncpy(mhdr.title, save_title, TTLEN);
	mhdr.savemode = '\0';
	if(vedit(genbuf, YEA, NULL) == -1) {
	    unlink(genbuf);
	    clear();
	    return -2;
	}
	clear();
	sethomedir(fpath, userid);
	if(append_record(fpath, &mhdr, sizeof(mhdr)) == -1)
	    return -1;
	
	hold_mail(genbuf, userid);
	return 0;
    }
}

void my_send(char *uident) {
    switch(do_send(uident, NULL)) {
    case -1:
	outs(err_uid);
	break;
    case -2:
	outs(msg_cancel);
	break;
    case -3:
	prints("使用者 [%s] 無法收信", uident);
	break;
    }
    pressanykey();
}

int m_send() {
    char uident[40];

    stand_title("且聽風的話");
    usercomplete(msg_uid, uident);
    showplans(uident);
    if(uident[0])
	my_send(uident);
    return 0;
}

/* 群組寄信、回信 : multi_send, multi_reply */
extern struct word_t *toplev;

static void multi_list(int *reciper) {
    char uid[16];
    char genbuf[200];

    while(1) {
	stand_title("群組寄信名單");
	ShowNameList(3, 0, msg_cc);
	getdata(1, 0,
		"(I)引入好友 (O)引入上線通知 (N)引入新文章通知 "
		"(0-9)引入其他特別名單\n"
		"(A)增加     (D)刪除         (M)確認寄信名單   (Q)取消 ？[M]",
		genbuf, 4, LCECHO);
	switch(genbuf[0]) {
	case 'a':
	    while(1) {
		move(1, 0);
		usercomplete("請輸入要增加的代號(只按 ENTER 結束新增): ", uid);
		if(uid[0] == '\0')
		    break;
		
		move(2, 0);
		clrtoeol();
		
		if(!searchuser(uid))
		    outs(err_uid);
		else if(!InNameList(uid)) {
		    AddNameList(uid);
		    (*reciper)++;
		}
		ShowNameList(3, 0, msg_cc);
	    }
	    break;
	case 'd':
	    while(*reciper) {
		move(1, 0);
		namecomplete("請輸入要刪除的代號(只按 ENTER 結束刪除): ", uid);
		if(uid[0] == '\0')
		    break;
		if(RemoveNameList(uid))
		    (*reciper)--;
		ShowNameList(3, 0, msg_cc);
	    }
	    break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	    listfile[5] = genbuf[0];
	    genbuf[0] = '1';
	case 'i':
	    setuserfile(genbuf, genbuf[0] == '1' ? listfile : fn_overrides);
	    ToggleNameList(reciper, genbuf, msg_cc);
	    break;
	case 'o':
	    setuserfile(genbuf, "alohaed");
	    ToggleNameList(reciper, genbuf, msg_cc);
	    break;
	case 'n':
	    setuserfile(genbuf, "postlist");
	    ToggleNameList(reciper, genbuf, msg_cc);
	    break;
	case 'q':
	    *reciper = 0;
	    return;
	default:
	    return;
	}
    }
}

static void multi_send(char *title) {
    FILE *fp;
    struct word_t *p;
    fileheader_t mymail;
    char fpath[TTLEN], *ptr;
    int reciper, listing;
    char genbuf[256];
    
    CreateNameList();
    listing = reciper = 0;
    if(*quote_file) {
	AddNameList(quote_user);
	reciper = 1;
	fp = fopen(quote_file, "r");
	while(fgets(genbuf, 256, fp)) {
	    if(strncmp(genbuf, "※ ", 3)) {
		if(listing)
		    break;
	    } else {
		if(listing) {
		    strtok(ptr = genbuf + 3, " \n\r");
		    do {
			if(searchuser(ptr) && !InNameList(ptr) &&
			   strcmp(cuser.userid, ptr)) {
			    AddNameList(ptr);
			    reciper++;
			}
		    } while((ptr = (char *)strtok(NULL, " \n\r")));
		} else if(!strncmp(genbuf + 3, "[通告]", 6))
		    listing = 1;
	    }
	}
	ShowNameList(3, 0, msg_cc);
    }
    
    multi_list(&reciper);
    move(1, 0);
    clrtobot();
    
    if(reciper) {
	setutmpmode(SMAIL);
	if(title)
	    do_reply_title(2, title);
	else {
	    getdata(2, 0, "主題：", fpath, 64, DOECHO);
	    sprintf(save_title, "[通告] %s", fpath);
	}
	
	setuserfile(fpath, fn_notes);
	
	if((fp = fopen(fpath, "w"))) {
	    fprintf(fp, "※ [通告] 共 %d 人收件", reciper);
	    listing = 80;

	    for(p = toplev; p; p = p->next) {
		reciper = strlen(p->word) + 1;
		if(listing + reciper > 75) {
		    listing = reciper;
		    fprintf(fp, "\n※");
		} else
		    listing += reciper;
		
		fprintf(fp, " %s", p->word);
	    }
	    memset(genbuf, '-', 75);
	    genbuf[75] = '\0';
	    fprintf(fp, "\n%s\n\n", genbuf);
	    fclose(fp);
	}
	
	curredit |= EDIT_LIST;

	if(vedit(fpath, YEA, NULL) == -1) {
	    unlink(fpath);
	    curredit = 0;
	    outs(msg_cancel);
	    pressanykey();
	    return;
	}
	
	stand_title("寄信中...");
	refresh();
	
	listing = 80;
	
	for(p = toplev; p; p = p->next) {
	    reciper = strlen(p->word) + 1;
	    if(listing + reciper > 75) {
		listing = reciper;
		outc('\n');
	    } else {
		listing += reciper;
		outc(' ');
	    }
	    outs(p->word);
	    if(searchuser(p->word) && strcmp(STR_GUEST, p->word) )
		sethomepath(genbuf, p->word);
	    else
		continue;
	    stampfile(genbuf, &mymail);
	    unlink(genbuf);
	    Link(fpath, genbuf);
	    
	    strcpy(mymail.owner, cuser.userid);
	    strcpy(mymail.title, save_title);
	    mymail.savemode = 'M';    /* multi-send flag */
	    sethomedir(genbuf, p->word);
	    if(append_record(genbuf, &mymail, sizeof(mymail)) == -1)
		outs(err_uid);
	}
	hold_mail(fpath, NULL);
	unlink(fpath);
	curredit = 0;
    } else
	outs(msg_cancel);
    pressanykey();
}

static int multi_reply(int ent, fileheader_t *fhdr, char *direct) {
    if(fhdr->savemode != 'M')
	return mail_reply(ent, fhdr, direct);

    stand_title("群組回信");
    strcpy(quote_user, fhdr->owner);
    setuserfile(quote_file, fhdr->filename);
    multi_send(fhdr->title);
    return 0;
}

int mail_list() {
    stand_title("群組作業");
    multi_send(NULL);
    return 0;
}

int mail_all() {
    FILE *fp;
    fileheader_t mymail;
    char fpath[TTLEN];
    char genbuf[200];
    extern struct uhash_t *uhash;
    int i, unum;
    char *userid;
    
    stand_title("給所有使用者的系統通告");
    setutmpmode(SMAIL);
    getdata(2, 0, "主題：", fpath, 64, DOECHO);
    sprintf(save_title, "[系統通告]\033[1;32m %s\033[m", fpath);
    
    setuserfile(fpath, fn_notes);
    
    if((fp = fopen(fpath, "w"))) {
	fprintf(fp, "※ [\033[1m系統通告\033[m] 這是封給所有使用者的信\n");
	fprintf(fp, "-----------------------------------------------------"
		"----------------------\n");
	fclose(fp);
    }
    
    *quote_file = 0;
    
    curredit |= EDIT_MAIL;
    curredit &= ~EDIT_ITEM;
    if(vedit(fpath, YEA, NULL) == -1) {
	curredit = 0;
	unlink(fpath);
	outs(msg_cancel);
	pressanykey();
	return 0;
    }
    curredit = 0;
    
    setutmpmode(MAILALL);
    stand_title("寄信中...");
    
    sethomepath(genbuf, cuser.userid);
    stampfile(genbuf, &mymail);
    unlink(genbuf);
    Link(fpath, genbuf);
    unlink(fpath);
    strcpy(fpath, genbuf);
    
    strcpy(mymail.owner, cuser.userid);  /*站長 ID*/
    strcpy(mymail.title, save_title);
    mymail.savemode = 0;
    /* mymail.filemode |= FILE_TAGED;    Ptt 公告改成不會mark */
    
    sethomedir(genbuf, cuser.userid);
    if(append_record(genbuf, &mymail, sizeof(mymail)) == -1)
	outs(err_uid);
    
    for(unum = uhash->number, i = 0; i < unum; i++) {
	if(bad_user_id(uhash->userid[i]))
	    continue; /* Ptt */
	
	userid = uhash->userid[i];
	if(strcmp(userid,STR_GUEST) && strcmp(userid, "new") &&
	   strcmp(userid, cuser.userid)) {
	    sethomepath(genbuf, userid);
	    stampfile(genbuf, &mymail);
	    unlink(genbuf);
	    Link(fpath, genbuf);
	    
	    strcpy(mymail.owner, cuser.userid);
	    strcpy(mymail.title, save_title);
	    mymail.savemode = 0;
	    /* mymail.filemode |= FILE_MARKED; Ptt 公告改成不會mark */
	    sethomedir(genbuf, userid);
	    if(append_record(genbuf, &mymail, sizeof(mymail)) == -1)
		outs(err_uid);
	    sprintf(genbuf, "%*s %5d / %5d", IDLEN + 1, userid, i + 1, unum);
	    outmsg(genbuf);
	    refresh();
	}
    }
    return 0;
}

int mail_mbox() {
    char cmd[100];
    fileheader_t fhdr;

    sprintf(cmd, "/tmp/%s.uu", cuser.userid);
    sprintf(fhdr.title, "%s 私人資料", cuser.userid);
    doforward(cmd, &fhdr, 'Z');
    return 0;
}

static int m_forward(int ent, fileheader_t *fhdr, char *direct) {
    char uid[STRLEN];

    stand_title("轉達信件");
    usercomplete(msg_uid, uid);
    if(uid[0] == '\0')
	return FULLUPDATE;

    strcpy(quote_user, fhdr->owner);
    setuserfile(quote_file, fhdr->filename);
    sprintf(save_title, "%.64s (fwd)", fhdr->title);
    move(1, 0);
    clrtobot();
    prints("轉信給: %s\n標  題: %s\n", uid, save_title);

    switch(do_send(uid, save_title)) {
    case -1:
	outs(err_uid);
	break;
    case -2:
	outs(msg_cancel);
	break;
    case -3:
	prints("使用者 [%s] 無法收信", uid);
	break;
    }
    pressanykey();
    return FULLUPDATE;
}

static int delmsgs[128];
static int delcnt;
static int mrd;

static int read_new_mail(fileheader_t *fptr) {
    static int idc;
    char done = NA, delete_it;
    char fname[256];
    char genbuf[4];
    
    if(fptr == NULL) {
	delcnt = 0;
	idc = 0;
	return 0;
    }
    idc++;
    if(fptr->filemode)
	return 0;
    clear();
    move(10, 0);
    prints("您要讀來自[%s]的訊息(%s)嗎？", fptr->owner, fptr->title);
    getdata(11, 0, "請您確定(Y/N/Q)?[Y] ", genbuf, 3, DOECHO);
    if(genbuf[0] == 'q')
	return QUIT;
    if(genbuf[0] == 'n')
	return 0;
    
    setuserfile(fname, fptr->filename);
    fptr->filemode |= FILE_READ;
    if(substitute_record(currmaildir, fptr, sizeof(*fptr), idc))
	return -1;
    
    mrd = 1;
    delete_it = NA;
    while(!done) {
	int more_result = more(fname, YEA);

	switch(more_result) {
	case 1:
	    return READ_PREV;
	case 2:
	    return RELATE_PREV;
	case 3:
	    return READ_NEXT;
	case 4:
	    return RELATE_NEXT;
	case 5:
	    return RELATE_FIRST;
	case 6:
	    return 0;
	case 7:
	    mail_reply(idc, fptr, currmaildir);
	    return FULLUPDATE;
	case 8:
	    multi_reply(idc, fptr, currmaildir);
	    return FULLUPDATE;
	}
	move(b_lines, 0);
	clrtoeol();
	outs(msg_mailer);
	refresh();
	
	switch(egetch()) {
	case 'r':
	case 'R':
	    mail_reply(idc, fptr, currmaildir);
	    break;
	case 'x':
	    m_forward(idc, fptr, currmaildir);
	    break;
	case 'y':
	    multi_reply(idc, fptr, currmaildir);
	    break;
	case 'd':
	case 'D':
	    delete_it = YEA;
	default:
	    done = YEA;
	}
    }
    if(delete_it) {
	clear();
	prints("刪除信件《%s》", fptr->title);
	getdata(1, 0, msg_sure_ny, genbuf, 2, LCECHO);
	if(genbuf[0] == 'y') {
	    unlink(fname);
	    delmsgs[delcnt++] = idc;
	}
    }
    clear();
    return 0;
}

int m_new() {
    clear();
    mrd = 0;
    setutmpmode(RMAIL);
    read_new_mail(NULL);
    clear();
    curredit |= EDIT_MAIL;
    curredit &= ~EDIT_ITEM;
    if(apply_record(currmaildir, read_new_mail, sizeof(fileheader_t)) == -1) {
	outs("沒有新信件了");
	pressanykey();
	return -1;
    }
    curredit = 0;
    if(delcnt) {
	while(delcnt--)
	    delete_record(currmaildir, sizeof(fileheader_t), delmsgs[delcnt]);
    }
    outs(mrd ? "信已閱\畢" : "沒有新信件了");
    pressanykey();
    return -1;
}

static void mailtitle() {
    char buf[100] = "";

    showtitle("\0郵件選單", BBSName);
    outs("[←]離開  [↑,↓]選擇  [→,r]閱\讀信件  [R]回信   [x]轉達  "
	 "[y]群組回信  求助[h]\n\033[7m"
	 "編號   日 期  作 者          信  件  標  題     \033[32m");
    if(mailsumlimit) {
	sprintf(buf,"(容量:%d/%dk %d/%d篇)", mailsum, mailsumlimit,
		mailkeep, mailmaxkeep);
    }
    sprintf(buf, "%s%*s\033[m", buf, 29 - strlen(buf), "");
    outs(buf);
}

static void maildoent(int num, fileheader_t *ent) {
    char *title, *mark, color, type = "+ Mm"[ent->filemode];

    if(ent->filemode & FILE_TAGED)
	type = 'D';

    title = subject(mark = ent->title);
    if(title == mark) {
	color = '1';
	mark = "◇";
    } else {
	color = '3';
	mark = "R:";
    }

    if(strncmp(currtitle, title, 40))
	prints("%5d %c %-7s%-15.14s%s %.46s\n", num, type,
	       ent->date, ent->owner, mark, title);
    else
	prints("%5d %c %-7s%-15.14s\033[1;3%cm%s %.46s\033[0m\n", num, type,
	       ent->date, ent->owner, color, mark, title);
}

#ifdef POSTBUG
extern int bug_possible;
#endif

static int mail_del_tag(int ent, fileheader_t *fhdr, char *direct) {
    char genbuf[3];

    getdata(1, 0, "確定刪除標記信件(Y/N)? [Y]", genbuf, 3, LCECHO);
    if(genbuf[0] != 'n') {
	currfmode = FILE_TAGED;
	if(delete_files(direct, cmpfmode, 0))
	    return DIRCHANGED;
    }
    return FULLUPDATE;
}

static int m_idle(int ent, fileheader_t *fhdr, char *direct) {
    t_idle();
    return FULLUPDATE;
}

static int mail_del(int ent, fileheader_t *fhdr, char *direct) {
    char genbuf[200];

    if(fhdr->filemode & FILE_MARKED)
	return DONOTHING;

    getdata(1, 0, msg_del_ny, genbuf, 3, LCECHO);
    if(genbuf[0] == 'y') {
	strcpy(currfile, fhdr->filename);
	if(!delete_file(direct, sizeof(*fhdr), ent, cmpfilename)) {
	    setdirpath(genbuf, direct, fhdr->filename);
	    unlink(genbuf);
	    if((currmode & MODE_SELECT)) {
		int now;
		
		sethomedir(genbuf, cuser.userid);
		now = getindex(genbuf, fhdr->filename, sizeof(fileheader_t));
		delete_file(genbuf, sizeof(fileheader_t), now, cmpfilename);
	    }
	    return DIRCHANGED;
	}
    }
    return FULLUPDATE;
}

static int mail_read(int ent, fileheader_t *fhdr, char *direct) {
    char buf[64];
    char done, delete_it, replied;

    clear();
    setdirpath(buf, direct, fhdr->filename);
    strncpy(currtitle, subject(fhdr->title), 40);
    done = delete_it = replied = NA;
    while(!done) {
	int more_result = more(buf, YEA);
	
	if(more_result != -1) {
	    fhdr->filemode |= FILE_READ;
	    if((currmode & MODE_SELECT)) {
		int now;
		
		now = getindex(currmaildir, fhdr->filename,
			       sizeof(fileheader_t));
		substitute_record(currmaildir, fhdr, sizeof(*fhdr), now);
		substitute_record(direct, fhdr, sizeof(*fhdr), ent);
	    }
	    else
		substitute_record(currmaildir, fhdr, sizeof(*fhdr), ent);
	}
	switch(more_result) {
	case 1:
	    return READ_PREV;
	case 2:
	    return RELATE_PREV;
	case 3:
	    return READ_NEXT;
	case 4:
	    return RELATE_NEXT;
	case 5:
	    return RELATE_FIRST;
	case 6:
	    return FULLUPDATE;
	case 7:
	    mail_reply(ent, fhdr, direct);
	    return FULLUPDATE;
	case 8:
	    multi_reply(ent, fhdr, direct);
	    return FULLUPDATE;
	}
	move(b_lines, 0);
	clrtoeol();
	refresh();
	outs(msg_mailer);
	
	switch(egetch()) {
	case 'r':
	case 'R':
	    replied = YEA;
	    mail_reply(ent, fhdr, direct);
	    break;
	case 'x':
	    m_forward(ent, fhdr, direct);
	    break;
	case 'y':
	    multi_reply(ent, fhdr, direct);
	    break;
	case 'd':
	    delete_it = YEA;
	default:
	    done = YEA;
	}
    }
    if(delete_it)
	mail_del(ent, fhdr, direct);
    else {
	fhdr->filemode |= FILE_READ;
#ifdef POSTBUG
	if(replied)
	    bug_possible = YEA;
#endif
	if((currmode & MODE_SELECT)) {
	    int now;
	    
	    now = getindex(currmaildir, fhdr->filename, sizeof(fileheader_t));
	    substitute_record(currmaildir, fhdr, sizeof(*fhdr), now);
	    substitute_record(direct, fhdr, sizeof(*fhdr), ent);
	} else
	    substitute_record(currmaildir, fhdr, sizeof(*fhdr), ent);
#ifdef POSTBUG
	bug_possible = NA;
#endif
    }
    return FULLUPDATE;
}

/* in boards/mail 回信給原作者，轉信站亦可 */
int mail_reply(int ent, fileheader_t *fhdr, char *direct) {
    char uid[STRLEN];
    char *t;
    FILE *fp;
    char genbuf[512];

    stand_title("回  信");

    /* 判斷是 boards 或 mail */
    if(curredit & EDIT_MAIL)
	setuserfile(quote_file, fhdr->filename);
    else
	setbfile(quote_file, currboard, fhdr->filename);

    /* find the author */
    strcpy(quote_user, fhdr->owner);
    if(strchr(quote_user, '.')) {
	genbuf[0] = '\0';
	if((fp = fopen(quote_file, "r"))) {
	    fgets(genbuf, 512, fp);
	    fclose(fp);
	}
	
	t = strtok(genbuf, str_space);
	if(!strcmp(t, str_author1) || !strcmp(t, str_author2))
	    strcpy(uid, strtok(NULL, str_space));
	else {
	    outs("錯誤: 找不到作者。");
	    pressanykey();
	    return FULLUPDATE;
	}
    } else
	strcpy(uid, quote_user);
    
    /* make the title */
    do_reply_title(3, fhdr->title);
    prints("\n收信人: %s\n標  題: %s\n", uid, save_title);
    
    /* edit, then send the mail */
    ent = curredit;
    switch(do_send(uid, save_title)) {
    case -1:
	outs(err_uid);
	break;
    case -2:
	outs(msg_cancel);
	break;
    case -3:
	prints("使用者 [%s] 無法收信", uid);
	break;
    }
    curredit = ent;
    pressanykey();
    return FULLUPDATE;
}

static int mail_edit(int ent, fileheader_t *fhdr, char *direct) {
    char genbuf[200];

    if(!HAS_PERM(PERM_SYSOP) && 
       strcmp(cuser.userid, fhdr->owner) &&
       strcmp("[備.忘.錄]", fhdr->owner))
	return DONOTHING;

    setdirpath(genbuf, direct, fhdr->filename);
    vedit(genbuf, NA, NULL);
    return FULLUPDATE;
}

static int mail_mark(int ent, fileheader_t *fhdr, char *direct) {
    fhdr->filemode ^= FILE_MARKED;
    fhdr->filemode &= ~FILE_TAGED;
    
    if((currmode & MODE_SELECT)) {
	int now;

	now = getindex(currmaildir, fhdr->filename, sizeof(fileheader_t));
	substitute_record(currmaildir, fhdr, sizeof(*fhdr), now);
	substitute_record(direct, fhdr, sizeof(*fhdr), ent);
    } else
	substitute_record(currmaildir, fhdr, sizeof(*fhdr), ent);
    return PART_REDRAW;
}

static int mail_tag(int ent, fileheader_t *fhdr, char *direct) {
    fhdr->filemode ^= FILE_TAGED;
    if((currmode & MODE_SELECT)) {
	int now;
	
	now = getindex(currmaildir, fhdr->filename, sizeof(fileheader_t));
	substitute_record(currmaildir, fhdr, sizeof(*fhdr), now);
	substitute_record(direct, fhdr, sizeof(*fhdr), ent);
    } else
	substitute_record(currmaildir, fhdr, sizeof(*fhdr), ent);
    return POS_NEXT;
}

/* help for mail reading */
static char *mail_help[] = {
    "\0電子信箱操作說明",
    "\01基本命令",
    "(p)(↑)    前一篇文章",
    "(n)(↓)    下一篇文章",
    "(P)(PgUp)  前一頁",
    "(N)(PgDn)  下一頁",
    "(##)(cr)   跳到第 ## 筆",
    "($)        跳到最後一筆",
    "\01進階命令",
    "(r)(→)    讀信",
    "(R)        回信",
    "(c/z)      收入此信件進入私人信件夾/進入私人信件夾",
    "(x/X)      轉達信件/轉錄文章到其他看板",
    "(y)        群組回信",
    "(F)        將信傳送回您的電子信箱",
    "(d)        殺掉此信",
    "(D)        殺掉指定範圍的信",
    "(m)        將信標記，以防被清除",
    "(^G)       立即重建信箱 (信箱毀損時用)",
    "(t)        標記欲刪除信件",
    "(^D)       刪除已標記信件",
    NULL
};

static int m_help() {
    show_help(mail_help);
    return FULLUPDATE;
}

static int mail_cross_post(int ent, fileheader_t *fhdr, char *direct) {
    char xboard[20], fname[80], xfpath[80], xtitle[80], inputbuf[10];
    fileheader_t xfile;
    FILE *xptr;
    int author = 0;
    char genbuf[200];
    char genbuf2[4];
    
    make_blist();
    move(2, 0);
    clrtoeol();
    move(3, 0);
    clrtoeol();
    move(1, 0);
    namecomplete("轉錄本文章於看板：", xboard);
    if(*xboard == '\0' || !haspostperm(xboard))
	return FULLUPDATE;
    
    ent = 1;
    if(HAS_PERM(PERM_SYSOP) || !strcmp(fhdr->owner, cuser.userid)) {
	getdata(2, 0, "(1)原文轉載 (2)舊轉錄格式？[1] ",
		genbuf, 3, DOECHO);
	if(genbuf[0] != '2') {
	    ent = 0;
	    getdata(2, 0, "保留原作者名稱嗎?[Y] ", inputbuf, 3, DOECHO);
	    if(inputbuf[0] != 'n' && inputbuf[0] != 'N')
		author = 1;
	}
    }
    
    if(ent)
	sprintf(xtitle, "[轉錄]%.66s", fhdr->title);
    else
	strcpy(xtitle, fhdr->title);
    
    sprintf(genbuf, "採用原標題《%.60s》嗎?[Y] ", xtitle);
    getdata(2, 0, genbuf, genbuf2, 4, LCECHO);
    if(*genbuf2 == 'n')
	if(getdata(2, 0, "標題：", genbuf, TTLEN, DOECHO))
	    strcpy(xtitle, genbuf);
    
    getdata(2, 0, "(S)存檔 (L)站內 (Q)取消？[Q] ", genbuf, 3, LCECHO);
    if(genbuf[0] == 'l' || genbuf[0] == 's') {
	int currmode0 = currmode;

	currmode = 0;
	setbpath(xfpath, xboard);
	stampfile(xfpath, &xfile);
	if(author)
	    strcpy(xfile.owner, fhdr->owner);
	else
	    strcpy(xfile.owner, cuser.userid);
	strcpy(xfile.title, xtitle);
	if(genbuf[0] == 'l') {
	    xfile.savemode = 'L';
	    xfile.filemode = FILE_LOCAL;
	} else
	    xfile.savemode = 'S';
	
	setuserfile(fname, fhdr->filename);
	if(ent) {
	    xptr = fopen(xfpath, "w");
	    
	    strcpy(save_title, xfile.title);
	    strcpy(xfpath, currboard);
	    strcpy(currboard, xboard);
	    write_header(xptr);
	    strcpy(currboard, xfpath);
	    
	    fprintf(xptr, "※ [本文轉錄自 %s 信箱]\n\n", cuser.userid);
	    
	    b_suckinfile(xptr, fname);
	    addsignature(xptr,0);
	    fclose(xptr);
	} else {
	    unlink(xfpath);
	    Link(fname, xfpath);
	}
	
	setbdir(fname, xboard);
	append_record(fname, &xfile, sizeof(xfile));
	if(!xfile.filemode)
	    outgo_post(&xfile, xboard);
	cuser.numposts++;
	passwd_update(usernum, &cuser);
	outs("文章轉錄完成");
	pressanykey();
	currmode = currmode0;
    }
    return FULLUPDATE;
}

int mail_man() {
    char buf[64],buf1[64];
    if (HAS_PERM(PERM_MAILLIMIT)) {
	int mode0 = currutmp->mode;
	int stat0 = currstat;
	
	sethomeman(buf, cuser.userid);
	sprintf(buf1, "%s 的信件夾", cuser.userid);
	a_menu(buf1, buf, 1);
	currutmp->mode = mode0;
	currstat = stat0;
	return FULLUPDATE;
    }
    return DONOTHING;
}

static int mail_cite(int ent, fileheader_t *fhdr, char *direct) {
    char fpath[256];
    char title[TTLEN + 1];
    static char xboard[20];
    char buf[20];
    boardheader_t *bp;

    setuserfile(fpath, fhdr->filename);
    strcpy(title, "◇ ");
    strncpy(title+3, fhdr->title, TTLEN-3);
    title[TTLEN] = '\0';
    a_copyitem(fpath, title, 0, 1);

    if(cuser.userlevel >= PERM_BM) {
	move(2, 0);
	clrtoeol();
	move(3, 0);
	clrtoeol();
	move(1, 0);
	make_blist();
	namecomplete("輸入看版名稱 (直接Enter進入私人信件夾)：", buf);
	if(*buf)
	    strcpy(xboard, buf);
	if(*xboard && (bp = getbcache(getbnum(xboard)))) {
	    setapath(fpath, xboard);
	    setutmpmode(ANNOUNCE);
	    a_menu(xboard, fpath, HAS_PERM(PERM_ALLBOARD) ? 2 :
		   is_BM(bp->BM) ? 1 : 0);
	} else {
	    mail_man();
	    fhdr->filemode |= FILE_TAGED;
	}
	return FULLUPDATE;
    } else {
	mail_man();
        fhdr->filemode |= FILE_TAGED;
        return FULLUPDATE;
    }
}

static int mail_save(int ent, fileheader_t *fhdr, char *direct) {
    char fpath[256];
    char title[TTLEN+1];

    if(HAS_PERM(PERM_MAILLIMIT)) {
	setuserfile(fpath, fhdr->filename);
	strcpy(title, "◇ ");
	strncpy(title + 3, fhdr->title, TTLEN - 3);
	title[TTLEN] = '\0';
	a_copyitem(fpath, title, fhdr->owner, 1);
	sethomeman(fpath, cuser.userid);
	a_menu(cuser.userid, fpath, 1);
	return FULLUPDATE;
    }
    return DONOTHING;
}

static struct onekey_t mail_comms[] = {
    {'z', mail_man},
    {'c', mail_cite},
    {'s', mail_save},
    {'d', mail_del},
    {'D', del_range},
    {'r', mail_read},
    {'R', mail_reply},
    {'E', mail_edit},
    {'m', mail_mark},
    {'t', mail_tag},
    {'T', edit_title},
    {'x', m_forward},
    {'X', mail_cross_post},
    {Ctrl('D'), mail_del_tag},
    {Ctrl('G'), built_mail_index}, /* 修信箱 */
    {'y', multi_reply},
    {Ctrl('I'), m_idle},
    {'h', m_help},
    {'\0', NULL}
};

int m_read() {
    if(get_num_records(currmaildir, sizeof(fileheader_t))) {
	curredit = EDIT_MAIL;
	curredit &= ~EDIT_ITEM;
	i_read(RMAIL, currmaildir, mailtitle, maildoent, mail_comms, NULL);
	currfmode = FILE_TAGED;
	if(search_rec(currmaildir, cmpfmode))
	    mail_del_tag(0, 0, currmaildir);
	curredit = 0;
	return 0;
    } else {
	outs("您沒有來信");
	return XEASY;
    }
}

/* 寄站內信 */
static int send_inner_mail(char *fpath, char *title, char *receiver) {
    char genbuf[256];
    fileheader_t mymail;
    
    if(!searchuser(receiver))
	return -2;
    sethomepath(genbuf, receiver);
    stampfile(genbuf, &mymail);
    if(!strcmp(receiver, cuser.userid)) {
	strcpy(mymail.owner, "[" BBSNAME "]");
	mymail.filemode = FILE_READ;
    } else
	strcpy(mymail.owner, cuser.userid);
    strncpy(mymail.title, title, TTLEN);
    unlink(genbuf);
    Link(fpath, genbuf);
    sethomedir(genbuf, receiver);
    return do_append(genbuf, &mymail, sizeof(mymail));    
}

#include <netdb.h>
#include <pwd.h>
#include <time.h>

#ifndef USE_BSMTP
static int bbs_sendmail(char *fpath, char *title, char *receiver) {
    static int configured = 0;
    static char myhostname[STRLEN];
    static char myusername[20];
    struct hostent *hbuf;
    struct passwd *pbuf;
    char *ptr;
    char genbuf[256];
    FILE *fin, *fout;

    /* 中途攔截 */
    if((ptr = strchr(receiver, ';'))) {
	struct tm *ptime;
	time_t now;
	
	*ptr = '\0';
    }
    
    if((ptr = strstr(receiver, str_mail_address)) || !strchr(receiver,'@')) {
	char hacker[20];
	int len;

	if(strchr(receiver,'@')) {
            len = ptr - receiver;
            memcpy(hacker, receiver, len);
            hacker[len] = '\0';
        } else
            strcpy(hacker,receiver);
	return send_inner_mail(fpath, title, hacker);
    }
    
    /* setup the hostname and username */
    if(!configured) {
	/* get host name */
	gethostname(myhostname, STRLEN);
	hbuf = gethostbyname(myhostname);
	if(hbuf)
	    strncpy(myhostname, hbuf->h_name, STRLEN);

	/* get bbs uident */
	pbuf = getpwuid(getuid());
	if(pbuf)
	    strncpy(myusername, pbuf->pw_name, 20);
	if(hbuf && pbuf)
	    configured = 1;
	else
	    return -1;
    }
    
    /* Running the sendmail */
    if(fpath == NULL) {
	sprintf(genbuf, "/usr/sbin/sendmail %s > /dev/null", receiver);
	fin = fopen("etc/confirm", "r");
    } else {
	sprintf(genbuf, "/usr/sbin/sendmail -f %s%s %s > /dev/null",
		cuser.userid, str_mail_address, receiver);
	fin = fopen(fpath, "r");
    }
    fout = popen(genbuf, "w");
    if(fin == NULL || fout == NULL)
	return -1;
    
    if(fpath)
	fprintf(fout, "Reply-To: %s%s\nFrom: %s%s\n",
		cuser.userid, str_mail_address, cuser.userid,
		str_mail_address);
    fprintf(fout, "To: %s\nSubject: %s\n", receiver, title);
    fprintf(fout, "X-Disclaimer: " BBSNAME "對本信內容恕不負責。\n\n");
    
    while(fgets(genbuf, 255, fin)) {
	if(genbuf[0] == '.' && genbuf[1] == '\n')
	    fputs(". \n", fout);
	else
	    fputs(genbuf, fout);
    }
    fclose(fin);
    fprintf(fout, ".\n");
    pclose(fout);
    return 0;
}
#else /* USE_BSMTP */

int bsmtp(char *fpath, char *title, char *rcpt, int method) {
    char buf[80], *ptr;
    time_t chrono;
    MailQueue mqueue;

    /* check if the mail is a inner mail */
    if((ptr = strstr(rcpt, str_mail_address)) || !strchr(rcpt, '@')) {
	char hacker[20];
	int len;

	if(strchr(rcpt,'@')) {
            len = ptr - rcpt;
            memcpy(hacker, rcpt, len);
            hacker[len] = '\0';
        } else
            strcpy(hacker, rcpt);
	return send_inner_mail(fpath, title, hacker);
    }
    
    chrono = time(NULL);
    if(method != MQ_JUSTIFY) { /* 認證信 */
	/* stamp the queue file */
	strcpy(buf, "out/");
	for(;;) {
	    sprintf(buf + 4,"M.%ld.A", ++chrono);
	    if(!dashf(buf)) {
		Link(fpath, buf);
		break;
	    }
	}
	
	fpath = buf;

	strcpy(mqueue.filepath, fpath);
	strcpy(mqueue.subject, title);
    }
    /* setup mail queue */    
    mqueue.mailtime = chrono;
    mqueue.method = method;
    strcpy(mqueue.sender, cuser.userid);
    strcpy(mqueue.username, cuser.username);
    strcpy(mqueue.rcpt, rcpt);
    if(do_append("out/.DIR", (fileheader_t *)&mqueue, sizeof(mqueue)) < 0)
	return 0;
    return chrono;
}
#endif /* USE_BSMTP */

int doforward(char *direct, fileheader_t *fh, int mode) {
    static char address[60];
    char fname[500];
    int return_no;
    char genbuf[200];
    
    if(!address[0])
	strcpy(address, cuser.email);

    if(address[0]) {
	sprintf(genbuf, "確定轉寄給 [%s] 嗎(Y/N/Q)？[Y] ", address);
	getdata(b_lines - 1, 0, genbuf, fname, 3, LCECHO);
	
	if(fname[0] == 'q') {
	    outmsg("取消轉寄");
	    return 1;
	}
	if(fname[0] == 'n')
	    address[0] = '\0';
    }
    
    if(!address[0]) {
	do{
	    getdata(b_lines - 1, 0, "請輸入轉寄地址：", fname, 60, DOECHO);
	    if(fname[0]) {
		if(strchr(fname, '.'))
		    strcpy(address, fname);
		else
		    sprintf(address, "%s.bbs@%s", fname, MYHOSTNAME);
	    } else {
		outmsg("取消轉寄");
		return 1;
	    }
	}while(mode=='Z' && strstr(address, MYHOSTNAME));
    }
    if(invalidaddr(address))
	return -2;
    
    sprintf(fname, "正轉寄給 %s, 請稍候...", address);
    outmsg(fname);
    move(b_lines - 1, 0);
    refresh();
    
    /* 追蹤使用者 */
    if(HAS_PERM(PERM_LOGUSER)) {
	time_t now = time(NULL);
	char msg[200];
	
	sprintf(msg, "%s mailforward to %s at %s",
		cuser.userid, address, Cdate(&now));
	log_user(msg);
    }
    
    if(mode == 'Z') {
	sprintf(fname, TAR_PATH " cfz - home/%c/%s | "
		"/usr/bin/uuencode %s.tgz > %s",
		cuser.userid[0], cuser.userid, cuser.userid, direct);
	system(fname);
	strcpy(fname, direct);
    } else if(mode == 'U') {
	char tmp_buf[128];

	sprintf(fname, "/tmp/bbs.uu%05d", currpid);
	sprintf(tmp_buf, "/usr/bin/uuencode %s/%s uu.%05d > %s",
		direct, fh->filename, currpid, fname);
	system(tmp_buf);
    } else if (mode == 'F'){
	char tmp_buf[128];
	
	sprintf(fname, "/tmp/bbs.f%05d", currpid);
	sprintf(tmp_buf, "cp %s/%s %s",direct,fh->filename,fname);
	system(tmp_buf);
    } else
	return -1;
    
    return_no =
#ifndef USE_BSMTP
	bbs_sendmail(fname, fh->title, address);
#else
    bsmtp(fname, fh->title, address,mode);
#endif
    unlink(fname);
    return (return_no);
}

int chkmail(int rechk) {
    static time_t lasttime = 0;
    static int ismail = 0;
    struct stat st;
    int fd;
    register int numfiles;
    fileheader_t my_mail;

    if(!HAS_PERM(PERM_BASIC))
	return 0;
    if(stat(currmaildir, &st) < 0)
	return (ismail = 0);
    if((lasttime >= st.st_mtime) && !rechk)
	return ismail;
    lasttime = st.st_mtime;
    numfiles = st.st_size / sizeof(fileheader_t);
    if(numfiles <= 0)
	return (ismail = 0);
    
    /* 看看有沒有信件還沒讀過？從檔尾回頭檢查，效率較高 */    
    if((fd = open(currmaildir, O_RDONLY)) > 0) {
	lseek(fd, st.st_size - sizeof(fileheader_t), SEEK_SET);
	while(numfiles--) {
	    read(fd, &my_mail, sizeof(fileheader_t));
	    if(!(my_mail.filemode & FILE_READ)) {
		close(fd);
		return (ismail = 1);
	    }
	    lseek(fd, -(off_t)2 * sizeof(fileheader_t), SEEK_CUR);
	}
	close(fd);
    }
    return (ismail = 0);
}

#ifdef  EMAIL_JUSTIFY
static void mail_justify(userec_t muser) {
    fileheader_t mhdr;
    char title[128], buf1[80];
    FILE* fp;
    
    sethomepath(buf1, muser.userid);
    stampfile(buf1, &mhdr);
    unlink(buf1);
    strcpy(mhdr.owner, cuser.userid);
    strncpy(mhdr.title, "[審核通過]", TTLEN);
    mhdr.savemode = 0;
    mhdr.filemode = 0;

    if(valid_ident(muser.email) && !invalidaddr(muser.email)) {
	char title[80], *ptr;
	unsigned short checksum;            /* 16-bit is enough */
	char ch;
	
	checksum = getuser(muser.userid);
	ptr = muser.email;
	while((ch = *ptr++)) {
	    if(ch <= ' ')
		break;
	    if(ch >= 'A' && ch <= 'Z')
		ch |= 0x20;
	    checksum = (checksum << 1) ^ ch;
	}
	
	sprintf(title, "[PTT BBS]To %s(%d:%d) [User Justify]",
		muser.userid, getuser(muser.userid) + MAGIC_KEY, checksum);
	if(
#ifndef USE_BSMTP
	    bbs_sendmail(NULL, title, muser.email)
#else
	    bsmtp(NULL, title, muser.email, MQ_JUSTIFY);
#endif
	    < 0)
	    Link("etc/bademail", buf1);
	else
	    Link("etc/replyemail", buf1);
    } else
	Link("etc/bademail", buf1);
    sethomedir(title, muser.userid);
    append_record(title, &mhdr, sizeof(mhdr));
}
#endif /* EMAIL_JUSTIFY */
