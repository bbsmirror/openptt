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
        mid = "\033[41;5m   郵差來按鈴囉   " TITLE_COLOR;
        spc = 22;
    } else if(HAS_PERM(PERM_SYSOP) && (nreg = dashs(fn_register)/163) > 10) {
        /* 超過十個人未審核 */
        sprintf(numreg, "\033[41;5m  有%03d/%03d未審核  " TITLE_COLOR,
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
    prints(TITLE_COLOR "【%s】%s\033[33m%s%s%s\033[3%s《%s》\033[0m\n",
           title, buf, mid, buf, " " + pad,
           currmode & MODE_SELECT ? "6m系列" : currmode & MODE_ETC ? "5m其他" :
           currmode & MODE_DIGEST ? "2m文摘" : "7m看板", currboard);
}

/* 動畫處理 */
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
    static char myweek[] = "天一二三四五六";
    const char *msgs[] = {"關閉", "打開", "拔掉", "防水","好友"};
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
        clrtoline(1 + FILMROW); /* 清掉上次的 */
        Jaky_outs(ptt->notes[i], 11); /* 只印11行就好 */
        outs(reset_color);
    }
    i = ptime->tm_wday << 1;
    sprintf(mystatus, "\033[34;46m[%d/%d 星期%c%c %d:%02d]\033[1;33;45m%-14s"
            "\033[30;47m 目前坊裡有 \033[31m%d\033[30m人, 我是\033[31m%-12s"
            "\033[30m[扣機]\033[31m%s\033[0m",
            ptime->tm_mon + 1, ptime->tm_mday, myweek[i], myweek[i + 1],
            ptime->tm_hour, ptime->tm_min, currutmp->birth ?
            "生日要請客唷" : ptt->today_is,
            count_ulist(), cuser.userid, msgs[currutmp->pager]);
    outmsg(mystatus);
    refresh();
}

static int show_menu(commands_t *p) {
    register int n = 0;
    register char *s;
    const char *state[4]={"用功\型", "安逸型", "自定型", "SHUTUP"};
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
    {m_user, PERM_ACCOUNTS,           "UUser          使用者資料"},
    {search_user_bypwd, PERM_SYSOP,   "SSearch User   特殊搜尋使用者"},
    {search_user_bybakpwd,PERM_SYSOP, "OOld User data 查閱\備份使用者資料"},
    {m_board, PERM_SYSOP,             "BBoard         設定看板"},
    {m_register, PERM_SYSOP,          "RRegister      審核註冊表單"},
    {cat_register, PERM_SYSOP,        "CCatregister   無法審核時用的"},
    {p_touch_boards, PERM_SYSOP,      "TTouch Boards  更新BCACHE"},
    {x_file, PERM_SYSOP|PERM_VIEWSYSOP,     "XXfile         編輯系統檔案"},
    {give_money, PERM_SYSOP|PERM_VIEWSYSOP, "GGivemoney     紅包雞"},
#ifdef  HAVE_MAILCLEAN
    {m_mclean, PERM_SYSOP,            "MMail Clean    清理使用者個人信箱"},
#endif
#ifdef  HAVE_REPORT
    {m_trace, PERM_SYSOP,             "TTrace         設定是否記錄除錯資訊"},
#endif
    {NULL, 0, NULL}
};

/* mail menu */
static commands_t maillist[] = {
    {m_new, PERM_READMAIL,      "RNew           閱\讀新進郵件"},
    {m_read, PERM_READMAIL,     "RRead          多功\能讀信選單"},
    {m_send, PERM_BASIC,        "RSend          站內寄信"},
    {mail_list, PERM_BASIC,     "RMail List     群組寄信"},
    {setforward, PERM_LOGINOK,"FForward       \033[32m設定信箱自動轉寄\033[m"},
    {m_sysop, 0,                "YYes, sir!     諂媚站長"},
    {m_internet, PERM_INTERNET, "RInternet      寄信到 Internet"},
    {mail_mbox, PERM_INTERNET,  "RZip UserHome  把所有私人資料打包回去"},
    {built_mail_index, PERM_LOGINOK, "SSavemail      把信件救回來"},
    {mail_all, PERM_SYSOP,      "RAll           寄信給所有使用者"},
    {NULL, 0, NULL}
};

/* Talk menu */
static commands_t talklist[] = {
    {t_users, 0,            "UUsers         完全聊天手冊"},
    {t_monitor, PERM_BASIC, "MMonitor       監視所有站友動態"},
    {t_pager, PERM_BASIC,   "PPager         切換呼叫器"},
    {t_idle, 0,             "IIdle          發呆"},
    {t_query, 0,            "QQuery         查詢網友"},
    {t_qchicken, 0,         "WWatch Pet     查詢寵物"},
    {t_talk, PERM_PAGE,     "TTalk          找人聊聊"},
    {t_chat, PERM_CHAT,     "CChat          找家茶坊喫茶去"},
#ifdef HAVE_MUD
    {x_mud, 0,              "VVrChat        \033[1;32m虛擬實業聊天廣場\033[m"},
#endif
    {t_display, 0,          "DDisplay       顯示上幾次熱訊"},
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
    {t_override, PERM_LOGINOK,"OOverRide      好友名單"},
    {t_reject, PERM_LOGINOK,  "BBlack         壞人名單"},
    {t_aloha,PERM_LOGINOK,    "AALOHA         上站通知名單"},
#ifdef POSTNOTIFY
    {t_post,PERM_LOGINOK,     "NNewPost       新文章通知名單"},
#endif
    {t_special,PERM_LOGINOK,  "SSpecial       其他特別名單"},
    {NULL, 0, NULL}
};

/* User menu */
static commands_t userlist[] = {
    {u_info, PERM_LOGINOK,          "IInfo          設定個人資料與密碼"},
    {calendar, PERM_LOGINOK,          "CCalendar      個人行事曆"},
    {u_editcalendar, PERM_LOGINOK,    "CCalendarEdit  編輯個人行事曆"},
    {u_loginview, PERM_LOGINOK,     "LLogin View    選擇進站畫面"},
    {u_ansi, 0, "AANSI          切換 ANSI \033[36m彩\033[35m色\033[37m/"
     "\033[30;47m黑\033[1;37m白\033[m模示"},
    {u_movie, 0,                    "MMovie         切換動畫模示"},
#ifdef  HAVE_SUICIDE
    {u_kill, PERM_BASIC,            "IKill          自殺！！"},
#endif
    {u_editplan, PERM_LOGINOK,      "QQueryEdit     編輯名片檔"},
    {u_editsig, PERM_LOGINOK,       "SSignature     編輯簽名檔"},
#if HAVE_FREECLOAK
    {u_cloak, PERM_LOGINOK,           "CCloak         隱身術"},
#else
    {u_cloak, PERM_CLOAK,           "CCloak         隱身術"},
#endif
    {u_register, PERM_BASIC,        "RRegister      填寫《註冊申請單》"},
    {u_list, PERM_BASIC,            "UUsers         列出註冊名單"},
    {NULL, 0, NULL}
};

/* XYZ tool menu */
static commands_t xyzlist[] = {
#ifdef  HAVE_LICENSE
    {x_gpl, 0,       "LLicense       GNU 使用執照"},
#endif
#ifdef HAVE_INFO
    {x_program, 0,   "PProgram       本程式之版本與版權宣告"},
#endif
    {x_boardman,0,   "MMan Boards    《看版精華區排行榜》"},
//    {x_boards,0,     "HHot Boards    《看版人氣排行榜》"},
    {x_note, 0,      "NNote          《酸甜苦辣流言版》"},
    {x_login,0,      "SSystem        《系統重要公告》"},
    {x_week, 0,      "WWeek          《本週五十大熱門話題》"},
    {x_issue, 0,     "IIssue         《今日十大熱門話題》"},
    {x_today, 0,     "TToday         《今日上線人次統計》"},
    {x_yesterday, 0, "YYesterday     《昨日上線人次統計》"},
    {x_user100 ,0,   "UUsers         《使用者百大排行榜》"},
    {x_birth, 0,     "BBirthday      《今日壽星大觀》"},
    {p_sysinfo, 0,   "XXload         《查看系統負荷》"},
    {NULL, 0, NULL}
};

/* Ptt money menu */
static commands_t moneylist[] = {
    {p_give, 0,         "00Give        給其他人錢"},
    {save_violatelaw, 0,"11ViolateLaw  繳罰單"},
#if !HAVE_FREECLOAK
    {p_cloak, 0,        "22Cloak       切換 隱身/現身   $19  /次"},
#endif
    {p_from, 0,         "33From        暫時修改故鄉     $49  /次"},
    {ordersong,0,       "44OSong       歐桑動態點歌機   $200 /次"},
    {p_exmail, 0,       "55Exmail      購買信箱         $1000/封"},
    {NULL, 0, NULL}
};

static int p_money() {
    domenu(PSALE, "Ｐtt量販店", '0', moneylist);
    return 0;
}

/* Ptt Play menu */
static commands_t playlist[] = {
    {note, PERM_LOGINOK,     "NNote        【 刻刻流言版 】"},
    {x_history, 0,           "HHistory     【 我們的成長 】"},
    {x_weather,0 ,           "WWeather     【 氣象預報 】"},
    {x_stock,0 ,             "SStock       【 股市行情 】"},
    {topsong,PERM_LOGINOK,   "TTop Songs   【\033[1;32m歐桑點歌排行榜\033[m】"},
    {p_money,PERM_LOGINOK,   "PPay         【\033[1;31m Ｐtt量販店 \033[m】"},
    {chicken_main,PERM_LOGINOK, "CChicken     【\033[1;34m Ｐtt養雞場 \033[m】"},
    {NULL, 0, NULL}
};

/* main menu */

static int admin() {
    domenu(ADMIN, "系統維護", 'X', adminlist);
    return 0;
}

static int Mail() {
    domenu(MAIL, "電子郵件", 'R', maillist);
    return 0;
}

static int Talk() {
    domenu(TMENU, "聊天說話", 'U', talklist);
    return 0;
}

static int User() {
    domenu(UMENU, "個人設定", 'A', userlist);
    return 0;
}

static int Xyz() {
    domenu(XMENU, "工具程式", 'M', xyzlist);
    return 0;
}

static int Play_Play() {
    domenu(PMENU, "網路遊樂場", 'H', playlist);
    return 0;
}

static int Name_Menu() {
    domenu(NMENU, "白色恐怖", 'O', namelist);
    return 0;
}

commands_t cmdlist[] = {
    {admin,PERM_SYSOP|PERM_VIEWSYSOP, "00Admin       【 系統維護區 】"},
    {Announce, 0,                     "AAnnounce     【 精華公佈欄 】"},
    {Boards, 0,                       "BBoards       【 佈告討論區 】"},
    {root_board, 0,                   "CClass        【 分組討論區 】"},
    {Mail, PERM_BASIC,                "MMail         【 私人信件區 】"},
    {Talk, 0,                         "TTalk         【 休閒聊天區 】"},
    {User, 0,                         "UUser         【 個人設定區 】"},
    {Xyz, 0,                          "XXyz          【 系統工具區 】"},
    {Play_Play,0,                     "PPlay         【 遊樂場/大學查榜】"},
    {Name_Menu,PERM_LOGINOK,          "NNamelist     【 編特別名單 】"},
    {Goodbye, 0,                      "GGoodbye       離開，再見……"},
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
    "\0閱\讀文章功\能鍵使用說明",
    "\01游標移動功\能鍵",
    "(↑)                  上捲一行",
    "(↓)(Enter)           下捲一行",
    "(^B)(PgUp)(BackSpace) 上捲一頁",
    "(→)(PgDn)(Space)     下捲一頁",
    "(0)(g)(Home)          檔案開頭",
    "($)(G) (End)          檔案結尾",
    "\01其他功\能鍵",
    "(/)                   搜尋字串",
    "(n/N)                 重複正/反向搜尋",
    "(TAB)                 URL連結",
    "(Ctrl-T)              存到暫存檔",
    "(:/f/b)               跳至某頁/下/上篇",
    "(F/B)                 跳至同一搜尋主題下/上篇",
    "(a/A)                 跳至同一作者下/上篇",
    "([/])                 主題式閱\讀 上/下",
    "(t)                   主題式循序閱\讀",
    "(Ctrl-C)              小計算機",
    "(q)(←)               結束",
    "(h)(H)(?)             輔助說明畫面",
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
    static char *head[4] = {"作者", "標題", "時間" ,"轉信"};
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
	if(numbytes) {              /* 一般資料處理 */
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
			
			/* 第一行太長了 */			
			if(!pos && viewed > 79) {
                            /* 第二行不是 [標....] */
			    if(memcmp( buf, head[1], 2)) { 
				/* 讀下一行進來處理 */
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

	    /* ※處理引用者 & 引言 */
	    if((buf[1] == ' ') && (buf[0] == ':' || buf[0] == '>'))
		word = "\033[36m";
	    else if(!strncmp(buf, "※", 2) || !strncmp(buf, "==>", 3))
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

	    if(line < b_lines)       /* 一般資料讀取 */
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
	    } else if(pos == b_lines)  /* 捲動螢幕 */
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
	    viewed += numbytes;       /* 累計讀過資料 */
	} else
	    line = b_lines;           /* end of END */
	
	if(promptend &&
	   ((!searching && line == b_lines) || viewed == st.st_size)) {
	    /* Kaede 剛好 100% 時不停 */
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

	    prints("\033[m\033[%sm  瀏覽 P.%d(%d%%)  %s  %-30.30s%s",
		   printcolor[(int)color], 
		   pageno, 
		   (int)((viewed * 100) / st.st_size),
		   pagemode ? "\033[30;47m" : "\033[31;47m",
		   pagemode ? http[pagemode-1] : "(h)\033[30m求助  \033[31m→↓[PgUp][",
		   pagemode ? "\033[31m[TAB]\033[30m切換 \033[31m[Enter]\033[30m選定 \033[31m←\033[30m放棄\033[m" : "PgDn][Home][End]\033[30m游標移動  \033[31m←[q]\033[30m結束   \033[m");
	    
	    
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
		    getdata_buf(b_lines - 1, 0,"[搜尋]關鍵字:", search_str,
				40, DOECHO);
		    if(*search_str) {
			searching = 1;
			if(getdata(b_lines - 1, 0, "區分大小寫(Y/N/Q)? [N] ",
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
		case ']':       /* Kaede 為了主題閱讀方便 */
		    fclose(fp);
		    return 4;
		case '[':       /* Kaede 為了主題閱讀方便 */
		    fclose(fp);
		    return 2;
		case '=':       /* Kaede 為了主題閱讀方便 */
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
		    getdata(b_lines - 2, 0, "把這篇文章收入到暫存檔？[y/N] ",
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
	    outs("\033[41m 您的權限不足無法使用internet mail... \033[m");
	    refresh();
	    return 0;
	}
        if(!invalidaddr(&ch[7]) &&
	   getdata(b_lines - 1, 0, "[寄信]主題：", genbuf, 40, DOECHO))
	    do_send(&ch[7], genbuf);
	else {
	    move(b_lines - 1,0);
	    outs("\033[41m 收信人email 或 標題 有誤... \033[m");
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
    sprintf(genbuf, "\033[33;44m 正在連往%s.(proxy:%s).....請稍候....\033[m",
	    hostname, PROXYSERVER);
#else
    sprintf(genbuf, "\033[33;44m 正在連往%s......請稍候....\033[m", hostname);
#endif
    outs(genbuf);
    refresh();

#ifdef LOCAL_PROXY
/* 先找 local disk 的 proxy */
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
	outs("\033[33;44m 找不到這個proxy server!..\033[m");
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
	outs("\033[1;44m 連接到proxy受到拒絕 ! \033[m");
	refresh();
	return;
    }
    sprintf(genbuf,"GET http://%s/%s HTTP/1.1\n",hostname,file);
#else
    if(!(h = gethostbyname(hostname))) {
	outs("\033[33;44m 找不到這個server!..\033[m");
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
	outs("\033[1;44m 連接受到拒絕 ! \033[m");
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
				strncpy(&genbuf[k], "\n◎ ", 4);
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
