/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"
#include "pttstruct.h"
#include "common.h"
#include "modes.h"
#include "perm.h"
#include "proto.h"

extern int showansi;
extern int t_lines, t_columns;  /* Screen size / width */
extern int b_lines;             /* Screen bottom line number: t_lines-1 */
extern char *str_author1;
extern char *str_author2;
extern char *str_post1;
extern char *str_post2;
extern char *msg_seperator;
extern char reset_color[];

#define MAXPATHLEN 256
static char *more_help[] = {
    "\0�\\Ū�峹�\\����ϥλ���",
    "\01��в��ʥ\\����",
    "(��)                  �W���@��",
    "(��)(Enter)           �U���@��",
    "(^B)(PgUp)(BackSpace) �W���@��",
    "(��)(PgDn)(Space)     �U���@��",
    "(0)(g)(Home)          �ɮ׶}�Y",
    "($)(G) (End)          �ɮ׵���",
    "\01��L�\\����",
    "(/)                   �j�M�r��",
    "(n/N)                 ���ƥ�/�ϦV�j�M",
    "(TAB)                 URL�s��",
    "(Ctrl-T)              �s��Ȧs��",
    "(:/f/b)               ���ܬY��/�U/�W�g",
    "(F/B)                 ���ܦP�@�j�M�D�D�U/�W�g",
    "(a/A)                 ���ܦP�@�@�̤U/�W�g",
    "([/])                 �D�D���\\Ū �W/�U",
    "(t)                   �D�D���`�Ǿ\\Ū",
    "(Ctrl-C)              �p�p���",
    "(q)(��)               ����",
    "(h)(H)(?)             ���U�����e��",
    NULL
};

int beep = 0;

static int readln(FILE *fp, char *buf) {
    register int ch, i, len, bytes, in_ansi;

    len = bytes = in_ansi = i = 0;
    while(len < 80 && i < ANSILINELEN && (ch = getc(fp)) != EOF) {
	bytes++;
	if(ch == '\n')
	    break;
	else if(ch == '\t')
	    do {
		buf[i++] = ' ';
	    } while((++len & 7) && len < 80);
	else if(ch == '\a')
	    beep = 1;
	else if(ch == '\033') {
	    if(showansi)
		buf[i++] = ch;
	    in_ansi = 1;
	} else if(in_ansi) {
	    if(showansi)
		buf[i++] = ch;
	    if(!strchr("[0123456789;,", ch))
		in_ansi = 0;
	} else if(isprint2(ch)) {
	    len++;
	    buf[i++] = ch;
	}
    }
    buf[i] = '\0';
    return bytes;
}

extern userec_t cuser;

int more(char *fpath, int promptend) {
    extern char* strcasestr();
    static char *head[4] = {"�@��", "���D", "�ɶ�" ,"��H"};
    char *ptr, *word = NULL, buf[ANSILINELEN + 1], *ch1;
    struct stat st;
    FILE *fp;
    unsigned int pagebreak[MAX_PAGES], pageno, lino = 0;
    int line, ch, viewed, pos, numbytes;
    int header = 0;
    int local = 0;
    char search_char0=0;
    static char search_str[81]="";
    typedef char* (*FPTR)();
    static FPTR fptr;
    int searching = 0;
    int scrollup = 0;
    char *printcolor[3]= {"44","33;45","0;34;46"}, color =0;

    memset(pagebreak, 0, sizeof(pagebreak));
    if(*search_str)
	search_char0 = *search_str;
    *search_str = 0;
    if(!(fp = fopen(fpath, "r")))
	return -1;
    
    if(fstat(fileno(fp), &st))
	return -1;
    
    pagebreak[0] = pageno = viewed = line = pos = 0;
    clear();
    
    while((numbytes = readln(fp, buf)) || (line == t_lines)) {
	if(scrollup) {
	    rscroll();
	    move(0, 0);
	}
	if(numbytes) {              /* �@���ƳB�z */
	    if(!viewed) {             /* begin of file */
		if(showansi) {          /* header processing */
		    if(!strncmp(buf, str_author1, LEN_AUTHOR1)) {
			line = 3;
			word = buf + LEN_AUTHOR1;
			local = 1;
		    } else if(!strncmp(buf, str_author2, LEN_AUTHOR2)) {
			line = 4;
			word = buf + LEN_AUTHOR2;
		    }
		    
		    while(pos < line) {
			if(!pos && ((ptr = strstr(word, str_post1)) ||
				    (ptr = strstr(word, str_post2)))) {
			    ptr[-1] = '\0';
			    prints("\033[47;34m %s \033[44;37m%-53.53s"
				   "\033[47;34m %.4s \033[44;37m%-13s\033[m\n",
				   head[0], word, ptr, ptr + 5);
			} else if (pos < line)
			    prints("\033[47;34m %s \033[44;37m%-72.72s"
				   "\033[m\n", head[pos], word);
			
			viewed += numbytes;
			numbytes = readln(fp, buf);
			
			/* �Ĥ@��Ӫ��F */			
			if(!pos && viewed > 79) {
                            /* �ĤG�椣�O [��....] */
			    if(memcmp( buf, head[1], 2)) { 
				/* Ū�U�@��i�ӳB�z */
				viewed += numbytes;
				numbytes = readln(fp, buf);
			    }
			}
			pos++;
		    }
		    if(pos) {
			header = 1;
			
			prints("\033[36m%s\033[m\n", msg_seperator);
			line++; pos++;
		    }
		}
		lino = pos;
		word = NULL;
	    }

	    /* ���B�z�ޥΪ� & �ި� */
	    if((buf[1] == ' ') && (buf[0] == ':' || buf[0] == '>'))
		word = "\033[36m";
	    else if(!strncmp(buf, "��", 2) || !strncmp(buf, "==>", 3))
		word = "\033[32m";
	    
	    ch1 = buf;
	    if(word)
		outs(word);
	    {
		char msg[1024], *pos;
		
		if(*search_str && (pos = fptr(buf, search_str))) {
		    char SearchStr[81];
		    char buf1[100], *pos1;
		    
		    strncpy(SearchStr, pos, strlen(search_str));
		    SearchStr[strlen(search_str)] = 0;
		    searching = 0;
		    sprintf(msg, "%.*s\033[7m%s\033[m", pos - buf, buf,
			    SearchStr);
		    while((pos = fptr(pos1 = pos + strlen(search_str),
				      search_str))) {
			sprintf(buf1, "%.*s\033[7m%s\033[m", pos - pos1,
				pos1, SearchStr);
			strcat(msg, buf1);
		    }
		    strcat(msg, pos1);
		    outs(Ptt_prints(msg,NO_RELOAD));
		} else
		    outs(Ptt_prints(buf,NO_RELOAD));
	    }
	    if(word) {
		outs("\033[m");
		word = NULL;
	    }
	    outch('\n');
	    
	    if(beep) {
		bell();
		beep = 0;
	    }

	    if(line < b_lines)       /* �@����Ū�� */
		line++;
	    
	    if(line == b_lines && searching == -1) {
		if(pageno > 0)
		    fseek(fp, viewed = pagebreak[--pageno], SEEK_SET);
		else
		    searching = 0;
		lino = pos = line = 0;
		clear();
		continue;
	    }
	    
	    if(scrollup) {
		move(line = b_lines, 0);
		clrtoeol();
		for(pos = 1; pos < b_lines; pos++)
		    viewed += readln(fp, buf);
	    } else if(pos == b_lines)  /* ���ʿù� */
		scroll();
	    else
		pos++;

	    if(!scrollup && ++lino >= b_lines && pageno < MAX_PAGES - 1) {
		pagebreak[++pageno] = viewed;
		lino = 1;
	    }

	    if(scrollup) {
		lino = scrollup;
		scrollup = 0;
	    }
	    viewed += numbytes;       /* �֭pŪ�L��� */
	} else
	    line = b_lines;           /* end of END */
	
	if(promptend &&
	   ((!searching && line == b_lines) || viewed == st.st_size)) {
	    /* Kaede ��n 100% �ɤ��� */
	    move(b_lines, 0);
	    if(viewed == st.st_size) {
		if(searching == 1)
		    searching = 0;
		color = 0;
	    } else if(pageno == 1 && lino == 1) {
		if(searching == -1)
		    searching = 0;
		color = 1;
	    } else
		color = 2;

	    prints("\033[m\033[%sm  �s�� P.%d(%d%%)  %s  %-30.30s%s",
		   printcolor[(int)color], 
		   pageno, 
		   (int)((viewed * 100) / st.st_size),
		   "\033[31;47m",
		   "(h)\033[30m�D�U  \033[31m����[PgUp][",
		   "PgDn][Home][End]\033[30m��в���  \033[31m��[q]\033[30m����   \033[m");
	    
	    
	    while(line == b_lines || (line > 0 && viewed == st.st_size)) {
		switch((ch = egetch())) {
		case ':':
		{
		    char buf[10];
		    int i = 0;
		    
		    getdata(b_lines - 1, 0, "Goto Page: ", buf, 5, DOECHO);
		    sscanf(buf, "%d", &i);
		    if(0 < i && i <  MAX_PAGES && (i == 1 || pagebreak[i - 1]))
			pageno = i - 1;
		    else if(pageno)
			pageno--;
		    lino = line = 0;
		    break;
		}
		case '/':
		{
		    char ans[4] = "n";
		    
		    *search_str = search_char0;
		    getdata_buf(b_lines - 1, 0,"[�j�M]����r:", search_str,
				40, DOECHO);
		    if(*search_str) {
			searching = 1;
			if(getdata(b_lines - 1, 0, "�Ϥ��j�p�g(Y/N/Q)? [N] ",
				   ans, 4, LCECHO) && *ans == 'y')
			    fptr = strstr;
			else
			    fptr = strcasestr;
		    }
		    if(*ans == 'q')
			searching = 0;
		    if(pageno)
			pageno--;
		    lino = line = 0;
		    break;
		}
		case 'n':
		    if(*search_str) {
			searching = 1;
			if(pageno)
			    pageno--;
			lino = line = 0;
		    }
		    break;
		case 'N':
		    if(*search_str) {
			searching = -1;
			if(pageno)
			    pageno--;
			lino = line = 0;
		    }
		    break;
		case 'r':
		case 'R':
		case 'Y':
		    fclose(fp);
		    return 7;
		case 'y':
		    fclose(fp);
		    return 8;
		case 'A':
		    fclose(fp);
		    return 9;
		case 'a':
		    fclose(fp);
		    return 10;
		case 'F':
		    fclose(fp);
		    return 11;
		case 'B':
		    fclose(fp);
		    return 12;
		case KEY_LEFT:
		    fclose(fp);
		    return 6;
		case 'q':
		    fclose(fp);
		    return 0;
		case 'b':
		    fclose(fp);
		    return 1;
		case 'f':
		    fclose(fp);
		    return 3;
		case ']':       /* Kaede ���F�D�D�\Ū��K */
		    fclose(fp);
		    return 4;
		case '[':       /* Kaede ���F�D�D�\Ū��K */
		    fclose(fp);
		    return 2;
		case '=':       /* Kaede ���F�D�D�\Ū��K */
		    fclose(fp);
		    return 5;
		case Ctrl('F'):
		case KEY_PGDN:
		    line = 1;
		    break;
		case 't':
		    if(viewed == st.st_size) {
			fclose(fp);
			return 4;
		    }
		    line = 1;
		    break;
		case ' ':
		    if(viewed == st.st_size) {
			fclose(fp);
			return 3;
		    }
		    line = 1;
		    break;
		case KEY_RIGHT:
		    if(viewed == st.st_size) {
			fclose(fp);
			return 0;
		    }
		    line = 1;
		    break;
		case '\r':
		case '\n':
		case KEY_DOWN:
		    if(viewed == st.st_size ||
		       (promptend == 2 && (ch == '\r' || ch == '\n'))) {
			fclose(fp);
			return 3;
		    }
		    line = t_lines - 2;
		    break;
		case '$':
		case 'G':
		case KEY_END:
		    line = t_lines;
		    break;
		case '0':
		case 'g':
		case KEY_HOME:
		    pageno = line = 0;
		    break;
		case 'h':
		case 'H':
		case '?':
		    /* Kaede Buggy ... */
		    show_help(more_help);
		    if(pageno)
			pageno--;
		    lino = line = 0;
		    break;
		case 'E':
		    if(HAS_PERM(PERM_SYSOP) && strcmp(fpath, "etc/ve.hlp")) {
			fclose(fp);
			vedit(fpath, NA, NULL);
			return 0;
		    }
		    break;
		case Ctrl('C'):
		    cal();
		    if(pageno)
			pageno--;
		    lino = line = 0;
		    break;

		case Ctrl('T'):
		    getdata(b_lines - 2, 0, "��o�g�峹���J��Ȧs�ɡH[y/N] ",
			    buf, 4, LCECHO);
		    if(buf[0] == 'y') {
			char tmpbuf[128];
			
			setuserfile(tmpbuf, ask_tmpbuf(b_lines - 1));
			sprintf(buf, "cp -f %s %s", fpath, tmpbuf);
			system(buf);
		    }
		    if(pageno)
			pageno--;
		    lino = line = 0;
		    break;
		case KEY_UP:
		    line = -1;
		    break;
		case Ctrl('B'):
		case KEY_PGUP:
		    if(pageno > 1) {
			if(lino < 2)
			    pageno -= 2;
			else
			    pageno--;
			lino = line = 0;
		    } else if(pageno && lino > 1)
			pageno = line = 0;
		    break;
		case Ctrl('H'):
		    if(pageno > 1) {
			if(lino < 2)
			    pageno -= 2;
			else
			    pageno--;
			lino = line = 0;
		    } else if(pageno && lino > 1)
			pageno = line = 0;
		    else {
			fclose(fp);
			return 1;
		    }
		}
	    }
	    
	    if(line > 0) {
		move(b_lines, 0);
		clrtoeol();
		refresh();
	    } else if(line < 0) {                      /* Line scroll up */
		if(pageno <= 1) {
		    if(lino == 1 || !pageno) {
			fclose(fp);
			return 1;
		    }
		    if(header && lino <= 5) {
			fseek(fp, viewed = pagebreak[scrollup = lino =
						    pageno = 0] = 0, SEEK_SET);
			clear();
		    }
		}
		if(pageno && lino > 1 + local) {
		    line =  (lino - 2) - local;
		    if(pageno > 1 && viewed == st.st_size)
			line += local;
		    scrollup = lino - 1;
		    fseek(fp, viewed = pagebreak[pageno - 1], SEEK_SET);
		    while(line--)
			viewed += readln(fp, buf);
		} else if(pageno > 1) {
		    scrollup = b_lines - 1;
		    line = (b_lines - 2) - local;
		    fseek(fp, viewed = pagebreak[--pageno - 1], SEEK_SET);
		    while(line--)
			viewed += readln(fp, buf);
		}
		line = pos = 0;
	    } else {
		pos = 0;
		fseek(fp, viewed = pagebreak[pageno], SEEK_SET);
		move(0,0);
		clear();
	    }
	}
    }

    fclose(fp);
    if(promptend) {
	pressanykey();
	clear();
    } else
	outs(reset_color);
    return 0;
}

