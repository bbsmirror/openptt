# $Id$
BBSHOME?=$(HOME)

# FreeBSD
CFLAGS_FreeBSD=	-DFreeBSD
LDFLAGS_FreeBSD=
LIBS_FreeBSD=

# Linux
CFLAGS_linux=	-DLinux
LDFLAGS_linux=
LIBS_linux=

# common
CFLAGS_common=	-pipe -Wall -g -DBBSHOME='"$(BBSHOME)"' -I..
LDFLAGS_common=	-pipe -Wall -g
LIBS_common=

CC=	gcc
PROG=	bbsd
HDRS=	proto.h struct.h
SRCS=	bbsd.c daemon.c
OBJS=	bbsd.o daemon.o
CFLAGS=	$(CFLAGS_common) $(CFLAGS_$(OSTYPE))
LDFLAGS=$(LDFLAGS_common) $(LDFLAGS_$(OSTYPE))
LIBS=	$(LIBS_common) $(LIBS_$(OSTYPE))

.c.o:
	$(CC) $(CFLAGS) -c $*.c

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS)

install: $(PROG)
	install -d $(BBSHOME)/bin/
	install -c -m 755 $(PROG) $(BBSHOME)/bin/

depend:
	mkdep $(CFLAGS) $(SRCS) $(HDRS)

depclean: clean
	rm -f .depend

clean:
	rm -f $(OBJS) $(PROG)
