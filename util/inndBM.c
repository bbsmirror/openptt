/* 依據 .BOARD檔 & newsfeeds.bbs 列出參與轉信的所有板資料 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "config.h"

#define BOARD_REC BBSHOME "/etc/BOARD.rec"
#define INND_BD   BBSHOME "/innd/newsfeeds.bbs"
#define fn_board  ".BOARDS"

int main()
{
    fpos_t B_pos = 0;
    FILE *b_fp, *i_fp;
    char str1[128], str2[128], *bname;

    if (!((b_fp = fopen(BOARD_REC, "r")) &&
	  (i_fp = fopen(INND_BD, "r"))))
    {
	printf("Error open file!(check the BOARD.rec and newsfeeds.bbs)\n");
	exit(1);
    }

    while (fgets(str1, 128, i_fp))
    {
	if (str1[0] == '#' || str1[0] == ' ')
	    continue;
	else
	    bname = strtok(str1 + 32, " \n\0");		/*32th個ch開始為boardname */

	while (fgets(str2, 128, b_fp))
	{
	    if (!strncmp(str2, bname, strlen(bname)))
	    {
		printf("%s", str2);
//        check_set_innd(bname);
		break;
	    }
	}
	fsetpos(b_fp, &B_pos);
    }
    fclose(b_fp);
    fclose(i_fp);
    return 0;
}
