/* $Id$ */
/* 看板一覽表(sorted) */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "config.h"
#include "pttstruct.h"

boardheader_t allbrd[MAX_BOARD];

int board_cmp(const void *va, const void *vb)
{
  boardheader_t *a=(boardheader_t *)va, *b=(boardheader_t *)vb;
  return (strcasecmp(a->brdname,b->brdname));
}


void usage(char *cmdname)
{
  printf("This will print all board information.\n");
  printf("Usage: %s [-bc] _.BOARDS_file ", cmdname);
}

int main(int argc, char *argv[])
{
    int inf, i, count;
    int optch, f_complete=0, f_bad_board_also=0;

    if (argc < 2)
    {
      usage(argv[0]);
      exit(1);
    }

    while( (optch=getopt(argc, argv, "")) != -1) {
      switch(optch) {
        case 'b':
          f_bad_board_also = 1;
        case 'c':
          f_complete = 1;
          break;
        default:
         usage(argv[0]);
      }
    }
    

    inf = open(argv[1], O_RDONLY);
    if (inf == -1)
    {
	printf("Error open file %s\n", argv[1]);
	exit(1);
    }

/* read in all boards */

    i = 0;
    memset(allbrd, 0, MAX_BOARD * sizeof(boardheader_t));
    while (read(inf, &allbrd[i], sizeof(boardheader_t)) == sizeof(boardheader_t)) {
	if (f_bad_board_also || allbrd[i].brdname[0])
	    i++;
    }
    close(inf);

/* sort them by name */
    count = i;
    qsort(allbrd, count, sizeof(boardheader_t), board_cmp);

/* write out the target file */

    if(f_complete) {
      for (i = 0; i < count; i++) {
        printf("%s %s %s %c %d %d\n",
                                     allbrd[i].brdname, 
                                     allbrd[i].title, 
                                     allbrd[i].BM, 
                                     allbrd[i].bvote, 
                                     allbrd[i].uid, 
                                     allbrd[i].gid);
      }
    }
    else {
      printf(
        "看板名稱     板主                     類別   中文敘述\n"
        "----------------------------------------------------------------------\n");
        for (i = 0; i < count; i++) {
          printf("%-13s%-25.25s%s\n", allbrd[i].brdname, allbrd[i].BM, allbrd[i].title);
        }
    }
        
    
    printf("Total %d boards.\n", count);
    return 0;
}
