# FreeBSD
#CC=	gcc
#CFLAGS=	-pipe -Wall -g -O3 -DHAVE_SETPROCTITLE -I../include
#LIBS=	

# Linux
CC=gcc
CFLAGS=	-pipe -Wall -g -O3 -DHAVE_DES_CRYPT -I../include
LIBS=	

DEPPROGS=	bbsmail BM_money post account birth deluserfile expire\
		mandex horoscope openvice parse_news openticket topusr\
		yearsold
NORPROGS=	uhash_loader poststat showboard inndBM\
		antispam countalldice webgrep bbsrf
STDPROGS=	bmda
SHLPROGS=	BM_money.sh backpasswd.sh mailog.sh opendice.sh openticket.sh\
		stock.sh topsong.sh weather.sh
PERLPROGS=	stock.perl weather.perl
PROGS=	$(DEPPROGS) $(NORPROGS) $(STDPROGS) $(SHLPROGS) $(PERLPROGS)

all: $(PROGS)

bmda: bmda.c util_cache.c util_record.c util_passwd.c
	$(CC) -pipe -Wall -g -DHAVE_SETPROCTITLE -I../include -o $* $*.c util_cache.c util_record.c util_passwd.c -lresolv

$(DEPPROGS): $*.c util_cache.c util_record.c util_passwd.c
	$(CC) $(CFLAGS) -o $* $*.c util_cache.c util_record.c util_passwd.c

$(NORPROGS): $*.c
	$(CC) $(CFLAGS) -o $* $*.c

$(SHLPROGS):

$(PERLPROGS):

install: $(PROGS)
	install -c -o bbs -g bbs -m 755 $(PROGS) $(HOME)/bin

clean:
	rm -f $(DEPPROGS) $(NORPROGS) $(STDPROGS)
