/* µo²¼¶}¼ú¤pµ{¦¡ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "config.h"
#include "struct.h"
#include "util.h"

#define VICE_COUNT "/home/bbs/etc/vice.count"
#define VICE_SHOW  "/home/bbs/etc/vice.show"
#define VICE_BINGO "/home/bbs/etc/vice.bingo"
#define VICE_BASE  "/home/bbs/etc/vice.base"
#define VICE_NEW   "vice.new"
#define VICE_DATA  "vice.data"
#define MAX_BINGO  99999999

int main()
{
    char TABLE[5][3] =
    {"¤@", "¤G", "¤T", "¥|", "¤­"}, buf0[256], buf1[256], *ptr, id[16];

    int i = 0, bingo, base = 0;


    FILE *fp = fopen(VICE_SHOW, "w"), *fb = fopen(VICE_BINGO, "w"),
    *fbe = fopen(VICE_BASE, "r+"), *fc = fopen(VICE_COUNT, "r");

    extern struct utmpfile_t *utmpshm;
    resolve_utmp();

    srand(utmpshm->number);

    if (!fp || !fb || !fbe)
	perror("error open file");

    fgets(buf0, 9, fbe);
    if (buf0)
	base = atoi(buf0);

    printf("base to use:%d\n", base);

    bingo = (base + rand()) % MAX_BINGO;
    fprintf(fp, "%1c[1;33m²Î¤@µo²¼¤¤¼ú¸¹½X[m\n", ' ');
    fprintf(fp, "%1c[1;37m================[m\n", ' ');
    fprintf(fp, "%1c[1;31m¯S§O¼ú[m: [1;31m%08d[m\n\n", ' ', bingo);
    fprintf(fb, "%d\n", bingo);

    while (i < 5)
    {
	bingo = (base + rand()) % MAX_BINGO;
	fprintf(fp, "%1c[1;36m²Ä%s¼ú[m: [1;37m%08d[m\n", ' ', TABLE[i], bingo);
	fprintf(fb, "%08d\n", bingo);
	i++;
    }
    fclose(fp);
    fclose(fb);
    while (fgets(id, 15, fc))
    {
	if (id[0] == '\n')
	    continue;
	if ((ptr = strchr(id, '\n')))
	    *ptr = 0;
	if (id[0] == ' ')
	    continue;
	sprintf(buf0, BBSHOME "/home/%c/%s/%s", id[0], id, VICE_NEW);
	sprintf(buf1, BBSHOME "/home/%c/%s/%s", id[0], id, VICE_DATA);
	printf("%s\n%s\n", buf0, buf1);
	rename(buf0, buf1);
	unlink(buf0);
    }
    base = rand() % MAX_BINGO;
    printf("%d", base);
    rewind(fbe);
    fprintf(fbe, "%08d", base);

    fclose(fbe);
    fclose(fc);
    return 0;
}
