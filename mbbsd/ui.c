/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "config.h"
#include "pttstruct.h"
#include "common.h"
#include "perm.h"
#include "modes.h"
#include "proto.h"

extern struct utmpfile_t *utmpshm;
extern int talkrequest;
extern char *fn_register;
extern char currboard[];        /* name of currently selected board */
extern int currmode;
extern unsigned int currstat;
extern char reset_color[];
extern userinfo_t *currutmp;
extern char *BBSName;
extern int b_lines;             /* Screen bottom line number: t_lines-1 */

/* help & menu processring */
static int refscreen = NA;
extern char *boardprefix;

int egetch() {
    int rval;

    while(1) {
        rval = igetkey();
        if(talkrequest) {
            talkreply();
            refscreen = YEA;
            return rval;
        }
        if(rval != Ctrl('L'))
            return rval;
        redoscr();
    }
}

extern userec_t cuser;

void showtitle(char *title, char *mid) {
    char buf[40];
    char numreg[50];
    int nreg;
    int spc = 0, pad;

    spc = strlen(mid);
    if(title[0] == 0)
        title++;
    else if(chkmail(0)) {
        mid = "\033[41;5m   �l�t�ӫ��a�o   " TITLE_COLOR;
        spc = 22;
    } else if(HAS_PERM(PERM_SYSOP) && (nreg = dashs(fn_register)/163) > 10) {
        /* �W�L�Q�ӤH���f�� */
        sprintf(numreg, "\033[41;5m  ��%03d/%03d���f��  " TITLE_COLOR,
		nreg,
		(int)dashs("register.new.tmp") / 163);
        mid = numreg;
        spc = 22;
    }
    spc = 66 - strlen(title) - spc - strlen(currboard);
    if(spc < 0)
        spc = 0;
    pad = 1 - (spc & 1);
    memset(buf, ' ', spc >>= 1);
    buf[spc] = '\0';
    
    clear();
    prints(TITLE_COLOR "�i%s�j%s\033[33m%s%s%s\033[3%s�m%s�n\033[0m\n",
           title, buf, mid, buf, " " + pad,
           currmode & MODE_SELECT ? "6m�t�C" : currmode & MODE_ETC ? "5m��L" :
           currmode & MODE_DIGEST ? "2m��K" : "7m�ݪO", currboard);
}

/* �ʵe�B�z */
#define FILMROW 11
static unsigned char menu_row = 12;
static unsigned char menu_column = 20;
static char mystatus[160];

static int u_movie() {
    cuser.uflag ^= MOVIE_FLAG;
    return 0;
}

void movie(int i) {
    extern struct pttcache_t *ptt;
    static short history[MAX_HISTORY];
    static char myweek[] = "�Ѥ@�G�T�|����";
    const char *msgs[] = {"����", "���}", "�ޱ�", "����","�n��"};
    time_t now = time(NULL);
    struct tm *ptime = localtime(&now);

    if((currstat != CLASS) && (cuser.uflag & MOVIE_FLAG) &&
       !ptt->busystate && ptt->max_film > 0) {
        if(currstat == PSALE) {
            i = PSALE;
            reload_money();
        } else {
            do {
                if(!i)
                    i = 1 + (int)(((float)ptt->max_film *
                                   rand()) / (RAND_MAX + 1.0));

                for(now = ptt->max_history; now >= 0; now--)
                    if(i == history[now]) {
                        i = 0;
                        break;
                    }
            } while(i == 0);
        }

        memcpy(history, &history[1], ptt->max_history * sizeof(short));
        history[ptt->max_history] = now = i;

        if(i == 999)       /* Goodbye my friend */
            i = 0;

        move(1, 0);
        clrtoline(1 + FILMROW); /* �M���W���� */
        Jaky_outs(ptt->notes[i], 11); /* �u�L11��N�n */
        outs(reset_color);
    }
    i = ptime->tm_wday << 1;
    sprintf(mystatus, "\033[34;46m[%d/%d �P��%c%c %d:%02d]\033[1;33;45m%-14s"
            "\033[30;47m �ثe�{�̦� \033[31m%d\033[30m�H, �ڬO\033[31m%-12s"
            "\033[30m[����]\033[31m%s\033[0m",
            ptime->tm_mon + 1, ptime->tm_mday, myweek[i], myweek[i + 1],
            ptime->tm_hour, ptime->tm_min, currutmp->birth ?
            "�ͤ�n�Ыȭ�" : ptt->today_is,
            count_ulist(), cuser.userid, msgs[currutmp->pager]);
    outmsg(mystatus);
    refresh();
}

static int show_menu(commands_t *p) {
    register int n = 0;
    register char *s;
    const char *state[4]={"�Υ\\��", "�w�h��", "�۩w��", "SHUTUP"};
    char buf[80];

    movie(currstat);

    move(menu_row, 0);
    while((s = p[n].desc)) {
        if(HAS_PERM(p[n].level)) {
            sprintf(buf, s + 2, state[cuser.proverb % 4]);
            prints("%*s  (\033[1;36m%c\033[0m)%s\n", menu_column, "", s[1],
                   buf);
        }
        n++;
    }
    return n - 1;
}

void domenu(int cmdmode, char *cmdtitle, int cmd, commands_t cmdtable[]) {
    int lastcmdptr;
    int n, pos, total, i;
    int err;
    int chkmailbox();
    static char cmd0[LOGIN];

    if(cmd0[cmdmode])
        cmd = cmd0[cmdmode];

    setutmpmode(cmdmode);

    showtitle(cmdtitle, BBSName);

    total = show_menu(cmdtable);

    outmsg(mystatus);
    lastcmdptr = pos = 0;

    do {
        i = -1;
        switch(cmd) {
        case Ctrl('C'):
            cal();
            i = lastcmdptr;
            refscreen = YEA;
            break;
        case Ctrl('I'):
            t_idle();
            refscreen = YEA;
            i = lastcmdptr;
            break;
        case Ctrl('N'):
            New();
            refscreen = YEA;
            i = lastcmdptr;
            break;
        case Ctrl('A'):
            if(mail_man() == FULLUPDATE)
                refscreen = YEA;
            i = lastcmdptr;
            break;
        case KEY_DOWN:
            i = lastcmdptr;
        case KEY_HOME:
        case KEY_PGUP:
            do {
                if(++i > total)
                    i = 0;
            } while(!HAS_PERM(cmdtable[i].level));
            break;
        case KEY_END:
        case KEY_PGDN:
            i = total;
            break;
        case KEY_UP:
            i = lastcmdptr;
            do {
                if(--i < 0)
                    i = total;
            } while(!HAS_PERM(cmdtable[i].level));
            break;
        case KEY_LEFT:
        case 'e':
        case 'E':
            if(cmdmode == MMENU)
                cmd = 'G';
            else if((cmdmode == MAIL) && chkmailbox())
                cmd = 'R';
            else
                return;
        default:
            if((cmd == 's'  || cmd == 'r') &&
               (currstat == MMENU || currstat == TMENU || currstat == XMENU)) {
                if(cmd == 's')
                    ReadSelect();
                else
                    Read();
                refscreen = YEA;
                i = lastcmdptr;
                break;
            }

            if(cmd == '\n' || cmd == '\r' || cmd == KEY_RIGHT) {
                move(b_lines, 0);
                clrtoeol();

                currstat = XMODE;

                if((err = (*cmdtable[lastcmdptr].cmdfunc) ()) == QUIT)
                    return;

		MPROTECT_UTMP_RW;
		currutmp->mode = currstat = cmdmode;
		MPROTECT_UTMP_R;

                if(err == XEASY) {
                    refresh();
                    safe_sleep(1);
                } else if(err != XEASY + 1 || err == FULLUPDATE)
                    refscreen = YEA;

                if(err != -1)
                    cmd = cmdtable[lastcmdptr].desc[0];
                else
                    cmd = cmdtable[lastcmdptr].desc[1];
                cmd0[cmdmode] = cmdtable[lastcmdptr].desc[0];
            }

            if(cmd >= 'a' && cmd <= 'z')
                cmd &= ~0x20;
            while(++i <= total)
                if(cmdtable[i].desc[1] == cmd)
                    break;
        }

        if(i > total || !HAS_PERM(cmdtable[i].level))
            continue;

        if(refscreen) {
            showtitle(cmdtitle, BBSName);

            show_menu(cmdtable);

            outmsg(mystatus);
            refscreen = NA;
        }
        cursor_clear(menu_row + pos, menu_column);
        n = pos = -1;
        while(++n <= (lastcmdptr = i))
            if(HAS_PERM(cmdtable[n].level))
                pos++;

        cursor_show(menu_row + pos, menu_column);
    } while(((cmd = egetch()) != EOF) || refscreen);

    abort_bbs(0);
}
/* INDENT OFF */

/* administrator's maintain menu */
static commands_t adminlist[] = {
    {m_user, PERM_ACCOUNTS,           "UUser          �ϥΪ̸��"},
    {search_user_bypwd, PERM_SYSOP,   "SSearch User   �S��j�M�ϥΪ�"},
    {search_user_bybakpwd,PERM_SYSOP, "OOld User data �d�\\�ƥ��ϥΪ̸��"},
    {m_board, PERM_SYSOP,             "BBoard         �]�w�ݪO"},
    {m_register, PERM_SYSOP,          "RRegister      �f�ֵ��U���"},
    {cat_register, PERM_SYSOP,        "CCatregister   �L�k�f�֮ɥΪ�"},
    {p_touch_boards, PERM_SYSOP,      "TTouch Boards  ��sBCACHE"},
    {x_file, PERM_SYSOP|PERM_VIEWSYSOP,     "XXfile         �s��t���ɮ�"},
    {give_money, PERM_SYSOP|PERM_VIEWSYSOP, "GGivemoney     ���]��"},
#ifdef  HAVE_MAILCLEAN
    {m_mclean, PERM_SYSOP,            "MMail Clean    �M�z�ϥΪ̭ӤH�H�c"},
#endif
#ifdef  HAVE_REPORT
    {m_trace, PERM_SYSOP,             "TTrace         �]�w�O�_�O��������T"},
#endif
    {NULL, 0, NULL}
};

/* mail menu */
static commands_t maillist[] = {
    {m_new, PERM_READMAIL,      "RNew           �\\Ū�s�i�l��"},
    {m_read, PERM_READMAIL,     "RRead          �h�\\��Ū�H���"},
    {m_send, PERM_BASIC,        "RSend          �����H�H"},
    {mail_list, PERM_BASIC,     "RMail List     �s�ձH�H"},
    {setforward, PERM_LOGINOK,"FForward       \033[32m�]�w�H�c�۰���H\033[m"},
    {m_sysop, 0,                "YYes, sir!     �ԴA����"},
    {m_internet, PERM_INTERNET, "RInternet      �H�H�� Internet"},
    {mail_mbox, PERM_INTERNET,  "RZip UserHome  ��Ҧ��p�H��ƥ��]�^�h"},
    {built_mail_index, PERM_LOGINOK, "SSavemail      ��H��Ϧ^��"},
    {mail_all, PERM_SYSOP,      "RAll           �H�H���Ҧ��ϥΪ�"},
    {NULL, 0, NULL}
};

/* Talk menu */
static commands_t talklist[] = {
    {t_users, 0,            "UUsers         ������Ѥ�U"},
    {t_monitor, PERM_BASIC, "MMonitor       �ʵ��Ҧ����ͰʺA"},
    {t_pager, PERM_BASIC,   "PPager         �����I�s��"},
    {t_idle, 0,             "IIdle          �o�b"},
    {t_query, 0,            "QQuery         �d�ߺ���"},
    {t_qchicken, 0,         "WWatch Pet     �d���d��"},
    {t_talk, PERM_PAGE,     "TTalk          ��H���"},
    {t_chat, PERM_CHAT,     "CChat          ��a���{����h"},
#ifdef HAVE_MUD
    {x_mud, 0,              "VVrChat        \033[1;32m������~��Ѽs��\033[m"},
#endif
    {t_display, 0,          "DDisplay       ��ܤW�X�����T"},
    {NULL, 0, NULL}
};

/* name menu */
static int t_aloha() {
    friend_edit(FRIEND_ALOHA);
    return 0;
}

static int t_special() {
    friend_edit(FRIEND_SPECIAL);
    return 0;
}

static commands_t namelist[] = {
    {t_override, PERM_LOGINOK,"OOverRide      �n�ͦW��"},
    {t_reject, PERM_LOGINOK,  "BBlack         �a�H�W��"},
    {t_aloha,PERM_LOGINOK,    "AALOHA         �W���q���W��"},
#ifdef POSTNOTIFY
    {t_post,PERM_LOGINOK,     "NNewPost       �s�峹�q���W��"},
#endif
    {t_special,PERM_LOGINOK,  "SSpecial       ��L�S�O�W��"},
    {NULL, 0, NULL}
};

/* User menu */
static commands_t userlist[] = {
    {u_info, PERM_LOGINOK,          "IInfo          �]�w�ӤH��ƻP�K�X"},
    {calendar, PERM_LOGINOK,          "CCalendar      �ӤH��ƾ�"},
    {u_editcalendar, PERM_LOGINOK,    "CCalendarEdit  �s��ӤH��ƾ�"},
    {u_loginview, PERM_LOGINOK,     "LLogin View    ��ܶi���e��"},
    {u_ansi, 0, "AANSI          ���� ANSI \033[36m�m\033[35m��\033[37m/"
     "\033[30;47m��\033[1;37m��\033[m�ҥ�"},
    {u_movie, 0,                    "MMovie         �����ʵe�ҥ�"},
#ifdef  HAVE_SUICIDE
    {u_kill, PERM_BASIC,            "IKill          �۱��I�I"},
#endif
    {u_editplan, PERM_LOGINOK,      "QQueryEdit     �s��W����"},
    {u_editsig, PERM_LOGINOK,       "SSignature     �s��ñ�W��"},
#if HAVE_FREECLOAK
    {u_cloak, PERM_LOGINOK,           "CCloak         �����N"},
#else
    {u_cloak, PERM_CLOAK,           "CCloak         �����N"},
#endif
    {u_register, PERM_BASIC,        "RRegister      ��g�m���U�ӽг�n"},
    {u_list, PERM_BASIC,            "UUsers         �C�X���U�W��"},
    {NULL, 0, NULL}
};

/* XYZ tool menu */
static commands_t xyzlist[] = {
#ifdef  HAVE_LICENSE
    {x_gpl, 0,       "LLicense       GNU �ϥΰ���"},
#endif
#ifdef HAVE_INFO
    {x_program, 0,   "PProgram       ���{���������P���v�ŧi"},
#endif
    {x_boardman,0,   "MMan Boards    �m�ݪ���ذϱƦ�]�n"},
//    {x_boards,0,     "HHot Boards    �m�ݪ��H��Ʀ�]�n"},
    {x_note, 0,      "NNote          �m�Ĳ��W���y�����n"},
    {x_login,0,      "SSystem        �m�t�έ��n���i�n"},
    {x_week, 0,      "WWeek          �m���g���Q�j�������D�n"},
    {x_issue, 0,     "IIssue         �m����Q�j�������D�n"},
    {x_today, 0,     "TToday         �m����W�u�H���έp�n"},
    {x_yesterday, 0, "YYesterday     �m�Q��W�u�H���έp�n"},
    {x_user100 ,0,   "UUsers         �m�ϥΪ̦ʤj�Ʀ�]�n"},
    {x_birth, 0,     "BBirthday      �m����جP�j�[�n"},
    {p_sysinfo, 0,   "XXload         �m�d�ݨt�έt���n"},
    {NULL, 0, NULL}
};

/* Ptt money menu */
static commands_t moneylist[] = {
    {p_give, 0,         "00Give        ����L�H��"},
    {save_violatelaw, 0,"11ViolateLaw  ú�@��"},
#if !HAVE_FREECLOAK
    {p_cloak, 0,        "22Cloak       ���� ����/�{��   $19  /��"},
#endif
    {p_from, 0,         "33From        �Ȯɭק�G�m     $49  /��"},
    {ordersong,0,       "44OSong       �ڮ�ʺA�I�q��   $200 /��"},
    {p_exmail, 0,       "55Exmail      �ʶR�H�c         $1000/��"},
    {NULL, 0, NULL}
};

static int p_money() {
    domenu(PSALE, "��tt�q�c��", '0', moneylist);
    return 0;
}

/* Ptt Play menu */
static commands_t playlist[] = {
    {note, PERM_LOGINOK,     "NNote        �i ���y���� �j"},
    {x_history, 0,           "HHistory     �i �ڭ̪����� �j"},
    {x_weather,0 ,           "WWeather     �i ��H�w�� �j"},
    {x_stock,0 ,             "SStock       �i �ѥ��污 �j"},
    {topsong,PERM_LOGINOK,   "TTop Songs   �i\033[1;32m�ڮ��I�q�Ʀ�]\033[m�j"},
    {p_money,PERM_LOGINOK,   "PPay         �i\033[1;31m ��tt�q�c�� \033[m�j"},
    {chicken_main,PERM_LOGINOK, "CChicken     �i\033[1;34m ��tt�i���� \033[m�j"},
    {NULL, 0, NULL}
};

/* main menu */

static int admin() {
    domenu(ADMIN, "�t�κ��@", 'X', adminlist);
    return 0;
}

static int Mail() {
    domenu(MAIL, "�q�l�l��", 'R', maillist);
    return 0;
}

static int Talk() {
    domenu(TMENU, "��ѻ���", 'U', talklist);
    return 0;
}

static int User() {
    domenu(UMENU, "�ӤH�]�w", 'A', userlist);
    return 0;
}

static int Xyz() {
    domenu(XMENU, "�u��{��", 'M', xyzlist);
    return 0;
}

static int Play_Play() {
    domenu(PMENU, "�����C�ֳ�", 'H', playlist);
    return 0;
}

static int Name_Menu() {
    domenu(NMENU, "�զ⮣��", 'O', namelist);
    return 0;
}

commands_t cmdlist[] = {
    {admin,PERM_SYSOP|PERM_VIEWSYSOP, "00Admin       �i �t�κ��@�� �j"},
    {Announce, 0,                     "AAnnounce     �i ��ؤ��G�� �j"},
    {Boards, 0,                       "BBoards       �i �G�i�Q�װ� �j"},
    {root_board, 0,                   "CClass        �i ���հQ�װ� �j"},
    {Mail, PERM_BASIC,                "MMail         �i �p�H�H��� �j"},
    {Talk, 0,                         "TTalk         �i �𶢲�Ѱ� �j"},
    {User, 0,                         "UUser         �i �ӤH�]�w�� �j"},
    {Xyz, 0,                          "XXyz          �i �t�Τu��� �j"},
    {Play_Play,0,                     "PPlay         �i �C�ֳ�/�j�Ǭd�]�j"},
    {Name_Menu,PERM_LOGINOK,          "NNamelist     �i �s�S�O�W�� �j"},
    {Goodbye, 0,                      "GGoodbye       ���}�A�A���K�K"},
    {NULL, 0, NULL}
};

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

static int more_web(char *fpath, int promptend);

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
    char *http[80]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
    /* Ptt */
    char pagemode = 0;
    char pagecount = 0;

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
			} else if (pos < 4)
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
			line = pos = 4;
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
	    while(1) {
		int i;
		char e,*ch2;

		if((ch2 = strstr(ch1, "http://")))
		    ;
		else if((ch2 = strstr(ch1,"gopher://")))
		    ;
		else if((ch2 = strstr(ch1,"mailto:")))
		    ;
		else
		    break;
		for(e = 0; ch2[(int)e] != ' ' && ch2[(int)e] != '\n' &&
			ch2[(int)e] != '\0' && ch2[(int)e] != '"' &&
			ch2[(int)e] != ';' && ch2[(int)e] != ']'; e++);
		for(i = 0; http[i] && i < 80; i++)
		    if(!strncmp(http[i], ch2, e) && http[(int)e] == 0)
			break;
		if(!http[i]) {
		    http[i] = (char *)malloc(e + 1);
		    strncpy(http[i], ch2, e);
		    http[i][(int)e] = 0;
		    pagecount++;
                }
		ch1 = &ch2[7];
	    }
	    if(word)
		outs(word);
	    {
		char msg[500], *pos;
		
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
		   pagemode ? "\033[30;47m" : "\033[31;47m",
		   pagemode ? http[pagemode-1] : "(h)\033[30m�D�U  \033[31m����[PgUp][",
		   pagemode ? "\033[31m[TAB]\033[30m���� \033[31m[Enter]\033[30m��w \033[31m��\033[30m���\033[m" : "PgDn][Home][End]\033[30m��в���  \033[31m��[q]\033[30m����   \033[m");
	    
	    
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
		    if(pagemode) {
			pagemode = 0;
			*search_str = 0;
			if(pageno)
			    pageno--;
			lino = line = 0;
			break;
		    }
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
		    if(pagemode) {
			more_web(http[pagemode-1],YEA);
			pagemode = 0;
			*search_str = 0;
			if(pageno)
			    pageno--;
			lino = line = 0;
			break;
		    }
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
		case Ctrl('I'):
		    if(!pagecount)
			break;
		    pagemode = (pagemode % pagecount) + 1;
		    strncpy(search_str,http[pagemode-1],80);
		    search_str[80] =0;
		    fptr = strstr;
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

static int more_web(char *fpath, int promptend) {
    char *ch, *ch1 = NULL;
    char *hostname = fpath,userfile[MAXPATHLEN],file[MAXPATHLEN]="/";
    char genbuf[200];
    time_t dtime;
#if !defined(USE_LYNX) && defined(USE_PROXY)
    int a;
    FILE *fp;
    struct hostent *h;
    struct sockaddr_in sin;
#endif

    if((ch = strstr(fpath, "mailto:"))) {
        if(!HAS_PERM(PERM_LOGINOK)) {
	    move(b_lines - 1,0);
	    outs("\033[41m �z���v�������L�k�ϥ�internet mail... \033[m");
	    refresh();
	    return 0;
	}
        if(!invalidaddr(&ch[7]) &&
	   getdata(b_lines - 1, 0, "[�H�H]�D�D�G", genbuf, 40, DOECHO))
	    do_send(&ch[7], genbuf);
	else {
	    move(b_lines - 1,0);
	    outs("\033[41m ���H�Hemail �� ���D ���~... \033[m");
	    refresh();
	}
        return 0;
    }
    if((ch = strstr(fpath, "gopher://"))) {
	item_t item;

	strcpy(item.X.G.server, &ch[9]);
	strcpy(item.X.G.path, "1/");
	item.X.G.port = 70;
	gem(fpath , &item, 0);
        return 0;
    }
    if((ch = strstr(fpath, "http://")))
	hostname=&ch[7];
    if((ch = strchr(hostname, '/'))) {
        *ch = 0;
        if(&ch1[1])
	    strcat(file,&ch[1]);
    }
    if(file[strlen(file) - 1] == '/')
	strcat(file,"index.html");
    move(b_lines-1,0);
    clrtoeol();
#ifdef USE_PROXY
    sprintf(genbuf, "\033[33;44m ���b�s��%s.(proxy:%s).....�еy��....\033[m",
	    hostname, PROXYSERVER);
#else
    sprintf(genbuf, "\033[33;44m ���b�s��%s......�еy��....\033[m", hostname);
#endif
    outs(genbuf);
    refresh();

#ifdef LOCAL_PROXY
/* ���� local disk �� proxy */
    time(&dtime);
    sprintf(userfile,"hproxy/%s%s",hostname,file);
    if(dashf(userfile) && (dtime - dasht(userfile)) <  HPROXYDAY * 24 * 60
       && more(userfile,promptend)) {
        return 1;
    }
    ch=userfile - 1;
    while((ch1 = strchr(ch + 1,'/'))) {
        *ch1 = 0;
        if(!dashd(ch))
	    mkdir(ch+1,0755);
        chdir(ch+1);
        *ch1 = '/';
        ch = ch1;
    }
    chdir(BBSHOME);
#endif

#ifndef USE_LYNX
#ifdef USE_PROXY
    if(!(h = gethostbyname(PROXYSERVER))) {
	outs("\033[33;44m �䤣��o��proxy server!..\033[m");
	refresh();
	return;
    }
    ()memset((char *)&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;

    if(h == NULL)
	sin.sin_addr.s_addr = inet_addr(PROXYSERVER);
    else
	()memcpy(&sin.sin_addr.s_addr, h->h_addr, h->h_length);

    sin.sin_port = htons((ushort)PROXYPORT);  /* HTTP port */
    a = socket(AF_INET, SOCK_STREAM, 0);
    if((connect(a, (struct sockaddr *) & sin, sizeof sin)) < 0) {
	outs("\033[1;44m �s����proxy����ڵ� ! \033[m");
	refresh();
	return;
    }
    sprintf(genbuf,"GET http://%s/%s HTTP/1.1\n",hostname,file);
#else
    if(!(h = gethostbyname(hostname))) {
	outs("\033[33;44m �䤣��o��server!..\033[m");
	refresh();
	return;
    }
    ()memset((char *) &sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    
    if(h == NULL)
	sin.sin_addr.s_addr = inet_addr(hostname);
    else
	()memcpy(&sin.sin_addr.s_addr, h->h_addr, h->h_length);
    
    sin.sin_port =  htons((ushort)80);  
    a = socket(AF_INET, SOCK_STREAM, 0);
    if((connect(a, (struct sockaddr *) & sin, sizeof sin)) < 0) {
	outs("\033[1;44m �s������ڵ� ! \033[m");
	refresh();
	return;
    }
    sprintf(genbuf, "GET %s\n", file);
#endif

    for(i = strlen(file); file[i - 1] != '/' && i > 0 ; i--);
    file[i] = 0;

    i = strlen(genbuf);
    write(a, genbuf, i);

#define BLANK   001
#define ISPRINT 002
#define PRE     004
#define CENTER  010
    if((fp = fopen(userfile,"w"))) {
	int flag = 2, c;
	char path[MAXPATHLEN];
	unsigned char j, k;

	while((i = read(a,genbuf,200))) {
	    if(i < 0)
		return;
	    genbuf[i]=0;
	    
	    for(j = 0, k = 0; genbuf[j] && j < i; j++) {
		if((flag & ISPRINT) && genbuf[j] == '<')
		    flag |= BLANK;
		else if((flag & ISPRINT) && genbuf[j] == '>')
		    flag &= ~BLANK;
		else {
		    if(!(flag & BLANK)) {
			if(j != k && (genbuf[j] != '\n' || flag & PRE))
			    genbuf[k++] = genbuf[j];
		    } else {
			switch(char_lower(genbuf[j])) {
			case 'a':
			    break;
			case 'b':
			    if(genbuf[j + 1] == 'r' && genbuf[j + 2] == '>') 
				genbuf[k++] = '\n';
			    break;
			case 'h':
			    if(genbuf[j + 1] == 'r' && 
			       (genbuf[j + 2] == '>' ||
				genbuf[j + 2] == 's')) {
				strncpy(&genbuf[k], "\n--\n", 4);
				k += 4;
			    }
			    break;
			case 'l':
			    if(genbuf[j + 1] == 'i' && genbuf[j + 2]=='>') {
				strncpy(&genbuf[k], "\n�� ", 4);
				k += 4;
			    }
			    break;
			case 'p':
			    if(genbuf[j + 1]=='>') {
				genbuf[k++] = '\n';
				genbuf[k++] = '\n';
			    } else if(genbuf[j + 1] == 'r' &&
				      genbuf[j + 2] == 'e')
				flag ^= PRE;
			    break;
			case 't':
			    if(genbuf[j + 1] == 'd' && genbuf[j + 2]=='>') {
				strncpy(&genbuf[k], "\n-\n", 3);
				k += 3;
			    }
			    break;
			}
		    }
		    if((genbuf[j] & 0x80) && (flag & ISPRINT))
			flag &= ~ISPRINT;
		    else
			flag |= ISPRINT;
                }
	    }
	    genbuf[k]=0;
	    fputs(genbuf, fp);
	}
	fclose(fp);
	close(a);
	return more(userfile, promptend);
    }
    return 0;
#else  /* use lynx dump */
    sprintf(genbuf, "lynx -dump http://%s%s > %s", hostname, file, userfile);
    system(genbuf);
    return more(userfile, promptend);
#endif
}
