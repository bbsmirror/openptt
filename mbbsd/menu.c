/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include "config.h"
#include "pttstruct.h"
#include "common.h"
#include "perm.h"
#include "modes.h"
#include "proto.h"

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
    sprintf(mystatus, "\033[34;46m[%d/%d 星期%c%c %d:%02d]"
	    "\033[30;47m 目前坊裡有 \033[31m%d\033[30m人, 我是\033[31m%-12s"
            "\033[30m[扣機]\033[31m%s                  \033[0m",
            ptime->tm_mon + 1, ptime->tm_mday, myweek[i], myweek[i + 1],
            ptime->tm_hour, ptime->tm_min,
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

                currutmp->mode = currstat = cmdmode;

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
    {t_talk, PERM_PAGE,     "TTalk          找人聊聊"},
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
    {Name_Menu,PERM_LOGINOK,          "NNamelist     【 編特別名單 】"},
    {Goodbye, 0,                      "GGoodbye       離開，再見……"},
    {NULL, 0, NULL}
};
