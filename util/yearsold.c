/* target : 闹参璸                                 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include "config.h"
#include "struct.h"

#define DOTPASSWDS "/home/bbs/.PASSWDS"

#define MAX_LINE        16

struct userec_t cuser;

void
 outs(fp, buf, mode)
FILE *fp;
char buf[], mode;
{
    static char state = '0';

    if (state != mode)
	fprintf(fp, "[3%cm", state = mode);
    if (buf[0])
    {
	fprintf(fp, buf);
	buf[0] = 0;
    }
}

int main()
{
    int i, j;
    char buf[256];
    FILE *fp;
    int year, max, item, maxyear;
    long totalyear;
    int c = 0;
    int act[25];
    time_t now;
    struct tm *ptime;

    now = time(NULL);
    ptime = localtime(&now);

    fp = fopen(DOTPASSWDS, "r");
    if (!fp)
	printf("unable to open file %s", DOTPASSWDS);
    memset(act, 0, sizeof(act));
    while ((fread(&cuser, sizeof(cuser), 1, fp)) > 0)
    {
	printf("[%d]", c++);
	if (((ptime->tm_year - cuser.year) < 10) || ((ptime->tm_year - cuser.year) >
						     33))
	    continue;

	act[ptime->tm_year - cuser.year - 10]++;
	act[24]++;
	printf("[%d]", c++);
    }
    fclose(fp);


    for (i = max = totalyear = maxyear = 0; i < 24; i++)
    {
	totalyear += act[i] * (i + 10);
	if (act[i] > max)
	{
	    max = act[i];
	    maxyear = i;
	}
    }

    item = max / MAX_LINE + 1;

    if ((fp = fopen("etc/yearsold", "w")) == NULL)
    {
	printf("cann't open etc/yearsold\n");
	return 1;
    }

    fprintf(fp, "\t\t\t [1;33;45m у金金 闹参璸 [%02d/%02d/%02d] [40m\n\n",
	    ptime->tm_year % 100, ptime->tm_mon, ptime->tm_mday);
    for (i = MAX_LINE + 1; i > 0; i--)
    {
	strcpy(buf, "   ");
	for (j = 0; j < 24; j++)
	{
	    max = item * i;
	    year = act[j];
	    if (year && (max > year) && (max - item <= year))
	    {
		outs(fp, buf, '7');
		fprintf(fp, "%-3d", year);
	    }
	    else if (max <= year)
	    {
		outs(fp, buf, '4');
		fprintf(fp, " ");
	    }
	    else
		strcat(buf, "   ");
	}
	fprintf(fp, "\n");
    }


    fprintf(fp, "   [32m"
	    "10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33\n\n"
	    "\t\t      [36mΤ参璸Ω[37m%-9d[36mキА闹[37m%d[40;0m\n"
	    ,act[24], (int)totalyear / act[24]);
    fclose(fp);
    return 0;
}
