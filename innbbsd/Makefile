# ------------------------------------------------------- #
#  innbbsd/Makefile	( NTHU CS MapleBBS Ver 2.36 )     #
# ------------------------------------------------------- #
#  target : Makefile for 轉信程式 innbbsd by skhuang      #
#  create : 95/03/29                                      #
#  update : 95/12/15                                      #
# ------------------------------------------------------- #

####################################################
# this is a bbs <--> news gateway
#####################################################
VERSION=0.50beta-4
LASTVERSION=0.50beta-3
ADMINUSER= root@your.domain.name
BBSHOME?=$(HOME)
BBS_SRC = ..
TARGET  = $(INNBBSD) $(BBSNNRP) $(BBSLINK)
####################################################
CC=gcc
INSTALL=cp
#
OPT= -O3 -s -pipe -fomit-frame-pointer

BBS_DEP = MapleBBS 
BBS_REC = $(BBS_SRC)/mbbsd/record.o r_port.o

#########################
#
###############
DEBUGOBJ = /usr/lib/debug/mallocmap.o
CFLAGS= -c -I. -I$(BBS_SRC)/include -I$(BBS_SRC)/mbbsd -D$(BBS_DEP) \
$(EXTRAFLAGS) -DDBZDEBUG -DBBSHOME='"$(BBSHOME)"' \
-D_PATH_BBSHOME=\"$(BBSHOME)\" \
-DVERSION=\"$(VERSION)\" -DADMINUSER=\"$(ADMINUSER)\" 
#
####################################################

OBJS = inndchannel.o innbbsd.o connectsock.o rfc931.o \
       daemon.o file.o pmain.o his.o dbz.o \
       closeonexec.o dbztool.o inntobbs.o receive_article.o \
       bbslib.o
       # $(BBS_REC)
SRCS = inndchannel.c innbbsd.c connectsock.c rfc931.c \
       daemon.c file.c pmain.c parsdate.y his.c dbz.c \
       closeonexec.c dbztool.c inntobbs.c bbslib.c receive_article.c \
       port.c

MOBJS    = makedbz.o bbslib.o file.o dbz.o closeonexec.o 
HOBJS    = mkhistory.o bbslib.o file.o his.o dbz.o port.o closeonexec.o
DBZOBJS  = dbzserver.o dbzinnbbsd.o pmain.o connectsock.o bbslib.o his.o \
	   daemon.o file.o dbz.o dbztool.o rfc931.o port.o closeonexec.o \
           morelog.o
NNRPOBJS = bbsnnrp.o pmain.o bbslib.o connectsock.o file.o 
LINKOBJS = bbslink.o pmain.o inntobbs.o echobbslib.o connectsock.o file.o port.o
LINKOBJS2 = bbslink2.o pmain.o inntobbs.o echobbslib.o connectsock.o file.o port.o
CTLOBJS  = ctlinnbbsd.o pmain.o bbslib.o connectsock.o file.o 
INNBBSD  = innbbsd mkhistory bbsnnrp ctlinnbbsd $(BBSLINK)
BBSNNRP  = bbsnnrp
BBSLINK  = bbslink
BBSLINK2  = bbslink2
EXES     = $(TARGET)

.c.o:
	$(CC) $(OPT) $(CFLAGS) $*.c


all: $(OSTYPE)

sun: 
	@$(MAKE) EXTRAFLAGS="-DMMAP -DGETRUSAGE -DMALLOCMAP" LDFLAGS="$(DEBUGOBJ)" target


aix: 
	@$(MAKE) CC=cc EXTRAFLAGS="-DMMAP -DGETRUSAGE -DAIX -DSYSV" target

linux: 
	@$(MAKE) EXTRAFLAGS="-DLINUX -DGETRUSAGE" target

hpux: 
	@$(MAKE) OPT= EXTRAFLAGS="-DMMAP -DHPUX -DSYSV" target

irix: 
	@$(MAKE) EXTRAFLAGS="-DMMAP -DIRIX -DSYSV" target

solaris:
	@$(MAKE) EXTRAFLAGS="-DMMAP -DSOLARIS -DSYSV" LDFLAGS="-lsocket -lnsl" YACC="bison -y" target

FreeBSD:
	@$(MAKE) CC=cc EXTRAFLAGS="-DBSD44 -DMMAP -DGETRUSAGE" LIBCRYPT=-lcrypt  target

bsd:
	@$(MAKE) CC=cc EXTRAFLAGS="-DMMAP -DGETRUSAGE" target

alpha:
	@$(MAKE) CC=cc EXTRAFLAGS="-DMMAP -DDEC_ALPHA -DGETRUSAGE" target

sysv: 
	@$(MAKE) OPT= EXTRAFLAGS="-DMMAP -DSYSV" target

target: $(EXES)

r_port.o: r_port.c
	$(CC) $(OPT) $(CFLAGS) -DWITH_RECORD_O r_port.c 

echobbslib.o: echobbslib.c
	$(CC) $(OPT) $(CFLAGS) -DWITH_ECHOMAIL echobbslib.c

tar: 
	test -d innbbsd-$(VERSION) || mkdir innbbsd-$(VERSION)
	cp *.c *.h Makefile innbbsd-$(VERSION)
	tar zcf innbbsd-$(VERSION).tar.gz innbbsd-$(VERSION)
	cp innbbsd-$(VERSION).tar.gz /net/ftphome/CSIE/innbbsd

patch:
	rm -rf /tmp/innbbsd-$(LASTVERSION)
	zcat /net/ftphome/ftp-data/bbs/misc/innbbsd/innbbsd-$(LASTVERSION).tar.gz | (cd /tmp; tar xf -)	
	(cd /tmp/innbbsd-$(VERSION); make clean; rm -f r_port.c dbzserver.c dbzinnbbsd.c  echobbslib.c )
	(cd /tmp/innbbsd-$(LASTVERSION); make clean; rm -f r_port.c dbzserver.c dbzinnbbsd.c  echobbslib.c )
	(cd /tmp; diff -rcs innbbsd-$(LASTVERSION) innbbsd-$(VERSION) > /tmp/innbbsd-patch.$(LASTVERSION)-$(VERSION); echo "diff -rcs")
	(cd /tmp/innbbsd-$(VERSION); ln -s port.c r_port.c)
	(cd /tmp/innbbsd-$(VERSION); ln -s inndchannel.c dbzserver.c)
	(cd /tmp/innbbsd-$(VERSION); ln -s innbbsd.c dbzinnbbsd.c)
	(cd /tmp/innbbsd-$(VERSION); ln -s bbslib.c echobbslib.c)


distribution:
	rm -rf  /tmp/innbbsd-$(VERSION)
	test -d /tmp/innbbsd-$(VERSION) || mkdir /tmp/innbbsd-$(VERSION)
	test -d /tmp/innbbsd-$(VERSION)/innd || mkdir /tmp/innbbsd-$(VERSION)/innd
	rm -f r_port.c
	rm -f dbzserver.c
	rm -f dbzinnbbsd.c
	rm -f echobbslib.c
	cp -r doc hisconvert FEATURES CHANGES README* TODO *.c *.h *.y Makefile boards /tmp/innbbsd-$(VERSION)
	(cd /tmp/innbbsd-$(VERSION); ln -s port.c r_port.c)
	(cd /tmp/innbbsd-$(VERSION); ln -s inndchannel.c dbzserver.c)
	(cd /tmp/innbbsd-$(VERSION); ln -s innbbsd.c dbzinnbbsd.c)
	(cd /tmp/innbbsd-$(VERSION); ln -s bbslib.c echobbslib.c)
	(cd innd ; cp -r Makefile README innbbs.conf in.bntpd mailpost *.pl bntplink bntplink.palmbbs *.active* *.c *.y *.bbs src /tmp/innbbsd-$(VERSION)/innd)
	(cd /tmp/innbbsd-$(VERSION); make clean)
	(cd /tmp; tar zcf innbbsd-$(VERSION).tar.gz innbbsd-$(VERSION) ) 
	ln -s port.c r_port.c
	ln -s inndchannel.c dbzserver.c
	ln -s innbbsd.c dbzinnbbsd.c
	ln -s bbslib.c echobbslib.c

remotetest: distribution
	/usr/local/krb4/rcp /tmp/innbbsd-$(VERSION).tar.gz bbs_src.tar.gz skhuang:/tmp
	/usr/local/krb4/rcp /tmp/innbbsd-$(VERSION).tar.gz bbs_src.tar.gz linux:/tmp
	mv /u/staff/skhuang/.tcshrc /u/staff/skhuang/skhuang.tcshrc
	rcp /tmp/innbbsd-$(VERSION).tar.gz bbs_src.tar.gz ccsun36:/tmp
	rcp /tmp/innbbsd-$(VERSION).tar.gz bbs_src.tar.gz ccibm1:/tmp
	mv /u/staff/skhuang/skhuang.tcshrc /u/staff/skhuang/.tcshrc
	rcp /tmp/innbbsd-$(VERSION).tar.gz bbs_src.tar.gz cciris3:/tmp

ftp: distribution patch
	rsh ccsun42 mv /home8/ftp/pub/bbs/misc/innbbsd/innbbsd-$(VERSION).tar.gz /home8/ftp/pub/bbs/misc/innbbsd/.innbbsd-$(VERSION).tar.gz
	rcp /tmp/innbbsd-$(VERSION).tar.gz ccsun42:/home8/ftp/pub/bbs/misc/innbbsd
	rcp README* ccsun42:/home8/ftp/pub/bbs/misc/innbbsd
	rcp /tmp/innbbsd-patch.$(LASTVERSION)-$(VERSION) ccsun42:/home8/ftp/pub/bbs/misc/innbbsd

innbbspost.o: innbbspost.c
	$(CC) $(CFLAGS) -I$(BBS_SRC) -c innbbspost.c 

makedbz: $(MOBJS)
	$(CC) $(OPT) -o makedbz $(MOBJS) $(LDFLAGS)

dbzserver.o: dbzserver.c
	$(CC) $(CFLAGS) -DDBZSERVER -I$(BBS_SRC) -c dbzserver.c   

dbzinnbbsd.o: dbzinnbbsd.c
	$(CC) $(CFLAGS) -DDBZSERVER -I$(BBS_SRC) -c dbzinnbbsd.c   

dbzserver: $(DBZOBJS)
	$(CC) $(OPT) -o dbzserver $(DBZOBJS) $(LDFLAGS)

bbsnnrp: $(NNRPOBJS)
	$(CC) -o bbsnnrp $(NNRPOBJS) $(OPT) $(LDFLAGS)

bbslink: $(LINKOBJS)
	$(CC) -o bbslink $(LINKOBJS) $(OPT) $(LDFLAGS)

bbslink2: $(LINKOBJS2)
	$(CC) -o bbslink2 $(LINKOBJS2) $(OPT) $(LDFLAGS)

ctlinnbbsd: $(CTLOBJS)
	$(CC) $(OPT) -o ctlinnbbsd $(CTLOBJS) $(LDFLAGS)

mkhistory: $(HOBJS)
	$(CC) $(OPT) -o mkhistory $(HOBJS) $(LDFLAGS)

dbz_query_sample: dbz_query_sample.o
	$(CC) $(OPT) -o dbz_query_sample dbz_query_sample.o $(LDFLAGS)


innbbsd: $(OBJS)
#	mv tnrpd.exe tnrpd.exe.old
	$(CC) -o innbbsd $(OBJS) $(OPT) $(LDFLAGS)

install:
	install -d $(BBSHOME)/innd/
	install -c -m 755 $(EXES) $(BBSHOME)/innd/

clean:
	rm -f *.o $(EXES) core innd/src/*.o

depend:
	 makedepend -- $(CFLAGS) -- $(SRCS)

# DO NOT DELETE THIS LINE -- make depend depends on it.

inndchannel.o: innbbsconf.h /usr/include/stdio.h /usr/include/sys/cdefs.h
inndchannel.o: /usr/include/machine/ansi.h /usr/include/syslog.h
inndchannel.o: /usr/include/sys/types.h /usr/include/sys/inttypes.h
inndchannel.o: /usr/include/machine/types.h /usr/include/machine/endian.h
inndchannel.o: /usr/include/sys/socket.h /usr/include/netinet/in.h
inndchannel.o: /usr/include/netinet6/in6.h /usr/include/sys/queue.h
inndchannel.o: /usr/include/netdb.h /usr/include/sys/un.h
inndchannel.o: /usr/include/sys/param.h /usr/include/sys/syslimits.h
inndchannel.o: /usr/include/sys/signal.h /usr/include/sys/_posix.h
inndchannel.o: /usr/include/machine/signal.h /usr/include/machine/trap.h
inndchannel.o: /usr/include/sys/ucontext.h /usr/include/machine/ucontext.h
inndchannel.o: /usr/include/machine/param.h /usr/include/machine/limits.h
inndchannel.o: /usr/include/sys/wait.h /usr/include/unistd.h
inndchannel.o: /usr/include/sys/unistd.h /usr/include/signal.h
inndchannel.o: /usr/include/sys/time.h /usr/include/time.h
inndchannel.o: /usr/include/fcntl.h /usr/include/string.h
inndchannel.o: /usr/include/ctype.h /usr/include/runetype.h
inndchannel.o: /usr/include/errno.h /usr/include/malloc.h
inndchannel.o: /usr/include/stdlib.h /usr/include/sys/stat.h
inndchannel.o: /usr/include/sys/file.h /usr/include/sys/fcntl.h daemon.h
inndchannel.o: bbslib.h
innbbsd.o: innbbsconf.h /usr/include/stdio.h /usr/include/sys/cdefs.h
innbbsd.o: /usr/include/machine/ansi.h /usr/include/syslog.h
innbbsd.o: /usr/include/sys/types.h /usr/include/sys/inttypes.h
innbbsd.o: /usr/include/machine/types.h /usr/include/machine/endian.h
innbbsd.o: /usr/include/sys/socket.h /usr/include/netinet/in.h
innbbsd.o: /usr/include/netinet6/in6.h /usr/include/sys/queue.h
innbbsd.o: /usr/include/netdb.h /usr/include/sys/un.h
innbbsd.o: /usr/include/sys/param.h /usr/include/sys/syslimits.h
innbbsd.o: /usr/include/sys/signal.h /usr/include/sys/_posix.h
innbbsd.o: /usr/include/machine/signal.h /usr/include/machine/trap.h
innbbsd.o: /usr/include/sys/ucontext.h /usr/include/machine/ucontext.h
innbbsd.o: /usr/include/machine/param.h /usr/include/machine/limits.h
innbbsd.o: /usr/include/sys/wait.h /usr/include/unistd.h
innbbsd.o: /usr/include/sys/unistd.h /usr/include/signal.h
innbbsd.o: /usr/include/sys/time.h /usr/include/time.h /usr/include/fcntl.h
innbbsd.o: /usr/include/string.h /usr/include/ctype.h /usr/include/runetype.h
innbbsd.o: /usr/include/errno.h /usr/include/malloc.h /usr/include/stdlib.h
innbbsd.o: /usr/include/sys/stat.h /usr/include/sys/file.h
innbbsd.o: /usr/include/sys/fcntl.h daemon.h innbbsd.h /usr/include/dirent.h
innbbsd.o: /usr/include/sys/dirent.h bbslib.h inntobbs.h nntp.h
connectsock.o: innbbsconf.h /usr/include/stdio.h /usr/include/sys/cdefs.h
connectsock.o: /usr/include/machine/ansi.h /usr/include/syslog.h
connectsock.o: /usr/include/sys/types.h /usr/include/sys/inttypes.h
connectsock.o: /usr/include/machine/types.h /usr/include/machine/endian.h
connectsock.o: /usr/include/sys/socket.h /usr/include/netinet/in.h
connectsock.o: /usr/include/netinet6/in6.h /usr/include/sys/queue.h
connectsock.o: /usr/include/netdb.h /usr/include/sys/un.h
connectsock.o: /usr/include/sys/param.h /usr/include/sys/syslimits.h
connectsock.o: /usr/include/sys/signal.h /usr/include/sys/_posix.h
connectsock.o: /usr/include/machine/signal.h /usr/include/machine/trap.h
connectsock.o: /usr/include/sys/ucontext.h /usr/include/machine/ucontext.h
connectsock.o: /usr/include/machine/param.h /usr/include/machine/limits.h
connectsock.o: /usr/include/sys/wait.h /usr/include/unistd.h
connectsock.o: /usr/include/sys/unistd.h /usr/include/signal.h
connectsock.o: /usr/include/sys/time.h /usr/include/time.h
connectsock.o: /usr/include/fcntl.h /usr/include/string.h
connectsock.o: /usr/include/ctype.h /usr/include/runetype.h
connectsock.o: /usr/include/errno.h /usr/include/malloc.h
connectsock.o: /usr/include/stdlib.h /usr/include/sys/stat.h
connectsock.o: /usr/include/sys/file.h /usr/include/sys/fcntl.h daemon.h
connectsock.o: /usr/include/setjmp.h /usr/include/machine/setjmp.h
rfc931.o: /usr/include/stdio.h /usr/include/sys/cdefs.h
rfc931.o: /usr/include/machine/ansi.h /usr/include/syslog.h
rfc931.o: /usr/include/sys/types.h /usr/include/sys/inttypes.h
rfc931.o: /usr/include/machine/types.h /usr/include/machine/endian.h
rfc931.o: /usr/include/sys/socket.h /usr/include/netinet/in.h
rfc931.o: /usr/include/netinet6/in6.h /usr/include/sys/queue.h
rfc931.o: /usr/include/setjmp.h /usr/include/machine/setjmp.h
rfc931.o: /usr/include/signal.h /usr/include/sys/_posix.h
rfc931.o: /usr/include/sys/signal.h /usr/include/machine/signal.h
rfc931.o: /usr/include/machine/trap.h /usr/include/sys/ucontext.h
rfc931.o: /usr/include/machine/ucontext.h /usr/include/sys/time.h
rfc931.o: /usr/include/time.h
daemon.o: daemon.h /usr/include/stdio.h /usr/include/sys/cdefs.h
daemon.o: /usr/include/machine/ansi.h /usr/include/time.h
daemon.o: /usr/include/sys/_posix.h
file.o: /usr/include/stdio.h /usr/include/sys/cdefs.h
file.o: /usr/include/machine/ansi.h /usr/include/sys/stat.h
file.o: /usr/include/sys/time.h /usr/include/sys/types.h
file.o: /usr/include/sys/inttypes.h /usr/include/machine/types.h
file.o: /usr/include/machine/endian.h /usr/include/time.h
file.o: /usr/include/sys/_posix.h /usr/include/varargs.h
pmain.o: innbbsconf.h /usr/include/stdio.h /usr/include/sys/cdefs.h
pmain.o: /usr/include/machine/ansi.h /usr/include/syslog.h
pmain.o: /usr/include/sys/types.h /usr/include/sys/inttypes.h
pmain.o: /usr/include/machine/types.h /usr/include/machine/endian.h
pmain.o: /usr/include/sys/socket.h /usr/include/netinet/in.h
pmain.o: /usr/include/netinet6/in6.h /usr/include/sys/queue.h
pmain.o: /usr/include/netdb.h /usr/include/sys/un.h /usr/include/sys/param.h
pmain.o: /usr/include/sys/syslimits.h /usr/include/sys/signal.h
pmain.o: /usr/include/sys/_posix.h /usr/include/machine/signal.h
pmain.o: /usr/include/machine/trap.h /usr/include/sys/ucontext.h
pmain.o: /usr/include/machine/ucontext.h /usr/include/machine/param.h
pmain.o: /usr/include/machine/limits.h /usr/include/sys/wait.h
pmain.o: /usr/include/unistd.h /usr/include/sys/unistd.h
pmain.o: /usr/include/signal.h /usr/include/sys/time.h /usr/include/time.h
pmain.o: /usr/include/fcntl.h /usr/include/string.h /usr/include/ctype.h
pmain.o: /usr/include/runetype.h /usr/include/errno.h /usr/include/malloc.h
pmain.o: /usr/include/stdlib.h /usr/include/sys/stat.h
pmain.o: /usr/include/sys/file.h /usr/include/sys/fcntl.h daemon.h
his.o: innbbsconf.h /usr/include/stdio.h /usr/include/sys/cdefs.h
his.o: /usr/include/machine/ansi.h /usr/include/syslog.h
his.o: /usr/include/sys/types.h /usr/include/sys/inttypes.h
his.o: /usr/include/machine/types.h /usr/include/machine/endian.h
his.o: /usr/include/sys/socket.h /usr/include/netinet/in.h
his.o: /usr/include/netinet6/in6.h /usr/include/sys/queue.h
his.o: /usr/include/netdb.h /usr/include/sys/un.h /usr/include/sys/param.h
his.o: /usr/include/sys/syslimits.h /usr/include/sys/signal.h
his.o: /usr/include/sys/_posix.h /usr/include/machine/signal.h
his.o: /usr/include/machine/trap.h /usr/include/sys/ucontext.h
his.o: /usr/include/machine/ucontext.h /usr/include/machine/param.h
his.o: /usr/include/machine/limits.h /usr/include/sys/wait.h
his.o: /usr/include/unistd.h /usr/include/sys/unistd.h /usr/include/signal.h
his.o: /usr/include/sys/time.h /usr/include/time.h /usr/include/fcntl.h
his.o: /usr/include/string.h /usr/include/ctype.h /usr/include/runetype.h
his.o: /usr/include/errno.h /usr/include/malloc.h /usr/include/stdlib.h
his.o: /usr/include/sys/stat.h /usr/include/sys/file.h
his.o: /usr/include/sys/fcntl.h bbslib.h his.h dbz.h
dbz.o: /usr/include/stdio.h /usr/include/sys/cdefs.h
dbz.o: /usr/include/machine/ansi.h /usr/include/sys/types.h
dbz.o: /usr/include/sys/inttypes.h /usr/include/machine/types.h
dbz.o: /usr/include/machine/endian.h /usr/include/sys/stat.h
dbz.o: /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/_posix.h
dbz.o: /usr/include/ctype.h /usr/include/runetype.h /usr/include/errno.h
dbz.o: dbz.h clibrary.h /usr/include/string.h /usr/include/memory.h
closeonexec.o: /usr/include/stdio.h /usr/include/sys/cdefs.h
closeonexec.o: /usr/include/machine/ansi.h /usr/include/sys/types.h
closeonexec.o: /usr/include/sys/inttypes.h /usr/include/machine/types.h
closeonexec.o: /usr/include/machine/endian.h /usr/include/errno.h clibrary.h
closeonexec.o: /usr/include/string.h /usr/include/memory.h
closeonexec.o: /usr/include/sgtty.h /usr/include/sys/ioctl.h
closeonexec.o: /usr/include/sys/ttycom.h /usr/include/sys/ioccom.h
closeonexec.o: /usr/include/sys/filio.h /usr/include/sys/sockio.h
closeonexec.o: /usr/include/sys/ioctl_compat.h /usr/include/sys/ttychars.h
closeonexec.o: /usr/include/sys/ttydefaults.h /usr/include/sys/ttydev.h
closeonexec.o: /usr/include/fcntl.h
dbztool.o: /usr/include/sys/file.h /usr/include/sys/fcntl.h
dbztool.o: /usr/include/sys/unistd.h /usr/include/sys/_posix.h his.h
dbztool.o: /usr/include/stdio.h /usr/include/sys/cdefs.h
dbztool.o: /usr/include/machine/ansi.h /usr/include/fcntl.h
dbztool.o: /usr/include/sys/types.h /usr/include/sys/inttypes.h
dbztool.o: /usr/include/machine/types.h /usr/include/machine/endian.h
dbztool.o: /usr/include/ctype.h /usr/include/runetype.h
dbztool.o: /usr/include/sys/stat.h /usr/include/sys/time.h
dbztool.o: /usr/include/time.h /usr/include/sys/param.h
dbztool.o: /usr/include/sys/syslimits.h /usr/include/sys/signal.h
dbztool.o: /usr/include/machine/signal.h /usr/include/machine/trap.h
dbztool.o: /usr/include/sys/ucontext.h /usr/include/machine/ucontext.h
dbztool.o: /usr/include/machine/param.h /usr/include/machine/limits.h dbz.h
inntobbs.o: /usr/include/stdio.h /usr/include/sys/cdefs.h
inntobbs.o: /usr/include/machine/ansi.h daemon.h /usr/include/time.h
inntobbs.o: /usr/include/sys/_posix.h bbslib.h inntobbs.h
bbslib.o: /usr/include/varargs.h innbbsconf.h /usr/include/stdio.h
bbslib.o: /usr/include/sys/cdefs.h /usr/include/machine/ansi.h
bbslib.o: /usr/include/syslog.h /usr/include/sys/types.h
bbslib.o: /usr/include/sys/inttypes.h /usr/include/machine/types.h
bbslib.o: /usr/include/machine/endian.h /usr/include/sys/socket.h
bbslib.o: /usr/include/netinet/in.h /usr/include/netinet6/in6.h
bbslib.o: /usr/include/sys/queue.h /usr/include/netdb.h /usr/include/sys/un.h
bbslib.o: /usr/include/sys/param.h /usr/include/sys/syslimits.h
bbslib.o: /usr/include/sys/signal.h /usr/include/sys/_posix.h
bbslib.o: /usr/include/machine/signal.h /usr/include/machine/trap.h
bbslib.o: /usr/include/sys/ucontext.h /usr/include/machine/ucontext.h
bbslib.o: /usr/include/machine/param.h /usr/include/machine/limits.h
bbslib.o: /usr/include/sys/wait.h /usr/include/unistd.h
bbslib.o: /usr/include/sys/unistd.h /usr/include/signal.h
bbslib.o: /usr/include/sys/time.h /usr/include/time.h /usr/include/fcntl.h
bbslib.o: /usr/include/string.h /usr/include/ctype.h /usr/include/runetype.h
bbslib.o: /usr/include/errno.h /usr/include/malloc.h /usr/include/stdlib.h
bbslib.o: /usr/include/sys/stat.h /usr/include/sys/file.h
bbslib.o: /usr/include/sys/fcntl.h bbslib.h
receive_article.o: innbbsconf.h /usr/include/stdio.h /usr/include/sys/cdefs.h
receive_article.o: /usr/include/machine/ansi.h /usr/include/syslog.h
receive_article.o: /usr/include/sys/types.h /usr/include/sys/inttypes.h
receive_article.o: /usr/include/machine/types.h /usr/include/machine/endian.h
receive_article.o: /usr/include/sys/socket.h /usr/include/netinet/in.h
receive_article.o: /usr/include/netinet6/in6.h /usr/include/sys/queue.h
receive_article.o: /usr/include/netdb.h /usr/include/sys/un.h
receive_article.o: /usr/include/sys/param.h /usr/include/sys/syslimits.h
receive_article.o: /usr/include/sys/signal.h /usr/include/sys/_posix.h
receive_article.o: /usr/include/machine/signal.h /usr/include/machine/trap.h
receive_article.o: /usr/include/sys/ucontext.h
receive_article.o: /usr/include/machine/ucontext.h
receive_article.o: /usr/include/machine/param.h /usr/include/machine/limits.h
receive_article.o: /usr/include/sys/wait.h /usr/include/unistd.h
receive_article.o: /usr/include/sys/unistd.h /usr/include/signal.h
receive_article.o: /usr/include/sys/time.h /usr/include/time.h
receive_article.o: /usr/include/fcntl.h /usr/include/string.h
receive_article.o: /usr/include/ctype.h /usr/include/runetype.h
receive_article.o: /usr/include/errno.h /usr/include/malloc.h
receive_article.o: /usr/include/stdlib.h /usr/include/sys/stat.h
receive_article.o: /usr/include/sys/file.h /usr/include/sys/fcntl.h daemon.h
receive_article.o: bbslib.h inntobbs.h antisplam.h ../mbbsd/record.c
receive_article.o: ../include/config.h ../pttbbs.conf ../include/struct.h
receive_article.o: ../include/modes.h ../include/proto.h
port.o: innbbsconf.h /usr/include/stdio.h /usr/include/sys/cdefs.h
port.o: /usr/include/machine/ansi.h /usr/include/syslog.h
port.o: /usr/include/sys/types.h /usr/include/sys/inttypes.h
port.o: /usr/include/machine/types.h /usr/include/machine/endian.h
port.o: /usr/include/sys/socket.h /usr/include/netinet/in.h
port.o: /usr/include/netinet6/in6.h /usr/include/sys/queue.h
port.o: /usr/include/netdb.h /usr/include/sys/un.h /usr/include/sys/param.h
port.o: /usr/include/sys/syslimits.h /usr/include/sys/signal.h
port.o: /usr/include/sys/_posix.h /usr/include/machine/signal.h
port.o: /usr/include/machine/trap.h /usr/include/sys/ucontext.h
port.o: /usr/include/machine/ucontext.h /usr/include/machine/param.h
port.o: /usr/include/machine/limits.h /usr/include/sys/wait.h
port.o: /usr/include/unistd.h /usr/include/sys/unistd.h /usr/include/signal.h
port.o: /usr/include/sys/time.h /usr/include/time.h /usr/include/fcntl.h
port.o: /usr/include/string.h /usr/include/ctype.h /usr/include/runetype.h
port.o: /usr/include/errno.h /usr/include/malloc.h /usr/include/stdlib.h
port.o: /usr/include/sys/stat.h /usr/include/sys/file.h
port.o: /usr/include/sys/fcntl.h
