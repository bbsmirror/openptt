#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include "config.h"
#include "struct.h"


#define  PASSWD "/home/bbs/.PASSWDS"
#define  OUTPASS "PASSWDS.NEW"
char yn[10] = "";
int coun = 0, count;
userec_t cuser;

int
 bad_user_id()
{
    register char ch;
    int j;

    if (strlen(cuser.userid) < 2 || !isalpha(cuser.userid[0]))
	return 1;

    if (cuser.numlogins == 0 || cuser.numlogins > 150000)
	return 1;

    if (cuser.numposts > 150000)
	return 1;
/*
   if (!strcasecmp(cuser.userid, "new"))
   return 1;
 */

    for (j = 1; (ch = cuser.userid[j]); j++)
    {
	if (!isalnum(ch))
	    return 1;
    }
    return 0;
}

char *
 Cdate(clock)
time_t *clock;
{
    static char foo[22];
    struct tm *mytm = localtime(clock);

    strftime(foo, 22, "%D %T %a", mytm);
    return (foo);
}

int main()
{
    userec_t new;
    userec_t blank;
    FILE *fp1 = fopen(PASSWD, "r");
    FILE *fp2 = fopen(OUTPASS, "w");

    memset(&blank, 0, sizeof(blank));

    printf("size [%d] to size [%d]\n", sizeof(userec_t), sizeof(new));
    if (sizeof(new) != sizeof(userec_t))
	return 0;
    while ((fread(&cuser, sizeof(cuser), 1, fp1)) > 0)
    {

	memset(&new, 0, sizeof(new));
	memcpy(&new, &cuser, sizeof(new) - sizeof(new.pad));

	coun++;
	if (!(coun % 1000))
	    printf("%d.. \n", coun);

	if (coun > 30000 && count > 100)
	{
	    printf("不接受此後所有資料\n");
	    break;
	}
	else if (bad_user_id())
	{
	    fwrite(&blank, sizeof(blank), 1, fp2);
	    count++;
	}
	else
	{
	    count = 0;
	    fwrite(&new, sizeof(new), 1, fp2);
	}
    }
    fclose(fp1);
    fclose(fp2);
    return 0;
}
