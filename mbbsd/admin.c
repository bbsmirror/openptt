/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"
#include "pttstruct.h"
#include "common.h"
#include "perm.h"
#include "modes.h"
#include "proto.h"

extern char *msg_uid;
extern userec_t xuser;
extern char *err_uid;

/* �ϥΪ̺޲z */
int m_user() {
    userec_t muser;
    int id;
    char genbuf[200];
    
    stand_title("�ϥΪ̳]�w");
    usercomplete(msg_uid, genbuf);
    if(*genbuf) {
	move(2, 0);
	if((id = getuser(genbuf))) {
	    memcpy(&muser, &xuser, sizeof(muser));
	    user_display(&muser, 1);
	    uinfo_query(&muser, 1, id);
	} else {
	    outs(err_uid);
	    clrtoeol();
	    pressanykey();
	}
    }
    return 0;
}

extern int b_lines;

static int search_key_user(char *passwdfile, int mode) {
    userec_t user;
    int ch;
    int coun = 0;
    FILE *fp1 = fopen(passwdfile, "r");
    char buf[128], key[22], genbuf[8];
    
    clear();
    getdata(0, 0, mode ? "�п�J�ϥΪ�����r[�q��|�a�}|�m�W|�W���a�I|"
	    "email|�p��id] :" : "�п�Jid :", key, 21, DOECHO);
    while((fread(&user, sizeof(user), 1, fp1)) > 0 && coun < MAX_USERS) {
	if(!(++coun & 15)) {
	    move(1, 0);
	    sprintf(buf, "�� [%d] �����\n", coun);
	    outs(buf);
	    refresh();
	}
	if(!strcasecmp(user.userid, key) ||
	   (mode && (
	       strstr(user.realname, key) || strstr(user.username, key) ||
	       strstr(user.lasthost, key) || strstr(user.email, key) ||
	       strstr(user.address, key) || strstr(user.justify, key) ||
	       strstr(user.mychicken.name, key)))) {
	    move(1, 0);
	    sprintf(buf, "�� [%d] �����\n", coun);
	    outs(buf);
	    refresh();
	    
	    user_display(&user, 1);
	    uinfo_query(&user, 1, coun);
	    outs("\033[44m               �ť���\033[37m:�j�M�U�@��"
		 "          \033[33m Q\033[37m: ���}");
	    outs(mode ? "                        \033[m " :
		 "      S: ���γƥ����    \033[m ");
	    while(1) {
	    	while((ch = igetch()) == 0);
	    	if(ch == ' ')
	    	    break;
		if(ch == 'q' || ch == 'Q')
		    return 0;
		if(ch == 's' && !mode) {
		    if((ch = getuser(user.userid))) {
			passwd_update(ch, &user);
			return 0;
		    } else {
			move(b_lines - 1, 0);
			genbuf[0] = 'n';
			getdata(0, 0,
				"�ثe�� PASSWD �ɨS���� ID�A�s�W�ܡH[y/N]",
				genbuf, 3, LCECHO);
			if(genbuf[0] == 'n') {
			    outs("�ثe��PASSWDS�ɨS����id "
				 "�Х�new�@�ӳo��id���b��");
			} else {
			    int allocid = getnewuserid();
			    
			    if(allocid > MAX_USERS || allocid <= 0) {
				fprintf(stderr, "�����H�f�w�F���M�I\n");
				exit(1);
			    }
			    
			    if(passwd_update(allocid, &user) == -1) {
				fprintf(stderr, "�Ⱥ��F�A�A���I\n");
				exit(1);
			    }
			    setuserid(allocid, user.userid);
			    if(!searchuser(user.userid)) {
				fprintf(stderr, "�L�k�إ߱b��\n");
				exit(1);
			    }
			    return 0;
			}
		    }
		}
	    }
	}
    }
    
    fclose(fp1);
    return 0;
}

/* �H���N key �M��ϥΪ� */
int search_user_bypwd() {
    search_key_user(FN_PASSWD, 1);
    return 0;
}

/* �M��ƥ����ϥΪ̸�� */
int search_user_bybakpwd() {
    char *choice[] = {
	"PASSWDS.NEW1", "PASSWDS.NEW2", "PASSWDS.NEW3",
	"PASSWDS.NEW4", "PASSWDS.NEW5", "PASSWDS.NEW6",
	"PASSWDS.BAK"
    };
    int ch;
    
    clear();
    move(1, 1);
    outs("�п�J�A�n�ΨӴM��ƥ����ɮ� �Ϋ� 'q' ���}\n");
    outs(" [\033[1;31m1\033[m]�@�ѫe, [\033[1;31m2\033[m]��ѫe, "
	 "[\033[1;31m3\033[m]�T�ѫe\n");
    outs(" [\033[1;31m4\033[m]�|�ѫe, [\033[1;31m5\033[m]���ѫe, "
	 "[\033[1;31m6\033[m]���ѫe\n");
    outs(" [7]�ƥ���\n");
    do {
	move(5, 1);
	outs("��� => ");
	ch = igetch();
	if(ch == 'q' || ch == 'Q')
	    return 0;
    } while (ch < '1' || ch > '8');
    ch -= '1';
    search_key_user(choice[ch], 0);
    return 0;
}

static void bperm_msg(boardheader_t *board) {
    prints("\n�]�w [%s] �ݪO��(%s)�v���G", board->brdname,
	   board->brdattr & BRD_POSTMASK ? "�o��" : "�\\Ū");
}

extern char* str_permboard[];

unsigned int setperms(unsigned int pbits, char *pstring[]) {
    register int i;
    char choice[4];
    
    move(4, 0);
    for(i = 0; i < NUMPERMS / 2; i++) {
	prints("%c. %-20s %-15s %c. %-20s %s\n",
	       'A' + i, pstring[i],
	       ((pbits >> i) & 1 ? "��" : "��"),
	       i < 10 ? 'Q' + i : '0' + i - 10,
	       pstring[i + 16],
	       ((pbits >> (i + 16)) & 1 ? "��" : "��"));
    }
    clrtobot();
    while(getdata(b_lines - 1, 0, "�Ы� [A-5] �����]�w�A�� [Return] �����G",
		  choice, 3, LCECHO)) {
	i = choice[0] - 'a';
	if(i < 0)
	    i = choice[0] - '0' + 26;
	if(i >= NUMPERMS)
	    bell();
	else {
	    pbits ^= (1 << i);
	    move(i % 16 + 4, i <= 15 ? 24 : 64);
	    prints((pbits >> i) & 1 ? "��" : "��");
	}
    }
    return pbits;
}

/* �۰ʳ]�ߺ�ذ� */
static void setup_man(boardheader_t * board) {
    char genbuf[200];
    
    setapath(genbuf, board->brdname);
    mkdir(genbuf, 0755);
}

extern char *fn_board;
extern char *err_bid;
extern userec_t cuser;
extern char *msg_sure_ny;
extern char* str_permid[];

int m_mod_board(char *bname) {
    boardheader_t bh, newbh;
    int bid;
    char genbuf[256], ans[4];
    
    bid = getbnum(bname);
    if(get_record(fn_board, &bh, sizeof(bh), bid) == -1) {
	outs(err_bid);
	pressanykey();
	return -1;
    }
    
    prints("�ݪO�W�١G%s\n�ݪO�����G%s\n�ݪOUID�G%d\n�ݪOGID�G%d\n"
	   "�O�D�W��G%s", bh.brdname, bh.title, bh.uid, bh.gid, bh.BM);
    bperm_msg(&bh);
    
    /* Ptt �o���_��|�ɨ�U�� */
    move(9, 0);
    sprintf(genbuf, "�ݪO (E)�]�w (V)�H�k/�Ѱ� %s[Q]�����H",
	    HAS_PERM(PERM_SYSOP) ?
	    "(D)�R�� (G)GID (U)UID (B)BVote (S)�Ϧ^�峹" : "");
    getdata(10, 0, genbuf, ans, 3, LCECHO);
    
    switch(*ans) {
    case 's':
	if(HAS_PERM(PERM_SYSOP)) {
	    char actionbuf[512];
	    
	    sprintf(actionbuf, BBSHOME "/bin/buildir boards/%s &",
		    bh.brdname);
	    system(actionbuf);
	}
	break;
    case 'b':
	if(HAS_PERM(PERM_SYSOP)) {
	    char bvotebuf[10];
	    
	    memcpy(&newbh, &bh, sizeof(bh));
	    sprintf(bvotebuf, "%d", newbh.bvote);
	    move(20, 0);
	    prints("�ݪ� %s ��Ӫ� BVote�G%d", bh.brdname, bh.bvote);
	    getdata_str(21, 0, "�s�� Bvote�G", genbuf, 5, LCECHO, bvotebuf);
	    newbh.bvote = atoi(genbuf);
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
	    log_usies("SetBoardBvote", newbh.brdname);
	    break;
	} else
	    break;
    case 'g':
	if(HAS_PERM(PERM_SYSOP)) {
	    char gidbuf[10];
	    
	    memcpy(&newbh, &bh, sizeof(bh));
	    sprintf(gidbuf, "%d", newbh.gid);
	    move(20, 0);
	    prints("�ݪ� %s ��Ӫ� GID�G%d", bh.brdname, bh.gid);
	    getdata_str(21, 0, "�s�� GID�G", genbuf, 5, LCECHO, gidbuf);
	    newbh.gid = atoi(genbuf);
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
	    log_usies("SetBoardGID", newbh.brdname);
	    break;
	} else
	    break;
    case 'u':
	if(HAS_PERM(PERM_SYSOP)) {
	    char uidbuf[10];
	    
	    memcpy(&newbh, &bh, sizeof(bh));
	    sprintf(uidbuf, "%d", newbh.uid);
	    move(20, 0);
	    prints("�ݪ� %s ��Ӫ� UID�G%d", bh.brdname, bh.uid);
	    getdata_str(21, 0, "�s�� UID�G", genbuf, 5, LCECHO, uidbuf);
	    newbh.uid = atoi(genbuf);
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
	    log_usies("SetBoardUID", newbh.brdname);
	    break;
	} else
	    break;
    case 'v':
	memcpy(&newbh, &bh, sizeof(bh));
	outs("�ݪ��ثe��");
	outs((bh.brdattr & BRD_BAD) ? "�H�k" : "���`");
	getdata(21, 0, "�T�w���H", genbuf, 5, LCECHO);
	if(genbuf[0] == 'y') {
	    if(newbh.brdattr & BRD_BAD)
		newbh.brdattr = newbh.brdattr & (!BRD_BAD);
	    else
		newbh.brdattr = newbh.brdattr | BRD_BAD;
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
	    log_usies("ViolateLawSet", newbh.brdname);
	}
	break;
    case 'd':
	if(!HAS_PERM(PERM_SYSOP))
	    break;
	getdata_str(9, 0, msg_sure_ny, genbuf, 3, LCECHO, "N");
	if(genbuf[0] != 'y')
	    outs(MSG_DEL_CANCEL);
	else {
	    strcpy(bname, bh.brdname);
	    sprintf(genbuf, "/bin/rm -fr boards/%s man/%s", bname, bname);
	    system(genbuf);
	    memset(&bh, 0, sizeof(bh));
	    sprintf(bh.title, "[%s] deleted by %s", bname, cuser.userid);
	    substitute_record(fn_board, &bh, sizeof(bh), bid);
	    touch_boards();
	    log_usies("DelBoard", bh.title);
	    outs("�R�O����");
	}
	break;
    case 'e':
	move(8, 0);
	outs("������ [Return] ���ק�Ӷ��]�w");
	memcpy(&newbh, &bh, sizeof(bh));

	while(getdata(9, 0, "�s�ݪO�W�١G", genbuf, IDLEN + 1, DOECHO)) {
	    if(getbnum(genbuf)) {
		move(3, 0);
		outs("���~! �O�W�p�P");
	    } else if(!invalid_brdname(genbuf)) {
		strcpy(newbh.brdname, genbuf);
		break;
	    }
	}
	
	do {
	    getdata_str(12, 0, "�ݪO���O�G", genbuf, 5, DOECHO, bh.title);
	    if(strlen(genbuf) == 4)
		break;
	} while (1);

	if(strlen(genbuf) >= 4)
	    strncpy(newbh.title, genbuf, 4);
	
	newbh.title[4] = ' ';

	getdata_str(14, 0, "�ݪO�D�D�G", genbuf, BTLEN + 1, DOECHO, 
		    bh.title + 7);
	if(genbuf[0])
	    strcpy(newbh.title + 7, genbuf);
	if(getdata_str(15, 0, "�s�O�D�W��G", genbuf, IDLEN * 3 + 3, DOECHO,
		       bh.BM)) {
	    trim(genbuf);
	    strcpy(newbh.BM, genbuf);
	}
	
	if(HAS_PERM(PERM_SYSOP)) {
	    move(1, 0);
	    clrtobot();
	    newbh.brdattr = setperms(newbh.brdattr, str_permboard);
	    move(1, 0);
	    clrtobot();
	}
	
	if(newbh.brdattr & BRD_GROUPBOARD)
	    strncpy(newbh.title + 5, "�U", 2);
	else if(newbh.brdattr & BRD_NOTRAN)
	    strncpy(newbh.title + 5, "��", 2);
	else
	    strncpy(newbh.title + 5, "��", 2);
	
	if(HAS_PERM(PERM_SYSOP) && !(newbh.brdattr & BRD_HIDE)) {
	    getdata_str(14, 0, "�]�wŪ�g�v��(Y/N)�H", ans, 4, LCECHO, "N");
	    if(*ans == 'y') {
		getdata_str(15, 0, "���� [R]�\\Ū (P)�o��H", ans, 4, LCECHO,
			    "R");
		if(*ans == 'p')
		    newbh.brdattr |= BRD_POSTMASK;
		else
		    newbh.brdattr &= ~BRD_POSTMASK;
		
		move(1, 0);
		clrtobot();
		bperm_msg(&newbh);
		newbh.level = setperms(newbh.level, str_permid);
		clear();
	    }
	}
	if(HAS_PERM(PERM_SYSOP)) {
	    char gidbuf[10];
	    
	    sprintf(gidbuf, "%d", newbh.gid);
	    gidbuf[4] = 0;
	    getdata_str(21, 0, "�ݪOGID�G", genbuf, 5, LCECHO, gidbuf);
	    newbh.gid = atoi(genbuf);
	}
	
	getdata_str(b_lines - 1, 0, msg_sure_ny, genbuf, 4, LCECHO, "Y");

	if((*genbuf == 'y') && memcmp(&newbh, &bh, sizeof(bh))) {
	    if(strcmp(bh.brdname, newbh.brdname)) {
		char src[60], tar[60];
		
		setbpath(src, bh.brdname);
		setbpath(tar, newbh.brdname);
		Rename(src, tar);
		
		setapath(src, bh.brdname);
		setapath(tar, newbh.brdname);
		Rename(src, tar);
	    }
	    setup_man(&newbh);
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
	    log_usies("SetBoard", newbh.brdname);
	}
    }
    return 0;
}

extern char *msg_bid;

/* �]�w�ݪ� */
int m_board() {
    char bname[32];
    
    stand_title("�ݪO�]�w");
    make_blist();
    namecomplete(msg_bid, bname);
    if(!*bname)
	return 0;
    m_mod_board(bname);
    return 0;
}

/* �]�w�t���ɮ� */
int x_file() {
    int aborted;
    char ans[4], *fpath;
    
    move(b_lines - 4, 0);
    /* Ptt */
    outs("�]�w (1)�����T�{�H (4)post�`�N�ƶ� (5)���~�n�J�T��\n");
    outs("     (6)���U�d�� (7)�q�L�T�{�q�� (8)email post�q�� (9)�t�Υ\\����F (A)����\n");
    outs("     (B)�����W�� (C)email�q�L�T�{ (D)�s�ϥΪ̻ݪ� (E)�����T�{��k (F)�w��e��\n");
    getdata(b_lines - 1, 0, "     (G)�i���e�� (H)�ݪO���� (I)�G�m (J)�X���e�� (K)�ͤ�d (L)�`�� [Q]�����H", ans, 3, LCECHO);
    
    switch(ans[0]) {
    case '1':
	fpath = "etc/confirm";
	break;
    case '4':
	fpath = "etc/post.note";
	break;
    case '5':
	fpath = "etc/goodbye";
	break;
    case '6':
	fpath = "etc/register";
	break;
    case '7':
	fpath = "etc/registered";
	break;
    case '8':
	fpath = "etc/emailpost";
	break;
    case '9':
	fpath = "etc/hint";
	break;
    case 'a':
	fpath = "etc/teashop";
	break;
    case 'b':
	fpath = "etc/sysop";
	break;
    case 'c':
	fpath = "etc/bademail";
	break;
    case 'd':
	fpath = "etc/newuser";
	break;
    case 'e':
	fpath = "etc/justify";
	break;
    case 'f':
	fpath = "etc/Welcome";
	break;
    case 'g':
	fpath = "etc/Welcome_login";
	break;
    case 'h':
	fpath = "etc/expire.conf";
	break;
    case 'i':
	fpath = "etc/domain_name_query";
	break;
    case 'j':
	fpath = "etc/Logout";
	break;
    case 'k':
	fpath = "etc/Welcome_birth";
	break;
    case 'l':
	fpath = "etc/feast";
	break;
    default:
	return FULLUPDATE;
    }
    aborted = vedit(fpath, NA, NULL);
    prints("\n\n�t���ɮ�[%s]�G%s", fpath,
	   (aborted == -1) ? "������" : "��s����");
    pressanykey();
    return FULLUPDATE;
}

extern int numboards;
extern boardheader_t *bcache;

static int b_newuid() {
    int i, used[MAX_BOARD];

    /* race condition may occured here */
    memset(used, 0, sizeof(used));
    for(i = 0; i < numboards; i++)
	used[bcache[i].uid] = 1;
    for(i = 1; used[i] && i < MAX_BOARD; i++);
    if(i == MAX_BOARD)
	i = -1;
    return i;
}

int m_newbrd(int recover) {
    boardheader_t newboard;
    char ans[4];
    int bid;
    char genbuf[200];
    extern int class_uid;
    
    stand_title("�إ߷s�O");
    memset(&newboard, 0, sizeof(newboard));
    
    newboard.gid = class_uid;
    newboard.uid = b_newuid();
    if(newboard.uid == -1) {
	move(6, 0);
	outs("�ݪO�`�Ƥw�F���M!");
	pressanykey();
	return -1;
    } else if (newboard.gid == -1) {
	move(6, 0);
	outs("�Х���ܤ@�����O�A�}�O!");
	pressanykey();
	return -1;
    }
    
    do {
	if(!getdata(3, 0, msg_bid, newboard.brdname, IDLEN + 1, DOECHO))
	    return -1;
    } while(invalid_brdname(newboard.brdname));

    do {
	getdata(6, 0, "�ݪO���O�G", genbuf, 5, DOECHO);
	if(strlen(genbuf) == 4)
	    break;
    } while (1);
    
    if(strlen(genbuf) >= 4)
	strncpy(newboard.title, genbuf, 4);
    
    newboard.title[4] = ' ';
    
    getdata(8, 0, "�ݪO�D�D�G", genbuf, BTLEN + 1, DOECHO);
    if(genbuf[0])
	strcpy(newboard.title + 7, genbuf);
    setbpath(genbuf, newboard.brdname);
    if(!recover &&
       (getbnum(newboard.brdname) > 0 || mkdir(genbuf, 0755) == -1)) {
	outs(err_bid);
	pressanykey();
	return -1;
    }
    getdata(9, 0, "�O�D�W��G", newboard.BM, IDLEN * 3 + 3, DOECHO);
    
    newboard.brdattr = BRD_NOTRAN;
    
    if(HAS_PERM(PERM_SYSOP)) {
	move(1, 0);
	clrtobot();
	newboard.brdattr = setperms(newboard.brdattr, str_permboard);
	move(1, 0);
	clrtobot();
    }

    if(newboard.brdattr & BRD_GROUPBOARD)
	strncpy(newboard.title + 5, "�U", 2);
    else if(newboard.brdattr & BRD_NOTRAN)
	strncpy(newboard.title + 5, "��", 2);
    else
	strncpy(newboard.title + 5, "��", 2);
    
    newboard.level = 0;
    if(HAS_PERM(PERM_SYSOP) && !(newboard.brdattr & BRD_HIDE)) {
	getdata_str(14, 0, "�]�wŪ�g�v��(Y/N)�H", ans, 4, LCECHO, "N");
	if(*ans == 'y') {
	    getdata_str(15, 0, "���� [R]�\\Ū (P)�o��H", ans, 4, LCECHO, "R");
	    if(*ans == 'p')
		newboard.brdattr |= BRD_POSTMASK;
	    else
		newboard.brdattr &= (~BRD_POSTMASK);
	    
	    move(1, 0);
	    clrtobot();
	    bperm_msg(&newboard);
	    newboard.level = setperms(newboard.level, str_permid);
	    clear();
	}
    }
    
    if((bid = getbnum("")) > 0)
	substitute_record(fn_board, &newboard, sizeof(newboard), bid);
    else if(append_record(fn_board, (fileheader_t *) & newboard,
			  sizeof(newboard)) == -1) {
	pressanykey();
	return -1;
    }
    setup_man(&newboard);
    touch_boards();
    outs("\n�s�O����");
    post_newboard(newboard.title, newboard.brdname, newboard.BM);
    log_usies("NewBoard", newboard.title);
    pressanykey();
    return 0;
}

static int auto_scan(char fdata[][STRLEN], char ans[]) {
    int good = 0;
    int count = 0;
    int i;
    char temp[10];
    
    if(!strncmp(fdata[2], "�p", 2) || strstr(fdata[2], "�X")
	|| strstr(fdata[2], "��") || strstr(fdata[2], "��")) {
	ans[0] = '0';
	return 1;
    }
    
    strncpy(temp, fdata[2], 2);
    temp[2] = '\0';
    
    /* �|�r */
    if(!strncmp(temp, &(fdata[2][2]), 2)) {
	ans[0] = '0';
	return 1;
    }

    if(strlen(fdata[2]) >= 6) {
	if(strstr(fdata[2], "������")) {
	    ans[0] = '0';
	    return 1;
	}
	
	if(strstr("�����]���P�d�G��", temp))
	    good++;
	else if(strstr("���C���L���x�E���B", temp))
	    good++;
	else if(strstr("Ĭ��d�f����i����Ĭ", temp))
	    good++;
	else if(strstr("�}�¥ۿc�I���έ�", temp))
	    good++;
    }
    
    if(!good)
	return 0;

    if(!strcmp(fdata[3], fdata[4]) ||
       !strcmp(fdata[3], fdata[5]) ||
       !strcmp(fdata[4], fdata[5])) {
	ans[0] = '4';
	return 5;
    }
    
    if(strstr(fdata[3], "�j")) {
	if (strstr(fdata[3], "�x") || strstr(fdata[3], "�H") ||
	    strstr(fdata[3], "��") || strstr(fdata[3], "�F") ||
	    strstr(fdata[3], "�M") || strstr(fdata[3], "ĵ") ||
	    strstr(fdata[3], "�v") || strstr(fdata[3], "�ʶ�") ||
	    strstr(fdata[3], "����") || strstr(fdata[3], "��") ||
	    strstr(fdata[3], "��") || strstr(fdata[3], "�F�d"))
	    good++;
    } else if(strstr(fdata[3], "�k��"))
	good++;
    
    if(strstr(fdata[4], "�a�y") || strstr(fdata[4], "�t�z") ||
       strstr(fdata[4], "�H�c")) {
	ans[0] = '2';
	return 3;
    }
    
    if(strstr(fdata[4], "��") || strstr(fdata[4], "��")) {
	if(strstr(fdata[4], "��") || strstr(fdata[4], "��")) {
	    if(strstr(fdata[4], "��"))
		good++;
	}
    }
    
    for(i = 0; fdata[5][i]; i++) {
	if(isdigit(fdata[5][i]))
	    count++;
    }
    
    if(count <= 4) {
	ans[0] = '3';
	return 4;
    } else if(count >= 7)
	good++;

    if(good >= 3) {
	ans[0] = 'y';
	return -1;
    } else
	return 0;
}

/* �B�z Register Form */
int scan_register_form(char *regfile, int automode, int neednum) {
    char genbuf[200];
    static char *logfile = "register.log";
    static char *field[] = {
	"num", "uid", "name", "career", "addr", "phone", "email", NULL
    };
    static char *finfo[] = {
	"�b����m", "�ӽХN��", "�u��m�W", "�A�ȳ��", "�ثe��}",
	"�s���q��", "�q�l�l��H�c", NULL
    };
    static char *reason[] = {
	"��J�u��m�W", "�Զ�Ǯլ�t�P�~��", "��g���㪺��}���",
	"�Զ�s���q��", "�T���g���U�ӽЪ�", "�Τ����g�ӽг�", NULL
    };
    static char *autoid = "AutoScan";
    userec_t muser;
    FILE *fn, *fout, *freg;
    char fdata[7][STRLEN];
    char fname[STRLEN], buf[STRLEN];
    char ans[4], *ptr, *uid;
    int n, unum;
    int nSelf = 0, nAuto = 0;
    
    uid = cuser.userid;
    sprintf(fname, "%s.tmp", regfile);
    move(2, 0);
    if(dashf(fname)) {
	if(neednum == 0) { /* �ۤv�i Admin �Ӽf�� */
	    outs("��L SYSOP �]�b�f�ֵ��U�ӽг�");
	    pressanykey();
	}
	return -1;
    }
    Rename(regfile, fname);
    if((fn = fopen(fname, "r")) == NULL) {
	prints("�t�ο��~�A�L�kŪ�����U�����: %s", fname);
	pressanykey();
	return -1;
    }
    if(neednum) { /* �Q�j���f�� */
	move(1, 0);
	clrtobot();
	prints("�U��㦳�����v�����H�A���U��ֿn�W�L�@�ʥ��F�A�·бz�����f %d ��\n", neednum);
	prints("�]�N�O�j���G�Q�����@���ƶq�A��M�A�z�]�i�H�h�f\n�S�f�����e�A�t�Τ��|���A���X��I����");
	pressanykey();
    }
    memset(fdata, 0, sizeof(fdata));
    while(fgets(genbuf, STRLEN, fn)) {
	if ((ptr = (char *) strstr(genbuf, ": "))) {
	    *ptr = '\0';
	    for(n = 0; field[n]; n++) {
		if(strcmp(genbuf, field[n]) == 0) {
		    strcpy(fdata[n], ptr + 2);
		    if((ptr = (char *) strchr(fdata[n], '\n')))
			*ptr = '\0';
		}
	    }
	} else if ((unum = getuser(fdata[1])) == 0) {
	    move(2, 0);
	    clrtobot();
	    outs("�t�ο��~�A�d�L���H\n\n");
	    for (n = 0; field[n]; n++)
		prints("%s     : %s\n", finfo[n], fdata[n]);
	    pressanykey();
	    neednum--;
	} else {
	    neednum--;
	    memcpy(&muser, &xuser, sizeof(muser));
	    if(automode)
		uid = autoid;
	    
	    if(!automode || !auto_scan(fdata, ans)) {
		uid = cuser.userid;
		
		move(1, 0);
		prints("�b����m    �G%d\n", unum);
		user_display(&muser, 1);
		move(14, 0);
		prints("\033[1;32m------------- �Я����Y��f�֨ϥΪ̸�ơA�z�٦� %d ��---------------\033[m\n", neednum);
		for(n = 0; field[n]; n++) {
		    if(n >= 2 && n <= 5)
			prints("%d.", n - 2);
		    else
			prints("  ");
		    prints("%-12s�G%s\n", finfo[n], fdata[n]);
		}
		if(muser.userlevel & PERM_LOGINOK) {
		    getdata(b_lines - 1, 0, "\033[1;32m���b���w�g�������U, "
			    "��s(Y/N/Skip)�H\033[m[N] ", ans, 3, LCECHO);
		    if(ans[0] != 'y' && ans[0] != 's')
			ans[0] = 'd';
		} else {
		    getdata(b_lines - 1, 0,
			    "�O�_���������(Y/N/Q/Del/Skip)�H[Y] ",
			    ans, 3, LCECHO);
		}
		nSelf++;
	    } else
		nAuto++;
	    if(neednum > 0 && ans[0] == 'q') {
		move(2, 0);
		clrtobot();
		prints("�S�f������h�X");
		pressanykey();
		ans[0] = 's';
	    }
	    
	    switch(ans[0]) {
	    case 'q':
		if((freg = fopen(regfile, "a"))) {
		    for(n = 0; field[n]; n++)
			fprintf(freg, "%s: %s\n", field[n], fdata[n]);
		    fprintf(freg, "----\n");
		    while(fgets(genbuf, STRLEN, fn))
			fputs(genbuf, freg);
		    fclose(freg);
		}
	    case 'd':
		break;
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case 'n':
		if(ans[0] == 'n') {
		    for(n = 0; field[n]; n++)
			prints("%s: %s\n", finfo[n], fdata[n]);
		    move(9, 0);
		    prints("�д��X�h�^�ӽЪ��]�A�� <enter> ����\n");
		    for (n = 0; reason[n]; n++)
			prints("%d) ��%s\n", n, reason[n]);
		} else
		    buf[0] = ans[0];
		if(ans[0] != 'n' ||
		    getdata(10 + n, 0, "�h�^��]�G", buf, 60, DOECHO))
		    if((buf[0] - '0') >= 0 && (buf[0] - '0') < n) {
			int i;
			fileheader_t mhdr;
			char title[128], buf1[80];
			FILE *fp;
			
			i = buf[0] - '0';
			strcpy(buf, reason[i]);
			sprintf(genbuf, "[�h�^��]] ��%s", buf);
			
			sethomepath(buf1, muser.userid);
			stampfile(buf1, &mhdr);
			strcpy(mhdr.owner, cuser.userid);
			strncpy(mhdr.title, "[���U����]", TTLEN);
			mhdr.savemode = 0;
			mhdr.filemode = 0;
			sethomedir(title, muser.userid);
			if(append_record(title, &mhdr, sizeof(mhdr)) != -1) {
			    fp = fopen(buf1, "w");
			    fprintf(fp, "%s\n", genbuf);
			    fclose(fp);
			}
			if((fout = fopen(logfile, "a"))) {
			    for(n = 0; field[n]; n++)
				fprintf(fout, "%s: %s\n", field[n], fdata[n]);
			    n = time(NULL);
			    fprintf(fout, "Date: %s\n", Cdate((time_t *) & n));
			    fprintf(fout, "Rejected: %s [%s]\n----\n",
				    uid, buf);
			    fclose(fout);
			}
			break;
		    }
		move(10, 0);
		clrtobot();
		prints("�����h�^�����U�ӽЪ�");
	    case 's':
		if((freg = fopen(regfile, "a"))) {
		    for(n = 0; field[n]; n++)
			fprintf(freg, "%s: %s\n", field[n], fdata[n]);
		    fprintf(freg, "----\n");
		    fclose(freg);
		}
		break;
	    default:
		prints("�H�U�ϥΪ̸�Ƥw�g��s:\n");
		mail_muser(muser, "[���U���\\�o]", "etc/registered");
		muser.userlevel |= (PERM_LOGINOK | PERM_POST);
		strcpy(muser.realname, fdata[2]);
		strcpy(muser.address, fdata[4]);
		strcpy(muser.email, fdata[6]);
		sprintf(genbuf, "%s:%s:%s", fdata[5], fdata[3], uid);
		strncpy(muser.justify, genbuf, REGLEN);
		sethomefile(buf, muser.userid, "justify");
		log_file(buf, genbuf);
		passwd_update(unum, &muser);
		
		if((fout = fopen(logfile, "a"))) {
		    for(n = 0; field[n]; n++)
			fprintf(fout, "%s: %s\n", field[n], fdata[n]);
		    n = time(NULL);
		    fprintf(fout, "Date: %s\n", Cdate((time_t *) & n));
		    fprintf(fout, "Approved: %s\n", uid);
		    fprintf(fout, "----\n");
		    fclose(fout);
		}
		break;
	    }
	}
    }
    fclose(fn);
    unlink(fname);

    move(0, 0);
    clrtobot();

    move(5, 0);
    prints("�z�f�F %d �����U��AAutoScan �f�F %d ��", nSelf, nAuto);

/**	DickG: �N�f�F�X����������� post �� Security �O�W	***********/
/*    DickG: �]���s�������W���ݼf�֤�סA�O�G�S�����n�d�U record ������
      strftime(buf, 200, "%Y/%m/%d/%H:%M", pt);

      strcpy(xboard, "Security");
      setbpath(xfpath, xboard);
      stampfile(xfpath, &xfile);
      strcpy(xfile.owner, "�t��");
      strcpy(xfile.title, "[���i] �f�ְO��");
      xfile.savemode = 'S';
      xptr = fopen(xfpath, "w");
      fprintf(xptr, "\n�ɶ��G%s 
      %s �f�F %d �����U��\n
      AutoScan �f�F %d �����U��\n
      �@�p %d ���C", 
      buf, cuser.userid, nSelf, nAuto, nSelf+nAuto);
      fclose(xptr);
      setbdir(fname, xboard);
      append_record(fname, &xfile, sizeof(xfile));
      outgo_post(&xfile, xboard);
      inbtotal(getbnum(xboard), 1);
      cuser.numposts++;        
*/
/*********************************************/
    pressanykey();
    return (0);
}

extern char* fn_register;
extern int t_lines;

int m_register() {
    FILE *fn;
    int x, y, wid, len;
    char ans[4];
    char genbuf[200];
    
    if((fn = fopen(fn_register, "r")) == NULL) {
	outs("�ثe�õL�s���U���");
	return XEASY;
    }
    
    stand_title("�f�֨ϥΪ̵��U���");
    y = 2;
    x = wid = 0;
    
    while(fgets(genbuf, STRLEN, fn) && x < 65) {
	if(strncmp(genbuf, "uid: ", 5) == 0) {
	    move(y++, x);
	    outs(genbuf + 5);
	    len = strlen(genbuf + 5);
	    if(len > wid)
		wid = len;
	    if(y >= t_lines - 3) {
		y = 2;
		x += wid + 2;
	    }
	}
    }
    fclose(fn);
    getdata(b_lines - 1, 0, "�}�l�f�ֶ�(Auto/Yes/No)�H[N] ", ans, 3, LCECHO);
    if(ans[0] == 'a')
	scan_register_form(fn_register, 1, 0);
    else if(ans[0] == 'y')
	scan_register_form(fn_register, 0, 0);
    
    return 0;
}

int cat_register() {
    if(system("cat register.new.tmp >> register.new") == 0 &&
       system("rm -f register.new.tmp") == 0)
	mprints(22, 0, "OK �P~~ �~��h�İ��a!!                                                ");
    else
	mprints(22, 0, "�S��kCAT�L�h�O �h�ˬd�@�U�t�Χa!!                                    ");
    pressanykey();
    return 0;
}

static void give_id_money(char *user_id, int money, FILE *log_fp, char *mail_title, time_t t) {
    char tt[TTLEN + 1] = {0};
    
    if(inumoney(user_id, money) < 0) {
        move(12, 0);
        clrtoeol();
        prints("id:%s money:%d ����a!!", user_id, money);
        pressanykey();
    } else {
//  	move(12, 0);
//	clrtoeol();
//      prints("%s %d", user_id, money);
        fprintf(log_fp, "%ld %s %d", t, user_id, money);
        sprintf(tt, "%s : %d ptt ��", mail_title, money);
        mail_id(user_id, tt, "~bbs/etc/givemoney.why", "[PTT �Ȧ�]");
    }
}            

int give_money() {
    FILE *fp, *fp2;
    char *ptr, *id, *mn;
    char buf[200] = {0}, tt[TTLEN + 1] = {0}; 
    time_t t = time(NULL);
    struct tm *pt = localtime(&t);
    int to_all = 0, money = 0;
    
    getdata(0, 0, "���w�ϥΪ�(S) �����ϥΪ�(A) ����(Q)�H[S]", buf, 3, LCECHO);
    if(buf[0] == 'q')
	return 1;
    else if( buf[0] == 'a') {
	to_all = 1;
	getdata(1, 0, "�o�h�ֿ��O?", buf, 20, DOECHO);
	money = atoi(buf);
	if(money <= 0) {
	    move(2, 0);
	    prints("��J���~!!");
	    pressanykey();
	    return 1;
	}
    } else {
	if(vedit("etc/givemoney.txt", NA, NULL) < 0)
	    return 1;
    }
    
    clear();
    getdata(0, 0, "�n�o���F��(Y/N)[N]", buf, 3, LCECHO);
    if(buf[0] != 'y')
	return 1;

    if(!(fp2 = fopen("etc/givemoney.log", "a")))
	return 1;
    strftime(buf, 200, "%Y/%m/%d/%H:%M", pt);
    fprintf(fp2, "%s\n", buf);
    
    getdata(1, 0, "���]�U���D �G", tt, TTLEN, DOECHO);
    move(2, 0);
    
    prints("�s���]�U���e");
    pressanykey();
    if(vedit("etc/givemoney.why", NA, NULL) < 0)
	return 1;
    
    stand_title("�o����...");
    if(to_all) {
	extern struct uhash_t *uhash;
	int i, unum;
	for(unum = uhash->number, i=0; i<unum; i++) {
	    if(bad_user_id(uhash->userid[i]))
		continue;
	    id = uhash->userid[i];
	    give_id_money(id, money, fp2, tt, t);
	}
    } else {
	if(!(fp = fopen("etc/givemoney.txt", "r+"))) {
	    fclose(fp2);
	    return 1;
	}
	while(fgets(buf, 255, fp)) {
//  	    clear();
	    if (!(ptr = strchr(buf, ':')))
		continue;
	    *ptr = '\0';
	    id = buf;
	    mn = ptr + 1;
	    give_id_money(id, atoi(mn), fp2, tt, t);
	}
	fclose(fp);
    }
    
    fclose(fp2);
    pressanykey();
    return FULLUPDATE;
}
