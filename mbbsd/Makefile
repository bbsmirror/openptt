# $Id$

BBSHOME?=$(HOME)

# FreeBSD
CFLAGS_FreeBSD=	-pipe -Wall -g -O3 -DHAVE_SETPROCTITLE -DBBSHOME='"$(BBSHOME)"' -DFreeBSD -I../include
LDFLAGS_FreeBSD=-pipe -Wall -g -O3
LIBS_FreeBSD=	-lutil -lkvm

# Linux
CFLAGS_linux=	-pipe -Wall -g -O3 -DHAVE_DES_CRYPT -DBBSHOME='"$(BBSHOME)"' -DLinux -I../include
LDFLAGS_linux=	-pipe -Wall -g -O3
LIBS_linux=	-lcrypt

CFLAGS=	$(CFLAGS_$(OSTYPE))
LDFLAGS=$(LDFLAGS_$(OSTYPE))
LIBS=	../libptt/libptt.a $(LIBS_$(OSTYPE))

CC=	gcc
PROG=	mbbsd
OBJS=	admin.o bbs.o cal.o edit.o mbbsd.o osdep.o read.o term.o toolkit.o\
	ui.o xyz.o

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $*.c

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS)

install: $(PROG)
	install -d $(BBSHOME)/bin/
	install -c -m 755 $(PROG) $(BBSHOME)/bin/

clean:
	rm -f $(OBJS) $(PROG)
