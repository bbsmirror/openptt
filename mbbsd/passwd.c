/* $Id$ */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "config.h"
#include "struct.h"
#include "modes.h"

extern char *fn_passwd;

static userec_t *passwd_image = NULL;
static int passwd_image_size;
static int semid = -1;

int passwd_mmap() {
    int fd;
    
    fd = open(fn_passwd, O_RDWR);
    if(fd > 0) {
	struct stat st;
	
	fstat(fd, &st);
	passwd_image_size = st.st_size;
	passwd_image = mmap(NULL, passwd_image_size,
			    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(passwd_image == (userec_t *)-1) {
	    perror("mmap");
	    return -1;
	}
	
	semid = semget(PASSWDSEM_KEY, 1, SEM_R | SEM_A | IPC_CREAT | IPC_EXCL);
	if(semid == -1) {
	    if(errno == EEXIST) {
		semid = semget(PASSWDSEM_KEY, 1, SEM_R | SEM_A);
		if(semid == -1) {
		    perror("semget");
		    exit(1);
		}
	    } else {
		perror("semget");
		exit(1);
	    }
	} else {
	    union semun s;
	    
	    s.val = 1;
	    if(semctl(semid, 0, SETVAL, s) == -1) {
		perror("semctl");
		exit(1);
	    }
	}
    } else {
	perror(fn_passwd);
	return -1;
    }
    return 0;
}

int passwd_update(int num, userec_t *buf) {
    if(num < 1 || num > MAX_USERS)
	return -1;
    memcpy(&passwd_image[num - 1], buf, sizeof(userec_t));
    return 0;
}

int passwd_query(int num, userec_t *buf) {
    if(num < 1 || num > MAX_USERS)
	return -1;
    memcpy(buf, &passwd_image[num - 1], sizeof(userec_t));
    return 0;
}

int passwd_apply(int (*fptr)(userec_t *)) {
    int i;

    for(i = 0; i < MAX_USERS; i++)
	if((*fptr)(&passwd_image[i]) == QUIT)
	    return QUIT;
    return 0;
}

void passwd_lock() {
    struct sembuf buf = { 0, -1, SEM_UNDO };
    
    if(semop(semid, &buf, 1)) {
	perror("semop");
	exit(1);
    }
}

void passwd_unlock() {
    struct sembuf buf = { 0, 1, SEM_UNDO };
    
    if(semop(semid, &buf, 1)) {
	perror("semop");
	exit(1);
    }
}
