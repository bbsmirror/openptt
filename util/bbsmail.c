/* $Id$ */

#define _BBS_UTIL_C_
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "config.h"
#include "pttstruct.h"
#include "util.h"
#include "common.h"

#define	LOG_FILE	(BBSHOME "/etc/mailog")

#ifdef HMM_USE_ANTI_SPAM
extern char *notitle[], *nofrom[], *nocont[];
#endif

void
mailog(msg)
    char *msg;
{
    FILE *fp;

    if ((fp = fopen(LOG_FILE, "a")))
    {
	time_t now;
	struct tm *p;

	time(&now);
	p = localtime(&now);
	fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d <bbsmail> %s\n",
		p->tm_year % 100, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec,
		msg);
	fclose(fp);
    }
}


int mail2bbs(char *userid) {
    fileheader_t mymail;
    char genbuf[256], title[TTLEN], sender[80], filename[80], *ip, *ptr;
    char msgid[80];
    time_t tmp_time;
    struct stat st;
    FILE *fout;

    /* check if the userid is in our bbs now */
    if(!searchuser(userid)) {
	snprintf(genbuf, sizeof(genbuf), "BBS user <%s> not existed", userid);
	puts(genbuf);
	mailog(genbuf);
	return -1;//EX_NOUSER;
    }
    
    snprintf(filename, sizeof(filename), BBSHOME "/home/%c/%s", userid[0], userid);
    
    if(stat(filename, &st) == -1) {
	if(mkdir(filename, 0755) == -1) {
	    printf("mail box create error %s \n", filename);
	    return -1;
	}
    } else if(!(st.st_mode & S_IFDIR)) {
	printf("mail box error\n");
	return -1;
    }
    
    printf("dir: %s\n", filename);
    
    /* allocate a file for the new mail */
    
    stampfile(filename, &mymail);
    printf("file: %s\n", filename);
    
    /* copy the stdin to the specified file */
    
    /* parse header */
    sender[0] = title[0] = msgid[0] = '\0';
    while(fgets(genbuf, 255, stdin)) {
	if(strncmp(genbuf, "From", 4) == 0) {
	    if((ip = strchr(genbuf, '<')) && (ptr = strrchr(ip, '>'))) {
		*ptr = '\0';
		if(ip[-1] == ' ')
		    ip[-1] = '\0';
		ptr = (char *) strchr(genbuf, ' ');
		while(*++ptr == ' ');
		snprintf(sender, sizeof(sender), "%s (%s)", ip + 1, ptr);
	    } else {
		strtok(genbuf, " \t\n\r");
		strncpy(sender, strtok(NULL, " \t\n\r"), sizeof(sender));
		sender[sizeof(sender) - 1] = '\0';
	    }
	} else if(strncmp(genbuf, "Subject: ", 9) == 0) {
	    strncpy(title, genbuf + 9, sizeof(title));
	    title[sizeof(title) - 1] = '\0';
	} else if(strncasecmp(genbuf, "Message-ID: ", 12) == 0) {
	    strncpy(msgid, genbuf + 12, sizeof(msgid));
	    msgid[sizeof(msgid) - 1] = 0;
	} else if(genbuf[0] == '\n')
	    break;
    }
    
    if((ptr = strchr(sender, '\n')) != NULL)
	*ptr = '\0';
    
    if((ptr = strchr(title, '\n')) != NULL)
	*ptr = '\0';
    
    if((ptr = strchr(msgid, '\n')) != NULL)
	*ptr = '\0';
    
    if((ptr = strchr(sender, '@')) == NULL) { /* 由 local host 寄信 */
	strncpy(ptr, "@" MYHOSTNAME, sizeof(sender) - (ptr - sender));
	sender[sizeof(sender) - 1] = '\0';
    }
    
    time(&tmp_time);
    
    if((fout = fopen(filename, "w")) == NULL) {
	printf("Cannot open %s\n", filename);
	return -1;
    }
    
    if(title[0] == '\0')
	snprintf(title, sizeof(title), "來自 %s", sender);
    
    if(sender[0])
	fprintf(fout, "作者: %s\n", sender);
    if(title[0])
	fprintf(fout, "標題: %s\n", title);
    
    strtok(strcpy(genbuf, ctime(&tmp_time)), "\n");
    fprintf(fout, "時間: %s\n", genbuf);
    
    if(msgid[0])
	fprintf(fout, "\33[32m識別: %s\33[m\n", msgid);
    
    fprintf(fout, "\n");
    
    while(fgets(genbuf, 255, stdin))
	fputs(genbuf, fout);
    fclose(fout);
    
    sprintf(genbuf, "%s => %s", sender, userid);
    mailog(genbuf);
    
    /* append the record to the MAIL control file */
    
    strcpy(mymail.title, title);
    
    if(strtok(sender, " .@\t\n\r"))
	strcat(sender, ".");
    sender[IDLEN + 1] = '\0';
    strcpy(mymail.owner, sender);
    
    sprintf(genbuf, BBSHOME "/home/%c/%s/.DIR", userid[0], userid);
    return append_record(genbuf, &mymail, sizeof(mymail));
}

int
main(int argc, char* argv[])
{
    char receiver[256];

/* argv[1] is userid in bbs   */

    if (argc < 2)
    {
	printf("Usage:\t%s <bbs_uid>\n", argv[0]);
	exit(-1);
    }
    (void) setgid(BBSGID);
    (void) setuid(BBSUID);

    strcpy(receiver, argv[1]);

    if (mail2bbs(receiver))
    {
	/* eat mail queue */
	while (fgets(receiver, sizeof(receiver), stdin)) ;
    }
    return 0;
}
