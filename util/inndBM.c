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

/*-------------------------------------------------------*/
/* .BOARDS cache                                         */
/*-------------------------------------------------------*/
/*
   boardheader *bcache;
   struct BCACHE *brdshm;
   int numboards = -1;

   static void
   attach_err(shmkey, name)
   int shmkey;
   char *name;
   {
   fprintf(stderr, "[%s error] key = %x\n", name, shmkey);
   exit(1);
   }

   static void *
   attach_shm(shmkey, shmsize)
   int shmkey, shmsize;
   {
   void *shmptr;
   int shmid;

   shmid = shmget(shmkey, shmsize, 0);
   if (shmid < 0)
   {
   shmid = shmget(shmkey, shmsize, IPC_CREAT | 0600);
   if (shmid < 0)
   attach_err(shmkey, "shmget");
   shmptr = (void *) shmat(shmid, NULL, 0);
   if (shmptr == (void *) -1)
   attach_err(shmkey, "shmat");
   memset(shmptr, 0, shmsize);
   }
   else
   {
   shmptr = (void *) shmat(shmid, NULL, 0);
   if (shmptr == (void *) -1)
   attach_err(shmkey, "shmat");
   }
   return shmptr;
   }
 */
/*
   int
   ci_strcmp(s1, s2)
   register char *s1, *s2;
   {
   register int c1, c2, diff;

   do
   {
   c1 = *s1++;
   c2 = *s2++;
   if (c1 >= 'A' && c1 <= 'Z')
   c1 |= 32;
   if (c2 >= 'A' && c2 <= 'Z')
   c2 |= 32;
   if (diff = c1 - c2)
   return (diff);
   } while (c1);
   return 0;
   }
 */
/*
   int
   get_record(fpath, rptr, size, id)
   char *fpath;
   char *rptr;
   int size, id;
   {
   int fd;

   if ((fd = open(fpath, O_RDONLY, 0)) != -1)
   {
   if (lseek(fd, size * (id - 1), SEEK_SET) != -1)
   {
   if (read(fd, rptr, size) == size)
   {
   close(fd);
   return 0;
   }
   }
   close(fd);
   }
   return -1;
   }
 */
/*
   void
   resolve_boards()
   {
   if (brdshm == NULL)
   {
   brdshm = attach_shm(BRDSHM_KEY, sizeof(*brdshm));
   if (brdshm->touchtime == 0)
   brdshm->touchtime = 1;
   bcache = brdshm->bcache;
   }
   while (brdshm->uptime < brdshm->touchtime)
   {
   if (brdshm->busystate)
   {
   sleep(1);
   }
   else
   {
   int fd;

   brdshm->busystate = 1;

   if ((fd = open(".BOARDS", O_RDONLY)) > 0)
   {
   brdshm->number = read(fd, bcache, MAXBOARD * sizeof(boardheader))
   / sizeof(boardheader);
   close(fd);
   }
 */
      /* 等所有 boards 資料更新後再設定 uptime */
/*        
   brdshm->uptime = brdshm->touchtime;
   brdshm->busystate = 0;
   }
   }
   numboards = brdshm->number;
   }
 */
/*
   int
   getbnum(bname)
   char *bname;
   {
   register int i;
   register boardheader *bhdr;

   resolve_boards();
   for (i = 0, bhdr = bcache; i++ < numboards; bhdr++)
       *//* if (Ben_Perm(bhdr)) */
  /*  if (!ci_strcmp(bname, bhdr->brdname))
     return i;
     return 0;
     }
   */
/*
   int check_set_innd(char *boardname){
   boardheader inndbd;
   int inndbid;

   inndbid = getbnum(boardname);
   get_record(fn_board, &inndbd, sizeof(inndbd), inndbid);
   printf("board(%s)\n", inndbd.brdname);
   if(inndbd.brdattr & BRD_NOTRAN){
   strncpy (inndbd.title + 5,"●", 2);
   inndbd.brdattr &= ~BRD_NOTRAN;
   printf("Change the board(%s) to TAN\n", inndbd.brdname);
   return 1;
   }
   return 0;
   }
 */

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
