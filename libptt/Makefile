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
LIBS=	$(LIBS_$(OSTYPE))

CC=	gcc
LIB=	libptt
OBJS=	announce.o board.o cache.o descrypt.o friend.o mail.o name.o\
	passwd.o record.o register.o stuff.o talk.o user.o var.o

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $*.c

all: $(LIB)

$(LIB): $(OBJS)
	rm -f $(LIB).a
	ar cr $(LIB).a $(OBJS)
	ranlib $(LIB).a

install: $(LIB)

clean:
	rm -f $(OBJS) $(LIB).a
