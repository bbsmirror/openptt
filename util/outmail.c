#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "struct.h"

#define SPOOL BBSHOME "/out"
#define INDEX SPOOL "/.DIR"
#define NEWINDEX SPOOL "/.DIR.sending"
#define FROM ".bbs@" MYHOSTNAME
#define RELAYSERVERIP "127.0.0.1"
#define SMTPPORT 25

int waitReply(int sock) {
    char buf[256];
    
    if(read(sock, buf, sizeof(buf)) <= 0)
	return -1;
    else
	return buf[0] - '0';
}

int sendRequest(int sock, char *request) {
    return write(sock, request, strlen(request)) < 0 ? -1 : 0;
}

int connectMailServer() {
    int sock;
    struct sockaddr_in addr;
    
    if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
	perror("socket");
	return -1;
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SMTPPORT);
    addr.sin_addr.s_addr = inet_addr(RELAYSERVERIP);
    
    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	perror(RELAYSERVERIP);
	close(sock);
	return -1;
    }
    
    if(waitReply(sock) != 2) {
	close(sock);
	return -1;
    }
    
    if(sendRequest(sock, "helo " MYHOSTNAME "\n") || waitReply(sock) != 2) {
	close(sock);
	return -1;
    } else
	return sock;
}

void disconnectMailServer(int sock) {
    sendRequest(sock, "quit\n");
    /* drop the reply :p */
    close(sock);
}

void doSendBody(int sock, FILE *fp, char *from, char *to, char *subject) {
    int n;
    char buf[2048];
    
    n = snprintf(buf, sizeof(buf), "From: %s\nTo: %s\nSubject: %s\n\n",
		 from, to, subject);
    write(sock, buf, n);
    
    while(fgets(buf, sizeof(buf), fp)) {
	if(buf[0] == '.' && buf[1] == '\n')
	    strcpy(buf, "..\n");
	write(sock, buf, strlen(buf));
    }
}

void doSendMail(int sock, FILE *fp, char *from, char *to, char *subject) {
    char buf[256];
    
    snprintf(buf, sizeof(buf), "mail from: %s\n", from);
    if(sendRequest(sock, buf) || waitReply(sock) != 2)
	return;
    
    snprintf(buf, sizeof(buf), "rcpt to: %s\n", to);
    if(sendRequest(sock, buf) || waitReply(sock) != 2)
	return;
    
    if(sendRequest(sock, "data\n") || waitReply(sock) != 3)
	return;
    
    doSendBody(sock, fp, from, to, subject);

    if(sendRequest(sock, ".\n") || waitReply(sock) != 2)
	return;
}

void sendMail() {
    if(link(INDEX, NEWINDEX) || unlink(INDEX)) {
	/* nothing to do */
	return;
    } else {
	int fd, sock;
	MailQueue mq;
	
	if((sock = connectMailServer()) < 0)
	    return;
	
	fd = open(NEWINDEX, O_RDONLY);
	flock(fd, LOCK_EX);
	while(read(fd, &mq, sizeof(mq)) > 0) {
	    FILE *fp;
	    char buf[256];
	    
	    snprintf(buf, sizeof(buf), "%s%s", mq.sender, FROM);
	    if((fp = fopen(mq.filepath, "r")) >= 0) {
		doSendMail(sock, fp, buf, mq.rcpt, mq.subject);
		fclose(fp);
		unlink(mq.filepath);
	    } else {
		perror(mq.filepath);
	    }
	}
	flock(fd, LOCK_UN);
	close(fd);
	unlink(NEWINDEX);
	
	disconnectMailServer(sock);
    }
}

void listQueue() {
    int fd;
    
    if((fd = open(INDEX, O_RDONLY)) >= 0) {
	int counter = 0;
	MailQueue mq;
	
	flock(fd, LOCK_EX);
	while(read(fd, &mq, sizeof(mq)) > 0) {
	    printf("%s:%s -> %s:%s\n", mq.filepath, mq.username, mq.rcpt,
		   mq.subject);
	    counter++;
	}
	flock(fd, LOCK_UN);
	close(fd);
	printf("\nTotal: %d mails in queue\n", counter);
    } else {
	perror(INDEX);
    }
}

void usage() {
    fprintf(stderr, "usage: outmail [-qh]\n");
}

int main(int argc, char **argv) {
    int ch;
    
    while((ch = getopt(argc, argv, "qh")) != -1) {
	switch(ch) {
	case 'q':
	    listQueue();
	    return 0;
	default:
	    usage();
	    return 0;
	}
    }
    for(;;) {
	sendMail();
	sleep(60 * 3); /* send mail every 3 minute */
    }
    return 0;
}
