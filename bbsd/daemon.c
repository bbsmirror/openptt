/* $Id$ */
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "proto.h"

#define LISTEN_BACKLOG 5

int daemon_run(client_t *clt, int port) {
    int s, opt, len;
    struct sockaddr_in addr;
    struct hostent *h;
    
    /* create new session */
    switch(fork()) {
    case 0:
	setsid();
	break;
    case -1:
	syslog(LOG_ERR, "fork(): %m");
	return 1;
    default:
	exit(0);
	break;
    }
    
    if((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	syslog(LOG_ERR, "socket(): %m");
	return -1;
    }

    opt = 1;
    if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
	syslog(LOG_ERR, "setsockopt(): SO_REUSEADDR: %m");
	return -1;
    }
    
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
	syslog(LOG_ERR, "bind(): %m");
	return -1;
    }
    
    if(listen(s, LISTEN_BACKLOG) == -1) {
	syslog(LOG_ERR, "listen(): %m");
	return -1;
    }
    
    for(;;) {
	len = sizeof(addr);
	if((clt->fd = accept(s, (struct sockaddr *)&addr, &len)) == -1) {
	    if(errno == EINTR)
		continue;
	    else {
		syslog(LOG_ERR, "accept(): %m");
		return -1;
	    }
	}
	
	switch(fork()) {
	case 0:
	    close(s);
	    /* get remote host */
	    h = gethostbyaddr((const char *)&addr.sin_addr,
			      sizeof(addr.sin_addr), AF_INET);
	    strncpy(clt->remote_host,
		    h == NULL ? inet_ntoa(addr.sin_addr) : h->h_name,
		    MAX_HOST_LEN);
	    clt->remote_host[MAX_HOST_LEN - 1] = '\0';
	    return 0;
	case -1:
	    syslog(LOG_ERR, "fork(): %m");
	    break;
	default:
	    close(clt->fd);
	    break;
	}
    }
}
