# $Id$

include ../Makefile.inc

CFLAGS+=$(CFLAGS_MBBSD) -pipe -Wall -g -O3 -I../include
LDFLAGS=-pipe -Wall -g -O3

PROG=	mbbsd
OBJS=	admin.o announce.o args.o bbs.o board.o cache.o cal.o\
	chat.o chicken.o\
	edit.o friend.o io.o\
	kaede.o mail.o mbbsd.o menu.o more.o name.o osdep.o\
	read.o record.o register.o screen.o stuff.o\
	talk.o term.o topsong.o user.o vote.o xyz.o\
	voteboard.o syspost.o var.o descrypt.o toolkit.o passwd.o

SRCS=	admin.c announce.c args.c bbs.c board.c cache.c cal.c\
	chat.c chicken.c\
	edit.c friend.c io.c\
	kaede.c mail.c mbbsd.c menu.c more.c name.c osdep.c\
	read.c record.c register.c screen.c stuff.c\
	talk.c term.c topsong.c user.c vote.c xyz.c\
	voteboard.c syspost.c var.c descrypt.c toolkit.c passwd.c

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $*.c

all: $(PROG)

depend:
	env MKDEP_CPP_OPTS=-MM mkdep $(CFLAGS) $(SRCS)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS_MBBSD)

install: $(PROG)
	install -d $(BBSHOME)/bin/
	install -c -m 755 $(PROG) $(BBSHOME)/bin/

clean:
	rm -f $(OBJS) $(PROG)
