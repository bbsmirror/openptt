#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include "config.h"
#include "struct.h"
#include "perm.h"
#include "util.h"

time_t now;

int check(userec_t *u) {
    time_t d;
    char buf[256];
    
    d = now - u->lastlogin;
    if((d > MAX_GUEST_LIFE && (u->userlevel & PERM_LOGINOK) == 0) ||
       (d > MAX_LIFE && (u->userlevel & PERM_XEMPT) == 0)) {
	/* expired */
	strcpy(buf, ctime(&u->lastlogin));
	strtok(buf, "\n");
	syslog(LOG_NOTICE, "kill user: %s %s", u->userid, buf);
	sprintf(buf, "mv home/%c/%s tmp/", u->userid[0], u->userid);
	if(system(buf))
	    syslog(LOG_ERR, "can't move user home: %s", u->userid);
	u->userid[0] = '\0';
    }
    return 0;
}

int main() {
    now = time(NULL);
    openlog("reaper", LOG_PID | LOG_PERROR, SYSLOG_FACILITY);
    chdir(BBSHOME);
    
    if(passwd_mmap())
	exit(1);
    passwd_apply(check);
    
    return 0;
}
