/* $Id$ */
#ifndef INCLUDE_CONFIG_H
#define INCLUDE_CONFIG_H

#include "pttbbs.conf"

#define BBSPROG         BBSHOME "/bin/mbbsd"         /* 主程式 */
#define BAN_FILE        "BAN"                        /* 關站通告 */
#define LOAD_FILE       "/proc/loadavg"              /* for Linux */

#ifndef MAX_USERS
#define MAX_USERS         (120000)       /* 最高註冊人數 */
#endif

#ifndef HAVE_SEARCHALL
#define HAVE_SEARCHALL    0              /* 搜尋某使用者在所有板上的文章 */
#endif

/* 以下還未整理 */
#define MAX_FRIEND        (256)          /* 載入 cache 之最多朋友數目 */
#define MAX_REJECT        (32)           /* 載入 cache 之最多壞人數目 */
#define MAX_MSGS          (10)           /* 水球(熱訊)忍耐上限 */
#define MAX_BOARD         (4096)         /* 最大開版個數 */
#define MAX_MOVIE         (999)          /* 最多動態看版數 */
#define MAX_MOVIE_SECTION (10)		 /* 最多動態看板類別 */
#define MAX_FROM          (300)          /* 最多故鄉數 */
#define MAX_ACTIVE        (1024)         /* 最多同時上站人數 */
#define MAX_ITEMS         (1000)         /* 一個目錄最多有幾項 */
#define MAX_HISTORY       (12)           /* 動態看板保持 12 筆歷史記錄 */
#define MAX_CROSSNUM      (9) 	         /* 最多crosspost次數 */
#define MAX_QUERYLINES    (16)           /* 顯示 Query/Plan 訊息最大行數 */
#define MAX_LOGIN_INFO    (128)          /* 最多上線通知人數 */
#define MAX_POST_INFO     (32)           /* 最多新文章通知人數 */
#define MAX_NAMELIST      (128)          /* 最多其他特別名單人數 */
#define MAX_PAGES         (999)          /* more.c 中文章頁數上限(lines/22) */
#define MAX_KEEPMAIL      (200)          /* 最多保留幾封 MAIL？ */
#define MAX_EXKEEPMAIL    (1000)         /* 最多信箱加大多少封 */
#define MAX_NOTE          (20)           /* 最多保留幾篇留言？ */
#define MAX_SIGLINES      (6)            /* 簽名檔引入最大行數 */
#define MAX_CROSSNUM      (9) 	         /* 最多crosspost次數 */
#define MAX_REVIEW        (7)		 /* 最多水球回顧 */
#define NUMVIEWFILE       (14)           /* 進站畫面最多數 */
#define MAX_CPULOAD       (70)           /* CPU 最高load */
#define MAX_SWAPUSED      (0.7)          /* SWAP最高使用率 */
#define LOGINATTEMPTS     (3)            /* 最大進站失誤次數 */
#define WHERE                            /* 是否有故鄉功能 */
#undef  LOG_BOARD  			 /* 看版是否log */
#undef SUPPORT_GB      			 /* 是否支援gb */


#define HAVE_CHKLOAD            /* check cpu load */
#undef  INITIAL_SETUP           /* 剛開站，還沒建立預設看板[SYSOP] */
#define DEFAULTBOARD   "SYSOP"  /* 預設看板 */
#define LOGINASNEW              /* 採用上站申請帳號制度 */
#define NO_WATER_POST           /* 防止BlahBlah式灌水 */
#define USE_BSMTP               /* 使用opus的BSMTP 寄收信? */
#define HAVE_ANONYMOUS          /* 提供 Anonymous 板 */
#undef  POSTNOTIFY              /* 新文章通知功能 */
#define INTERNET_EMAIL          /* 支援 InterNet Email 功能(含 Forward) */
#define HAVE_ORIGIN             /* 顯示 author 來自何處 */
#undef  HAVE_MAILCLEAN          /* 清理所有使用者個人信箱 */
#undef  HAVE_SUICIDE            /* 提供使用者自殺功能 */
#undef  HAVE_REPORT             /* 系統追蹤報告 */
#undef  HAVE_INFO               /* 顯示程式版本說明 */
#undef  HAVE_LICENSE            /* 顯示 GNU 版權畫面 */
#undef  HAVE_TIN                /* 提供 news reader */
#undef  HAVE_GOPHER             /* 提供 gopher */
#undef  HAVE_WWW                /* 提供 www browser */
#define HAVE_CAL                /* 提功計算機 */
#undef  HAVE_ARCHIE             /* have arche */
#undef  POSTBUG                 /* board/mail post 沒有 bug 了 */
#undef  HAVE_REPORT             /* 系統追蹤報告 */
#undef  EMAIL_JUSTIFY           /* 發出 InterNet Email 身份認證信函 */
#undef  NEWUSER_LIMIT           /* 新手上路的三天限制 */
#undef  HAVE_X_BOARDS

#define USE_LYNX   	        /* 使用外部lynx dump ? */
#undef  USE_PROXY
#ifdef  USE_PROXY
#define PROXYSERVER "140.112.28.165"
#define PROXYPORT   3128
#endif
#define LOCAL_PROXY             /* 是否開啟local 的proxy */
#ifdef  LOCAL_PROXY
#define HPROXYDAY   1           /* local的proxy refresh天數 */
#endif

#define SHOWMIND                /* 看見心情 */
#define SHOWUID                 /* 看見使用者 UID */
#define SHOWBOARD               /* 看見使用者看板 */
#define SHOWPID                 /* 看見使用者 PID */

#define REALINFO                /* 真實姓名 */
#ifdef  REALINFO
#undef  ACTS_REALNAMES          /* 主目錄的 (U)ser 顯示真實姓名 */
#undef  POST_REALNAMES          /* 貼文件時附上真實姓名 */
#undef  MAIL_REALNAMES          /* 寄站內信件時附上真實姓名 */
#endif

#define DOTIMEOUT
#ifdef  DOTIMEOUT
#define IDLE_TIMEOUT    (30*60) /* 一般情況之 timeout */
#define MONITOR_TIMEOUT (20*60) /* monitor 時之 timeout */
#define SHOW_IDLE_TIME          /* 顯示閒置時間 */
#endif

#define SEM_ENTER      -1      /* enter semaphore */
#define SEM_LEAVE      1       /* leave semaphore */
#define SEM_RESET      0       /* reset semaphore */

#define MAGIC_KEY       1234    /* 身分認證信函編碼 */

#define BRDSHM_KEY      1216
#define UHASH_KEY       1219	/* userid->uid hash */
#define UTMPSHM_KEY     2219
#define PTTSHM_KEY      1220    /* 動態看版 , 節日 */
#define FROMSHM_KEY     1223    /* whereis, 最多使用者 */

#define BRDSEM_KEY      2005    /* semaphore key */
#define PTTSEM_KEY      2000    /* semaphore key */
#define FROMSEM_KEY     2003    /* semaphore key */
#define PASSWDSEM_KEY   2010

#define NEW_CHATPORT    3838
#define CHATPORT        5722

#define MAX_ROOM         16              /* 最多有幾間包廂？ */

#define EXIT_LOGOUT     0
#define EXIT_LOSTCONN   -1
#define EXIT_CLIERROR   -2
#define EXIT_TIMEDOUT   -3
#define EXIT_KICK       -4

#define CHAT_LOGIN_OK       "OK"
#define CHAT_LOGIN_EXISTS   "EX"
#define CHAT_LOGIN_INVALID  "IN"
#define CHAT_LOGIN_BOGUS    "BG"
#define BADCIDCHARS " *"        /* Chat Room 中禁用於 nick 的字元 */

#define ALLPOST "ALLPOST"

#endif
