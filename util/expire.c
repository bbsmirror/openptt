/* $Id$ */
/* 自動砍信工具程式 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "config.h"
#include "pttstruct.h"
#include "util.h"

#define QCAST int (*)(const void *, const void *)

#define	DEF_DAYS	50
#define	DEF_MAXP	2000
#define	DEF_MINP	300

#define	EXPIRE_CONF	BBSHOME "/etc/expire.conf"

char *bpath = BBSHOME "/boards";

struct life
{
    char bname[16];		/* board ID */
    int days;			/* expired days */
    int maxp;			/* max post */
    int minp;			/* min post */
};
typedef struct life life;


void expire(life *brd) {
    fileheader_t head;
    struct stat state;
    char lockfile[128], tmpfile[128], bakfile[128];
    char fpath[128], index[128], *fname;
    int total;
    int fd, fdr, fdw, done, keep;
    int duetime, ftime;

    printf("%s\n", brd->bname);

#ifdef	VERBOSE
    if (brd->days < 1)
    {
	printf(":Err: expire time must more than 1 day.\n");
	return;
    }
    else if (brd->maxp < 100)
    {
	printf(":Err: maxmum posts number must more than 100.\n");
	return;
    }
#endif

    sprintf(index, "%s/%c/%s/.DIR", bpath, *(brd->bname), brd->bname);
    sprintf(lockfile, "%s.lock", index);
    if ((fd = open(lockfile, O_RDWR | O_CREAT | O_APPEND, 0644)) == -1)
	return;
    flock(fd, LOCK_EX);

    strcpy(fpath, index);
    fname = (char *) strrchr(fpath, '.');

    duetime = time(NULL) - brd->days * 24 * 60 * 60;
    done = 0;
    if ((fdr = open(index, O_RDONLY, 0)) > 0)
    {
	fstat(fdr, &state);
	total = state.st_size / sizeof(head);
	sprintf(tmpfile, "%s.new", index);
	unlink(tmpfile);
	if ((fdw = open(tmpfile, O_WRONLY | O_CREAT | O_EXCL, 0644)) > 0)
	{
	    while (read(fdr, &head, sizeof head) == sizeof head)
	    {
		done = 1;
		ftime = atoi(head.filename + 2);
		if (head.owner[0] == '-')
		    keep = 0;
		else if (head.filemode & FILE_MARKED || total <= brd->minp)
		    keep = 1;
		else if (ftime < duetime || total > brd->maxp)
		    keep = 0;
		else
		    keep = 1;

		if (keep)
		{
		    if (write(fdw, (char *)&head, sizeof head) == -1)
		    {
			done = 0;
			break;
		    }
		}
		else
		{
		    strcpy(fname, head.filename);
		    unlink(fpath);
		    printf("\t%s\n", fname);
		    total--;
		}
	    }
	    close(fdw);
	}
	close(fdr);
    }

    if (done)
    {
	sprintf(bakfile, "%s.old", index);
	if (rename(index, bakfile) != -1)
	    rename(tmpfile, index);
    }
    flock(fd, LOCK_UN);
    close(fd);
}


int main(int argc, char **argv) {
    FILE *fin;
    int number, count;
    life db, table[MAX_BOARD], *key;
    struct dirent *de;
    DIR *dirp;
    char *ptr, *bname, buf[256];
    int i;
    
    db.days = ((argc > 1) && (number = atoi(argv[1])) > 0) ? number : DEF_DAYS;
    db.maxp = ((argc > 2) && (number = atoi(argv[2])) > 0) ? number : DEF_MAXP;
    db.minp = ((argc > 3) && (number = atoi(argv[3])) > 0) ? number : DEF_MINP;

/* --------------- */
/* load expire.ctl */
/* --------------- */

    count = 0;
    if((fin = fopen(EXPIRE_CONF, "r"))) {
	while(fgets(buf, 256, fin)) {
	    if(buf[0] == '#')
		continue;
	    
	    bname = strtok(buf, " \t\r\n");
	    if(bname && *bname) {
		ptr = strtok(NULL, " \t\r\n");
		if(ptr && (number = atoi(ptr)) > 0) {
		    key = &(table[count++]);
		    strcpy(key->bname, bname);
		    key->days = number;
		    key->maxp = db.maxp;
		    key->minp = db.minp;
		    
		    ptr = strtok(NULL, " \t\r\n");
		    if(ptr && (number = atoi(ptr)) > 0) {
			key->maxp = number;
			
			ptr = strtok(NULL, " \t\r\n");
			if(ptr && (number = atoi(ptr)) > 0) {
			    key->minp = number;
			}
		    }
		}
	    }
	}
	fclose(fin);
    }
    
    if(count > 1)
	qsort(table, count, sizeof(life), (QCAST)strcasecmp);
    
/* ---------------- */
/* visit all boards */
/* ---------------- */
    
    for(i = 0; i < 52; i++) {
	char bpathc[256];
	
	sprintf(bpathc, "%s/%c", bpath, (i < 26 ? 'A' + i : 'a' + i - 26));
	
	if(!(dirp = opendir(bpathc))) {
	    printf(":Err: unable to open %s\n", bpathc);
	    return -1;
	}
	
	while((de = readdir(dirp))) {
	    ptr = de->d_name;
	    if(ptr[0] > ' ' && ptr[0] != '.') {
		if(count)
		    key = (life *)bsearch(ptr, table, count, sizeof(life), (QCAST)strcasecmp);
		else
		    key = NULL;
		if(!key)
		    key = &db;
		strcpy(key->bname, ptr);
		expire(key);
	    }
	}
	closedir(dirp);
    }
    return 0;
}
