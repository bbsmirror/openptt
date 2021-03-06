/* $Id$ */
/* 使用者 上站記錄/文章篇數 排行榜 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "config.h"
#include "pttstruct.h"
#include "perm.h"
#include "common.h"
#include "util.h"

#define REAL_INFO
struct manrec
{
    char userid[IDLEN + 1];
    char username[23];
    ushort numlogins;
    ushort numposts;
};
typedef struct manrec manrec;
struct manrec allman[MAX_USERS];

userec_t aman;
int num;
FILE *fp;

#define TYPE_POST       0
#define TYPE_LOGIN      1

int
 login_cmp(b, a)
struct manrec *a, *b;
{
    return (a->numlogins - b->numlogins);
}


int
 post_cmp(b, a)
struct manrec *a, *b;
{
    return (a->numposts - b->numposts);
}

void
 top(type)
{
    static char *str_type[3] =
    {"發表次數", "進站次數", " 大富翁 "};
    int i, j, rows = (num + 1) / 2;
    char buf1[80], buf2[80];

    if (type != 2)
	fprintf(fp, "\n\n");

    fprintf(fp, "\
[1;36m  ╭─────╮           [%dm    %8.8s排行榜    [36;40m               ╭─────╮[m\n\
[1;36m  名次─代號───暱稱──────數目──名次─代號───暱稱──────數目[m\
", type + 44, str_type[type]);
    for (i = 0; i < rows; i++)
    {
	sprintf(buf1, "[%2d] %-11.11s%-16.16s%5d ",
		i + 1, allman[i].userid, allman[i].username,
		(int)(type == 1 ? allman[i].numlogins : allman[i].numposts));
	j = i + rows;
	sprintf(buf2, "[%2d] %-11.11s%-16.16s%4d ",
		j + 1, allman[j].userid, allman[j].username,
		(int)(type == 1 ? allman[j].numlogins : allman[j].numposts));
	if (i < 3)
	    fprintf(fp, "\n [1;%dm%-40s[0;37m%s", 31 + i, buf1, buf2);
	else
	    fprintf(fp, "\n %-40s%s", buf1, buf2);
    }
}


#ifdef  HAVE_TIN
int
 post_in_tin(char *name)
{
    char buf[256];
    FILE *fh;
    int counter = 0;

    sprintf(buf, "%s/home/%c/%s/.tin/posted", home_path, name[0], name);
    fh = fopen(buf, "r");
    if (fh == NULL)
	return 0;
    else
    {
	while (fgets(buf, 255, fh) != NULL)
	    counter++;
	fclose(fh);
	return counter;
    }
}
#endif				/* HAVE_TIN */
int
 not_alpha(ch)
register char ch;
{
    return (ch < 'A' || (ch > 'Z' && ch < 'a') || ch > 'z');
}

int
 not_alnum(ch)
register char ch;
{
    return (ch < '0' || (ch > '9' && ch < 'A') ||
	    (ch > 'Z' && ch < 'a') || ch > 'z');
}

int
 bad_user_id(userid)
char *userid;
{
    register char ch;
    if (strlen(userid) < 2)
	return 1;
    if (not_alpha(*userid))
	return 1;
    while((ch = *(++userid)))
    {
	if (not_alnum(ch))
	    return 1;
    }
    return 0;
}

int main(argc, argv)
int argc;
char **argv;
{
    int i, j;

    if (argc < 3)
    {
	printf("Usage: %s <num_top> <out-file>\n", argv[0]);
	exit(1);
    }

    num = atoi(argv[1]);
    if (num == 0)
	num = 30;

    if(passwd_mmap())
    {
	printf("Sorry, the data is not ready.\n");
	exit(0);
    }
    
    for(i = 0, j = 1; j <= MAX_USERS; j++) {
	passwd_query(j, &aman);
	if((aman.userlevel & PERM_NOTOP) || bad_user_id(aman.userid) ||
	   strchr(aman.userid, '.'))
	    continue;
	else {
	    strcpy(allman[i].userid, aman.userid);
	    strncpy(allman[i].username, aman.username, 23);
	    allman[i].numlogins = aman.numlogins;
	    allman[i].numposts = aman.numposts;
	    i++;
	}
    }
    
    if ((fp = fopen(argv[2], "w")) == NULL)
    {
	printf("cann't open topusr\n");
	return 0;
    }

    qsort(allman, i, sizeof(manrec), post_cmp);
    top(TYPE_POST);

    qsort(allman, i, sizeof(manrec), login_cmp);
    top(TYPE_LOGIN);

    fclose(fp);
    return 0;
}
