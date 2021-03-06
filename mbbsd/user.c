/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include "config.h"
#include "pttstruct.h"
#include "common.h"
#include "perm.h"
#include "modes.h"
#include "proto.h"

extern int numboards;
extern boardheader_t *bcache;
extern char *loginview_file[NUMVIEWFILE][2];
extern int b_lines;             /* Screen bottom line number: t_lines-1 */
extern time_t login_start_time;
extern char *msg_uid;
extern int usernum;
extern char *msg_sure_ny;
extern userinfo_t *currutmp;
extern int showansi;
extern char reset_color[];
extern char *fn_proverb;
extern char *fn_plans;
extern char *msg_del_ok;
extern char *fn_register;
extern char *msg_nobody;
extern userec_t cuser;
extern userec_t xuser;

static char *sex[8] = {
    MSG_BIG_BOY, MSG_BIG_GIRL, MSG_LITTLE_BOY, MSG_LITTLE_GIRL,
    MSG_MAN, MSG_WOMAN, MSG_PLANT, MSG_MIME
};

int u_loginview() {
    int i;
    unsigned int pbits = cuser.loginview;
    char choice[5];
    
    clear();
    move(4,0);
    for(i = 0; i < NUMVIEWFILE ; i++)
	prints("    %c. %-20s %-15s \n", 'A' + i,
	       loginview_file[i][1],((pbits >> i) & 1 ? "??" : "??"));
    
    clrtobot();
    while(getdata(b_lines - 1, 0, "?Ы? [A-N] ?????]?w?A?? [Return] ?????G",
		  choice, 3, LCECHO)) {
	i = choice[0] - 'a';
	if(i >= NUMVIEWFILE || i < 0)
	    bell();
	else {
	    pbits ^= (1 << i);
	    move( i + 4 , 28 );
	    prints((pbits >> i) & 1 ? "??" : "??");
	}
    }
    
    if(pbits != cuser.loginview) {
	cuser.loginview = pbits ;
	passwd_update(usernum, &cuser);
    }
    return 0;
}

void user_display(userec_t *u, int real) {
    int diff = 0;
    char genbuf[200];
    
    clrtobot();
    prints("        \033[30;41m?s?r?s?r?s?r\033[m  \033[45m????????????????"
	   "????????????????\033[m  \033[30;41m?s?r?s?r?s?r\033[m\n"
	   "        \033[30;41m?r?s?r?s?r?s\033[m  \033[1;37;45m  ????  ????"
	   "  ????????  ????  ??\033[m  \033[30;41m?r?s?r?s?r?s\033[m\n"
	   "        \033[30;41m?s?r?s?r?s?r\033[m  \033[45m  ????  ????  ??"
	   "??????  ????  ??\033[m  \033[30;41m?s?r?s?r?s?r\033[m\n"
	   "        \033[30;41m?r?s?r?s?r?s\033[m  \033[1;30;45m????????????"
	   "  ??????    ????????\033[m  \033[30;41m?r?s?r?s?r?s\033[m\n");
    prints("                ?N???ʺ?: %s(%s)\n"
	   "                ?u???m?W: %s\n"
	   "                ?~?????}: %s\n"
	   "                ?q?l?H?c: %s\n"
	   "                ??    ?O: %s\n",
	   u->userid, u->username, u->realname, u->address, u->email,
	   sex[u->sex % 8]);
    
    sethomedir(genbuf, u->userid);
    prints("                ?p?H?H?c: %d ??  (?ʶR?H?c: %d ??)\n"
	   "                ??    ??: %02i/%02i/%02i\n",
	   get_num_records(genbuf, sizeof(fileheader_t)),
	   u->exmailbox, u->month, u->day, u->year % 100);
    prints("                ???U????: %s", ctime(&u->firstlogin));
    prints("                ?e?????{: %s", ctime(&u->lastlogin));
    prints("                ?W???峹: %d ?? / %d ?g\n",
	   u->numlogins, u->numposts);
    
    if(real) {
	strcpy(genbuf, "bTCPRp#@XWBA#VSM0123456789ABCDEF");
	for(diff = 0; diff < 32; diff++)
	    if(!(u->userlevel & (1 << diff)))
		genbuf[diff] = '-';
	prints("                ?{?Ҹ???: %s\n"
	       "                user?v??: %s\n",
	       u->justify, genbuf);
    } else {
	diff = (time(0) - login_start_time) / 60;
	prints("                ???d????: %d ?p?? %2d ??\n",
	       diff / 60, diff % 60);
    }
    
    /* Thor: ?Q?ݬݳo?? user ?O???Ǫ??????D */
    if(u->userlevel >= PERM_BM) {
	int i;
	boardheader_t *bhdr;
	
	outs("                ?????O?D: ");
	
	for(i = 0, bhdr = bcache; i < numboards; i++, bhdr++) {
	    if(is_uBM(bhdr->BM,u->userid)) {
		outs(bhdr->brdname);
		outc(' ');
	    }
	}
	outc('\n');
    }
    outs("        \033[30;41m?r?s?r?s?r?s?r?s?r?s?r?s?r?s?r?s?r?s?r?s?r?s?r"
	 "?s?r?s?r?s?r?s\033[m");
    
    outs((u->userlevel & PERM_LOGINOK) ?
	 "\n?z?????U?{?Ǥw?g?????A?w???[?J????" :
	 "\n?p?G?n???@?v???A?аѦҥ??????G?????z???U");
    
#ifdef NEWUSER_LIMIT
    if((u->lastlogin - u->firstlogin < 3 * 86400) && !HAS_PERM(PERM_POST))
	outs("\n?s???W???A?T?ѫ??}???v??");
#endif
}

void mail_violatelaw(char* crime, char* police, char* reason, char* result){
    char genbuf[200];
    fileheader_t fhdr;
    time_t now;
    FILE *fp;            
    sprintf(genbuf, "home/%c/%s", crime[0], crime);
    stampfile(genbuf, &fhdr);
    if(!(fp = fopen(genbuf,"w")))
        return;
    now = time(NULL);
    fprintf(fp, "?@??: [Ptt?k?|]\n"
	    "???D: [???i] ?H?k?P?M???i\n"
	    "?ɶ?: %s\n"
	    "\033[1;32m%s\033[m?P?M?G\n     \033[1;32m%s\033[m"
	    "?]\033[1;35m%s\033[m?欰?A\n?H?ϥ??????W?A?B?H\033[1;35m%s\033[m?A?S???q??"
	    "\n?Ш? PttLaw ?d?߬????k?W???T?A?è? Play-Pay-ViolateLaw ú???@??",
	    ctime(&now), police, crime, reason, result);
    fclose(fp);
    sprintf(fhdr.title, "[???i] ?H?k?P?M???i");
    strcpy(fhdr.owner, "[Ptt?k?|]");
    sprintf(genbuf, "home/%c/%s/.DIR", crime[0], crime);
    append_record(genbuf, &fhdr, sizeof(fhdr));
}

static void violate_law(userec_t *u, int unum){
    char ans[4], ans2[4];
    char reason[128];
    move(1,0);
    clrtobot();
    move(2,0);
    prints("(1)Cross-post (2)?õo?s?i?H (3)?õo?s???H\n");
    prints("(4)???Z???W?ϥΪ? (8)???L?H?@???B?m?欰\n(9)?? id ?欰\n");
    getdata(5, 0, "(0)????",
            ans, 3, DOECHO);
    switch(ans[0]){
    case '1':
	sprintf(reason, "%s", "Cross-post");
	break;
    case '2':
	sprintf(reason, "%s", "?õo?s?i?H");
	break;
    case '3':
	sprintf(reason, "%s", "?õo?s???H");
	break;
    case '4':
        while(!getdata(7, 0, "?п??J?Q???|?z?ѥH?ܭt?d?G", reason, 50, DOECHO));
        strcat(reason, "[???Z???W?ϥΪ?]");     
        break;
    case '8':   
    case '9':
        while(!getdata(6, 0, "?п??J?z?ѥH?ܭt?d?G", reason, 50, DOECHO));        
        break;
    default:
	return;
    }
    getdata(7, 0, msg_sure_ny, ans2, 3, LCECHO);
    if(*ans2 != 'y')
	return;      
    if (ans[0]=='9'){
	char src[STRLEN], dst[STRLEN];
	sprintf(src, "home/%c/%s", u->userid[0], u->userid);
	sprintf(dst, "tmp/%s", u->userid);
	Rename(src, dst);
	log_usies("KILL", u->userid);
	post_violatelaw(u->userid, cuser.userid, reason, "?尣 ID");       
	u->userid[0] = '\0';
	setuserid(unum, u->userid);
	passwd_update(unum, u);
    }
    else{
        u->userlevel |= PERM_VIOLATELAW;
        u->vl_count ++;
	passwd_update(unum, u);
        post_violatelaw(u->userid, cuser.userid, reason, "?@???B??");
        mail_violatelaw(u->userid, cuser.userid, reason, "?@???B??");
    }                 
    pressanykey();
}

extern char* str_permid[];

void uinfo_query(userec_t *u, int real, int unum) {
    userec_t x;
    register int i = 0, fail, mail_changed;
    char ans[4], buf[STRLEN];
    char genbuf[200];
    int flag = 0, temp = 0;
    time_t now;
    
    fail = mail_changed = 0;
    
    memcpy(&x, u, sizeof(userec_t));
    getdata(b_lines - 1, 0, real ?
	    "(1)??????(2)?]?K?X(3)?]?v??(4)???b??(5)??ID"
	    "(7)?f?P [0]???? " :
	    "?п??? (1)?ק????? (2)?]?w?K?X ==> [0]???? ",
	    ans, 3, DOECHO);
    
    if(ans[0] > '2' && !real)
	ans[0] = '0';
    
    if(ans[0] == '1' || ans[0] == '3') {
	clear();
	i = 2;
	move(i++, 0);
	outs(msg_uid);
	outs(x.userid);
    }
    
    switch(ans[0]) {
    case '7':
	violate_law(&x, unum);
	return;
    case '1':
	move(0, 0);
	outs("?гv???ק??C");
	
	getdata_buf(i++, 0," ?? ??  ?G",x.username, 24, DOECHO);
	if(real) {
	    getdata_buf(i++, 0, "?u???m?W?G", x.realname, 20, DOECHO);
	    getdata_buf(i++, 0, "?~???a?}?G", x.address, 50, DOECHO);
	}
	getdata_str(i++, 0, "?q?l?H?c[?ܰʭn???s?{??]?G", buf, 50, DOECHO,
		    x.email);
	if(strcmp(buf,x.email) && strchr(buf, '@')) {
	    strcpy(x.email,buf);
	    mail_changed = 1 - real;
	}
	
	sprintf(genbuf, "%i", (u->sex + 1) % 8);
	getdata_str(i++, 0, "?ʧO (1)???? (2)?j?? (3)???} (4)???? (5)???? "
		    "(6)???? (7)?Ӫ? (8)?q???G",
		    buf, 3, DOECHO,genbuf);
	if(buf[0] >= '1' && buf[0] <= '8')
	    x.sex = (buf[0] - '1') % 8;
	else
	    x.sex = u->sex % 8;
	
	while(1) {
	    int len;
	    
	    sprintf(genbuf, "%02i/%02i/%02i",
		    u->month, u->day, u->year % 100);
	    len = getdata_str(i, 0, "?ͤ? ????/????/?褸?G", buf, 9,
			      DOECHO,genbuf);
	    if(len && len != 8)
		continue;
	    if(!len) {
		x.month = u->month;
		x.day = u->day;
		x.year = u->year;
	    } else if(len == 8) {
		x.month = (buf[0] - '0') * 10 + (buf[1] - '0');
		x.day   = (buf[3] - '0') * 10 + (buf[4] - '0');
		x.year  = (buf[6] - '0') * 10 + (buf[7] - '0');
	    } else
		continue;
	    if(!real && (x.month > 12 || x.month < 1 || x.day > 31 ||
			 x.day < 1 || x.year > 90 || x.year < 40))
		continue;
	    i++;
	    break;
	}
	if(real) {
	    unsigned long int l;
	    sprintf(genbuf, "%d", x.exmailbox);
	    if(getdata_str(i++, 0,"?ʶR?H?c?ơG", buf, 4, DOECHO,genbuf))
		if((l = atol(buf)) >= 0)
		    x.exmailbox = (int)l;
	    
	    getdata_buf(i++, 0, "?{?Ҹ??ơG", x.justify, 44, DOECHO);
	    getdata_buf(i++, 0, "?̪????{?????G", x.lasthost, 16, DOECHO);
	    
	    sprintf(genbuf, "%d", x.numlogins);
	    if(getdata_str(i++, 0,"?W?u???ơG", buf, 10, DOECHO,genbuf))
		if((fail = atoi(buf)) >= 0)
		    x.numlogins = fail;
	    
	    sprintf(genbuf,"%d", u->numposts);
	    if(getdata_str(i++, 0, "?峹?ƥءG", buf, 10, DOECHO,genbuf))
		if((fail = atoi(buf)) >= 0)
		    x.numposts = fail;
	    sprintf(genbuf, "%d", u->vl_count);
	    if (getdata_str(i++, 0, "?H?k?O???G", buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.vl_count = fail;
	    
	    fail = 0;
	}
	break;
	
    case '2':
	i = 19;
	if(!real) {
	    if(!getdata(i++, 0, "?п??J???K?X?G", buf, PASSLEN, NOECHO) ||
	       !checkpasswd(u->passwd, buf)) {
		outs("\n\n?z???J???K?X?????T\n");
		fail++;
		break;
	    }
	}
	else{
	    char witness[3][32];
	    time_t now = time(NULL);
	    for(i=0;i<3;i++){
		if(!getdata(19+i, 0, "?п??J???U?ҩ????ϥΪ̡G", witness[i], 32, DOECHO)){
		    outs("\n?????J?h?L?k????\n");
		    fail++;
		    break;
		}
		else if (!getuser(witness[i])){
		    outs("\n?d?L???ϥΪ?\n");
		    fail++;
		    break;
		}
		else if (now - xuser.firstlogin < 6*30*24*60*60){
		    outs("\n???U???W?L?b?~?A?Э??s???J\n");
		    i--;
		}
	    }
	    if (i < 3)
		break;
	    else
		i = 20;
	}    
	
	if(!getdata(i++, 0, "?г]?w?s?K?X?G", buf, PASSLEN, NOECHO)) {
	    outs("\n\n?K?X?]?w????, ?~???ϥ??±K?X\n");
	    fail++;
	    break;
	}
	strncpy(genbuf, buf, PASSLEN);
	
	getdata(i++, 0, "???ˬd?s?K?X?G", buf, PASSLEN, NOECHO);
	if(strncmp(buf, genbuf, PASSLEN)) {
	    outs("\n\n?s?K?X?T?{????, ?L?k?]?w?s?K?X\n");
	    fail++;
	    break;
	}
	buf[8] = '\0';
	strncpy(x.passwd, genpasswd(buf), PASSLEN);
	if (real)
	    x.userlevel &= (!PERM_LOGINOK);
	break;
	
    case '3':
	i = setperms(x.userlevel, str_permid);
	if(i == x.userlevel)
	    fail++;
	else {
	    flag=1;
	    temp=x.userlevel;
	    x.userlevel = i;
	}
	break;
	
    case '4':
	i = QUIT;
	break;
	
    case '5':
	if (getdata_str(b_lines - 3, 0, "?s???ϥΪ̥N???G", genbuf, IDLEN + 1,
			DOECHO,x.userid)) {
	    if(searchuser(genbuf)) {
		outs("???~! ?w?g???P?? ID ???ϥΪ?");
		fail++;
	    } else
		strcpy(x.userid, genbuf);
	}
	break;
    default:
	return;
    }

    if(fail) {
	pressanykey();
	return;
    }
    
    getdata(b_lines - 1, 0, msg_sure_ny, ans, 3, LCECHO);
    if(*ans == 'y') {
	if(flag)
	    post_change_perm(temp,i,cuser.userid,x.userid);
	if(strcmp(u->userid, x.userid)) {
	    char src[STRLEN], dst[STRLEN];
	    
	    sethomepath(src, u->userid);
	    sethomepath(dst, x.userid);
	    Rename(src, dst);
	    setuserid(unum, x.userid);
	}
	memcpy(u, &x, sizeof(x));
	if(mail_changed) {	
#ifdef EMAIL_JUSTIFY
	    x.userlevel &= ~PERM_LOGINOK;
	    mail_justify();
#endif
	}
	
	if(i == QUIT) {
	    char src[STRLEN], dst[STRLEN];
	    
	    sprintf(src, "home/%c/%s", x.userid[0], x.userid);
	    sprintf(dst, "tmp/%s", x.userid);
	    if(Rename(src, dst)) {
		sprintf(genbuf, "/bin/rm -fr %s", src);
/* do not remove
   system(genbuf);
*/
	    }	
	    log_usies("KILL", x.userid);
	    x.userid[0] = '\0';
	    setuserid(unum, x.userid);
	} else
	    log_usies("SetUser", x.userid);
	passwd_update(unum, &x);
	now = time(0);
    }
}

int u_info() {
    move(2, 0);
    user_display(&cuser, 0);
    uinfo_query(&cuser, 0, usernum);
    strcpy(currutmp->realname, cuser.realname);
    strcpy(currutmp->username, cuser.username);
    return 0;
}

int u_ansi() {
    showansi ^= 1;
    cuser.uflag ^= COLOR_FLAG;
    outs(reset_color);
    return 0;
}

int u_cloak() {
    outs((currutmp->invisible ^= 1) ? MSG_CLOAKED : MSG_UNCLOAK);
    return XEASY;
}

int u_switchproverb() {
/*  char *state[4]={"?Υ\\??","?w?h??","?۩w??","SHUTUP"}; */
    char buf[100];

    cuser.proverb =(cuser.proverb +1) %4;
    setuserfile(buf,fn_proverb);
    if(cuser.proverb==2 && dashd(buf)) {
	FILE *fp = fopen(buf,"a");
	
	fprintf(fp,"?y?k?ʪ??A??[?۩w??]?n?O?o?]?y?k?ʪ????e??!!");
	fclose(fp);
    }
    passwd_update(usernum, &cuser);
    return 0;
}

int u_editproverb() {
    char buf[100];
  
    setutmpmode(PROVERB);
    setuserfile(buf,fn_proverb);
    move(1,0);
    clrtobot();
    outs("\n\n ?Ф@???@???̧????J?Q?t?δ????A?????e,\n"
	 " ?x?s???O?o?⪬?A?]?? [?۩w??] ?~???@??\n"
	 " ?y?k?ʳ̦h100??");
    pressanykey();
    vedit(buf,NA, NULL);
    return 0;
}

void showplans(char *uid) {
    char genbuf[200];
    
    sethomefile(genbuf, uid, fn_plans);
    if(!show_file(genbuf, 7, MAX_QUERYLINES, ONLY_COLOR))
	prints("?m?ӤH?W???n%s ?ثe?S???W??", uid);
}

int showsignature(char *fname) {
    FILE *fp;
    char buf[256];
    int i, j;
    char ch;

    clear();
    move(2, 0);
    setuserfile(fname, "sig.0");
    j = strlen(fname) - 1;

    for(ch = '1'; ch <= '9'; ch++) {
	fname[j] = ch;
	if((fp = fopen(fname, "r"))) {
	    prints("\033[36m?i ñ?W??.%c ?j\033[m\n", ch);
	    for(i = 0; i++ < MAX_SIGLINES && fgets(buf, 256, fp); outs(buf));
	    fclose(fp);
	}
    }
    return j;
}

int u_editsig() {
    int aborted;
    char ans[4];
    int j;
    char genbuf[200];
    
    j = showsignature(genbuf);
    
    getdata(0, 0, "ñ?W?? (E)?s?? (D)?R?? (Q)?????H[Q] ", ans, 4, LCECHO);
    
    aborted = 0;
    if(ans[0] == 'd')
	aborted = 1;
    if(ans[0] == 'e')
	aborted = 2;
    
    if(aborted) {
	if(!getdata(1, 0, "?п???ñ?W??(1-9)?H[1] ", ans, 4, DOECHO))
	    ans[0] = '1';
	if(ans[0] >= '1' && ans[0] <= '9') {
	    genbuf[j] = ans[0];
	    if(aborted == 1) {
		unlink(genbuf);
		outs(msg_del_ok);
	    } else {
		setutmpmode(EDITSIG);
		aborted = vedit(genbuf, NA, NULL);
		if(aborted != -1)
		    outs("ñ?W?ɧ??s????");
	    }
	}
	pressanykey();
    }
    return 0;
}

int u_editplan() {
    char genbuf[200];
    
    getdata(b_lines - 1, 0, "?W?? (D)?R?? (E)?s?? [Q]?????H[Q] ",
	    genbuf, 3, LCECHO);
    
    if(genbuf[0] == 'e') {
	int aborted;
	
	setutmpmode(EDITPLAN);
	setuserfile(genbuf, fn_plans);
	aborted = vedit(genbuf, NA, NULL);
	if(aborted != -1)
	    outs("?W?????s????");
	pressanykey();
	return 0;
    } else if(genbuf[0] == 'd') {
	setuserfile(genbuf, fn_plans);
	unlink(genbuf);
	outmsg("?W???R??????");
    }
    return 0;
}

/* ?ϥΪ̶??g???U???? */
static void getfield(int line, char *info, char *desc, char *buf, int len) {
    char prompt[STRLEN];
    char genbuf[200];

    sprintf(genbuf, "?????]?w?G%-30.30s (%s)", buf, info);
    move(line, 2);
    outs(genbuf);
    sprintf(prompt, "%s?G", desc);
    if(getdata_str(line + 1, 2, prompt, genbuf, len, DOECHO, buf))
	strcpy(buf, genbuf);
    move(line, 2);
    prints("%s?G%s", desc, buf);
    clrtoeol();
}

static int removespace(char* s){
    int i, index;

    for(i=0, index=0;s[i];i++){
	if (s[i] != ' ')
	    s[index++] = s[i];
    }
    s[index] = '\0';
    return index;
}

int u_register() {
    char rname[20], addr[50];
    char phone[20], career[40], email[50],birthday[9],sex_is[2],year,mon,day;
    char ans[3], *ptr;
    FILE *fn;
    time_t now;
    char genbuf[200];
    
    if(cuser.userlevel & PERM_LOGINOK) {
	outs("?z???????T?{?w?g?????A???ݶ??g?ӽЪ?");
	return XEASY;
    }
    if((fn = fopen(fn_register, "r"))) {
	while(fgets(genbuf, STRLEN, fn)) {
	    if((ptr = strchr(genbuf, '\n')))
		*ptr = '\0';
	    if(strncmp(genbuf, "uid: ", 5) == 0 &&
	       strcmp(genbuf + 5, cuser.userid) == 0) {
		fclose(fn);
		outs("?z?????U?ӽг??|?b?B?z???A?Э@?ߵ???");
		return XEASY;
	    }
	}
	fclose(fn);
    }
    
    getdata(b_lines - 1, 0, "?z?T?w?n???g???U????(Y/N)?H[N] ", ans, 3, LCECHO);
    if(ans[0] != 'y')
	return FULLUPDATE;
    
    move(2, 0);
    clrtobot();
    strcpy(rname, cuser.realname);
    strcpy(addr, cuser.address);
    strcpy(email, cuser.email);
    sprintf(birthday, "%02i/%02i/%02i",
	    cuser.month, cuser.day, cuser.year % 100);
    sex_is[0]=(cuser.sex % 8)+'1';sex_is[1]=0;
    career[0] = phone[0] = '\0';
    while(1) {
	clear();
	move(3, 0);
	prints("%s(%s) ?z?n?A?оڹ????g?H?U??????:",
	       cuser.userid, cuser.username);
        do{
	    getfield(6, "?ХΤ???", "?u???m?W", rname, 20);
        }while(!removespace(rname) || isalpha(rname[0]));
        do{ 
	    getfield(8, "?Ǯըt?ũγ???¾??", "?A?ȳ???", career, 40);
        }while(!removespace(career));
        do{
	    getfield(10, "?]?A???ǩΪ??P???X", "?ثe???}", addr, 50);
        }while(!(addr[0]));
        do{
	    getfield(12, "?]?A???~?????ϰ??X", "?s???q??", phone, 20);
        }while(!removespace(phone));
	while(1) {
	    int len;
	    
	    getfield(14, "????/????/?褸 ?p:09/27/76","?ͤ?",birthday,9);
	    len = strlen(birthday);
	    if(!len) {
		sprintf(birthday, "%02i/%02i/%02i",
			cuser.month, cuser.day, cuser.year % 100);
		mon = cuser.month;
		day = cuser.day;
		year = cuser.year;
	    } else if(len == 8) {
		mon = (birthday[0] - '0') * 10 + (birthday[1] - '0');
		day = (birthday[3] - '0') * 10 + (birthday[4] - '0');
		year = (birthday[6] - '0') * 10 + (birthday[7] - '0');
	    } else
		continue;
	    if(mon > 12 || mon < 1 || day > 31 || day < 1 || year > 90 ||
	       year < 40)
		continue;
	    break;
	}
	getfield(16, "1.???? 2.?j?? ", "?ʧO", sex_is, 2);
	getfield(18, "?????{?ҥ?", "E-Mail Address", email, 50);
	
	getdata(b_lines - 1, 0, "?H?W???ƬO?_???T(Y/N)?H(Q)???????U [N] ",
		ans, 3, LCECHO);
	if(ans[0] == 'q')
	    return 0;
	if(ans[0] == 'y')
	    break;
    }
    strcpy(cuser.realname, rname);
    strcpy(cuser.address, addr);
    strcpy(cuser.email, email);
    cuser.sex= (sex_is[0] - '1') % 8;
    cuser.month = mon;
    cuser.day = day;
    cuser.year = year;
    if((fn = fopen(fn_register, "a"))) {
	now = time(NULL);
	trim(career);
	trim(addr);
	trim(phone);
	fprintf(fn, "num: %d, %s", usernum, ctime(&now));
	fprintf(fn, "uid: %s\n", cuser.userid);
	fprintf(fn, "name: %s\n", rname);
	fprintf(fn, "career: %s\n", career);
	fprintf(fn, "addr: %s\n", addr);
	fprintf(fn, "phone: %s\n", phone);
	fprintf(fn, "email: %s\n", email);
	fprintf(fn, "----\n");
	fclose(fn);
    }
    clear();
    move(9,3);
    prints("?̫?Post?@?g\033[32m?ۧڤ??Ф峹\033[m???j?a?a?A"
	   "?i?D?Ҧ??Ѱ??Y\033[31m?ڨӰ?^$?C\\n\n\n\n");
    pressanykey();
    cuser.userlevel |= PERM_POST;
    brc_initial("WhoAmI");
    set_board();
    do_post();
    cuser.userlevel &= ~PERM_POST;
    return 0;
}

/* ?C?X?Ҧ????U?ϥΪ? */
extern struct uhash_t *uhash;
static int usercounter, totalusers, showrealname;
static ushort u_list_special;

static int u_list_CB(userec_t *uentp) {
    static int i;
    char permstr[8], *ptr;
    register int level;
    
    if(uentp == NULL) {
	move(2, 0);
	clrtoeol();
	prints("\033[7m  ?ϥΪ̥N??   %-25s   ?W??  ?峹  %s  "
	       "?̪????{????     \033[0m\n",
	       showrealname ? "?u???m?W" : "?︹?ʺ?",
	       HAS_PERM(PERM_SEEULEVELS) ? "????" : "");
	i = 3;
	return 0;
    }
    
    if(bad_user_id(uentp->userid))
	return 0;
    
    if((uentp->userlevel & ~(u_list_special)) == 0)
	return 0;
    
    if(i == b_lines) {
	prints("\033[34;46m  ?w???? %d/%d ?H(%d%%)  \033[31;47m  "
	       "(Space)\033[30m ?ݤU?@??  \033[31m(Q)\033[30m ???}  \033[m",
	       usercounter, totalusers, usercounter * 100 / totalusers);
	i = igetch();
	if(i == 'q' || i == 'Q')
	    return QUIT;
	i = 3;
    }
    if(i == 3) {
	move(3, 0);
	clrtobot();
    }

    level = uentp->userlevel;
    strcpy(permstr, "----");
    if(level & PERM_SYSOP)
	permstr[0] = 'S';
    else if(level & PERM_ACCOUNTS)
	permstr[0] = 'A';
    else if(level & PERM_DENYPOST)
	permstr[0] = 'p';

    if(level & (PERM_BOARD))
	permstr[1] = 'B';
    else if(level & (PERM_BM))
	permstr[1] = 'b';

    if(level & (PERM_XEMPT))
	permstr[2] = 'X';
    else if(level & (PERM_LOGINOK))
	permstr[2] = 'R';

    if(level & (PERM_CLOAK | PERM_SEECLOAK))
	permstr[3] = 'C';
    
    ptr = (char *)Cdate(&uentp->lastlogin);
    ptr[18] = '\0';
    prints("%-14s %-27.27s%5d %5d  %s  %s\n",
	   uentp->userid,
	   showrealname ? uentp->realname : uentp->username,
	   uentp->numlogins, uentp->numposts,
	   HAS_PERM(PERM_SEEULEVELS) ? permstr : "", ptr);
    usercounter++;
    i++;
    return 0;
}

int u_list() {
    char genbuf[3];
    
    setutmpmode(LAUSERS);
    showrealname = u_list_special = usercounter = 0;
    totalusers = uhash->number;
    if(HAS_PERM(PERM_SEEULEVELS)) {
	getdata(b_lines - 1, 0, "?[?? [1]?S?????? (2)?????H",
		genbuf, 3, DOECHO);
	if(genbuf[0] != '2')
	    u_list_special = PERM_BASIC | PERM_CHAT | PERM_PAGE | PERM_POST | PERM_LOGINOK | PERM_BM;
    }
    if(HAS_PERM(PERM_CHATROOM) || HAS_PERM(PERM_SYSOP)) {
	getdata(b_lines - 1, 0, "???? [1]?u???m?W (2)?ʺ١H",
		genbuf, 3, DOECHO);
	if(genbuf[0] != '2')
	    showrealname = 1;
    }
    u_list_CB(NULL);
    if(passwd_apply(u_list_CB) == -1) {
	outs(msg_nobody);
	return XEASY;
    }
    move(b_lines, 0);
    clrtoeol();
    prints("\033[34;46m  ?w???? %d/%d ???ϥΪ?(?t?ήe?q?L?W??)  "
	   "\033[31;47m  (?Ы????N???~??)  \033[m", usercounter, totalusers);
    egetch();
    return 0;
}
