/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"
#include "pttstruct.h"

int dirselect(struct dirent *dir) {
    return strchr("MDSGH", dir->d_name[0]) && dir->d_name[1] == '.';
}

int time_sort(const void *a, const void *b)
{
  struct dirent *aa=*(struct dirent**)a;
  struct dirent *bb=*(struct dirent**)b;
  return atoi(aa->d_name+2)-atoi(bb->d_name+2);
}

int main(int argc, char **argv) {
    int k;
    
    if(argc < 2) {
	fprintf(stderr, "Usage: %s <path1> [<path2> ...]\n", argv[0]);
	return 1;
    }
    
    for(k = 1; k < argc; k++) {
	int fdir, count, total;
	char *ptr, path[MAXPATHLEN];
	struct dirent **dirlist;
	
	sprintf(path, "%s/.DIR", argv[k]);
	if((fdir = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
	    perror(path);
	    continue;
	}
	
	if((total = scandir(argv[k], &dirlist, dirselect, time_sort)) == -1) {
	    fprintf(stderr, "scandir failed!\n");
	    close(fdir);
	    continue;
	}
	
	ptr = strrchr(path, '.');
	for(count = 0; count < total; count++) {
	    FILE *fp;
	    struct stat st;
	    
	    strcpy(ptr, dirlist[count]->d_name);
	    if(stat(path, &st) == 0 && st.st_size > 0 &&
	       (fp = fopen(path, "r")) != NULL) {
		char buf[512];
		time_t filetime;
		fileheader_t fhdr;
		
		memset(&fhdr, 0, sizeof(fhdr));
		/* set file name */
		strcpy(fhdr.filename, dirlist[count]->d_name);
		
		/* set file time */
		filetime = atoi(dirlist[count]->d_name + 2);
		if(filetime > 740000000) {
		    struct tm *ptime = localtime(&filetime);
		    sprintf(fhdr.date, "%2d/%02d", ptime->tm_mon + 1,
			    ptime->tm_mday);
		} else
		    strcpy(fhdr.date, "     ");
		
		/* set file mode */
		fhdr.filemode = FILE_READ;
		
		/* set article owner */
		fgets(buf, sizeof(buf), fp);
		if(strncmp(buf, "作者: ", 6) == 0 ||
		   strncmp(buf, "發信人: ", 8) == 0) {
		    int i, j;
		    
		    for(i = 5; buf[i] != ' '; i++);
		    for(; buf[i] == ' '; i++);
		    for(j = i + 1; buf[j] != ' '; j++);
		    j -= i;
		    if(j > IDLEN + 1)
			j = IDLEN + 1;
		    strncpy(fhdr.owner, buf + i, j);
		    fhdr.owner[IDLEN + 1] = '\0';
		    strtok(fhdr.owner, " .@\t\n\r");
		    if(strtok(NULL, " .@\t\n\r"))
			strcat(fhdr.owner, ".");
		    
		    /* set article title */
		    while(fgets(buf, sizeof(buf), fp))
			if(strncmp(buf, "標題: ", 6) == 0 ||
			   strncmp(buf, "標  題: ", 8) == 0) {
			    char *p;
			    for(i = 5; buf[i] != ' '; i++);
			    for(; buf[i] == ' '; i++);
			    if((p=strchr(buf + i, '\n')))
				*p='\0';
			    strncpy(fhdr.title, buf + i, TTLEN);
			    if(p == buf + i)
				strcpy(fhdr.title, "無標題");
			    fhdr.title[TTLEN] = '\0';
			    break;
			}
		} else if(strncmp(buf, "☉ 歡迎光臨", 11) == 0) {
		    strcpy(fhdr.title, "會議記錄");
		} else if(strncmp(buf, "\33[1;33;46m★", 12) == 0||
			  strncmp(buf, "To", 2) == 0) {
		    strcpy(fhdr.title, "熱線記錄");
		}
		if(!fhdr.title[0])
		    strcpy(fhdr.title, dirlist[count]->d_name);
		fclose(fp);
		write(fdir, &fhdr, sizeof(fhdr));
	    }
	}
	close(fdir);
	for(total--; total >= 0; total--)
	    free(dirlist[total]);
	free(dirlist);
    }
    return 0;
}
