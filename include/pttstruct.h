/* $Id$ */
#ifndef INCLUDE_STRUCT_H
#define INCLUDE_STRUCT_H

#define IDLEN      12             /* Length of bid/uid */
#define PASSLEN    14             /* Length of encrypted passwd field */
#define REGLEN     38             /* Length of registration data */

typedef struct userec_t {
    char userid[IDLEN + 1];
    char realname[20];
    char username[24];
    char passwd[PASSLEN];
    unsigned char uflag;
    unsigned int userlevel;
    unsigned short numlogins;
    unsigned short numposts;
    time_t firstlogin;
    time_t lastlogin;
    char lasthost[16];
    char pad3[4];
    char remoteuser[3];           /* 保留 目前沒用到的 */
    char proverb;
    char email[50];
    char address[50];
    char justify[REGLEN + 1];
    unsigned char month;
    unsigned char day;
    unsigned char year;
    unsigned char sex;
    unsigned char state;
    unsigned char pager;
    unsigned char invisible;
    unsigned int  exmailbox;
    char pad2[132];
    unsigned int  loginview;
    unsigned char channel;        /* 動態看板 */
    unsigned short vl_count;      /* ViolateLaw counter */
    char pad[107];
} userec_t;
/* these are flags in userec_t.uflag */
#define SIG_FLAG        0x3     /* signature number, 2 bits */
#define PAGER_FLAG      0x4     /* true if pager was OFF last session */
#define CLOAK_FLAG      0x8     /* true if cloak was ON last session */
#define FRIEND_FLAG     0x10    /* true if show friends only */
#define BRDSORT_FLAG    0x20    /* true if the boards sorted alphabetical */
#define MOVIE_FLAG      0x40    /* true if show movie */
#define COLOR_FLAG      0x80    /* true if the color mode open */
#define MIND_FLAG       0x100   /* true if mind search mode open <-Heat*/

#define BTLEN      48             /* Length of board title */

typedef struct boardheader_t {
    char brdname[IDLEN + 1];             /* bid */
    char title[BTLEN + 1];
    char BM[IDLEN * 3 + 3];              /* BMs' uid, token '/' */
    unsigned int brdattr;                /* board的屬性 */
    char pad[3];                         /* 沒用到的 */
    time_t bupdate;                      /* note update time */
    char pad2[3];                        /* 沒用到的 */
    unsigned char bvote;                 /* Vote flags */
    time_t vtime;                        /* Vote close time */
    unsigned int level;                  /* 可以看此板的權限 */
    int uid;                             /* 看板的類別 ID */
    int gid;                             /* 看板所屬的類別 ID */
    char pad3[120];
} boardheader_t;

#define BRD_NOZAP             00001         /* 不可zap  */
#define BRD_NOCOUNT           00002         /* 不列入統計 */
#define BRD_NOTRAN            00004         /* 不轉信 */
#define BRD_GROUPBOARD        00010         /* 群組板 */
#define BRD_HIDE              00020         /* 隱藏板 (看板好友才可看) */
#define BRD_POSTMASK          00040         /* 限制發表或閱讀 */
#define BRD_ANONYMOUS         00100         /* 匿名板? */
#define BRD_DEFAULTANONYMOUS  00200         /* 預設匿名板 */
#define BRD_BAD		      00400         /* 違法改進中看板 */
#define BRD_VOTEBOARD         01000	    /* 連署機看板 */

#define TTLEN      64             /* Length of title */
#define FNLEN      33             /* Length of filename  */

typedef struct fileheader_t {
    char filename[FNLEN];         /* M.9876543210.A */
    char savemode;                /* file save mode */
    char owner[IDLEN + 2];        /* uid[.] */
    char date[6];                 /* [02/02] or space(5) */
    char title[TTLEN + 1];
    char pad[4];
    unsigned char filemode;       /* must be last field @ boards.c */
} fileheader_t;

#define FILE_LOCAL      0x1     /* local saved */
#define FILE_READ       0x1     /* already read : mail only */
#define FILE_MARKED     0x2     /* opus: 0x8 */
#define FILE_DIGEST     0x4     /* digest */
#define FILE_TAGED      0x8     /* taged */
#define FILE_SOLVED	0x10	/* problem solved, sysop only */

#define STRLEN     80             /* Length of most string data */


/* uhash is a userid->uid hash table -- jochang */

#define HASH_BITS 16
typedef struct uhash_t {
    char userid[MAX_USERS][IDLEN + 1];
    int next_in_hash[MAX_USERS];
    int hash_head[1 << HASH_BITS];
    int number;				/* # of users total */
    int loaded;				/* .PASSWD has been loaded? */
} uhash_t;

union xitem_t {
    struct {                    /* bbs_item */
	char fdate[9];          /* [mm/dd/yy] */
	char editor[13];        /* user ID */
	char fname[31];
    } B;
    struct {                    /* gopher_item */
	char path[81];
	char server[48];
	int port;
    } G;
};

typedef struct {
    char title[63];
    union xitem_t X;
} item_t;

typedef struct {
    item_t *item[MAX_ITEMS];
    char mtitle[STRLEN];
    char *path;
    int num, page, now, level;
} gmenu_t;

typedef struct msgque_t {
    pid_t last_pid;
    char last_userid[IDLEN + 1];
    char last_call_in[80];
} msgque_t;

#define FAVMAX     74		  /* Max boards of Myfavorite */
#define FAVGMAX    16             /* Max groups of Myfavorite */
#define FAVGSLEN    8		  /* Max Length of Description String */

typedef struct userinfo_t {
    int uid;                      /* Used to find user name in passwd file */
    pid_t pid;                    /* kill() to notify user of talk request */
    int sockaddr;                 /* ... */
    int destuid;                  /* talk uses this to identify who called */
    int destuip;                  /* dest index in utmpshm->uinfo[] */
    unsigned char active;         /* When allocated this field is true */
    unsigned char invisible;      /* Used by cloaking function in Xyz menu */
    unsigned char sockactive;     /* Used to coordinate talk requests */
    unsigned int userlevel;
    unsigned char mode;           /* UL/DL, Talk Mode, Chat Mode, ... */
    unsigned char pager;          /* pager toggle, YEA, or NA */
    unsigned char in_chat;        /* for in_chat commands   */
    unsigned char sig;            /* signal type */
    char userid[IDLEN + 1];
    char chatid[11];              /* chat id, if in chat mode */
    char realname[20];
    char username[24];
    char from[27];                /* machine name the user called in from */
    int from_alias;
    char birth;                   /* 是否是生日 Ptt*/
    char tty[11];                 /* tty port */
    int friend[MAX_FRIEND];
    int reject[MAX_REJECT];
    unsigned char msgcount;
    msgque_t msgs[MAX_MSGS];
    time_t uptime;
    time_t lastact;               /* 上次使用者動的時間 */
    unsigned int  brc_id;
    unsigned char lockmode;       /* 不准 multi_login 玩的東西 */
    char turn;                    /* for gomo */
    char mateid[IDLEN + 1];       /* for gomo */
    int myfavorite[FAVMAX];
    char gfavorite[FAVGMAX][FAVGSLEN+1];
    int ninGroup[FAVGMAX];
    int nGroup;
    int ninRoot;
    char color;
    int mind;
} userinfo_t;

typedef struct {
    fileheader_t *header;
    char mtitle[STRLEN];
    char *path;
    int num, page, now, level;
} menu_t;

typedef struct onekey_t {     /* Used to pass commands to the readmenu */
    int key;
    int (*fptr)();
} onekey_t;

#define ANSILINELEN (511)                /* Maximum Screen width in chars */

/* anti_crosspost */
typedef struct crosspost_t {
    int checksum[4]; /* 0 -> 'X' cross post  1-3 -> 簡查文章行 */
    int times;       /* 第幾次 */
} crosspost_t;

typedef struct bcache_t {
    boardheader_t bcache[MAX_BOARD];
    unsigned int total[MAX_BOARD];
    time_t lastposttime[MAX_BOARD];
    time_t uptime;
    time_t touchtime;
    int number;
    int busystate;
} bcache_t;

typedef struct keeploc_t {
    char *key;
    int top_ln;
    int crs_ln;
    struct keeploc_t *next;
} keeploc_t;

#define USHM_SIZE       (MAX_ACTIVE + 4)

struct utmpfile_t {
    userinfo_t uinfo[USHM_SIZE];
    time_t uptime;
    int number;
    int busystate;
};

struct pttcache_t {
    char notes[MAX_MOVIE][200*11];
    int n_notes[MAX_MOVIE_SECTION];          /* 一節中有幾個 看板 */
    int next_refresh[MAX_MOVIE_SECTION];     /* 下一次要refresh的 看板 */
    int max_film;
    int max_history;
    time_t uptime;
    time_t touchtime;
    int busystate;
};

typedef struct {
    unsigned char oldlen;                /* previous line length */
    unsigned char len;                   /* current length of line */
    unsigned char mode;                  /* status of line, as far as update */
    unsigned char smod;                  /* start of modified data */
    unsigned char emod;                  /* end of modified data */
    unsigned char sso;                   /* start stand out */
    unsigned char eso;                   /* end stand out */
    unsigned char data[ANSILINELEN + 1];
} screenline_t;

typedef struct {
    int r, c;
} rc_t;

#define BRD_ROW           10
#define BRD_COL           9

typedef int board_t[BRD_ROW][BRD_COL];

/* name.c 中運用的資料結構 */
typedef struct word_t {
    char *word;
    struct word_t *next;
} word_t;

typedef struct commands_t {
    int (*cmdfunc)();
    int level;
    char *desc;                   /* next/key/description */
} commands_t;

typedef struct MailQueue {
    char filepath[FNLEN];
    char subject[STRLEN];
    time_t mailtime;
    char sender[IDLEN + 1];
    char username[24];
    char rcpt[50];
    int method;
    char * niamod;
} MailQueue;

enum  {MQ_TEXT, MQ_UUENCODE, MQ_JUSTIFY};

#endif
