#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <setjmp.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "struct.h"
#include "util.h"

#undef	HAVE_PROC_TITLE
#define	WATCH_DOG
#define	SERVER_USAGE
#define	DEBUG


#define	MAIL_XMIT	BBSHOME "/out/.MM"
#define	MAIL_BACK	BBSHOME "/out/.MB"
#define	MAIL_UUFILE	BBSHOME "/out/.MU"


#define	MAIL_PIDFILE	BBSHOME "/adm/bmda.pid"
#define	MAIL_LOGFILE	BBSHOME "/adm/bmda.log"
#define	MAIL_DBGFILE	BBSHOME "/adm/bmda.dbg"


#define SMTP_PORT	25
#define BBS_HOST_IS_SMTP_HOST
#define SMTP_HOST       "ptt2.twbbs.org.tw"
#define	SMTP_PERIOD	(2 * 60)
#define SMTP_BANNER     "bmda "
#define	SMTP_CMDSIZ	1024	/* max size of SMTP commands */

#define HAVE_TIMEOUT
#define	SMTP_TIMEOUT	(20)
#define	SEND_TIMEOUT	(20)


#define	TCP_BUFSIZ	3072
#define	TCP_LINSIZ	512


#define	TCP_READ	0
#define	TCP_WRITE	1


#define	MAIL_POOLSIZE	32768


static int msize, mleng;
static char *mpool;
int
 dashf(fname)
char *fname;
{
    struct stat st;

    return (stat(fname, &st) == 0 && S_ISREG(st.st_mode));
}


/* ----------------------------------------------------- */
/* operation log and debug information                   */
/* ----------------------------------------------------- */

#ifdef	HAVE_PROC_TITLE
static char *main_title, *main_trail;


static void
 ap_title(str)
char *str;
{
    char *title, *trail;
    int cc;

    title = main_title;

    while (cc = *str)
    {
	*title++ = cc;
	str++;
    }

    trail = main_trail;

    while (title < trail)
    {
	*title++ = ' ';
    }
}
#else
#define	ap_title(x)		;
#endif


static FILE *flog;		/* log file */
static int gline;


#ifdef	WATCH_DOG
#define MYDOG	gline = __LINE__
#else
#define MYDOG			/* NOOP */
#endif


static void
 logit(msg)
char *msg;
{
    time_t now;
    struct tm *p;

    time(&now);
    p = localtime(&now);
    fprintf(flog, "%02d/%02d %02d:%02d:%02d %s\n",
	    p->tm_mon + 1, p->tm_mday,
	    p->tm_hour, p->tm_min, p->tm_sec, msg);
#ifdef DEBUG
    printf("%02d/%02d %02d:%02d:%02d %s\n",
	   p->tm_mon + 1, p->tm_mday,
	   p->tm_hour, p->tm_min, p->tm_sec, msg);
#endif

}


static void
 log_init()
{
    FILE *fp;

    fp = fopen(MAIL_PIDFILE, "w");
    fprintf(fp, "%d\n", getpid());
    fclose(fp);

    flog = fopen(MAIL_LOGFILE, "a");

}


static void
 check_pidfile()
{
    FILE *fp;
    int count, pid;

    if((fp = fopen(MAIL_PIDFILE, "r"))) {
	count = fscanf(fp, "%d\n", &pid);
	fclose(fp);
	if (count == 1 && pid > 0)
	{

#ifdef	SERVER_USAGE
	    if (!kill(pid, SIGPROF))
#else
	    if (!kill(pid, 0))
#endif

	    {
		fprintf(stderr, "I am running: %d\n", pid);
		exit(0);
	    }
	}
    }
}


#ifdef	SERVER_USAGE
#include <sys/resource.h>


static void
 server_usage()
{
    struct rusage ru;

    if (getrusage(RUSAGE_SELF, &ru))
	return;

    fprintf(flog, "\n[Server Usage]\n\n"
	    "user time: %.6f\n"
	    "system time: %.6f\n"
	    "maximum resident set size: %lu P\n"
	    "integral resident set size: %lu\n"
	    "page faults not requiring physical I/O: %d\n"
	    "page faults requiring physical I/O: %d\n"
	    "swaps: %d\n"
	    "block input operations: %d\n"
	    "block output operations: %d\n"
	    "messages sent: %d\n"
	    "messages received: %d\n"
	    "signals received: %d\n"
	    "voluntary context switches: %d\n"
	    "involuntary context switches: %d\ngline: %d\n\n",

	    (double) ru.ru_utime.tv_sec + (double) ru.ru_utime.tv_usec / 1000000.0,
	    (double) ru.ru_stime.tv_sec + (double) ru.ru_stime.tv_usec / 1000000.0,
	    ru.ru_maxrss,
	    ru.ru_idrss,
	    (int)ru.ru_minflt,
	    (int)ru.ru_majflt,
	    (int)ru.ru_nswap,
	    (int)ru.ru_inblock,
	    (int)ru.ru_oublock,
	    (int)ru.ru_msgsnd,
	    (int)ru.ru_msgrcv,
	    (int)ru.ru_nsignals,
	    (int)ru.ru_nvcsw,
	    (int)ru.ru_nivcsw, gline);

    fflush(flog);
}
#endif


/* ----------------------------------------------------- */
/* string table                                          */
/* ----------------------------------------------------- */


#define STHSIZE         256
#define STHMASK         (STHSIZE-1)


typedef struct STab
{
    struct STab *st_Next;
    int st_Hv;
    int st_Refs;
    unsigned long st_addr;
    char str[0];
}
STab;


static STab *STHash[STHSIZE];


static int
 StrHash(const char *s)
{
    unsigned int hv = 0xA45CD32F;

    while (*s)
    {
	hv = (hv << 5) ^ *s ^ (hv >> 23);
	++s;
    }
    return (hv ^ (hv > 16));
}


static char *
 GetStrTab(char *s)
{
    int hv = StrHash(s);
    STab **pst = &STHash[hv & STHMASK];
    STab *st;

    for (;;)
    {
	st = *pst;
	if (st == NULL)
	{
	    st = *pst = (STab *) malloc(sizeof(STab) + strlen(s) + 1);
	    st->st_Next = NULL;
	    st->st_Hv = hv;
	    st->st_Refs = 0;
	    st->st_addr = 0;
	    strcpy(st->str, s);
	    break;
	}

	if (st->st_Hv == hv && !strcmp(st->str, s))
	    break;

	pst = &st->st_Next;
    }

    ++st->st_Refs;
    return (st->str);
}


static void
 ExpStrTab()
{
    int i;
    STab *st, **pst;

    for (i = 0; i < STHSIZE; i++)
    {
	pst = &(STHash[i]);
	while((st = *pst))
	{
	    if ((st->st_Refs -= 8) <= 0)
	    {
		*pst = st->st_Next;
		free(st);
	    }
	    else
	    {
		pst = &st->st_Next;
	    }
	}
    }
}


/* ----------------------------------------------------- */
/* timeout routines                                      */
/* ----------------------------------------------------- */


#ifdef	HAVE_TIMEOUT
static jmp_buf ap_jmp;


static void
 ap_timeout()
{
    longjmp(ap_jmp, 1);
}
#endif


/* ----------------------------------------------------- */
/* buffered TCP I/O routines                             */
/* ----------------------------------------------------- */


static int tcp_pos;
static char tcp_pool[TCP_BUFSIZ];
static char smtp_reply[SMTP_CMDSIZ];


#if 0
static int
 tcp_wait(sock, mode)
int sock;
int mode;
{
    fd_set fset, xset, *rptr, *wptr;
    struct timeval tv;

    FD_ZERO(&fset);
    FD_SET(sock, &fset);
    xset = fset;
    tv.tv_sec = 600;
    tv.tv_usec = 0;

    if (mode == TCP_WRITE)
    {
	rptr = NULL;
	wptr = &fset;
    }
    else
    {
	wptr = NULL;
	rptr = &fset;
    }

    if (select(sock + 1, rptr, wptr, &xset, &tv) <= 0)
    {
	logit("timeout");
	return -1;
    }

    if (FD_ISSET(sock, &xset))
    {
	logit("except");
	return -1;
    }

    return 0;
}


static int
 tcp_drop(sock)
int sock;
{
    fd_set fset, rset;
    struct timeval tv;
    int cc;

    FD_ZERO(&fset);
    FD_SET(sock, &fset);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    for (;;)
    {
	rset = fset;
	cc = select(sock + 1, &rset, NULL, NULL, &tv);
	if (cc <= 0)
	    break;

	recv(sock, smtp_reply, SMTP_CMDSIZ, 0);
    }

    return 0;
}
#endif


static int			/* return 0 for success */
 tcp_send(sock, data, size)
int sock;
char *data;
int size;
{
    int len;
    fd_set fset, wset, xset;
    struct timeval tv;

    FD_ZERO(&fset);
    FD_SET(sock, &fset);
    tv.tv_sec = 600;
    tv.tv_usec = 0;

    for (;;)
    {
	wset = xset = fset;
	if (select(sock + 1, NULL, &wset, &xset, &tv) <= 0)
	    break;

	if (FD_ISSET(sock, &xset))
	{
	    logit("except");
	    break;
	}

	len = send(sock, data, size, 0);
	if (len < 0)
	    break;

	if (len == size)
	    return len;

	size -= len;
	data += len;
    }

    return -1;
}


static int
 tcp_command(sock, cmd, code)
int sock;
char *cmd;
int code;			/* expected return value, 3-letter "220" */
{
    int cc, len, pos;
    char *str;
    fd_set fset, rset, wset, xset, *rptr, *wptr;
    struct timeval tv;

    FD_ZERO(&fset);
    FD_SET(sock, &fset);
    tv.tv_sec = 600;
    tv.tv_usec = pos = 0;
    str = smtp_reply;

    for (;;)
    {
	if (pos >= 0)
	{
	    rset = fset;
	    rptr = &rset;
	}
	else
	{
	    rptr = NULL;
	}

	if (cmd)
	{
	    wset = fset;
	    wptr = &wset;
	}
	else
	{
	    wptr = NULL;
	}

	xset = fset;

	if (select(sock + 1, rptr, wptr, &xset, &tv) <= 0)
	{
	    logit("select");
	    break;
	}

	if (FD_ISSET(sock, &xset))
	{
	    logit("except");
	    break;
	}

	if (rptr && FD_ISSET(sock, rptr))
	{
	    cc = recv(sock, str + pos, SMTP_CMDSIZ - pos, 0);
	    if (cc < 0)
	    {
		logit("Error in read() from SMTP host");
		break;
	    }

	    pos += cc;
	    if (pos > 0 && str[pos - 1] != '\n')
		continue;	/* go on to read until EOL */

	    if (cmd)
	    {
		pos = -1;	/* go on to send command */
		continue;
	    }

	    cc = str[3];
	    str[3] = '\0';
	    if (code == atoi(str))
		return 0;

	    str[3] = cc;
	    str[pos - 2] = '\0';
	    logit(str);
	    break;
	}

	if (wptr && FD_ISSET(sock, wptr))
	{
	    len = strlen(cmd);
	    cc = send(sock, cmd, len, 0);
	    if (cc < 0)
	    {
		logit("Error in write() to SMTP host");
		break;
	    }

	    len -= cc;
	    if (len == 0)
	    {
		cmd = NULL;
		pos = 0;	/* go to to read response */
	    }
	    else
	    {
		cmd += cc;
	    }
	}
    }

    return -1;
}


static int
 tcp_puts(sock, msg, len)
int sock;
char *msg;
int len;
{
    int pos;
    char *head, *tail;

    pos = tcp_pos;

    head = tcp_pool;
    tail = head + pos;
    memcpy(tail, msg, len);
    tail[len] = '\r';
    tail[len + 1] = '\n';
    pos += (len + 2);
    len = 0;

    if (pos >= TCP_BUFSIZ - TCP_LINSIZ)
    {
	len = tcp_send(sock, head, pos);
	pos = 0;
    }

    tcp_pos = pos;
    return len;
}


static int
 tcp_flush(sock)
int sock;
{
    int pos;
    char *head, *tail;

    pos = tcp_pos;
    head = tcp_pool;
    tail = head + pos;
    memcpy(tail, "\r\n.\r\n", 5);
    return tcp_send(sock, head, pos + 5);
}


/* ----------------------------------------------------- */
/* DNS MX routines                                       */
/* ----------------------------------------------------- */


#include <arpa/nameser.h>


extern int errno;
extern int h_errno;


typedef struct
{
    unsigned char d[4];
}
ip_addr;


/*
 * *  The standard udp packet size PACKETSZ (512) is not sufficient for some *
 * nameserver answers containing very many resource records. The resolver *
 * may switch to tcp and retry if it detects udp packet overflow. *  Also
 * note that the resolver routines res_query and res_search return *  the
 * size of the *un*truncated answer in case the supplied answer buffer *  it
 * not big enough to accommodate the entire answer.
 */


#ifndef MAXPACKET
#define MAXPACKET 8192		/* max packet size used internally by BIND */
#endif


typedef union
{
    HEADER hdr;
    u_char buf[MAXPACKET];
}
querybuf;			/* response of DNS query */


#define	MAX_MXLIST	1024
#include <resolv.h>

static void
 dns_init()
{
    res_init();
/*
   _res.retrans = 3;
   _res.options |= RES_USEVC;
 */
}


static unsigned short
 getshort(c)
unsigned char *c;
{
    unsigned short u;

    u = c[0];
    return (u << 8) + c[1];
}


static int
 dns_query(name, qtype, ans)
char *name;			/* domain name */
int qtype;			/* type of query */
querybuf *ans;			/* buffer to put answer */
{
    querybuf buf;

    qtype = res_mkquery(QUERY, name, C_IN, qtype, (char *) NULL, 0, NULL,
			(char *) &buf, sizeof(buf));

    if (qtype >= 0)
    {
	qtype = res_send((char *) &buf, qtype, (char *) ans, sizeof(querybuf));

    /* avoid problems after truncation in tcp packets */

	if (qtype > sizeof(querybuf))
	    qtype = sizeof(querybuf);
    }

    return qtype;
}


static void
 dns_mx(domain, mxlist)
char *domain;
char *mxlist;
{
    querybuf ans;
    int n, ancount, qdcount;
    unsigned char *cp, *eom;
    int type;

    MYDOG;

    *mxlist = 0;

    n = dns_query(domain, T_MX, &ans);

    if (n < 0)
	return;

/* find first satisfactory answer */

    cp = (u_char *) & ans + sizeof(HEADER);
    eom = (u_char *) & ans + n;

    for (qdcount = ntohs(ans.hdr.qdcount); qdcount--; cp += n + QFIXEDSZ)
    {
	if ((n = dn_skipname(cp, eom)) < 0)
	    return;
    }

    ancount = ntohs(ans.hdr.ancount);
    domain = mxlist + MAX_MXLIST - MAXDNAME - 2;

    while (--ancount >= 0 && cp < eom)
    {
	if ((n = dn_expand((char *) &ans, eom, cp, mxlist, MAXDNAME)) < 0)
	    break;

	cp += n;

	type = getshort(cp);
	n = getshort(cp + 8);
	cp += 10;

	if (type == T_MX)
	{
	/* pref = getshort(cp); */
	    *mxlist = '\0';
	    if ((dn_expand((char *) &ans, eom, cp + 2, mxlist, MAXDNAME)) < 0)
		break;

	    if (!*mxlist)
		return;

	    while (*mxlist)
		mxlist++;
	    *mxlist++ = ':';

	    if (mxlist >= domain)
		break;
	}

	cp += n;
    }

    *mxlist = '\0';
}

int
 not_alnum(ch)
register char ch;
{
    return (ch < '0' || (ch > '9' && ch < 'A') ||
	    (ch > 'Z' && ch < 'a') || ch > 'z');
}



static int
 host_ipx(addr)
char *addr;
{
    int sock;
    ip_addr *ip;
    char *ptr;
    struct sockaddr_in sin;

    ptr = addr;

    if (*addr)
	while (*addr)
	{
	    if (not_alnum(*addr) && !strchr("[].%!@:-_", *addr))
		return -1;
	    addr++;
	}
    else
	return -1;

    sin.sin_family = AF_INET;
    sin.sin_port = htons(SMTP_PORT);
    memset(sin.sin_zero, 0, sizeof(sin.sin_zero));

    ip = (ip_addr *) & (sin.sin_addr.s_addr);

    if (*ptr == '[')
	ptr++;

    for (sock = 0; sock < 4; sock++)
    {
	if (!(addr = strchr(ptr, '.')))
	    return -1;		/* Ptt */
	ip->d[3 - sock] = atoi(ptr);
	ptr = addr++;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
	return sock;

    if (!connect(sock, (struct sockaddr *) &sin, sizeof(sin)))
	return sock;

    close(sock);

    return -1;
}



static int
 dns_ipx(host)
char *host;
{
    querybuf ans;
    int n, ancount, qdcount;
    unsigned char *cp, *eom;
    char buf[MAXDNAME];
    struct sockaddr_in sin;
    int type;

    MYDOG;

    n = dns_query(host, T_A, &ans);
    if (n < 0)
	return n;

/* find first satisfactory answer */

    cp = (u_char *) & ans + sizeof(HEADER);
    eom = (u_char *) & ans + n;

    for (qdcount = ntohs(ans.hdr.qdcount); qdcount--; cp += n + QFIXEDSZ)
    {
	if ((n = dn_skipname(cp, eom)) < 0)
	    return n;
    }

    ancount = ntohs(ans.hdr.ancount);

    while (--ancount >= 0 && cp < eom)
    {
	if ((n = dn_expand((char *) &ans, eom, cp, buf, MAXDNAME)) < 0)
	    return -1;

	cp += n;

	type = getshort(cp);
	n = getshort(cp + 8);
	cp += 10;

	if (type == T_A)
	{
	    int sock;
	    ip_addr *ip;

	    sin.sin_family = AF_INET;
	    sin.sin_port = htons(SMTP_PORT);
	    memset(sin.sin_zero, 0, sizeof(sin.sin_zero));

	    ip = (ip_addr *) & (sin.sin_addr.s_addr);

	    ip->d[0] = cp[0];
	    ip->d[1] = cp[1];
	    ip->d[2] = cp[2];
	    ip->d[3] = cp[3];

	    sock = socket(AF_INET, SOCK_STREAM, 0);
	    if (sock < 0)
		return sock;

	    if (!connect(sock, (struct sockaddr *) &sin, sizeof(sin)))
		return sock;

	    close(sock);
	}

	cp += n;
    }

    return -1;
}


/* ----------------------------------------------------- */
/* SMTP routines                                         */
/* ----------------------------------------------------- */


static void
 smtp_close(sock)
int sock;
{
    tcp_command(sock, "QUIT\r\n", 221);
    shutdown(sock, 2);
    close(sock);
}

static int
 smtp_open(site)
char *site;
{
    int sock;
    char *str, *ptr, buf[256], mxlist[MAX_MXLIST];

/* ap_title("dns_mx"); */

    dns_mx(site, str = mxlist);

    if (!*str)
	str = site;

    for (;;)
    {
	ptr = str;
	while (*ptr)
	{
	    if (strchr("@#$%^&*()_+=-!;\"'/?><,|\\", *ptr))	/* Ptt: 慮掉錯誤的host */
		return -1;
	    if (*ptr == ':' || *ptr == ' ')
	    {
		*ptr++ = '\0';
		break;
	    }
	    ptr++;
	}
	printf("a:%s", str);
	sock = -1;

	if (!*str)
	    break;
    /* ap_title(str); */
	printf("b:%s", str);
	MYDOG;
	if ((sock = host_ipx(str)) < 0)
	{
	    printf("c:%s", str);
	    sock = dns_ipx(str);
	}
	printf("d:%s", str);
	if (sock >= 0)
	{
	/* ap_title("helo"); */
	    if (!tcp_command(sock, NULL, 220))
	    {
		if (!tcp_command(sock, "HELO " MYHOSTNAME "\r\n", 250))
		    break;
		sprintf(buf, "smtp_helo: %s (%s)", site, str);
#ifdef DEBUG
		printf("%s\n", buf);
#endif
		logit(buf);
	    }
	    printf("d:%s", str);
	    sprintf(buf, "smtp_conn: %s (%s)", site, str);
#ifdef DEBUG
	    printf("%s\n", buf);
#endif
	    logit(buf);

	    shutdown(sock, 2);
	    close(sock);
	}

	str = ptr;
    }
    return sock;
}

/* ----------------------------------------------------- */
/* SMTP deliver                                          */
/* ----------------------------------------------------- */
/* > 0 : sucess                                          */
/* = 0 : ignore it                                       */
/* -1 : SMTP host error, try it later                    */
/* -2 : withdraw back to user                            */
/* ----------------------------------------------------- */

static int
 smtp_send(sock, mq)
int sock;
MailQueue *mq;
{
    int fd, fsize, method, ch, len;
    char *fpath, *data, *ptr, *body, buf[512];
    struct stat st;

    method = mq->method;
    fpath = mq->filepath;

/* --------------------------------------------------- */
/* 身分認證信函                                        */
/* --------------------------------------------------- */

    if (method == MQ_JUSTIFY)
    {
	fpath = "etc/valid";

    /*archiv32(str_hash(mq->rcpt, mq->mailtime), buf); */
	sprintf(mq->subject, "[MapleBBS]To %s(%s) [VALID]", mq->sender, buf);
    }

#ifdef DEBUG
    printf("smtp send: %s -> %s \n", mq->sender, mq->rcpt);
#endif
/* --------------------------------------------------- */
/* 檢查 mail file                                      */
/* --------------------------------------------------- */

  file_open:

    fd = open(fpath, O_RDONLY);
    if (fd < 0)
    {
	goto file_error;
    }

    if (fstat(fd, &st) || !S_ISREG(st.st_mode) || (fsize = st.st_size) <= 0)
    {
	close(fd);

      file_error:

	if (method != MQ_JUSTIFY)
	    unlink(fpath);

	sprintf(buf, "file: %s", fpath);
	logit(buf);

	return 0;
    }

    ptr = buf;

/* --------------------------------------------------- */
/* uuencode file                                       */
/* --------------------------------------------------- */

    if (method & MQ_UUENCODE)
    {
	close(fd);

	unlink(MAIL_UUFILE);
	sprintf(ptr, "/bin/uuencode %s %s.uu > %s", fpath, mq->sender, MAIL_UUFILE);
	system(ptr);

	unlink(fpath);

	fpath = MAIL_UUFILE;
	method ^= MQ_UUENCODE;
	goto file_open;
    }

/* --------------------------------------------------- */
/* load file                                           */
/* --------------------------------------------------- */

    body = mpool;
    if (msize <= fsize)
    {
	len = fsize + (fsize >> 2);
	body = (char *) realloc(body, len);
	if (body == NULL)
	{
	    logit("err: realloc");
	    return -1;
	}
	mpool = body;
	msize = len;
    }

    fsize = read(fd, body, fsize);
    close(fd);
    if (fsize <= 0)
    {
	goto file_error;
    }

    mleng = fsize;
    body[fsize] = '\0';

/* --------------------------------------------------- */
/* setup "From "                                       */
/* --------------------------------------------------- */

    if (method == MQ_JUSTIFY)	/* 身分認證信函 */
	sprintf(ptr, "bbs@%s", MYHOSTNAME);
    else
	sprintf(ptr, "%s.bbs@%s", mq->sender, MYHOSTNAME);

/* --------------------------------------------------- */
/* negotiation with mail server                        */
/* --------------------------------------------------- */

    data = tcp_pool;

    sprintf(data, "MAIL FROM:<%s>\r\n", ptr);
    if (tcp_command(sock, data, 250))
	return -1;

    sprintf(data, "RCPT TO:<%s>\r\n", mq->rcpt);
    if (tcp_command(sock, data, 250))
    {
	tcp_command(sock, "RSET\r\n", 250);	/* reset SMTP host */

	sprintf(buf, "Error in RCPT: <%s> by <%s> [%s]",
		mq->rcpt, mq->sender, fpath);
	logit(buf);

	if (method != MQ_JUSTIFY)
	    unlink(fpath);

	return -2;
    }

/* ap_title("data"); */

    strcpy(data, "DATA\r\n");
    if (tcp_command(sock, data, 354))
    {
	tcp_command(sock, "RSET\r\n", 250);	/* reset SMTP host */

	return -1;
    }

/* --------------------------------------------------- */
/* begin of mail header                                */
/* --------------------------------------------------- */

    sprintf(data, "From: %s\r\nTo: %s\r\nSubject: %s\r\nX-Forwarded-By: %s (%s)\r\nX-Disclaimer: [%s] 對本信內容恕不負責\r\n\r\n",
	ptr, mq->rcpt, mq->subject, mq->sender, mq->username, BOARDNAME);
    tcp_pos = strlen(data);

    if (method == MQ_JUSTIFY)	/* 身分認證信函 */
    {
	ptr = data + tcp_pos;
	sprintf(ptr, " ID: %s (%s)  E-mail: %s\r\n\r\n",
		mq->sender, mq->username, mq->rcpt);
	tcp_pos += strlen(ptr);
    }

/* --------------------------------------------------- */
/* begin of mail body                                  */
/* --------------------------------------------------- */

    ptr = body;
    for (;;)
    {
	ch = *ptr;
	if (ch == '\n' || ch == '\r' || ch == '\0')
	{
	    len = ptr - body;
	    if (ch == '\0' && len == 0)
		break;

	    if (*body == '.' && len == 1)
		len = tcp_puts(sock, "..", 2);
	    else
		len = tcp_puts(sock, body, len);

	    if (len < 0)	/* server down or busy */
	    {
		logit("tcp_puts");
		return -3;
	    }

	    if (ch == '\0')
		break;

	    ptr++;
	    if (*ptr == '\n' && ch == '\r')
		ptr++;

	    body = ptr;
	}
	else
	{
	    ptr++;
	}
    }
    tcp_flush(sock);

    tcp_command(sock, NULL, 250);

    if (method != MQ_JUSTIFY)
	unlink(fpath);

/* --------------------------------------------------- */
/* 記錄寄信                                            */
/* --------------------------------------------------- */

    sprintf(buf, "%-13s%c> %s", mq->sender,
	    ((method == MQ_JUSTIFY) ? '=' : '-'), mq->rcpt);
    logit(buf);

    return fsize;
}


/* ----------------------------------------------------- */
/* drawback to user                                      */
/* ----------------------------------------------------- */

extern uhash_t *uhash;

static int
 send_local_user(mqueue)
MailQueue *mqueue;
{
    fileheader_t mhdr;
    char buf[512], direct[512], userid[IDLEN + 1];
    int fd = -1, uid;
    FILE *fp;


/* check if the userid is in our bbs now */
    strtok(mqueue->rcpt, ". ");
    strcpy(userid, mqueue->rcpt);
    if (!(uid = searchuser(userid)))
    {
	sprintf(buf, " BBS user <%s> .DIR file not existed ", userid);
	logit(buf);
	return -1;
    }
    strcpy(userid, uhash->userid[uid - 1]);
    sprintf(direct, BBSHOME "/home/%c/%s/.DIR", userid[0], userid);
    if (!dashf(direct))
    {
	sprintf(buf, " BBS user <%s> not existed ", userid);
	logit(buf);
	return -1;
    }

/* allocate a file for the new mail */

    sprintf(buf, BBSHOME "/home/%c/%s", userid[0], userid);
    stampfile(buf, &mhdr);
    strcpy(mhdr.owner, mqueue->sender);
    sprintf(mhdr.title, "%.60s", mqueue->subject);

/* copy the queue-file to the specified file */

    if ((fp = fopen(buf, "w")) == NULL)
    {
	logit("Err fdopen()");
	close(fd);
	return -1;
    }

    fprintf(fp, "作者: %s (%s) \n標題: %s\n時間: %s\n",
     mhdr.owner, mqueue->username, mhdr.title, ctime(&mqueue->mailtime));

    if (mleng)
    {
	fwrite(mpool, mleng, 1, fp);
    }
    else
    {
	FILE *fpr;
	int len;

	if((fpr = fopen(mqueue->filepath, "r")))
	{
	    while ((len = fread(mpool, 1, 4096, fpr)) > 0)
		fwrite(mpool, 1, len, fp);
	    fclose(fpr);
	}
#ifdef DEBUG
	else
	{
	    printf("no mail file %s\n", mqueue->filepath);
	}
#endif
	unlink(mqueue->filepath);
    }
    fclose(fp);
/* append the record to the MAIL control file */

    append_record(direct, &mhdr, sizeof(mhdr));
    sprintf(buf, "%s  -> %s  <BBS USER>",
	    mqueue->sender, mqueue->rcpt);
    logit(buf);
    return 0;
}


static int
 draw_back(mqueue)
MailQueue *mqueue;
{
    fileheader_t mhdr;
    char buf[512], userid[IDLEN + 1];
    int fd = -1, fx;
    FILE *fp;


/* check if the userid is in our bbs now */
    strcpy(userid, mqueue->sender);
    sprintf(buf, "/home/bbs/home/%c/%s/.DIR", userid[0], userid);
    fx = open(buf, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fx < 0)
    {
	sprintf(buf, " BBS user <%s> not existed ", userid);
	logit(buf);
	return -1;
    }

/* allocate a file for the new mail */

    sprintf(buf, BBSHOME "/home/%c/%s", userid[0], userid);
    stampfile(buf, &mhdr);
    strcpy(mhdr.owner, "[Ptt郵差]");
    sprintf(mhdr.title, "退信通知：%.60s", mqueue->subject);

/* copy the queue-file to the specified file */

    if ((fp = fopen(buf, "w")) == NULL)
    {
	logit("Err fdopen()");
	close(fx);
	close(fd);
	return -1;
    }

    fprintf(fp, "作者: SYSOP (Ptt郵差)\n標題: %s\n時間: %s\n",
	    mhdr.title, mhdr.date);

    fprintf(fp, "%s (%s) 您好：\n\n", mqueue->sender, mqueue->username);
    fprintf(fp, "台端在 [%s] 寄出的信件，標題是：\n[%s]\n收信人是：<%s>\n"
	    "無法順利寄達。可能之常見原因是 E-mail address 打錯，或是\n"
	  "網路壅塞、對方機器無法收信等。特將原信退回如下，希請查照。\n",
	    ctime(&mqueue->mailtime), mqueue->subject, mqueue->rcpt);

    if (smtp_reply[0])
    {
	fprintf(fp, "\nPS. %s\n", smtp_reply);
	smtp_reply[0] = '\0';
    }

    fprintf(fp, "--\n\n");

    if (mleng)
    {
	fwrite(mpool, mleng, 1, fp);
    }
    else
    {
	FILE *fpr;
	int len;

	if((fpr = fopen(mqueue->filepath, "r"))) {
	    while ((len = fread(mpool, 1, 4096, fpr)) > 0)
		fwrite(mpool, 1, len, fp);
	    fclose(fpr);
	}
	unlink(mqueue->filepath);
    }
    fclose(fp);

/* append the record to the MAIL control file */

    write(fx, &mhdr, sizeof(mhdr));
    close(fx);
    sprintf(buf, "back: [%s] <%s> %s",
	    mqueue->sender, mqueue->rcpt, mqueue->subject);
    logit(buf);
    return 0;
}


/* ----------------------------------------------------- */
/* mail queue                                            */
/* ----------------------------------------------------- */


static int
 mq_cmp(x, y)
MailQueue *x, *y;
{
    int cmp;

    cmp = strcmp(x->niamod, y->niamod);
    if (!cmp)
	cmp = y->mailtime - x->mailtime;
    return cmp;
}


static void
 smtp()
{
    int cc, xmit;
    int count, bytes, back, qfile, servercc = -1;
    MailQueue *mq_pool;
    MailQueue *mq;
    char *host = NULL;
    char *currhost = NULL;
    struct stat st;

    static char str_null[] = "\0x7f";
    cc = open(MAIL_XMIT, O_RDONLY);
    if (cc < 0)
	return;

    if (fstat(cc, &st) || (count = st.st_size) <= 0)
    {
	close(cc);
	return;
    }

/* ap_title("smtp"); */

    mq_pool = (MailQueue *) malloc(count);
    count = read(cc, mq_pool, count) / sizeof(MailQueue);
    close(cc);

/* ap_title("open"); */

    if (count > 0)
    {
	mq = mq_pool + count;

    /* locate mail host */

	while (--mq >= mq_pool)
	{
	    char *niamod, buf[128];

	    host = mq->rcpt;
#ifdef DEBUG
	    printf("%s -> %s\n", mq->sender, mq->rcpt);
#endif
	    for (;;)
	    {
		cc = *host;

		if (cc == '\0')
		{
		    bytes = 0;
		    niamod = str_null;
		    break;
		}

		host++;

		if (cc == '@')
		{
		    bytes = host - mq->rcpt;
		    niamod = buf + sizeof(buf) - 1;

		    *niamod = '\0';
		    for (;;)
		    {
			cc = *host;
			if (!cc)
			{
			    niamod = GetStrTab(niamod);
			    break;
			}
			if (cc >= 'A' && cc <= 'Z')
			    cc += 0x20;
			*--niamod = cc;
			host++;
		    }
		    break;
		}
	    }
	    mq->filepath[76] = bytes;
	    mq->niamod = niamod;	/* reverse domain */
	}

    /* sort mail queue according to mail host */

    /* ap_title("sort"); */

	if (count > 1)
	    qsort(mq_pool, count, sizeof(MailQueue), mq_cmp);

    /* deliver the mails */

	MYDOG;
	ap_title("send");

	mq = mq_pool + count;

	cc = qfile = -1;
	back = count = bytes = 0;
	currhost = str_null;	/* something impossible */

	while (--mq >= mq_pool)
	{
	/* ap_title("queue"); */

	    mleng = 0;
	    xmit = mq->filepath[76];

	    if (xmit <= 0 ||
#ifdef BBS_HOST_IS_SMTP_HOST
		!strcasecmp(mq->rcpt + xmit, SMTP_HOST)
#endif
		)
	    {
#ifndef BBS_HOST_IS_SMTP_HOST
		logit("null host");
		strcpy(smtp_reply, "Error in E-mail address");
		draw_back(mq);
		continue;
#else
		send_local_user(mq);
		continue;
#endif
	    }
	    else
	    {
		host = mq->rcpt + xmit;
	    }

#ifdef DEBUG
	    printf("sm: (%s , %s) host strcasecmp \n", host, currhost);
#endif

	    if (strcasecmp(host, currhost))

	    {
#ifdef DEBUG
		printf("sm: %s is new, open new host\n", host);
#endif

		if (servercc >= 0)
		    smtp_close(servercc);

		MYDOG;
		currhost = host;
#ifdef  HAVE_TIMEOUT
		signal(SIGALRM, ap_timeout);
		alarm(SMTP_TIMEOUT);
		if (!setjmp(ap_jmp))
		    servercc = smtp_open(currhost);
		else
		    servercc = -1;
		alarm(0);
#endif
		if (servercc < 0)
		{
		    sprintf(mpool, "smtp_host: %s", currhost);
		    logit(mpool);

		    if (!smtp_reply[0])
			strcpy(smtp_reply, mpool);
		}
	    }
#ifdef DEBUG
	    printf("sm: %s -> %s servercc = [%d]\n", mq->sender, mq->rcpt, servercc);
#endif
	    if (servercc < 0)
	    {
		draw_back(mq);
		back++;
		continue;
	    }

	/* ap_title("xmit"); */

	    MYDOG;

#ifdef  HAVE_TIMEOUT
	    signal(SIGALRM, ap_timeout);
	    alarm(SEND_TIMEOUT);
	    if (!setjmp(ap_jmp))
		xmit = smtp_send(servercc, mq);
	    else
		xmit = -1;
	    alarm(0);
#endif

	    if (xmit > 0)
	    {
		bytes += xmit;
		count++;
	    }
	    else if (xmit < 0)
	    {
		if (xmit == -3)	/* server side error, drop it */
		{
		    smtp_close(servercc);
		    servercc = -1;
		}

		if (xmit != -2)
		{
		    if (qfile < 0)
			qfile = open(MAIL_BACK, O_WRONLY | O_CREAT | O_APPEND, 0600);
		    write(qfile, mq, sizeof(MailQueue));
		}

		draw_back(mq);
		back++;
	    }

	}
#ifdef DEBUG
	printf("sm: %s -> %s smtp complete\n", mq->sender, mq->rcpt);
#endif

	if (servercc >= 0)
	    smtp_close(servercc);

#ifdef DEBUG
	printf("sm: %s -> %s close qfile\n", mq->sender, mq->rcpt);
#endif

	if (qfile >= 0)
	    close(qfile);

	sprintf(mpool, "send: %d, byte: %d, back:%d\n",
		count, bytes, back);
	logit(mpool);
	fflush(flog);
    }

/* ap_title("free"); */

    unlink(MAIL_XMIT);
    free(mq_pool);

    ap_title("ready");
}


/* ----------------------------------------------------- */
/* signal routines                                       */
/* ----------------------------------------------------- */


static void
 sig_term()
{
    logit("TERMINATE");
    fclose(flog);
    exit(0);
}


static void
 sig_trap()
{
#ifdef DEBUG
    printf("signal kill\n");
#endif
    server_usage();
    logit("ABORT");
    fclose(flog);
    rename(MAIL_LOGFILE, MAIL_DBGFILE);
    exit(0);
}


static void
 main_signals()
{
    struct sigaction act;

    bzero(&act, sizeof(act));
    sigblock(sigmask(SIGPIPE));
    act.sa_handler = sig_term;
    sigaction(SIGTERM, &act, NULL);

    act.sa_handler = sig_trap;
    sigaction(SIGBUS, &act, NULL);
    sigaction(SIGSEGV, &act, NULL);

#ifdef  SERVER_USAGE
    act.sa_handler = server_usage;
    sigaction(SIGPROF, &act, NULL);
#endif
}


/* ----------------------------------------------------- */
/* main routines                                         */
/* ----------------------------------------------------- */


int main(argc, argv, envp)
int argc;
char **argv;
char **envp;
{
    time_t tick, tnow, texp;
    check_pidfile();
#ifndef DEBUG
    close(0);
    close(1);
    close(2);
#endif
    if (fork())
	exit(0);

    setsid();

    if (fork())
	exit(0);

    setgid(BBSGID);
    setuid(BBSUID);
    chdir(BBSHOME);
    umask(077);

#ifdef	HAVE_PROC_TITLE
/* --------------------------------------------------- */
/* set up proctitle                                    */
/* --------------------------------------------------- */

    for (tick = 0; envp[tick]; tick++)
	;
    if (tick > 0)
	str = envp[tick - 1];
    else
	str = argv[argc - 1];
    main_trail = str + strlen(str);

    str = argv[0];
    memcpy(str, SMTP_BANNER, sizeof(SMTP_BANNER) - 1);
    main_title = str + sizeof(SMTP_BANNER) - 1;
#endif
    ap_title("init");

    mpool = (char *) malloc(msize = MAIL_POOLSIZE);

    log_init();

    main_signals();

    dns_init();

    smtp();

    tick = 0;
    texp = time(0) + 50 * 60;

    for (;;)
    {
	tnow = time(0);
	if (tnow < tick)
	{
	    if (tnow > texp)
	    {
		ExpStrTab();	/* expire domain's string table */
		texp = tnow + 50 * 60;
	    }

	    sleep(tick - tnow);
	    tnow = tick;
	}
	tick = tnow + SMTP_PERIOD;

	if (!rename(BBSHOME "/out/.DIR", MAIL_XMIT))
	    smtp();
    }
}
