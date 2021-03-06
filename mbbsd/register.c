/* $Id$ */
#define _XOPEN_SOURCE

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"
#include "pttstruct.h"
#include "perm.h"
#include "common.h"
#include "proto.h"

extern char *str_new;
extern char *msg_uid;
extern int t_lines, t_columns;  /* Screen size / width */
extern char *str_mail_address;

/* password encryption */
static char pwbuf[14];

char *genpasswd(char *pw) {
    if(pw[0]) {
	char saltc[2], c;
	int i;
	
	i = 9 * getpid();
	saltc[0] = i & 077;
	saltc[1] = (i >> 6) & 077;
	
	for(i = 0; i < 2; i++) {
	    c = saltc[i] + '.';
	    if(c > '9')
		c += 7;
	    if(c > 'Z')
		c += 6;
	    saltc[i] = c;
	}
	strcpy(pwbuf, pw);
	return crypt(pwbuf, saltc);
    }
    return "";
}

int checkpasswd(char *passwd, char *test) {
    char *pw;
    
    strncpy(pwbuf, test, 14);
    pw = crypt(pwbuf, passwd);
    return (!strncmp(pw, passwd, 14));
}

/* 檢查 user 註冊情況 */
int bad_user_id(char *userid) {
    int len, i;
    len = strlen(userid);

    if(len < 2)
	return 1;

    if (not_alpha(userid[0]))
	return 1;
    for (i=1; i<len; i++)	//DickG:修正了只比較 userid 第一個字元的 bug
	if(not_alnum(userid[i]))
            return 1;
    
    if(!strcasecmp(userid, str_new))
	return 1;
    
    /*   while((ch = *(++userid)))
	 if(not_alnum(ch))
	 return 1;*/
    return 0;
}

/* -------------------------------- */
/* New policy for allocate new user */
/* (a) is the worst user currently  */
/* (b) is the object to be compared */
/* -------------------------------- */
static int compute_user_value(userec_t *urec, time_t clock) {
    int value;
    
    /* if (urec) has XEMPT permission, don't kick it */
    if((urec->userid[0] == '\0') || (urec->userlevel & PERM_XEMPT)
       /*|| (urec->userlevel & PERM_LOGINOK)*/
       /* Ptt: 暫時不自動砍認證過的人 */
       /* Jaky: 人口增加太快了, 必需要緩和一下 */
       || !strcmp(STR_GUEST,urec->userid))
	return 9999;
    value = (clock - urec->lastlogin) / 60;       /* minutes */
    
    /* new user should register in 30 mins */
    if(strcmp(urec->userid, str_new) == 0)
	return 30 - value;
#if 0    
    if (!urec->numlogins)         /* 未 login 成功者，不保留 */
	return -1;
    if (urec->numlogins <= 3)     /* #login 少於三者，保留 20 天 */
	return 20 * 24 * 60 - value;
#endif
    /* 未完成註冊者，保留 15 天 */
    /* 一般情況，保留 120 天 */
    return (urec->userlevel & PERM_LOGINOK ? 120 : 15) * 24 * 60 - value;
}

extern char *fn_passwd;

int getnewuserid() {
    static char *fn_fresh = ".fresh";
    userec_t utmp, zerorec;
    time_t clock;
    struct stat st;
    int fd, val, i;
    char genbuf[200];
    char genbuf2[200];
    
    memset(&zerorec, 0, sizeof(zerorec));
    clock = time(NULL);
    
    /* Lazy method : 先找尋已經清除的過期帳號 */
    if((i = searchnewuser(0)) == 0) {
	/* 每 1 個小時，清理 user 帳號一次 */
	if((stat(fn_fresh, &st) == -1) || (st.st_mtime < clock - 3600)) {
	    if((fd = open(fn_fresh, O_RDWR | O_CREAT, 0600)) == -1)
		return -1;
	    write(fd, ctime(&clock), 25);
	    close(fd);
	    log_usies("CLEAN", "dated users");
	    
	    fprintf(stdout, "尋找新帳號中, 請稍待片刻...\n\r");
	    
	    if((fd = open(fn_passwd, O_RDWR | O_CREAT, 0600)) == -1)
		return -1;
	    
	    /* 不曉得為什麼要從 2 開始... */
 	    for(i = 2; i <= MAX_USERS; i++) {
		passwd_query(i, &utmp);
		if((val = compute_user_value(&utmp, clock)) < 0) {
		    sprintf(genbuf, "#%d %-12s %15.15s %d %d %d",
			    i, utmp.userid, ctime(&(utmp.lastlogin)) + 4,
			    utmp.numlogins, utmp.numposts, val);
		    if(val > -1 * 60 * 24 * 365) {
			log_usies("CLEAN", genbuf);
			sprintf(genbuf, "home/%c/%s", utmp.userid[0],
				utmp.userid);
			sprintf(genbuf2, "tmp/%s", utmp.userid);
			if(dashd(genbuf) && Rename(genbuf, genbuf2)) {
			    sprintf(genbuf, "/bin/rm -fr home/%c/%s",
				    utmp.userid[0],utmp.userid);
			    system(genbuf);
			}
			passwd_update(i, &zerorec);
		        remove_from_uhash(i - 1);
		        add_to_uhash(i - 1, "");
		    } else
			log_usies("DATED", genbuf);
		}
	    }
	}
    }
    
    passwd_lock();
    i = searchnewuser(1);
    if((i <= 0) || (i > MAX_USERS)) {
	passwd_unlock();
	if(more("etc/user_full", NA) == -1)
	    fprintf(stdout, "抱歉，使用者帳號已經滿了，無法註冊新的帳號\n\r");
	safe_sleep(2);
	exit(1);
    }
    
    sprintf(genbuf, "uid %d", i);
    log_usies("APPLY", genbuf);
    
    strcpy(zerorec.userid, str_new);
    zerorec.lastlogin = clock;
    passwd_update(i, &zerorec);
    setuserid(i, zerorec.userid);
    passwd_unlock();
    return i;
}

void new_register() {
    userec_t newuser;
    char passbuf[STRLEN];
    int allocid, try;

    memset(&newuser, 0, sizeof(newuser));
    more("etc/register", NA);
    try = 0;
    while(1) {
	if(++try >= 6) {
	    outs("\n您嘗試錯誤的輸入太多，請下次再來吧\n");
	    refresh();
	    
	    pressanykey();
	    oflush();
	    exit(1);
	}
	getdata(17, 0, msg_uid, newuser.userid, IDLEN + 1, DOECHO);

	if(bad_user_id(newuser.userid))
	    outs("無法接受這個代號，請使用英文字母，並且不要包含空格\n");
	else if (searchuser(newuser.userid))
	    outs("此代號已經有人使用\n");
	else
	    break;
    }

    try = 0;
    while(1) {
	if(++try >= 6) {
	    outs("\n您嘗試錯誤的輸入太多，請下次再來吧\n");
	    refresh();
	    
	    pressanykey();
	    oflush();
	    exit(1);
	}
	if((getdata(19, 0, "請設定密碼：", passbuf, PASSLEN, NOECHO) < 3) ||
	   !strcmp(passbuf, newuser.userid)) {
	    outs("密碼太簡單，易遭入侵，至少要 4 個字，請重新輸入\n");
	    continue;
	}
	strncpy(newuser.passwd, passbuf, PASSLEN);
	getdata(20, 0, "請檢查密碼：", passbuf, PASSLEN, NOECHO);
	if(strncmp(passbuf, newuser.passwd, PASSLEN)) {
	    outs("密碼輸入錯誤, 請重新輸入密碼.\n");
	    continue;
	}
	passbuf[8] = '\0';
	strncpy(newuser.passwd, genpasswd(passbuf), PASSLEN);
	break;
    }
    newuser.userlevel = PERM_DEFAULT;
    newuser.uflag = COLOR_FLAG | BRDSORT_FLAG | MOVIE_FLAG;
    newuser.firstlogin = newuser.lastlogin = time(NULL);
    newuser.pager = 1;
    allocid = getnewuserid();
    if(allocid > MAX_USERS || allocid <= 0) {
	fprintf(stderr, "本站人口已達飽和！\n");
	exit(1);
    }
    
    if(passwd_update(allocid, &newuser) == -1) {
	fprintf(stderr, "客滿了，再見！\n");
	exit(1);
    }
    setuserid(allocid, newuser.userid);
    if(!dosearchuser(newuser.userid)) {
	fprintf(stderr, "無法建立帳號\n");
	exit(1);
    }
}

extern userec_t cuser;

void check_register() {
    char *ptr = NULL;
    
    stand_title("請詳細填寫個人資料");
    
    while(strlen(cuser.username) < 2)
	getdata(2, 0, "綽號暱稱：", cuser.username, 24, DOECHO);

    for(ptr = cuser.username; *ptr; ptr++) {
	if (*ptr == 9)              /* TAB convert */
	    *ptr = ' ';
    }
    while(strlen(cuser.realname) < 4)
	getdata(4, 0, "真實姓名：", cuser.realname, 20, DOECHO);
    
    while(strlen(cuser.address) < 8)
	getdata(6, 0, "聯絡地址：", cuser.address, 50, DOECHO);

    if(!strchr(cuser.email, '@')) {
	bell();
	move(t_lines - 4, 0);
	prints("※ 為了您的權益，請填寫真實的 E-mail address， "
	       "以資確認閣下身份，\n"
	       "格式為 \033[44muser@domain_name\033[0m 或 \033[44muser"
	       "@\\[ip_number\\]\033[0m。\n\n"
	       "※ 如果您真的沒有 E-mail，請直接按 [return] 即可。");

	do {
	    getdata(8, 0, "電子信箱：", cuser.email, 50, DOECHO);
	    if(!cuser.email[0])
		sprintf(cuser.email, "%s%s", cuser.userid, str_mail_address);
	} while(!strchr(cuser.email, '@'));

    }
    if(!HAS_PERM(PERM_SYSOP) && !HAS_PERM(PERM_LOGINOK)) {
	/* 回覆過身份認證信函，或曾經 E-mail post 過 */
	clear();
	move(9,3);
	prints("請詳填寫\033[32m註冊申請單\033[m，"
	       "通告站長以獲得進階使用權力。\n\n\n\n");
	u_register();
    }

#ifdef NEWUSER_LIMIT
    if(!(cuser.userlevel & PERM_LOGINOK) && !HAS_PERM(PERM_SYSOP)) {
	if(cuser.lastlogin - cuser.firstlogin < 3 * 86400)
	    cuser.userlevel &= ~PERM_POST;
	more("etc/newuser", YEA);
    }
#endif
}
