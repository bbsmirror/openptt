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
PROG=	mbbsd
OBJS=	admin.o announce.o args.o bbcall.o bbs.o board.o cache.o cal.o card.o\
	chat.o chc_draw.o chc_net.o chc_play.o chc_rule.o chicken.o dark.o\
	dice.o edit.o friend.o gamble.o gomo.o gomo1.o guess.o indict.o io.o\
	kaede.o lovepaper.o mail.o mbbsd.o menu.o more.o name.o osdep.o\
	othello.o page.o read.o record.o register.o screen.o stuff.o\
	talk.o term.o topsong.o user.o vice.o vote.o xyz.o\
	tmpjack.o voteboard.o syspost.o var.o descrypt.o toolkit.o passwd.o\
	calendar.o

SRCS=	admin.c announce.c args.c bbcall.c bbs.c board.c cache.c cal.c card.c\
	chat.c chc_draw.c chc_net.c chc_play.c chc_rule.c chicken.c dark.c\
	dice.c edit.c friend.c gamble.c gomo.c gomo1.c guess.c indict.c io.c\
	kaede.c lovepaper.c mail.c mbbsd.c menu.c more.c name.c osdep.c\
	othello.c page.c read.c record.c register.c screen.c stuff.c\
	talk.c term.c topsong.c user.c vice.c vote.c xyz.c\
	tmpjack.c voteboard.c syspost.c var.c descrypt.c toolkit.c passwd.c\
	calendar.c

.SUFFIXES: .c .o
.c.o[A:
	$(CC) $(CFLAGS) -c $*.c

all: $(PROG)

depend:
	mkdep $(CFLAGS) $(SRCS)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS)

install: $(PROG)
	install -d $(BBSHOME)/bin/
	install -c -m 755 $(PROG) $(BBSHOME)/bin/

clean:
	rm -f $(OBJS) $(PROG)
