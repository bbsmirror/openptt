/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"
#include "pttstruct.h"
#include "util.h"

#define MAX_LINE        16
#define ADJUST_M        6	/* adjust back 5 minutes */

extern struct pttcache_t *ptt;

void
 reset_garbage()
{
    if (ptt == NULL)
    {
	ptt = attach_shm(PTTSHM_KEY, sizeof(*ptt));
	if (ptt->touchtime == 0)
	    ptt->touchtime = 1;
    }

/* ¤£¾ã­Óreload?
   for(n=0;n<=ptt->max_film;n++)
   printf("\n**%d**\n %s \n",n,ptt->notes[n]);
 */
    ptt->uptime = 0;
    reload_pttcache();

    printf("\n°ÊºA¬ÝªO¼Æ[%d]\n", ptt->max_film);
/*
  for(n=0; n<MAX_MOVIE_SECTION; n++)
  printf("sec%d=> °_ÂI:%d ¤U¦¸­n´«ªº:%d\n ",n,ptt->n_notes[n],
  ptt->next_refresh[n]);
  printf("\n");         
*/
}

void
keeplog(fpath, board, title)
    char *fpath;
    char *board;
    char *title;
{
    fileheader_t fhdr;
    char genbuf[256], buf[256];

    if (!board)
	board = "Record";


    sprintf(genbuf, "boards/%c/%s", *board, board);
    stampfile(genbuf, &fhdr);
    sprintf(buf, "mv %s %s", fpath, genbuf);
    system(buf);
/*
  printf("keep record:[%s][%s][%s][%s]\n",fpath, board, title,genbuf);
*/
    strcpy(fhdr.title, title);
    strcpy(fhdr.owner, "[¾ú¥v¦Ñ®v]");
    sprintf(genbuf, "boards/%c/%s/.DIR", *board, board);
    append_record(genbuf, &fhdr, sizeof(fhdr));
}


static void
my_outs(fp, buf, mode)
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


void gzip(source, target, stamp)
    char *source, *target, *stamp;
{
    char buf[128];
    sprintf(buf, "gzip -f9n adm/%s%s", target, stamp);
    rename(source, &buf[14]);
    system(buf);
}

extern struct fromcache_t *fcache;
extern uhash_t *uhash;

int main() {
    int hour, max, item, total, i, j, mo, da, max_user = 0,
	mahour = 0;
    char *act_file = ".act";
    char *log_file = "usies";
    char buf[256], buf1[256], *p;
    FILE *fp;
    int act[27];		/* ¦¸¼Æ/²Ö­p®É¶¡/pointer */
    time_t now;
    struct tm *ptime;

    now = time(NULL) - ADJUST_M * 60;	/* back to ancent */
    ptime = localtime(&now);

    memset(act, 0, sizeof(act));
    printf("¦¸¼Æ/²Ö­p®É¶¡\n");
    if ((ptime->tm_hour != 0) && (fp = fopen(act_file, "r")))
    {
	fread(act, sizeof(act), 1, fp);
	fclose(fp);
    }
    if ((fp = fopen(log_file, "r")) == NULL)
    {
	printf("cann't open usies\n");
	return 1;
    }
    if (act[26])
	fseek(fp, act[26], 0);
    while (fgets(buf, 256, fp))
    {
	buf[11+2]=0;
	hour = atoi(buf + 11);
	if (hour < 0 || hour > 23)
	{
	    continue;
	}
//"09/06/1999 17:44:58 Mon "
// 012345678901234567890123
	if (strstr(buf + 20, "ENTER"))
	{
	    act[hour]++;
	    continue;
	}
	if ((p = (char *) strstr(buf + 40, "Stay:")))
	{
	    if((hour = atoi(p + 5))) {
		act[24] += hour;
		act[25]++;
	    }
	    continue;
	}
    }
    act[26] = ftell(fp);
    fclose(fp);
    for (i = max = total = 0; i < 24; i++)
    {
	total += act[i];
	if (act[i] > max)
	{
	    max_user = max = act[i];
	    mahour = i;
	}
    }
    item = max / MAX_LINE + 1;

    if (!ptime->tm_hour)
    {
	keeplog("etc/today", "Record", "¤W¯¸¤H¦¸²Î­p");
	keeplog("etc/money", "Security", "¥»¤éª÷¿ú©¹¨Ó°O¿ý");
	keeplog("etc/illegal_money", "Security", "¥»¤é¹HªkÁÈ¿ú°O¿ý");
	keeplog("etc/chicken", "Record", "Âû³õ³ø§i");
    }

    printf("¤W¯¸¤H¦¸²Î­p\n");
    if ((fp = fopen("etc/today", "w")) == NULL)
    {
	printf("cann't open etc/today\n");
	return 1;
    }
    fprintf(fp, "\t\t\t[1;33;46m ¨C¤p®É¤W¯¸¤H¦¸²Î­p [%02d/%02d/%02d] [40m\n\n", ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday);
    for (i = MAX_LINE + 1; i > 0; i--)
    {
	strcpy(buf, "   ");
	for (j = 0; j < 24; j++)
	{
	    max = item * i;
	    hour = act[j];
	    if (hour && (max > hour) && (max - item <= hour))
	    {
		my_outs(fp, buf, '3');
		fprintf(fp, "%-3d", hour / 10);
	    }
	    else if (max <= hour)
	    {
		my_outs(fp, buf, '4');
		fprintf(fp, "¢i ");
	    }
	    else
		strcat(buf, "   ");
	}
	fprintf(fp, "\n");
    }
    fprintf(fp, "   [32m"
	    "0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23\n\n"
	    "\t      [34m³æ¦ì: [37m10[34m ¤H");
    fprintf(fp, "  Á`¦@¤W¯¸¤H¦¸¡G[37m%-7d[34m¥­§¡¨Ï¥Î¤H¼Æ¡G[37m%d\n", total, total / 24);
    fclose(fp);

    if((fp = fopen(act_file, "w"))) {
	fwrite(act, sizeof(act), 1, fp);
	fclose(fp);
    }

/* -------------------------------------------------------------- */

    sprintf(buf, "-%02d%02d%02d",
	    ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday);

    now += ADJUST_M * 60;	/* back to future */


    ptime = localtime(&now);

    if (!ptime->tm_hour)
    {
	keeplog(".note", "Record", "¤ß±¡¯d¨¥ª©");
	system("/bin/cp etc/today etc/yesterday");
/*    system("rm -f note.dat"); */
/* Ptt */
	sprintf(buf1, "[¤½¦w³ø§i] ¨Ï¥ÎªÌ¤W½uºÊ±± [%02d/%02d:%02d]"
		,ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour);
	keeplog("usies", "Security", buf1);
	printf("[¤½¦w³ø§i] ¨Ï¥ÎªÌ¤W½uºÊ±±\n");
	gzip(log_file, "usies", buf);
	printf("À£ÁY¨Ï¥ÎªÌ¤W½uºÊ±±\n");
/* Ptt ¾ú¥v¨Æ¥ó³B²z */
	now = time(NULL) - ADJUST_M * 60;	/* back to ancent */
	ptime = localtime(&now);

	attach_uhash();
	now += ADJUST_M * 60;	/* back to future */
	ptime = localtime(&now);

	/* Ptt Åwªïµe­±³B²z */
	printf("Åwªïµe­±³B²z\n");

	if((fp = fopen("etc/Welcome.date", "r")))
	{
	    char temp[50];
	    while (fscanf(fp, "%d %d %s\n", &mo, &da, buf1) != EOF)
	    {
		if (ptime->tm_mday == da && ptime->tm_mon + 1 == mo)
		{
		    strcpy(temp, buf1);
		    sprintf(buf1, "cp -f etc/Welcomes/%s etc/Welcome", temp);
		    system(buf1);
		    break;
		}
	    }
	    fclose(fp);
	}
	printf("Åwªïµe­±³B²z\n");
	if (ptime->tm_wday == 0)
	{
	    keeplog("etc/week", "Record", "¥»¶g¼öªù¸ÜÃD");

	    gzip("bbslog", "bntplink", buf);
	    gzip("innd/bbslog", "innbbsd", buf);
	    gzip("etc/mailog", "mailog", buf);
	}

	if (ptime->tm_mday == 1)
	    keeplog("etc/month", "Record", "¥»¤ë¼öªù¸ÜÃD");

	if (ptime->tm_yday == 1)
	    keeplog("etc/year", "Record", "¦~«×¼öªù¸ÜÃD");
    }
    else if (ptime->tm_hour == 3 && ptime->tm_wday == 6)
    {
	char *fn1 = "tmp";
	char *fn2 = "suicide";
	rename(fn1, fn2);
	mkdir(fn1, 0755);
	sprintf(buf, "tar cfz adm/%s-%02d%02d%02d.tgz %s",
		fn2, ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday, fn2);
	system(buf);
	sprintf(buf, "/bin/rm -fr %s", fn2);
	system(buf);
    }
/* Ptt reset Ptt's share memory */
    printf("­«³]Pttcache »Pfcache\n");

    reset_garbage();
    return 0;
}
