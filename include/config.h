/* $Id$ */
#ifndef INCLUDE_CONFIG_H
#define INCLUDE_CONFIG_H

#include <syslog.h>

#define BBSPROG         BBSHOME "/bin/mbbsd"         /* 祘Α */
#define BAN_FILE        "BAN"                        /* 闽硄郎 */
#define LOAD_FILE       "/proc/loadavg"              /* for Linux */

#ifndef RELAY_SERVER_IP                     /* 盚獺 mail server */
#define RELAY_SERVER_IP    "127.0.0.1"
#endif

#ifndef MAX_USERS                           /* 程蔼爹计 */
#define MAX_USERS          (120000)
#endif

#ifndef MAX_ACTIVE
#define MAX_ACTIVE        (1024)         /* 程计 */
#endif

#ifndef MAX_CPULOAD
#define MAX_CPULOAD       (70)           /* CPU 程蔼load */
#endif

#ifndef MAX_POST_MONEY                      /* 祇ゅ彻絑禣 */
#define MAX_POST_MONEY     100
#endif

#ifndef MAX_CHICKEN_MONEY                   /* 緄蔓初矛 */
#define MAX_CHICKEN_MONEY  100
#endif

#ifndef MAX_GUEST_LIFE                      /* 程ゼ粄靡ㄏノ玂痙丁() */
#define MAX_GUEST_LIFE     (3 * 24 * 60 * 60)
#endif

#ifndef MAX_LIFE                            /* 程ㄏノ玂痙丁() */
#define MAX_LIFE           (120 * 24 * 60 * 60)
#endif

#ifndef HAVE_SEARCH_ALL                     /* 穓碝琘ㄏノ┮Τ狾ゅ彻 */
#define HAVE_SEARCH_ALL    0
#endif

#ifndef HAVE_FREECLOAK
#define HAVE_FREECLOAK    0
#endif

#ifndef FORCE_PROCESS_REGISTER_FORM
#define FORCE_PROCESS_REGISTER_FORM 0
#endif

#ifndef TITLE_COLOR
#define TITLE_COLOR       "\033[0;1;37;46m"
#endif

#ifndef SYSLOG_FACILITY
#define SYSLOG_FACILITY   LOG_LOCAL0
#endif

#ifndef TAR_PATH
#define TAR_PATH  "tar"
#endif

/* 临ゼ俱瞶 */
#define MAX_FRIEND        (256)          /* 更 cache ぇ程狟ね计ヘ */
#define MAX_REJECT        (32)           /* 更 cache ぇ程胊计ヘ */
#define MAX_MSGS          (10)           /* 瞴(荐癟)г瑻 */
#define MAX_BOARD         (4096)         /* 程秨计 */
#define MAX_MOVIE         (999)          /* 程笆篈计 */
#define MAX_MOVIE_SECTION (10)		 /* 程笆篈狾摸 */
#define MAX_ITEMS         (1000)         /* ヘ魁程Τ碭兜 */
#define MAX_HISTORY       (12)           /* 笆篈狾玂 12 掸菌癘魁 */
#define MAX_CROSSNUM      (9) 	         /* 程crosspostΩ计 */
#define MAX_QUERYLINES    (16)           /* 陪ボ Query/Plan 癟程︽计 */
#define MAX_LOGIN_INFO    (128)          /* 程絬硄计 */
#define MAX_POST_INFO     (32)           /* 程穝ゅ彻硄计 */
#define MAX_NAMELIST      (128)          /* 程ㄤ疭虫计 */
#define MAX_PAGES         (999)          /* more.c いゅ彻计(lines/22) */
#define MAX_KEEPMAIL      (200)          /* 程玂痙碭 MAIL */
#define MAX_EXKEEPMAIL    (1000)         /* 程獺絚ぶ */
#define MAX_NOTE          (20)           /* 程玂痙碭絞痙ē */
#define MAX_SIGLINES      (6)            /* 帽郎ま程︽计 */
#define MAX_CROSSNUM      (9) 	         /* 程crosspostΩ计 */
#define MAX_REVIEW        (7)		 /* 程瞴臮 */
#define NUMVIEWFILE       (14)           /* 秈礶程计 */
#define MAX_SWAPUSED      (0.7)          /* SWAP程蔼ㄏノ瞯 */
#define LOGINATTEMPTS     (3)            /* 程秈ア粇Ω计 */
#undef  LOG_BOARD  			 /* 琌log */


#undef  INITIAL_SETUP           /* 秨临⊿ミ箇砞狾[SYSOP] */
#define DEFAULTBOARD   "SYSOP"  /* 箇砞狾 */
#define LOGINASNEW              /* 蹦ノビ叫眀腹 */
#define NO_WATER_POST           /* ňゎBlahBlahΑ拈 */
#define USE_BSMTP               /* ㄏノopusBSMTP 盚Μ獺? */
#define HAVE_ANONYMOUS          /* 矗ㄑ Anonymous 狾 */
#undef  POSTNOTIFY              /* 穝ゅ彻硄 */
#define INTERNET_EMAIL          /* や穿 InterNet Email ( Forward) */
#define HAVE_ORIGIN             /* 陪ボ author ㄓ矪 */
#undef  HAVE_MAILCLEAN          /* 睲瞶┮Τㄏノ獺絚 */
#undef  HAVE_INFO               /* 陪ボ祘Αセ弧 */
#undef  HAVE_LICENSE            /* 陪ボ GNU 舦礶 */
#define HAVE_CAL                /* 矗ㄑ璸衡诀 */
#undef  POSTBUG                 /* board/mail post ⊿Τ bug  */
#undef  EMAIL_JUSTIFY           /* 祇 InterNet Email ō粄靡獺ㄧ */
#undef  NEWUSER_LIMIT           /* 穝も隔ぱ */

#define USE_LYNX   	        /* ㄏノ场lynx dump ? */
#undef  USE_PROXY
#ifdef  USE_PROXY
#define PROXYSERVER "140.112.28.165"
#define PROXYPORT   3128
#endif
#define LOCAL_PROXY             /* 琌秨币local proxy */
#ifdef  LOCAL_PROXY
#define HPROXYDAY   1           /* localproxy refreshぱ计 */
#endif

#define SHOWMIND                /* ǎみ薄 */
#define SHOWUID                 /* ǎㄏノ UID */
#define SHOWBOARD               /* ǎㄏノ狾 */
#define SHOWPID                 /* ǎㄏノ PID */

#define REALINFO                /* 痷龟﹎ */

#define DOTIMEOUT
#ifdef  DOTIMEOUT
#define IDLE_TIMEOUT    (30*60) /* 薄猵ぇ timeout */
#define MONITOR_TIMEOUT (20*60) /* monitor ぇ timeout */
#define SHOW_IDLE_TIME          /* 陪ボ盯竚丁 */
#endif

#define SEM_ENTER      -1      /* enter semaphore */
#define SEM_LEAVE      1       /* leave semaphore */
#define SEM_RESET      0       /* reset semaphore */

#define MAGIC_KEY       1234    /* ōだ粄靡獺ㄧ絪絏 */

#define BRDSHM_KEY      1216
#define UHASH_KEY       1219	/* userid->uid hash */
#define UTMPSHM_KEY     2219
#define PTTSHM_KEY      1220    /* 笆篈 , 竊ら */

#define BRDSEM_KEY      2005    /* semaphore key */
#define PTTSEM_KEY      2000    /* semaphore key */
#define PASSWDSEM_KEY   2010

#define NEW_CHATPORT    3838
#define CHATPORT        5722

#define MAX_ROOM         16              /* 程Τ碭丁碵 */

#define EXIT_LOGOUT     0
#define EXIT_LOSTCONN   -1
#define EXIT_CLIERROR   -2
#define EXIT_TIMEDOUT   -3
#define EXIT_KICK       -4

#define CHAT_LOGIN_OK       "OK"
#define CHAT_LOGIN_EXISTS   "EX"
#define CHAT_LOGIN_INVALID  "IN"
#define CHAT_LOGIN_BOGUS    "BG"
#define BADCIDCHARS " *"        /* Chat Room い窽ノ nick じ */

#define ALLPOST "ALLPOST"

#endif
