# FreeBSD
CFLAGS=	-pipe -Wall -g -O3 -DHAVE_SETPROCTITLE -I../include
LDFLAGS=-pipe -Wall -g -O3
LIBS=	-lutil -lkvm

# Linux
#CFLAGS=	-pipe -Wall -g -O3 -DHAVE_DES_CRYPT -I../include
#LDFLAGS=-pipe -Wall -g -O3
#LIBS=	-lcrypt


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

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $*.c

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS)

install: $(PROG)
	install -c -o bbs -g bbs -m 755 $(PROG) $(HOME)/bin

clean:
	rm -f $(OBJS) $(PROG)
