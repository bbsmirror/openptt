# $Id$

BBSHOME?=$(HOME)

# FreeBSD
CC_FreeBSD=	gcc
CFLAGS_FreeBSD=	-pipe -Wall -g -O3 -DHAVE_SETPROCTITLE -DBBSHOME='"$(BBSHOME)"' -I../include
LIBS_FreeBSD=
LIBMAIL_FreeBSD=-lutil
LIBCHAT_FreeBSD=

# Linux
CC_linux=	gcc
CFLAGS_linux=	-pipe -Wall -g -O3 -DHAVE_DES_CRYPT -DBBSHOME='"$(BBSHOME)"' -I../include
LIBS_linux=	-lresolv
LIBMAIL_linux=	
LIBCHAT_linux=	-lcrypt

CC=	$(CC_$(OSTYPE))
CFLAGS=	$(CFLAGS_$(OSTYPE))
LDFLAGS=$(LDFLAGS_$(OSTYPE))
LIBMAIL=$(LIBMAIL_$(OSTYPE))
LIBCHAT=$(LIBCHAT_$(OSTYPE))

OBJS=	util_cache.o util_record.o util_passwd.o

CPROGS=	bbsmail BM_money post account birth deluserfile expire mandex\
	horoscope openvice parse_news openticket topusr yearsold uhash_loader\
	poststat showboard inndBM antispam countalldice webgrep bbsrf\
	initbbs outmail xchatd userlist tunepasswd buildir reaper shmsweep\
	merge_passwd merge_board

SRCS=	bbsmail.c BM_money.c post.c account.c birth.c deluserfile.c expire.c\
	mandex.c horoscope.c openvice.c parse_news.c openticket.c topusr.c\
	yearsold.c uhash_loader.c poststat.c showboard.c inndBM.c antispam.c\
	countalldice.c webgrep.c bbsrf.c initbbs.c outmail.c xchatd.c\
	userlist.c tunepasswd.c buildir.c reaper.c shmsweep.c merge_passwd.c\
	merge_board.c

SCRIPTS=BM_money.sh backpasswd.sh cvslog.sh mailog.sh opendice.sh\
	openticket.sh stock.sh topsong.sh upgrade.sh weather.sh

PROGS=	$(CPROGS) $(SCRIPTS) stock.perl weather.perl

JUNK=	$(CPROGS) $(SCRIPTS) $(OBJS) sed.tmp

.SUFFIXES: .c .o .shtpl .sh

.c:
	$(CC) $(CFLAGS) -o $* $*.c

.c.o:
	$(CC) $(CFLAGS) -c $*.c

.shtpl.sh:
	sed "s/%BBSHOME%/`cat sed.tmp`/g" $*.shtpl > $@
	chmod 755 $@

all: $(PROGS)

depend:
	env MKDEP_CPP_OPTS=-MM mkdep $(CFLAGS) $(SRCS)

install: $(PROGS)
	install -d $(BBSHOME)/bin/
	install -c -m 755 $(PROGS) $(BBSHOME)/bin/
	chmod 4755 $(BBSHOME)/bin/post

clean:
	rm -f $(JUNK)

$(SCRIPTS): sed.tmp

sed.tmp: Makefile
	echo "$(BBSHOME)" | sed 's/\//\\\//g' > sed.tmp

bbsmail: bbsmail.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ bbsmail.c $(OBJS)

BM_money: BM_money.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ BM_money.c $(OBJS)

post: post.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ post.c $(OBJS)

account: account.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ account.c $(OBJS)

birth: birth.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ birth.c $(OBJS)

deluserfile: deluserfile.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ deluserfile.c $(OBJS)

expire: expire.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ expire.c $(OBJS)

mandex: mandex.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ mandex.c $(OBJS)

horoscope: horoscope.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ horoscope.c $(OBJS)

openvice: openvice.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ openvice.c $(OBJS)

parse_news: parse_news.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ parse_news.c $(OBJS)

openticket: openticket.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ openticket.c $(OBJS)

topusr: topusr.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ topusr.c $(OBJS)

yearsold: yearsold.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ yearsold.c $(OBJS)

reaper: reaper.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ reaper.c $(OBJS)

xchatd: xchatd.c descrypt.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ xchatd.c descrypt.c $(OBJS) $(LIBCHAT)

outmail: outmail.c
	$(CC) $(CFLAGS) -o $@ outmail.c $(LIBMAIL)
