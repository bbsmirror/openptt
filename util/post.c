/* $Id$ */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include "config.h"
#include "pttstruct.h"
#include "util.h"

void keeplog(FILE *fin, char *fpath, char *board, char *title, char *owner) {
    fileheader_t fhdr;
    char genbuf[256], buf[512];
    FILE *fout;
    time_t now=time(NULL);
    int bid;
    
    sprintf(genbuf, BBSHOME "/boards/%s", board);
    stampfile(genbuf, &fhdr);
    
    if(!(fout = fopen(genbuf, "w"))) {
	perror(genbuf);
	return;
    }

    fprintf(fout, "作者: %s 看板: %s\n標題: %.44s\n時間: %s\n",
	owner, board, title, ctime(&now));

    while(fgets(buf, 512, fin))
	fputs(buf, fout);
    
    fclose(fin);
    fclose(fout);
    
    strncpy(fhdr.title, title, sizeof(fhdr.title) - 1);
    fhdr.title[sizeof(fhdr.title) - 1] = '\0';
    
    strcpy(fhdr.owner, owner);
    sprintf(genbuf, BBSHOME "/boards/%s/.DIR", board);
    append_record(genbuf, &fhdr, sizeof(fhdr));
    if((bid = getbnum(board)) > 0)
	inbtotal(bid, 1);
}

int main(int argc, char **argv) {
    FILE *fp;
    
    if(argc != 5) {
	printf("usage: %s <board name> <title> <owner> <file>\n", argv[0]);
	return 0;
    }
    
    if(strcmp(argv[4], "-") == 0)
	fp = stdin;
    else {
	fp = fopen(argv[4], "r");
	if(!fp) {
	    perror(argv[4]);
	    return 1;
	}
    }
    keeplog(fp, argv[4], argv[1], argv[2], argv[3]);
    return 0;
}
