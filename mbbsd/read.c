/* $Id$ */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"
#include "pttstruct.h"
#include "modes.h"
#include "common.h"
#include "perm.h"
#include "proto.h"

#define MAXPATHLEN 256

extern int p_lines;             /* a Page of Screen line numbers: tlines-4 */
extern int b_lines;             /* Screen bottom line number: t_lines-1 */
extern char currowner[IDLEN + 2];
extern char currtitle[44];
extern char currauthor[IDLEN + 2];
extern char *str_reply;
extern char *msg_fwd_ok;
extern char *msg_fwd_err1;
extern char *msg_fwd_err2;
extern int currmode;
extern unsigned int currstat;
extern char currboard[];        /* name of currently selected board */
extern int KEY_ESC_arg;
extern int curredit;
extern char *msg_mailer;

char currdirect[64];
static fileheader_t *headers = NULL;
static int last_line;
static int hit_thread;

/* ----------------------------------------------------- */
/* cursor & reading record position control              */
/* ----------------------------------------------------- */
keeploc_t *getkeep(char *s, int def_topline, int def_cursline) {
    static struct keeploc_t *keeplist = NULL;
    struct keeploc_t *p;
    void *malloc();

    if(def_cursline >= 0)
	for(p = keeplist; p; p = p->next) {
	    if(!strcmp(s, p->key)) {
		if(p->crs_ln < 1)
		    p->crs_ln = 1;
		return p;
	    }
	}
    else
	def_cursline = -def_cursline;
    p = (keeploc_t *)malloc(sizeof(keeploc_t));
    p->key = (char *)malloc(strlen(s) + 1);
    strcpy(p->key, s);
    p->top_ln = def_topline;
    p->crs_ln = def_cursline;
    p->next = keeplist;
    return (keeplist = p);
}

void fixkeep(char *s, int first) {
    keeploc_t *k;
    
    k = getkeep(s, 1, 1);
    if(k->crs_ln >= first) {
	k->crs_ln = (first == 1 ? 1 : first - 1);
	k->top_ln = (first < 11 ? 1 : first - 10);
    }
}

/* calc cursor pos and show cursor correctly */
static int cursor_pos(keeploc_t *locmem, int val, int from_top) {
    int top;
    
    if(val > last_line) {
	bell();
	val = last_line;
    }
    if(val <= 0) {
	bell();
	val = 1;
    }

    top = locmem->top_ln;
    if(val >= top && val < top + p_lines) {
	cursor_clear(3 + locmem->crs_ln - top, 0);
	locmem->crs_ln = val;
	cursor_show(3 + val - top, 0);
	return DONOTHING;
    }
    locmem->top_ln = val - from_top;
    if(locmem->top_ln <= 0)
	locmem->top_ln = 1;
    locmem->crs_ln = val;
    return PARTUPDATE;
}

static int move_cursor_line(keeploc_t *locmem, int mode) {
    int top, crs;
    int reload = 0;

    top = locmem->top_ln;
    crs = locmem->crs_ln;
    if(mode == READ_PREV) {
	if(crs <= top) {
	    top -= p_lines - 1;
	    if(top < 1)
		top = 1;
	    reload = 1;
	}
	if(--crs < 1) {
	    crs = 1;
	    reload = -1;
	}
    } else if(mode == READ_NEXT) {
	if(crs >= top + p_lines - 1) {
	    top += p_lines - 1;
	    reload = 1;
	}
	if(++crs > last_line) {
	    crs = last_line;
	    reload = -1;
	}
    }
    locmem->top_ln = top;
    locmem->crs_ln = crs;
    return reload;
}

static int thread(keeploc_t *locmem, int stype) {
    static char a_ans[32], t_ans[32];
    char ans[32], s_pmt[64];
    register char *tag, *query = NULL;
    register int now, pos, match, near = 0;
    fileheader_t fh;
    int circulate_flag = 1;  /* circulate at end or begin */

    match = hit_thread = 0;
    now = pos = locmem->crs_ln;
    if(stype == 'A') {
	if(!*currowner)
	    return DONOTHING;
	str_lower(a_ans, currowner);
	query = a_ans;
	circulate_flag = 0;
	stype = 0;
    } else if(stype == 'a') {
	if(!*currowner)
	    return DONOTHING;
	str_lower(a_ans, currowner);
	query = a_ans;
	circulate_flag = 0;
	stype = RS_FORWARD;
    } else if(stype == '/') {
	if(!*t_ans)
	    return DONOTHING;
	query = t_ans;
	circulate_flag = 0;
	stype = RS_TITLE | RS_FORWARD;
    } else if(stype == '?') {
	if(!*t_ans)
	    return DONOTHING;
	circulate_flag = 0;
	query = t_ans;
	stype = RS_TITLE;
    } else if(stype & RS_RELATED) {
	tag = headers[pos - locmem->top_ln].title;
	if(stype & RS_CURRENT) {
	    if(stype & RS_FIRST) {
		if(!strncmp(currtitle, tag, 40))
		    return DONOTHING;
		near = 0;
	    }
	    query = currtitle;
	} else {
	    query = subject(tag);
	    if(stype & RS_FIRST) {
		if(query == tag)
		    return DONOTHING;
		near = 0;
	    }
	}
    } else if(!(stype & RS_THREAD)) {
	query = (stype & RS_TITLE) ? t_ans : a_ans;
	if(!*query && query == a_ans) {
	    if(*currowner)
		strcpy(a_ans, currowner);
	    else if (*currauthor)
		strcpy(a_ans, currauthor);
	}
	sprintf(s_pmt, "%s搜尋%s [%s] ",(stype & RS_FORWARD) ? "往後":"往前",
		(stype & RS_TITLE) ? "標題" : "作者", query);
	getdata(b_lines - 1, 0, s_pmt, ans, 30, DOECHO);
	if(*ans)
	    strcpy(query, ans);
	else if(*query == '\0')
	    return DONOTHING;
	str_lower(s_pmt, query);
	query = s_pmt;
    }

    tag = fh.owner;

    do {
	if(!circulate_flag || stype & RS_RELATED) {
	    if(stype & RS_FORWARD) {
		if(++now > last_line)
		    return DONOTHING;
	    } else {
		if(--now <= 0) {
		    if((stype & RS_FIRST) && (near)) {
			hit_thread = 1;
			return cursor_pos(locmem, near, 10);
		    }
		    return DONOTHING;
		}
	    }
	} else {
	    if(stype & RS_FORWARD) {
		if(++now > last_line)
		    now = 1;
	    } else if(--now <= 0)
		now = last_line;
	}
	
	get_record(currdirect, &fh, sizeof(fileheader_t), now);
	
	if(fh.owner[0] == '-')
	    continue;
	
	if(stype & RS_THREAD) {
	    if(strncasecmp(fh.title, str_reply, 3)) {
		hit_thread = 1;
		return cursor_pos(locmem, now, 10);
	    }
	    continue;
	}
	
	if(stype & RS_TITLE)
	    tag = subject(fh.title);
	
	if(((stype & RS_RELATED) && !strncmp(tag, query, 40)) ||
	   (!(stype & RS_RELATED) && ((query == currowner) ?
				      !strcmp(tag, query) :
				      strstr_lower(tag, query)))) {
	    if((stype & RS_FIRST) && tag != fh.title) {
		near = now;
		continue;
	    }
	    
	    hit_thread = 1;
	    match = cursor_pos(locmem, now, 10);
	    if((!(stype & RS_CURRENT)) &&
	       (stype & RS_RELATED) &&
	       strncmp(currtitle, query, 40)) {
		strncpy(currtitle, query, 40);
		match = PARTUPDATE;
	    }
	    break;
	}
    } while(now != pos);
    
    return match;
}

/* ------------------ */
/* 檔案傳輸工具副程式 */
/* ------------------ */
void z_download(char *fpath) {
    char cmd[100] = "/usr/bin/sz -a ";
    char pname[50];
    char fname[13];
    int i;
    
    getdata(b_lines - 1, 0, "使用 Z-Modem 下傳檔名:", fname, 9, LCECHO);
    for(i = 0; i < 8; i++)
	if(!(isalnum(fname[i]) || fname[i] == '-' || fname[i] == '_')) {
	    if(i)
		break;
	    else
		return;
	}
    fname[i] = 0;
    strcat(fname, ".bbs");
    setuserfile(pname, fname);
    unlink(pname);
    Link(fpath, pname);
    strcat(cmd, pname);
    system(cmd);
    unlink(pname);
}

#ifdef INTERNET_EMAIL
static void mail_forward(fileheader_t *fhdr, char *direct, int mode) {
    int i;
    char buf[STRLEN];
    char *p;
    
    strncpy(buf, direct, sizeof(buf));
    if((p = strrchr(buf, '/')))
	*p = '\0';
    switch(i = doforward(buf, fhdr, mode)) {
    case 0:
	outmsg(msg_fwd_ok);
	break;
    case -1:
	outmsg(msg_fwd_err1);
	break;
    case -2:
	outmsg(msg_fwd_err2);
	break;
    default:
	break;
    }
    refresh();
    sleep(1);
}
#endif

extern userec_t cuser;

static int select_read(keeploc_t *locmem, int sr_mode) {
    register char *tag,*query,*temp;
    fileheader_t fh;
    char fpath[80], genbuf[MAXPATHLEN], buf3[5];
    char static t_ans[TTLEN+1]="";
    char static a_ans[IDLEN+1]="";
    int fd, fr, size = sizeof(fileheader_t);
    struct stat st;

    if((currmode & MODE_SELECT))
	return -1;
    if(sr_mode == RS_TITLE)
	query = subject(headers[locmem->crs_ln - locmem->top_ln].title);
    else if(sr_mode == RS_NEWPOST)
    {
	strcpy(buf3, "Re: ");
	query = buf3;
    }
    else
    {
	char buff[80];

	query = (sr_mode == RS_RELATED) ? t_ans : a_ans;
	sprintf(buff, "搜尋%s [%s] ",
		(sr_mode == RS_RELATED) ? "標題" : "作者", query);
	if(getdata(b_lines, 0,buff,fpath, 30, DOECHO)) {
	    char buf[64];

	    str_lower(buf,fpath);
	    strcpy(query,buf);
	}
	if(!(*query))
	    return DONOTHING;
    }

    outmsg("搜尋中,請稍後...");refresh();
    if((fd = open(currdirect, O_RDONLY, 0)) != -1) {
	sprintf(genbuf,"SR.%s",cuser.userid);
	if(currstat==RMAIL)
	    sethomefile(fpath,cuser.userid,genbuf);
	else
	    setbfile(fpath,currboard,genbuf);
	if(((fr = open(fpath,O_WRONLY | O_CREAT | O_TRUNC,0600)) != -1)) {
	    switch(sr_mode) {
	    case RS_TITLE:
		while(read(fd,&fh,size) == size) {
		    tag = subject(fh.title);
		    if(!strncmp(tag, query, 40))
			write(fr,&fh,size);
		}
		break;
	    case RS_RELATED:
		while(read(fd,&fh,size) == size) {
		    tag = fh.title;
		    if(strstr_lower(tag,query))
			write(fr,&fh,size);
		}
		break;
	    case RS_NEWPOST:
		while(read(fd, &fh, size) == size) {
		    tag = fh.title;
		    temp = strstr(tag, query);
		    if(temp == NULL || temp != tag)
			write(fr, &fh, size);
		}
	    case RS_AUTHOR:
		while(read(fd,&fh,size) == size) {
		    tag = fh.owner;
		    if(strstr_lower(tag,query))
			write(fr,&fh,size);
		}
		break;
	    }
	    fstat(fr,&st);
	    close(fr);
	}
	close(fd);
	if(st.st_size) {
	    currmode |= MODE_SELECT;
	    strcpy(currdirect,fpath);
	}
    }
    return st.st_size;
}

extern userec_t xuser;

static int i_read_key(onekey_t *rcmdlist, keeploc_t *locmem, int ch) {
    int i, mode = DONOTHING;
    
    switch(ch) {
    case 'q':
    case 'e':
    case KEY_LEFT:
	return (currmode & MODE_SELECT) ? board_select() :
	    (currmode & MODE_ETC) ? board_etc() :
		(currmode & MODE_DIGEST) ? board_digest() : DOQUIT;
    case Ctrl('L'):
	redoscr();
	break;
    case KEY_ESC:
	if(KEY_ESC_arg == 'i') {
	    t_idle();
	    return FULLUPDATE;
	}
	break;
    case Ctrl('H'):
	if(select_read(locmem, RS_NEWPOST))
	    return NEWDIRECT;
	else
	    return READ_REDRAW;
    case 'a':
    case 'A':
	if(select_read(locmem,RS_AUTHOR))
	    return NEWDIRECT;
	else
	    return READ_REDRAW;
    case '/':
    case '?':
	if(select_read(locmem,RS_RELATED))
	    return NEWDIRECT;
	else
	    return READ_REDRAW;
    case 'S':
	if(select_read(locmem,RS_TITLE))
	    return NEWDIRECT;
	else
	    return READ_REDRAW;
	/* quick search title first */
    case '=':
	return thread(locmem, RELATE_FIRST);
    case '\\':
	return thread(locmem, CURSOR_FIRST);
	/* quick search title forword */
    case ']':
	return thread(locmem, RELATE_NEXT);
    case '+':
	return thread(locmem, CURSOR_NEXT);
	/* quick search title backword */
    case '[':
	return thread(locmem, RELATE_PREV);
    case '-':
	return thread(locmem, CURSOR_PREV);
    case '<':
    case ',':
	return thread(locmem, THREAD_PREV);
    case '.':
    case '>':
	return thread(locmem, THREAD_NEXT);
    case 'p':
    case 'k':
    case KEY_UP:
	return cursor_pos(locmem, locmem->crs_ln - 1, p_lines - 2);
    case 'n':
    case 'j':
    case KEY_DOWN:
	return cursor_pos(locmem, locmem->crs_ln + 1, 1);
    case ' ':
    case KEY_PGDN:
    case 'N':
    case Ctrl('F'):
	if(last_line >= locmem->top_ln + p_lines) {
	    if(last_line > locmem->top_ln + p_lines)
		locmem->top_ln += p_lines;
	    else
		locmem->top_ln += p_lines - 1;
	    locmem->crs_ln = locmem->top_ln;
	    return PARTUPDATE;
	}
	cursor_clear(3 + locmem->crs_ln - locmem->top_ln, 0);
	locmem->crs_ln = last_line;
	cursor_show(3 + locmem->crs_ln - locmem->top_ln, 0);
	break;
    case KEY_PGUP:
    case Ctrl('B'):
    case 'P':
	if(locmem->top_ln > 1) {
	    locmem->top_ln -= p_lines;
	    if(locmem->top_ln <= 0)
		locmem->top_ln = 1;
	    locmem->crs_ln = locmem->top_ln;
	    return PARTUPDATE;
	}
	break;
    case KEY_END:
    case '$':
	if(last_line >= locmem->top_ln + p_lines) {
	    locmem->top_ln = last_line - p_lines + 1;
	    if(locmem->top_ln <= 0)
		locmem->top_ln = 1;
	    locmem->crs_ln = last_line;
	    return PARTUPDATE;
	}
	cursor_clear(3 + locmem->crs_ln - locmem->top_ln, 0);
	locmem->crs_ln = last_line;
	cursor_show(3 + locmem->crs_ln - locmem->top_ln, 0);
	break;
    case 'F':
    case 'U':
	if(HAS_PERM(PERM_FORWARD)) {
	    mail_forward(&headers[locmem->crs_ln - locmem->top_ln],
			 currdirect, ch /*== 'U'*/);
	    /*by CharlieL*/
	    return FULLUPDATE;
	}
	break;
    case Ctrl('Q'):
	return my_query(headers[locmem->crs_ln - locmem->top_ln].owner);
    case Ctrl('S'):
	if(HAS_PERM(PERM_ACCOUNTS)) {
	    int id;
	    userec_t muser;

	    strcpy(currauthor, headers[locmem->crs_ln - locmem->top_ln].owner);
	    stand_title("使用者設定");
	    move(1, 0);
	    if((id = getuser(headers[locmem->crs_ln - locmem->top_ln].owner))){
		memcpy(&muser, &xuser, sizeof(muser));
		user_display(&muser, 1);
		uinfo_query(&muser, 1, id);
	    }
	    return FULLUPDATE;
	}
	break;
    case 'Z':
	if(HAS_PERM(PERM_FORWARD)) {
	    char fname[80];

	    setdirpath(fname, currdirect,
		       (char *)&headers[locmem->crs_ln - locmem->top_ln]);
	    z_download(fname);
	    return FULLUPDATE;
	}
	break;
    case '\n':
    case '\r':
    case 'l':
    case KEY_RIGHT:
	ch = 'r';
    default:
	for(i = 0; rcmdlist[i].fptr; i++) {
	    if(rcmdlist[i].key == ch) {
		mode = (*(rcmdlist[i].fptr))(locmem->crs_ln,
					     &headers[locmem->crs_ln -
						     locmem->top_ln], currdirect);
		break;
	    }
	    if(rcmdlist[i].key == 'h')
		if(currmode & (MODE_ETC | MODE_DIGEST))
		    return DONOTHING;
	}
    }
    return mode;
}

void i_read(int cmdmode, char *direct, void (*dotitle)(), void (*doentry)(), onekey_t *rcmdlist, int *num_record) {
    keeploc_t *locmem = NULL;
    int recbase = 0, mode, ch;
    int num, entries = 0;
    int i;
    int jump = 0;
    char genbuf[4];
    char currdirect0[64];
    int last_line0 = last_line;
    int hit_thread0 = hit_thread;
    fileheader_t *headers0 = headers;

    strcpy(currdirect0 ,currdirect);
#define FHSZ    sizeof(fileheader_t)
    headers = (fileheader_t *)calloc(p_lines, FHSZ);
    strcpy(currdirect, direct);
    mode = NEWDIRECT;

    do {
	/* 依據 mode 顯示 fileheader */
	setutmpmode(cmdmode);
	switch(mode) {
	case NEWDIRECT:             /* 第一次載入此目錄 */
	case DIRCHANGED:
	    last_line = get_num_records(currdirect, FHSZ);
	    if(num_record != NULL) {
		*num_record = last_line;
		num_record = NULL;
	    }
	    if(mode == NEWDIRECT) {
		if(last_line == 0) {
		    if(curredit & EDIT_ITEM) {
			outs("沒有物品");
			refresh();
			goto return_i_read;
		    } else if(curredit & EDIT_MAIL) {
			outs("沒有來信");
			refresh();
			goto return_i_read;
		    } else if(currmode & MODE_ETC) {
			board_etc(); /* Kaede */
			outmsg("尚未收錄其它文章");
			refresh();
		    } else if(currmode & MODE_DIGEST) {
			board_digest(); /* Kaede */
			outmsg("尚未收錄文摘");
			refresh();
		    } else if(currmode & MODE_SELECT) {
			board_select(); /* Leeym */
			outmsg("沒有此系列的文章");
			refresh();
		    } else {
			getdata(b_lines - 1, 0,
				"看板新成立 (P)發表文章 (Q)離開？[Q] ",
				genbuf, 4, LCECHO);
			if(genbuf[0] == 'p')
			    do_post();
			goto return_i_read;
		    }
		}
		num = last_line - p_lines + 1;
		locmem = getkeep(currdirect, num < 1 ? 1 : num, last_line);
	    }
	    recbase = -1;
	    
	case FULLUPDATE:
	    (*dotitle)();
	    
	case PARTUPDATE:
	    if(last_line < locmem->top_ln + p_lines) {
		num = get_num_records(currdirect, FHSZ);
		
		if(last_line != num) {
		    last_line = num;
		    recbase = -1;
		}
	    }
	    
	    if(last_line == 0)
		goto return_i_read;
	    else if(recbase != locmem->top_ln) {
		recbase = locmem->top_ln;
		if(recbase > last_line)	{
		    recbase = (last_line - p_lines) >> 1;
		    if(recbase < 1)
			recbase = 1;
		    locmem->top_ln = recbase;
		}
		entries = get_records(currdirect, headers, FHSZ, recbase,
				      p_lines);
	    }
	    if(locmem->crs_ln > last_line)
		locmem->crs_ln = last_line;
	    move(3, 0);
	    clrtobot();
	case PART_REDRAW:
	    move(3, 0);
	    for (i = 0; i < entries; i++)
		(*doentry) (locmem->top_ln + i, &headers[i]);
	case READ_REDRAW:
	    outmsg(curredit & EDIT_ITEM ?
		   "\033[44m 私人收藏 \033[30;47m 繼續? \033[m" : 
		   curredit & EDIT_MAIL ? msg_mailer : MSG_POSTER);
	    break;
	case READ_PREV:
	case READ_NEXT:
	case RELATE_PREV:
	case RELATE_NEXT:
	case RELATE_FIRST:
	case POS_NEXT:
	case 'A':
	case 'a':
	case '/':
	case '?':
	    jump = 1;
	    break;
	}

	/* 讀取鍵盤，加以處理，設定 mode */
	if(!jump) {
	    cursor_show(3 + locmem->crs_ln - locmem->top_ln, 0);
	    ch = egetch();
	    mode = DONOTHING;
	} else
	    ch = ' ';
	if(mode == POS_NEXT) {
	    mode = cursor_pos(locmem, locmem->crs_ln + 1, 1);
	    if(mode == DONOTHING)
		mode = PART_REDRAW;
	    jump = 0;
	} else if(ch >= '0' && ch <= '9') {
	    if((i = search_num(ch, last_line)) != -1)
		mode = cursor_pos(locmem, i + 1, 10);
	} else {
	    if(!jump)
		mode = i_read_key(rcmdlist, locmem, ch);
	    while(mode == READ_NEXT || mode == READ_PREV ||
		  mode == RELATE_FIRST || mode == RELATE_NEXT ||
		  mode == RELATE_PREV || mode == THREAD_NEXT ||
		  mode == THREAD_PREV || mode == 'A' || mode == 'a' ||
		  mode == '/' || mode == '?') {
		int reload;
		
		if(mode == READ_NEXT || mode == READ_PREV)
		    reload = move_cursor_line(locmem, mode);
		else {
		    reload = thread(locmem, mode);
		    if(!hit_thread) {
			mode = FULLUPDATE;
			break;
		    }
		}
		
		if(reload == -1) {
		    mode = FULLUPDATE;
		    break;
		} else if(reload) {
		    recbase = locmem->top_ln;
		    entries = get_records(currdirect, headers, FHSZ,
					  recbase, p_lines);
		    if(entries <= 0) {
			last_line = -1;
			break;
		    }
		}
		num = locmem->crs_ln - locmem->top_ln;
		if(headers[num].owner[0] != '-')
		    mode = i_read_key(rcmdlist, locmem, ch);
	    }
	}
    } while(mode != DOQUIT);
#undef  FHSZ

 return_i_read:
    free(headers);
    last_line = last_line0;
    hit_thread = hit_thread0;
    headers = headers0;
    strcpy(currdirect ,currdirect0);
    return;
}
