/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"
#include "pttstruct.h"
#include "perm.h"
#include "modes.h"
#include "common.h"
#include "proto.h"

static int g_board_names(boardheader_t *fhdr) {
    AddNameList(fhdr->brdname);
    return 0;
}

extern userec_t cuser;

static void mail_by_link(char* owner, char* title, char* path) {
    char genbuf[200];
    fileheader_t mymail;     
    
    sprintf(genbuf,BBSHOME"/home/%c/%s", cuser.userid[0], cuser.userid);
    stampfile(genbuf, &mymail);
    strcpy(mymail.owner, owner);
    sprintf(mymail.title, title); 
    mymail.savemode = 0;
    unlink(genbuf);
    Link(path, genbuf);
    sprintf(genbuf,BBSHOME"/home/%c/%s/.DIR",cuser.userid[0],cuser.userid); 
                                                                  
    append_record(genbuf, &mymail, sizeof(mymail));      
}

extern int usernum;

void anticrosspost() {
    char buf[200];
    time_t now = time(NULL);
    
    sprintf(buf,
	    "\033[1;33;46m%s \033[37;45mcross post 文章 \033[37m %s\033[m",
	    cuser.userid, ctime(&now));
    log_file("etc/illegal_money", buf);
    
    post_violatelaw(cuser.userid, "Ptt系統警察", "Cross-post", "罰單處份");    
    cuser.userlevel |= PERM_VIOLATELAW;
    cuser.vl_count ++;
    mail_by_link("Ptt警察部隊", "Cross-Post罰單",
		 BBSHOME "/etc/crosspost.txt");
    reload_money();
    passwd_update(usernum, &cuser);
    exit(0);
}

/* Heat CharlieL*/
int save_violatelaw() {
    char buf[128], ok[3];
    
    setutmpmode(VIOLATELAW); 
    clear();
    stand_title("繳罰單中心");

    if(!(cuser.userlevel & PERM_VIOLATELAW)) {
	mprints(22, 0, "\033[1;31m你無聊啊? 你又沒有被開罰單~~\033[m");
	pressanykey();
	return 0;
    }
  
    reload_money();
    if(cuser.money < (int)cuser.vl_count*1000) {
	sprintf(buf, "\033[1;31m這是你第 %d 次違反本站法規"
		"必須繳出 %d $Ptt ,你只有 %ld 元, 錢不夠啦!!\033[m",
		(int)cuser.vl_count, (int)cuser.vl_count * 1000, cuser.money);
	mprints(22, 0, buf);
	pressanykey();
	return 0;
    }
                            
    move(5,0);
    prints("\033[1;37m你知道嗎? 因為你的違法 "
	   "已經造成很多人的不便\033[m\n");
    prints("\033[1;37m你是否確定以後不會再犯了？\033[m\n");
    
    if(!getdata(10,0,"確定嗎？[y/n]:", ok, 2, LCECHO) ||
       ok[0] == 'n' || ok[0] == 'N') {
	mprints(22,0,"\033[1;31m等你想通了再來吧!! "
		"我相信你不會知錯不改的~~~\033[m");
	pressanykey();
	return 0;
    }

    sprintf(buf, "這是你第 %d 次違法 必須繳出 %d $Ptt",
	    cuser.vl_count, cuser.vl_count*1000);
    mprints(11,0,buf);

    if(!getdata(10, 0, "要付錢[y/n]:", ok, 2, LCECHO) || 
       ok[0] == 'N' || ok[0] == 'n') {

	mprints(22,0, "\033[1;31m 嗯 存夠錢 再來吧!!!\033[m");
	pressanykey();
	return 0;
    }
    
    demoney(cuser.vl_count*1000);
    cuser.userlevel &= (~PERM_VIOLATELAW);
    reload_money();   
    passwd_update(usernum, &cuser);
    return 0;
}

void make_blist() {
    CreateNameList();
    apply_boards(g_board_names);
}

extern int currbid;
extern char currBM[];
extern int currmode;
extern char currboard[];
static time_t board_note_time;
static char *brd_title;

void set_board() {
    boardheader_t *bp;

    bp = getbcache(currbid);
    board_note_time = bp->bupdate;
    brd_title = bp->BM;
    if(brd_title[0] <= ' ')
	brd_title = "徵求中";
    sprintf(currBM, "板主：%s", brd_title);
    brd_title = ((bp->bvote != 2 && bp->bvote) ? "本看板進行投票中" :
		 bp->title + 7);
    currmode = (currmode & (MODE_DIRTY | MODE_MENU)) | MODE_STARTED ;
		 
    if (HAS_PERM(PERM_ALLBOARD) || is_BM(bp->BM))
	currmode = currmode | MODE_BOARD | MODE_POST;
    else if(haspostperm(currboard))
    	currmode |= MODE_POST;
}

static void readtitle() {
    showtitle(currBM, brd_title);
    outs("[←]離開 [→]閱\讀 [^P]發表文章 [b]備忘錄 [d]刪除 [z]精華區 "
	 "[TAB]文摘 [h]elp\n\033[7m  編號   日 期  作  者       文  章  標  題"
	 "                                   \033[m");
}

extern int brc_num;
extern int brc_list[];
extern char currtitle[];

static void readdoent(int num, fileheader_t *ent) {
    int type;
    char *mark, *title, color;
    
    type = brc_unread(ent->filename,brc_num,brc_list) ? '+' : ' ';
    
    if((currmode & MODE_BOARD) && (ent->filemode & FILE_DIGEST))
	type = (type == ' ') ? '*' : '#';
    else if(currmode & MODE_BOARD || HAS_PERM(PERM_LOGINOK)) {
	if(ent->filemode & FILE_MARKED)
	    type = (type == ' ') ? 'm' : 'M';
	else if(ent->filemode & FILE_TAGED)
	    type = 'D';
	else if (ent->filemode & FILE_SOLVED)
	    type = 's';
    }
    
    title = subject(mark = ent->title);
    if(title == mark)
	color = '1', mark = "□";
    else
	color = '3', mark = "R:";
    
    if(title[47])
	strcpy(title + 44, " …");  /* 把多餘的 string 砍掉 */

    if(strncmp(currtitle, title, 40))
	prints("%6d %c %-7s%-13.12s%s %s\n", num, type,
	       ent->date, ent->owner, mark, title);
    else
	prints("%6d %c %-7s%-13.12s\033[1;3%cm%s %s\033[m\n", num, type,
	       ent->date, ent->owner, color, mark, title);
}

extern char currfile[];

int cmpfilename(fileheader_t *fhdr) {
    return (!strcmp(fhdr->filename, currfile));
}

extern unsigned char currfmode;

int cmpfmode(fileheader_t *fhdr) {
    return (fhdr->filemode & currfmode);
}

extern char currowner[];

int cmpfowner(fileheader_t *fhdr) {
    return !strcasecmp(fhdr->owner, currowner);
}

extern char *err_bid;
extern userinfo_t *currutmp;

static int do_select(int ent, fileheader_t *fhdr, char *direct) {
    char bname[20];
    char bpath[60];
    struct stat st;
    
    move(0, 0);
    clrtoeol();
    make_blist();
    namecomplete(MSG_SELECT_BOARD, bname);

    currbid = getbnum(bname);
    if(currbid)
	strcpy(bname, getbcache(currbid)->brdname);
    else
	return FULLUPDATE;
    
    setbpath(bpath, bname);
    if((*bname == '\0') || (stat(bpath, &st) == -1)) {
	move(2, 0);
	clrtoeol();
	outs(err_bid);
	return FULLUPDATE;
    }
    
    currutmp->brc_id = currbid;
    /*    */

    brc_initial(bname);
    set_board();
    setbdir(direct, currboard);

    move(1, 0);
    clrtoeol();
    return NEWDIRECT;
}

/* ----------------------------------------------------- */
/* 改良 innbbsd 轉出信件、連線砍信之處理程序             */
/* ----------------------------------------------------- */
void outgo_post(fileheader_t *fh, char *board) {
    FILE *foo;
    
    if((foo = fopen("innd/out.bntp", "a"))) {
	fprintf(foo, "%s\t%s\t%s\t%s\t%s\n", board,
		fh->filename, cuser.userid, cuser.username, fh->title);
	fclose(foo);
    }
}

extern char *str_author1;
extern char *str_author2;

static void cancelpost(fileheader_t *fh, int by_BM) {
    FILE *fin, *fout;
    char *ptr, *brd;
    fileheader_t postfile;
    char genbuf[200];
    char nick[STRLEN], fn1[STRLEN], fn2[STRLEN];
    
    setbfile(fn1, currboard, fh->filename);
    if((fin = fopen(fn1, "r"))) {
	brd = by_BM ? "deleted" : "junk";

	setbpath(fn2, brd);
	stampfile(fn2, &postfile);
	memcpy(postfile.owner, fh->owner, IDLEN + TTLEN + 10);
	postfile.savemode = 'D';

	if(fh->savemode == 'S') {
	    nick[0] = '\0';
	    while(fgets(genbuf, sizeof(genbuf), fin)) {
		if (!strncmp(genbuf, str_author1, LEN_AUTHOR1) ||
		    !strncmp(genbuf, str_author2, LEN_AUTHOR2)) {
		    if((ptr = strrchr(genbuf, ')')))
			*ptr = '\0';
		    if((ptr = (char *)strchr(genbuf, '(')))
			strcpy(nick, ptr + 1);
		    break;
		}
	    }

	    if((fout = fopen("innd/cancel.bntp", "a"))) {
		fprintf(fout, "%s\t%s\t%s\t%s\t%s\n", currboard, fh->filename,
			cuser.userid, nick, fh->title);
		fclose(fout);
	    }
	}
	
	fclose(fin);
	Rename(fn1, fn2);
	setbdir(genbuf, brd);
	append_record(genbuf, &postfile, sizeof(postfile));
    }
}

extern char *str_reply;
extern char save_title[];

/* ----------------------------------------------------- */
/* 發表、回應、編輯、轉錄文章                            */
/* ----------------------------------------------------- */
void do_reply_title(int row, char *title) {
    char genbuf[200];
    char genbuf2[4];

    if(strncasecmp(title, str_reply, 4))
	sprintf(save_title, "Re: %s", title);
    else
	strcpy(save_title, title);
    save_title[TTLEN - 1] = '\0';
    sprintf(genbuf, "採用原標題《%.60s》嗎?[Y] ", save_title);
    getdata(row, 0, genbuf, genbuf2, 4, LCECHO);
    if(genbuf2[0] == 'n' || genbuf2[0] == 'N')
	getdata(++row, 0, "標題：", save_title, TTLEN, DOECHO);
}

static void do_unanonymous_post(char* fpath) {
    fileheader_t mhdr;
    char title[128];
    char genbuf[200];

    setbpath(genbuf, "UnAnonymous");
    if(dashd(genbuf)) {
	stampfile(genbuf, &mhdr);
	unlink(genbuf);
	Link(fpath, genbuf);
	strcpy(mhdr.owner, cuser.userid);
	strcpy(mhdr.title, save_title);
	mhdr.savemode = 0;
	mhdr.filemode = 0;
	setbdir(title, "UnAnonymous");
	append_record(title, &mhdr, sizeof(mhdr));
    }
}

extern char quote_file[];
extern char quote_user[];
extern int curredit;
extern unsigned int currbrdattr;
extern char currdirect[];
extern char *err_uid;

#ifdef NO_WATER_POST
static time_t last_post_time = 0;
static time_t water_counts = 0;
#endif
int local_article;
char real_name[20];

static int do_general() {
    fileheader_t postfile;
    char fpath[80], buf[80];
    int aborted, defanony, ifuseanony;
    char genbuf[200],*owner;
    boardheader_t *bp;
    int islocal;
    
    ifuseanony = 0;
    bp = getbcache(currbid);
    
    clear();
    if(!(currmode & MODE_POST)) {
	move(5, 10);
	outs("對不起，您目前無法在此發表文章！");
	pressanykey();
	return FULLUPDATE;
    }

#ifdef NO_WATER_POST
    /* 三分鐘內最多發表五篇文章 */
    if(currutmp->lastact - last_post_time < 60 * 3) {
	if(water_counts >= 5) {
	    move(5, 10);
	    outs("對不起，您的文章太水囉，多思考一下，待會再post吧！");
	    pressanykey();
	    return FULLUPDATE;
        }
    } else {
	last_post_time = currutmp->lastact;
	water_counts = 0;
    }
#endif
    
    setbfile(genbuf, currboard, FN_POST_NOTE );
    
    if(more(genbuf,NA) == -1)
	more("etc/"FN_POST_NOTE , NA);
    
    move(19,0);
    prints("發表文章於【\033[33m %s\033[m 】 \033[32m%s\033[m 看板\n\n",
	   currboard, bp->title + 7);
    
    if(quote_file[0])
	do_reply_title(20, currtitle);
    else {
	getdata(21, 0, "標題：", save_title, TTLEN, DOECHO);
	strip_ansi(save_title,save_title,0);
    }
    if(save_title[0] == '\0')
	return FULLUPDATE;
    
    curredit &= ~EDIT_MAIL;
    curredit &= ~EDIT_ITEM;
    setutmpmode(POSTING);
    
    /* 未具備 Internet 權限者，只能在站內發表文章 */
    if(HAS_PERM(PERM_INTERNET))
	local_article = 0;
    else
	local_article = 1;

    /* build filename */
    setbpath(fpath, currboard);
    stampfile(fpath, &postfile);
    
    aborted = vedit(fpath, YEA, &islocal);
    if(aborted == -1) {
	unlink(fpath);
	pressanykey();
	return FULLUPDATE;
    }
    water_counts++;  /* po成功 */

    /* set owner to Anonymous , for Anonymous board */

#ifdef HAVE_ANONYMOUS
    /* Ptt and Jaky */
    defanony=currbrdattr & BRD_DEFAULTANONYMOUS;
    if((currbrdattr & BRD_ANONYMOUS) && 
       ((strcmp(real_name,"r") && defanony) || (real_name[0] && !defanony)) 
	) {
	strcat(real_name,".");
	owner = real_name;
	ifuseanony=1;
    } else
	owner = cuser.userid;
#else
    owner = cuser.userid;
#endif
    /* 錢 */
    aborted = (aborted > MAX_POST_MONEY * 2) ? MAX_POST_MONEY : aborted / 2;
    postfile.money = aborted;
    strcpy(postfile.owner, owner);
    strcpy(postfile.title, save_title);
    if(islocal) {            /* local save */
	postfile.savemode = 'L';
	postfile.filemode = FILE_LOCAL;
    } else
	postfile.savemode = 'S';
    
    setbdir(buf, currboard);
    if(append_record(buf, &postfile, sizeof(postfile)) != -1) {
	inbtotal(currbid, 1);

	if(currmode & MODE_SELECT)
	    append_record(currdirect,&postfile,sizeof(postfile));
	if(!islocal && !(bp->brdattr & BRD_NOTRAN))
	    outgo_post(&postfile, currboard);
	brc_addlist(postfile.filename);

	if(!(currbrdattr  & BRD_HIDE) &&
	   (!bp->level || (currbrdattr  & BRD_POSTMASK))) {
	    setbpath(genbuf, ALLPOST);
	    stampfile(genbuf, &postfile);
	    unlink(genbuf);

	    /* jochang: boards may spread across many disk */
	    /* link doesn't work across device,
	       Link doesn't work if we have same-time-across-device posts,
	       we try symlink now */
	    {
	    	/* we need absolute path for symlink */
	    	char abspath[256]=BBSHOME"/";
	    	strcat(abspath,fpath);
	    	symlink(abspath,genbuf);
	    }
	    strcpy(postfile.owner, owner);
	    strcpy(postfile.title, save_title);
	    postfile.savemode = 'L';
	    setbdir(genbuf, ALLPOST);
	    if(append_record(genbuf, &postfile, sizeof(postfile)) != -1) {
		inbtotal(getbnum(ALLPOST), 1);
	    }
	}
	
	outs("順利貼出佈告，");
	
	if(strcmp(currboard, "Test") && !ifuseanony) {
	    prints("這是您的第 %d 篇文章。 稿酬 %d 銀。",
		   ++cuser.numposts, aborted );
	    inmoney(aborted);
	} else
	    outs("測試信件不列入紀錄，敬請包涵。");
	
	/* 回應到原作者信箱 */
	
	if(curredit & EDIT_BOTH) {
	    char *str, *msg = "回應至作者信箱";

	    if((str = strchr(quote_user, '.'))) {
		if(
#ifndef USE_BSMTP
		    bbs_sendmail(fpath, save_title, str + 1)
#else
		    bsmtp(fpath, save_title, str + 1 ,0)
#endif
		    < 0)
		    msg = "作者無法收信";
	    } else {
		sethomepath(genbuf, quote_user);
		stampfile(genbuf, &postfile);
		unlink(genbuf);
		Link(fpath, genbuf);

		strcpy(postfile.owner, cuser.userid);
		strcpy(postfile.title, save_title);
		postfile.savemode = 'B';/* both-reply flag */
		sethomedir(genbuf, quote_user);
		if(append_record(genbuf, &postfile, sizeof(postfile)) == -1)
		    msg = err_uid;
	    }
	    outs(msg);
	    curredit ^= EDIT_BOTH;
	}
	if(currbrdattr & BRD_ANONYMOUS)
	    do_unanonymous_post(fpath);
    }
    pressanykey();
    return FULLUPDATE;
}

int do_post() {
    boardheader_t *bp;
    bp = getbcache(currbid);
    if(bp->brdattr & BRD_VOTEBOARD)
        return do_voteboard();
    else
        return do_general();
}

extern int b_lines;
extern int curredit;

static void do_generalboardreply(fileheader_t *fhdr){            
    char genbuf[200];
    getdata(b_lines - 1, 0,
	    "▲ 回應至 (F)看板 (M)作者信箱 (B)二者皆是 (Q)取消？[F] ",
	    genbuf, 3, LCECHO);
    switch(genbuf[0]) {
    case 'm':
	mail_reply(0, fhdr, 0);
    case 'q':
	break;

    case 'b':
	curredit = EDIT_BOTH;
    default:
	strcpy(currtitle, fhdr->title);
	strcpy(quote_user, fhdr->owner);
	quote_file[79] = fhdr->savemode;
	do_post();
    }
    *quote_file = 0;
}

int getindex(char *fpath, char *fname, int size) {
    int fd, now=0;
    fileheader_t fhdr;
    
    if((fd = open(fpath, O_RDONLY, 0)) != -1) {
	while((read(fd, &fhdr, size) == size)) {
	    now++;
	    if(!strcmp(fhdr.filename,fname)) {
		close(fd);
		return now;
	    }
	}
	close(fd);
    }
    return 0;
}

int invalid_brdname(char *brd) {
    register char ch;
    
    ch = *brd++;
    if(not_alnum(ch))
	return 1;
    while((ch = *brd++)) {
	if(not_alnum(ch) && ch != '_' && ch != '-' && ch != '.')
	    return 1;
    }
    return 0;
}       

static void do_reply(fileheader_t *fhdr) {
    boardheader_t *bp;
    bp = getbcache(currbid);
    if (bp->brdattr & BRD_VOTEBOARD)
        do_voteboardreply(fhdr);
    else
        do_generalboardreply(fhdr);
}

static int reply_post(int ent, fileheader_t *fhdr, char *direct) {
    if(!(currmode & MODE_POST))
	return DONOTHING;

    setdirpath(quote_file, direct, fhdr->filename);
    do_reply(fhdr);
    *quote_file = 0;
    return FULLUPDATE;
}

static int edit_post(int ent, fileheader_t *fhdr, char *direct) {
    char fpath[80], fpath0[80];
    char genbuf[200];
    fileheader_t postfile;
    boardheader_t *bp;
    bp = getbcache(currbid);
    if (!HAS_PERM(PERM_SYSOP) && (bp->brdattr & BRD_VOTEBOARD))
	return DONOTHING;

    if ((!HAS_PERM(PERM_SYSOP)) && 
        strcmp(fhdr->owner, cuser.userid))
	return DONOTHING;
 
    setdirpath(genbuf, direct, fhdr->filename);
    local_article = fhdr->filemode & FILE_LOCAL;
    strcpy(save_title, fhdr->title);

    if(vedit(genbuf, 0, NULL) != -1) {
	setbpath(fpath, currboard);
	stampfile(fpath, &postfile);
	unlink(fpath);
	setbfile(fpath0, currboard, fhdr->filename);
	Rename(fpath0, fpath);
	strcpy(fhdr->filename, postfile.filename);
	strcpy(fhdr->title, save_title);
	brc_addlist(postfile.filename);
	substitute_record(direct, fhdr, sizeof(*fhdr), ent);
    }
    return FULLUPDATE;
}

extern crosspost_t postrecord;
#define UPDATE_USEREC   (currmode |= MODE_DIRTY)

static int cross_post(int ent, fileheader_t *fhdr, char *direct) {
    char xboard[20], fname[80], xfpath[80], xtitle[80], inputbuf[10];
    fileheader_t xfile;
    FILE *xptr;
    int author = 0;
    char genbuf[200];
    char genbuf2[4];
    boardheader_t *bp;
    
    make_blist();
    move(2, 0);
    clrtoeol();
    move(3, 0);
    clrtoeol();
    move(1, 0);
    bp = getbcache(currbid);
    if (bp->brdattr & BRD_VOTEBOARD)
	return FULLUPDATE;
       
    namecomplete("轉錄本文章於看板：", xboard);
    if(*xboard == '\0' || !haspostperm(xboard))
	return FULLUPDATE;
	
    if((ent = str_checksum(fhdr->title)) != 0 &&
       ent == postrecord.checksum[0]) {
	/* 檢查 cross post 次數 */
	if(postrecord.times++ > MAX_CROSSNUM)
	    anticrosspost();
    } else {
	postrecord.times = 0;
	postrecord.checksum[0] = ent;
    }

    ent = 1;
    if(HAS_PERM(PERM_SYSOP) || !strcmp(fhdr->owner, cuser.userid)) {
	getdata(2, 0, "(1)原文轉載 (2)舊轉錄格式？[1] ",
		genbuf, 3, DOECHO);
	if(genbuf[0] != '2') {
	    ent = 0;
	    getdata(2, 0, "保留原作者名稱嗎?[Y] ", inputbuf, 3, DOECHO);
	    if (inputbuf[0] != 'n' && inputbuf[0] != 'N') author = 1;
	}
    }

    if(ent)
	sprintf(xtitle, "[轉錄]%.66s", fhdr->title);
    else
	strcpy(xtitle, fhdr->title);

    sprintf(genbuf, "採用原標題《%.60s》嗎?[Y] ", xtitle);
    getdata(2, 0, genbuf, genbuf2, 4, LCECHO);
    if(genbuf2[0] == 'n' || genbuf2[0] == 'N') {
	if(getdata_str(2, 0, "標題：", genbuf, TTLEN, DOECHO,xtitle))
	    strcpy(xtitle, genbuf);
    }
    
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

	setbfile(fname, currboard, fhdr->filename);
//	if(ent)	{
	xptr = fopen(xfpath, "w");

	strcpy(save_title, xfile.title);
	strcpy(xfpath, currboard);
	strcpy(currboard, xboard);
	write_header(xptr);
	strcpy(currboard, xfpath);

	fprintf(xptr, "※ [本文轉錄自 %s 看板]\n\n", currboard);

	b_suckinfile(xptr, fname);
	addsignature(xptr,0);
	fclose(xptr);
/* Cross fs有問題
   } else {
   unlink(xfpath);
   link(fname, xfpath);
   }
*/
	setbdir(fname, xboard);
	append_record(fname, &xfile, sizeof(xfile));
	bp = getbcache(getbnum(xboard));
	if(!xfile.filemode && !(bp->brdattr && BRD_NOTRAN))
	    outgo_post(&xfile, xboard);
	inbtotal(getbnum(xboard), 1);
	cuser.numposts++;
	UPDATE_USEREC;
	outs("文章轉錄完成");
	pressanykey();
	currmode = currmode0;
    }
    return FULLUPDATE;
}

static int read_post(int ent, fileheader_t *fhdr, char *direct) {
    char genbuf[200];
    int more_result;

    if(fhdr->owner[0] == '-')
	return DONOTHING;

    setdirpath(genbuf, direct, fhdr->filename);

    if((more_result = more(genbuf, YEA)) == -1)
	return FULLUPDATE;

    brc_addlist(fhdr->filename);
    strncpy(currtitle, subject(fhdr->title), 40);
    strncpy(currowner, subject(fhdr->owner), IDLEN + 2);

    switch (more_result) {
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
    case 8:
	if((currmode & MODE_POST)) {
	    strcpy(quote_file, genbuf);
	    do_reply(fhdr);
	    *quote_file = 0;
	}
	return FULLUPDATE;
    case 9:
	return 'A';
    case 10:
	return 'a';
    case 11:
	return '/';
    case 12:
	return '?';
    }


    outmsg("\033[34;46m  閱\讀文章  \033[31;47m  (R/Y)\033[30m回信 \033[31m"
	   "(=[]<>)\033[30m相關主題 \033[31m(↑↓)\033[30m上下封 \033[31m(←)"
	   "\033[30m離開  \033[m");

    switch(egetch()) {
    case 'q':
    case 'Q':
    case KEY_LEFT:
	break;

    case ' ':
    case KEY_RIGHT:
    case KEY_DOWN:
    case KEY_PGDN:
    case 'n':
    case Ctrl('N'):
	return READ_NEXT;

    case KEY_UP:
    case 'p':
    case Ctrl('P'):
    case KEY_PGUP:
	return READ_PREV;

    case '=':
	return RELATE_FIRST;

    case ']':
    case 't':
	return RELATE_NEXT;

    case '[':
	return RELATE_PREV;

    case '.':
    case '>':
	return THREAD_NEXT;

    case ',':
    case '<':
	return THREAD_PREV;

    case Ctrl('C'):
	cal();
	return FULLUPDATE;
	break;

    case Ctrl('I'):
	t_idle();
	return FULLUPDATE;
    case 'y':
    case 'r':
    case 'R':
    case 'Y':
	if((currmode & MODE_POST)) {
	    strcpy(quote_file, genbuf);
	    do_reply(fhdr);
	    *quote_file = 0;
	}
    }
    return FULLUPDATE;
}

/* ----------------------------------------------------- */
/* 採集精華區                                            */
/* ----------------------------------------------------- */
static int b_man() {
    char buf[64];
    
    setapath(buf, currboard);
    return a_menu(currboard, buf, HAS_PERM(PERM_ALLBOARD) ? 2 :
		  (currmode & MODE_BOARD ? 1 : 0));
}

static int cite_post(int ent, fileheader_t *fhdr, char *direct) {
    char fpath[256];
    char title[TTLEN + 1];
    
    setbfile(fpath, currboard, fhdr->filename);
    strcpy(title, "◇ ");
    strncpy(title+3, fhdr->title, TTLEN-3);
    title[TTLEN] = '\0';
    a_copyitem(fpath, title, 0, 1);
    b_man();
    return FULLUPDATE;
}

int edit_title(int ent, fileheader_t *fhdr, char *direct) {
    char genbuf[200];
    fileheader_t tmpfhdr = *fhdr;
    int dirty = 0;

    if(currmode & MODE_BOARD || !strcmp(cuser.userid,fhdr->owner)) {
        if(getdata(b_lines - 1, 0, "標題：", genbuf, TTLEN, DOECHO)) {
	    strcpy(tmpfhdr.title, genbuf);
	    dirty++;
        }
    }

    if(HAS_PERM(PERM_SYSOP)) {
        if(getdata(b_lines - 1, 0, "作者：", genbuf, IDLEN + 2, DOECHO)) {
	    strcpy(tmpfhdr.owner, genbuf);
	    dirty++;
        }
	
        if(getdata(b_lines - 1, 0, "日期：", genbuf, 6, DOECHO)) {
	    sprintf(tmpfhdr.date, "%.5s", genbuf);
	    dirty++;
        }
    }

    if(currmode & MODE_BOARD || !strcmp(cuser.userid,fhdr->owner)) {
        getdata(b_lines-1, 0, "確定(Y/N)?[n] ", genbuf, 3, DOECHO);
	if((genbuf[0] == 'y' || genbuf[0] == 'Y') && dirty) {
	    *fhdr = tmpfhdr;
	    substitute_record(direct, fhdr, sizeof(*fhdr), ent);
	    if((currmode & MODE_SELECT)) {
		int now;

		setbdir(genbuf, currboard);
		now = getindex(genbuf, fhdr->filename, sizeof(fileheader_t));
		substitute_record(genbuf, fhdr, sizeof(*fhdr), now);
	    }
	}
	return FULLUPDATE;
    }
    return DONOTHING;
}

extern unsigned int currstat;

static int post_tag(int ent, fileheader_t *fhdr, char *direct) {
    if((currstat != READING) || (currmode & MODE_BOARD)){
	int now;
	char genbuf[100];

	fhdr->filemode ^= FILE_TAGED;
	setbdir(genbuf, currboard);
	now = getindex(genbuf, fhdr->filename, sizeof(fileheader_t));
	substitute_record(genbuf, fhdr, sizeof(*fhdr), now);
	sprintf(genbuf, "boards/%c/%s/SR.%s", *currboard, currboard, cuser.userid);
	substitute_record(genbuf, fhdr, sizeof(*fhdr), ent);
	return POS_NEXT;
    }
/*    
      if((currstat != READING) || (currmode & MODE_BOARD)) {
      fhdr->filemode ^= FILE_TAGED;
      substitute_record(direct, fhdr, sizeof(*fhdr), ent);
      return POS_NEXT;
      }*/
    return DONOTHING;
}

static int post_del_tag(int ent, fileheader_t *fhdr, char *direct) {
    char genbuf[3];

    if((currstat != READING) || (currmode & MODE_BOARD)) {
	getdata(1, 0, "確定刪除標記文章(Y/N)? [N]", genbuf, 3, LCECHO);
	if(genbuf[0] == 'y') {
	    currfmode = FILE_TAGED;
	    if(currmode & MODE_SELECT){
		unlink(direct);
		currmode ^= MODE_SELECT;
		setbdir(direct, currboard);
		delete_files(direct, cmpfmode, 1);
	    }
	    if(delete_files(direct, cmpfmode, 1))
		return DIRCHANGED;
	    if(currmode & MODE_BOARD)
		touchbtotal(currbid);
	}
	return FULLUPDATE;
    }
    return DONOTHING;
}

static int solve_post(int ent, fileheader_t * fhdr, char *direct){
    if (HAS_PERM(PERM_SYSOP)) {
	fhdr->filemode ^= FILE_SOLVED;
	substitute_record(direct, fhdr, sizeof(*fhdr), ent);
	return PART_REDRAW;
    }
    return DONOTHING;
}

static int mark_post(int ent, fileheader_t *fhdr, char *direct) {
    if(currmode & MODE_BOARD) {
	fhdr->filemode ^= FILE_MARKED;
	substitute_record(direct, fhdr, sizeof(*fhdr), ent);
	return PART_REDRAW;
    }
    return DONOTHING;
}

extern char *msg_sure_ny;

int del_range(int ent, fileheader_t *fhdr, char *direct) {
    char num1[8], num2[8];
    int inum1, inum2;

    if((currstat != READING) || (currmode & MODE_BOARD)) {
	getdata(1, 0, "[設定刪除範圍] 起點：", num1, 5, DOECHO);
	inum1 = atoi(num1);
	if(inum1 <= 0) {
	    outmsg("起點有誤");
	    refresh();
	    /*safe_sleep(1);*/
	    return FULLUPDATE;
	}
	getdata(1, 28, "終點：", num2, 5, DOECHO);
	inum2 = atoi(num2);
	if(inum2 < inum1) {
	    outmsg("終點有誤");
	    refresh();
	    /*safe_sleep(1);*/
	    return FULLUPDATE;
	}
	getdata(1, 48, msg_sure_ny, num1, 3, LCECHO);
	if(*num1 == 'y') {
	    outmsg("處理中,請稍後...");
	    refresh();
	    if(currmode & MODE_SELECT) {
		int fd,size = sizeof(fileheader_t);
		char genbuf[100];
		fileheader_t rsfh;
		int i = inum1,now;
		if(currstat == RMAIL)
		    sethomedir(genbuf, cuser.userid);
		else
		    setbdir(genbuf,currboard);
		if((fd = (open(direct, O_RDONLY, 0))) != -1) {
		    if(lseek(fd, (off_t)(size * (inum1 - 1)), SEEK_SET) !=
		       -1) {
			while(read(fd,&rsfh,size) == size) {
			    if(i > inum2)
				break;
			    now = getindex(genbuf, rsfh.filename, size);
			    strcpy(currfile, rsfh.filename);
			    delete_file(genbuf, sizeof(fileheader_t), now,
					cmpfilename);
			    i++;
			}
		    }
		    close(fd);
		}
	    }
	    
	    delete_range(direct, inum1, inum2, NULL);
	    fixkeep(direct, inum1);

	    if(currmode & MODE_BOARD)
		touchbtotal(currbid);
	    
	    return DIRCHANGED;
	}
	return FULLUPDATE;
    }
    return DONOTHING;
}

extern char *msg_del_ny;
extern char *msg_del_ok;

static int del_post(int ent, fileheader_t *fhdr, char *direct) {
    char genbuf[100];
    int not_owned;
    boardheader_t *bp;
    
    bp = getbcache(currbid);

    if((fhdr->filemode & FILE_MARKED) || (fhdr->filemode & FILE_DIGEST) ||
       (fhdr->owner[0] == '-'))
	return DONOTHING;

    not_owned = strcmp(fhdr->owner, cuser.userid);
    if((!(currmode & MODE_BOARD) && not_owned) ||
       ((bp->brdattr & BRD_VOTEBOARD) && !HAS_PERM(PERM_SYSOP)) ||
       !strcmp(cuser.userid, STR_GUEST))
	return DONOTHING;

    getdata(1, 0, msg_del_ny, genbuf, 3, LCECHO);
    if(genbuf[0] == 'y' || genbuf[0] == 'Y') {
	strcpy(currfile, fhdr->filename);
	
	setbfile(genbuf,currboard,fhdr->filename);
	if(!delete_file (direct, sizeof(fileheader_t), ent, cmpfilename)) {
	    if(currmode & MODE_SELECT) {
		int now;

		setbdir(genbuf,currboard);
		now=getindex(genbuf,fhdr->filename,sizeof(fileheader_t));
		delete_file (genbuf, sizeof(fileheader_t),now,cmpfilename);
	    }
	    cancelpost(fhdr, not_owned);

	    inbtotal(currbid, -1);
	    if (fhdr->money < 0)
		fhdr->money = 0;
	    if (not_owned && strcmp(currboard, "Test")){		
		deumoney(fhdr->owner, fhdr->money);
	    }
	    if(!not_owned && strcmp(currboard, "Test")) {
		if(cuser.numposts)
		    cuser.numposts--;
		move(b_lines - 1, 0);
		clrtoeol();
		demoney(fhdr->money);
		prints("%s，您的文章減為 %d 篇，支付清潔費 %d 銀", msg_del_ok,
		       cuser.numposts,fhdr->money);
		refresh();
		pressanykey();
	    }
	    return DIRCHANGED;
	}
    }
    return FULLUPDATE;
}

static int view_postmoney(int ent, fileheader_t *fhdr, char *direct) {
    move(b_lines - 1, 0);
    clrtoeol();
    prints("這一篇文章值 %d 銀", fhdr->money);
    refresh();
    pressanykey();
    return FULLUPDATE;
}

static int sequent_ent;
static int continue_flag;

/* ----------------------------------------------------- */
/* 依序讀新文章                                          */
/* ----------------------------------------------------- */
static int sequent_messages(fileheader_t *fptr) {
    static int idc;
    char genbuf[200];

    if(fptr == NULL)
	return (idc = 0);

    if(++idc < sequent_ent)
	return 0;

    if(!brc_unread(fptr->filename,brc_num,brc_list))
	return 0;

    if(continue_flag)
	genbuf[0] = 'y';
    else {
	prints("讀取文章於：[%s] 作者：[%s]\n標題：[%s]",
	       currboard, fptr->owner, fptr->title);
	getdata(3, 0, "(Y/N/Quit) [Y]: ", genbuf, 3, LCECHO);
    }

    if(genbuf[0] != 'y' && genbuf[0]) {
	clear();
	return (genbuf[0] == 'q' ? QUIT : 0);
    }

    setbfile(genbuf, currboard, fptr->filename);
    brc_addlist(fptr->filename);

    if(more(genbuf, YEA) == 0)
	outmsg("\033[31;47m  \033[31m(R)\033[30m回信  \033[31m(↓,n)"
	       "\033[30m下一封  \033[31m(←,q)\033[30m離開  \033[m");
    continue_flag = 0;

    switch(egetch()) {
    case KEY_LEFT:
    case 'e':
    case 'q':
    case 'Q':
	break;
	
    case 'y':
    case 'r':
    case 'Y':
    case 'R':
	if(currmode & MODE_POST) {
	    strcpy(quote_file, genbuf);
	    do_reply(fptr);
	    *quote_file = 0;
	}
	break;

    case ' ':
    case KEY_DOWN:
    case '\n':
    case 'n':
	continue_flag = 1;
    }

    clear();
    return 0;
}

static int sequential_read(int ent, fileheader_t *fhdr, char *direct) {
    char buf[40];

    clear();
    sequent_messages((fileheader_t *) NULL);
    sequent_ent = ent;
    continue_flag = 0;
    setbdir(buf, currboard);
    apply_record(buf, sequent_messages, sizeof(fileheader_t));
    return FULLUPDATE;
}

extern char *fn_notes;
extern char *msg_cancel;
extern char *fn_board;

/* ----------------------------------------------------- */
/* 看板備忘錄、文摘、精華區                              */
/* ----------------------------------------------------- */
int b_note_edit_bname(int bid) {
    char buf[64];
    int aborted;
    boardheader_t *fh=getbcache(bid);

    setbfile(buf, fh->brdname, fn_notes);
    aborted = vedit(buf, NA, NULL);
    if(aborted == -1) {
	clear();
	outs(msg_cancel);
	pressanykey();
    } else {
	aborted = (fh->bupdate - time(0)) / 86400 + 1;
	sprintf(buf,"%d", aborted > 0 ? aborted : 0);
	getdata_buf(3, 0, "請設定有效期限(0 - 9999)天？", buf, 5, DOECHO);
	aborted = atoi(buf);
	fh->bupdate = aborted ? time(0) + aborted * 86400 : 0;
	substitute_record(fn_board, fh, sizeof(boardheader_t), bid);
    }
    return 0;
}

static int b_notes_edit() {
    if(currmode & MODE_BOARD) {
        b_note_edit_bname(currbid);
	return FULLUPDATE;
    }
    return 0;
}

static int b_water_edit() {
    if(currmode & MODE_BOARD) {
	friend_edit(BOARD_WATER);
	return FULLUPDATE;
    }
    return 0;
}

static int visable_list_edit() {
    if(currmode & MODE_BOARD) {
	friend_edit(BOARD_VISABLE);
	return FULLUPDATE;
    }
    return 0;
}

static int b_visitor() {
    char buf[200];

    if(currmode & MODE_BOARD) {
	setbfile(buf, currboard, "brdvisitor");
	more(buf,YEA);
	return FULLUPDATE;
    }
    return 0;
}

static int b_post_note() {
    char buf[200], yn[3];
  
    if(currmode & MODE_BOARD) {
	setbfile(buf, currboard,  FN_POST_NOTE );
	if(more(buf,NA) == -1)  more("etc/"FN_POST_NOTE , NA);
	getdata(b_lines - 2, 0, "是否要用自訂post注意事項?", yn, 3, LCECHO);
	if(yn[0] == 'y')
	    vedit(buf, NA, NULL);
	else
	    unlink(buf);
	return FULLUPDATE;
    }
    return 0;
}

static int b_application() {
    char buf[200];

    if(currmode & MODE_BOARD) {
	setbfile(buf, currboard,  FN_APPLICATION);
	vedit(buf, NA, NULL);
	return FULLUPDATE;
    }
    return 0;
}

static int can_vote_edit() {
    if(currmode & MODE_BOARD) {
	friend_edit(FRIEND_CANVOTE);
	return FULLUPDATE;
    }
    return 0;
}

static int bh_title_edit() {
    boardheader_t *bp;

    if(currmode & MODE_BOARD) {
	char genbuf[BTLEN];

	bp = getbcache(currbid);
	move(1,0);
	clrtoeol();
	getdata_str(1,0,"請輸入看板新中文敘述:", genbuf,BTLEN -
		    16,DOECHO, bp->title + 7);

	if(!genbuf[0])
	    return 0;
	strip_ansi( genbuf,genbuf,0);
	strcpy(bp->title + 7,genbuf);
	substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
	log_usies("SetBoard", currboard);
	return FULLUPDATE;
    }
    return 0;
}

static int b_notes() {
    char buf[64];
    
    setbfile(buf, currboard, fn_notes);
    if(more(buf, NA) == -1) {
	clear();
	move(4, 20);
	outs("本看板尚無「備忘錄」。");
    }
    pressanykey();
    return FULLUPDATE;
}

int board_select() {
    char fpath[80];
    char genbuf[100];

    currmode &= ~MODE_SELECT;
    sprintf(fpath, "SR.%s", cuser.userid);
    setbfile(genbuf, currboard, fpath);
    unlink(genbuf);
    if(currstat == RMAIL)
	sethomedir(currdirect, cuser.userid);
    else
	setbdir(currdirect, currboard);
    return NEWDIRECT;
}

int board_digest() {
    if(currmode & MODE_SELECT)
	board_select();
    currmode ^= MODE_DIGEST;
    if(currmode & MODE_DIGEST)
	currmode &= ~MODE_POST;
    else if (haspostperm(currboard))
	currmode |= MODE_POST;
    
    setbdir(currdirect, currboard);
    return NEWDIRECT;
}

int board_etc() {
    if(!HAS_PERM(PERM_SYSOP))
	return DONOTHING;
    currmode ^= MODE_ETC;
    if(currmode & MODE_ETC)
	currmode &= ~MODE_POST;
    else if(haspostperm(currboard))
	currmode |= MODE_POST;
    
    setbdir(currdirect, currboard);
    return NEWDIRECT;
}

extern char *fn_mandex;

static int good_post(int ent, fileheader_t *fhdr, char *direct) {
    char genbuf[200];
    char genbuf2[200];

    if((currmode & MODE_DIGEST) || !(currmode & MODE_BOARD))
	return DONOTHING;

    if(fhdr->filemode & FILE_DIGEST) {
	fhdr->filemode = (fhdr->filemode & ~FILE_DIGEST);
	if(!strcmp(currboard,"Note") || !strcmp(currboard,"PttBug") ||
	   !strcmp(currboard,"Artdsn") || !strcmp(currboard, "PttLaw")) {
	    deumoney(fhdr->owner,1000);
	    fhdr->money -= 1000;
	}
    } else {
	fileheader_t digest;
	char *ptr, buf[64];

	memcpy(&digest, fhdr, sizeof(digest));
	digest.filename[0] = 'G';
	strcpy(buf, direct);
	ptr = strrchr(buf, '/') + 1;
	ptr[0] = '\0';
	sprintf(genbuf, "%s%s", buf, digest.filename);
	if(!dashf(genbuf)) {
	    digest.savemode = digest.filemode = 0;
	    sprintf(genbuf2, "%s%s", buf, fhdr->filename);
	    Link(genbuf2, genbuf);
	    strcpy(ptr, fn_mandex);
	    append_record(buf, &digest, sizeof(digest));
	}
	fhdr->filemode = (fhdr->filemode & ~FILE_MARKED) | FILE_DIGEST;
	if(!strcmp(currboard, "Note") || !strcmp(currboard, "PttBug") ||
	   !strcmp(currboard,"Artdsn") || !strcmp(currboard, "PttLaw")) {
	    inumoney(fhdr->owner, 1000);
	    fhdr->money += 1000;
	}
    }
    substitute_record(direct, fhdr, sizeof(*fhdr), ent);
    if(currmode & MODE_SELECT) {
	int now;
	char genbuf[100];

	setbdir(genbuf, currboard);
	now=getindex(genbuf, fhdr->filename, sizeof(fileheader_t));
	substitute_record(genbuf, fhdr, sizeof(*fhdr), now);
    }
    return PART_REDRAW;
}

/* help for board reading */
static char *board_help[] = {
    "\0全功\能看板操作說明",
    "\01基本命令",
    "(p)(↑)   上移一篇文章         (^P)     發表文章",
    "(n)(↓)   下移一篇文章         (d)      刪除文章",
    "(P)(PgUp) 上移一頁             (S)      串連相關文章",
    "(N)(PgDn) 下移一頁             (##)     跳到 ## 號文章",
    "(r)(→)   閱\讀此篇文章         ($)      跳到最後一篇文章",
    "\01進階命令",
    "(tab)/z   文摘模式/精華區      (a)(A)   找尋作者",
    "(b)       展讀備忘錄           (?)(/)   找尋標題",
    "(V,R)     投票/查詢投票結果    (=)      找尋首篇文章",
    "(x)       轉錄文章到其他看板   ([]<>-+) 主題式閱\讀",
#ifdef INTERNET_EMAIL
    "(F)       文章寄回Internet郵箱 (U)      將文章 uuencode 後寄回郵箱",
#endif
    "(Z)/(E)   Z-modem傳檔/重編文章 \033[31m(^H)     列出所有的 New Post(s)\033[m",
    "\01板主命令",
    "(m)       保留此篇文章          (W/w/v) 編輯備忘錄/水桶名單/可看見名單",
    "(M/o)     舉行投票/編私投票名單 (c/g)   選錄精華/文摘",
    "(D)       刪除一段範圍的文章    (T/B)   重編文章標題/重編看版標題",
    "(i)       編輯申請入會表格      (t/^D)  標記文章/砍除標記的文章",
    "(O)       編輯Post注意事項",
    NULL
};

static int b_help() {
    show_help(board_help);
    return FULLUPDATE;
}

/* ----------------------------------------------------- */
/* 看板功能表                                            */
/* ----------------------------------------------------- */
struct onekey_t read_comms[] = {
    {KEY_TAB, board_digest},
    {'C', board_etc},
    {'b', b_notes},
    {'c', cite_post},
    {'r', read_post},
    {'z', b_man},
    {'D', del_range},
    {'S', sequential_read},
    {'E', edit_post},
    {'T', edit_title},
    {'s', do_select},
    {'R', b_results},
    {'V', b_vote},
    {'M', b_vote_maintain},
    {'B', bh_title_edit},
    {'W', b_notes_edit},
    {'O', b_post_note},
    {'t', post_tag},
    {'w', b_water_edit},
    {'v', visable_list_edit},
    {'i', b_application},
    {'o', can_vote_edit},
    {'f', b_visitor},
    {Ctrl('D'), post_del_tag},
    {'x', cross_post},
    {'h', b_help},
    {'g', good_post},
    {'y', reply_post},
    {'d', del_post},
    {'m', mark_post},
    {'L', solve_post},
    {Ctrl('P'), do_post},
    {'Q', view_postmoney},
    {'\0', NULL}
};

time_t board_visit_time;

int Read() {
    int mode0 = currutmp->mode;
    int stat0 = currstat;
    char buf[40];
#ifdef LOG_BOARD
    time_t usetime = time(0);
#endif 
    extern struct bcache_t *brdshm;

    setutmpmode(READING);
    set_board();

    if(board_visit_time < board_note_time) {
	setbfile(buf, currboard, fn_notes);
	more(buf, NA);
	pressanykey();
    }
    currutmp->brc_id = currbid;
    setbdir(buf, currboard);
    curredit &= ~EDIT_MAIL;
    i_read(READING, buf, readtitle, readdoent, read_comms,
	   &(brdshm->total[currbid - 1]));
#ifdef LOG_BOARD
    log_board(currboard, time(0) - usetime);
#endif
    brc_update();

    currutmp->brc_id =0;
    currutmp->mode = mode0;
    currstat = stat0;
    return 0;
}

void ReadSelect() {
    int mode0 = currutmp->mode;
    int stat0 = currstat;
    char genbuf[200];

    currstat = XMODE;
    if(do_select(0, 0, genbuf) == NEWDIRECT)
	Read();
    currutmp->mode = mode0;
    currstat = stat0;
}

#ifdef LOG_BOARD 
static void log_board(char *mode, time_t usetime) {
    time_t now;
    char buf[ 256 ];

    if(usetime > 30) {
	now = time(0);
	sprintf(buf, "USE %-20.20s Stay: %5ld (%s) %s",
		mode, usetime ,cuser.userid ,ctime(&now));
	log_file(FN_USEBOARD,buf);
    }
}
#endif

int Select() {
    char genbuf[200];

    setutmpmode(SELECT);
    do_select(0, NULL, genbuf);
    return 0;
}
