/* $Id$ */
#ifndef INCLUDE_CONFIG_H
#define INCLUDE_CONFIG_H

#include "pttbbs.conf"

#define BBSPROG         BBSHOME "/bin/mbbsd"         /* �D�{�� */
#define BAN_FILE        "BAN"                        /* �����q�i */
#define LOAD_FILE       "/proc/loadavg"              /* for Linux */

#ifndef MAX_USERS
#define MAX_USERS         (120000)       /* �̰����U�H�� */
#endif

#ifndef HAVE_SEARCHALL
#define HAVE_SEARCHALL    0              /* �j�M�Y�ϥΪ̦b�Ҧ��O�W���峹 */
#endif

/* �H�U�٥���z */
#define MAX_FRIEND        (256)          /* ���J cache ���̦h�B�ͼƥ� */
#define MAX_REJECT        (32)           /* ���J cache ���̦h�a�H�ƥ� */
#define MAX_MSGS          (10)           /* ���y(���T)�ԭ@�W�� */
#define MAX_BOARD         (4096)         /* �̤j�}���Ӽ� */
#define MAX_MOVIE         (999)          /* �̦h�ʺA�ݪ��� */
#define MAX_MOVIE_SECTION (10)		 /* �̦h�ʺA�ݪO���O */
#define MAX_FROM          (300)          /* �̦h�G�m�� */
#define MAX_ACTIVE        (1024)         /* �̦h�P�ɤW���H�� */
#define MAX_ITEMS         (1000)         /* �@�ӥؿ��̦h���X�� */
#define MAX_HISTORY       (12)           /* �ʺA�ݪO�O�� 12 �����v�O�� */
#define MAX_CROSSNUM      (9) 	         /* �̦hcrosspost���� */
#define MAX_QUERYLINES    (16)           /* ��� Query/Plan �T���̤j��� */
#define MAX_LOGIN_INFO    (128)          /* �̦h�W�u�q���H�� */
#define MAX_POST_INFO     (32)           /* �̦h�s�峹�q���H�� */
#define MAX_NAMELIST      (128)          /* �̦h��L�S�O�W��H�� */
#define MAX_PAGES         (999)          /* more.c ���峹���ƤW��(lines/22) */
#define MAX_KEEPMAIL      (200)          /* �̦h�O�d�X�� MAIL�H */
#define MAX_EXKEEPMAIL    (1000)         /* �̦h�H�c�[�j�h�֫� */
#define MAX_NOTE          (20)           /* �̦h�O�d�X�g�d���H */
#define MAX_SIGLINES      (6)            /* ñ�W�ɤޤJ�̤j��� */
#define MAX_CROSSNUM      (9) 	         /* �̦hcrosspost���� */
#define MAX_REVIEW        (7)		 /* �̦h���y�^�U */
#define NUMVIEWFILE       (14)           /* �i���e���̦h�� */
#define MAX_CPULOAD       (70)           /* CPU �̰�load */
#define MAX_SWAPUSED      (0.7)          /* SWAP�̰��ϥβv */
#define LOGINATTEMPTS     (3)            /* �̤j�i�����~���� */
#define WHERE                            /* �O�_���G�m�\�� */
#undef  LOG_BOARD  			 /* �ݪ��O�_log */
#undef SUPPORT_GB      			 /* �O�_�䴩gb */


#define HAVE_CHKLOAD            /* check cpu load */
#undef  INITIAL_SETUP           /* ��}���A�٨S�إ߹w�]�ݪO[SYSOP] */
#define DEFAULTBOARD   "SYSOP"  /* �w�]�ݪO */
#define LOGINASNEW              /* �ĥΤW���ӽбb����� */
#define NO_WATER_POST           /* ����BlahBlah����� */
#define USE_BSMTP               /* �ϥ�opus��BSMTP �H���H? */
#define HAVE_ANONYMOUS          /* ���� Anonymous �O */
#undef  POSTNOTIFY              /* �s�峹�q���\�� */
#define INTERNET_EMAIL          /* �䴩 InterNet Email �\��(�t Forward) */
#define HAVE_ORIGIN             /* ��� author �Ӧۦ�B */
#undef  HAVE_MAILCLEAN          /* �M�z�Ҧ��ϥΪ̭ӤH�H�c */
#undef  HAVE_SUICIDE            /* ���ѨϥΪ̦۱��\�� */
#undef  HAVE_REPORT             /* �t�ΰl�ܳ��i */
#undef  HAVE_INFO               /* ��ܵ{���������� */
#undef  HAVE_LICENSE            /* ��� GNU ���v�e�� */
#undef  HAVE_TIN                /* ���� news reader */
#undef  HAVE_GOPHER             /* ���� gopher */
#undef  HAVE_WWW                /* ���� www browser */
#define HAVE_CAL                /* ���\�p��� */
#undef  HAVE_ARCHIE             /* have arche */
#undef  POSTBUG                 /* board/mail post �S�� bug �F */
#undef  HAVE_REPORT             /* �t�ΰl�ܳ��i */
#undef  EMAIL_JUSTIFY           /* �o�X InterNet Email �����{�ҫH�� */
#undef  NEWUSER_LIMIT           /* �s��W�����T�ѭ��� */
#undef  HAVE_X_BOARDS

#define USE_LYNX   	        /* �ϥΥ~��lynx dump ? */
#undef  USE_PROXY
#ifdef  USE_PROXY
#define PROXYSERVER "140.112.28.165"
#define PROXYPORT   3128
#endif
#define LOCAL_PROXY             /* �O�_�}��local ��proxy */
#ifdef  LOCAL_PROXY
#define HPROXYDAY   1           /* local��proxy refresh�Ѽ� */
#endif

#define SHOWMIND                /* �ݨ��߱� */
#define SHOWUID                 /* �ݨ��ϥΪ� UID */
#define SHOWBOARD               /* �ݨ��ϥΪ̬ݪO */
#define SHOWPID                 /* �ݨ��ϥΪ� PID */

#define REALINFO                /* �u��m�W */
#ifdef  REALINFO
#undef  ACTS_REALNAMES          /* �D�ؿ��� (U)ser ��ܯu��m�W */
#undef  POST_REALNAMES          /* �K���ɪ��W�u��m�W */
#undef  MAIL_REALNAMES          /* �H�����H��ɪ��W�u��m�W */
#endif

#define DOTIMEOUT
#ifdef  DOTIMEOUT
#define IDLE_TIMEOUT    (30*60) /* �@�뱡�p�� timeout */
#define MONITOR_TIMEOUT (20*60) /* monitor �ɤ� timeout */
#define SHOW_IDLE_TIME          /* ��ܶ��m�ɶ� */
#endif

#define SEM_ENTER      -1      /* enter semaphore */
#define SEM_LEAVE      1       /* leave semaphore */
#define SEM_RESET      0       /* reset semaphore */

#define MAGIC_KEY       1234    /* �����{�ҫH��s�X */

#define BRDSHM_KEY      1216
#define UHASH_KEY       1219	/* userid->uid hash */
#define UTMPSHM_KEY     2219
#define PTTSHM_KEY      1220    /* �ʺA�ݪ� , �`�� */
#define FROMSHM_KEY     1223    /* whereis, �̦h�ϥΪ� */

#define BRDSEM_KEY      2005    /* semaphore key */
#define PTTSEM_KEY      2000    /* semaphore key */
#define FROMSEM_KEY     2003    /* semaphore key */
#define PASSWDSEM_KEY   2010

#define NEW_CHATPORT    3838
#define CHATPORT        5722

#define MAX_ROOM         16              /* �̦h���X���]�[�H */

#define EXIT_LOGOUT     0
#define EXIT_LOSTCONN   -1
#define EXIT_CLIERROR   -2
#define EXIT_TIMEDOUT   -3
#define EXIT_KICK       -4

#define CHAT_LOGIN_OK       "OK"
#define CHAT_LOGIN_EXISTS   "EX"
#define CHAT_LOGIN_INVALID  "IN"
#define CHAT_LOGIN_BOGUS    "BG"
#define BADCIDCHARS " *"        /* Chat Room ���T�Ω� nick ���r�� */

#define ALLPOST "ALLPOST"

#endif
