# $Id$

include ../Makefile.inc

VERSION=	0.50beta-4
ADMINUSER=	davidyu@oio.cx

CFLAGS+=$(CFLAGS_INND) -pipe -g -O3 -I../include -I../mbbsd\
	-DMapleBBS -DGETRUSAGE -DDBZDEBUG -DVERSION=\"$(VERSION)\"\
	-DADMINUSER=\"$(ADMINUSER)\"
LDFLAGS=-pipe -g -O3

TARGET=	innbbsd mkhistory bbsnnrp ctlinnbbsd bbslink
OBJS=	inndchannel.o innbbsd.o connectsock.o rfc931.o\
	daemon.o file.o pmain.o his.o dbz.o\
	closeonexec.o dbztool.o inntobbs.o receive_article.o\
	echobbslib.o
SRCS=	inndchannel.c innbbsd.c connectsock.c rfc931.c\
	daemon.c file.c pmain.c his.c dbz.c closeonexec.c dbztool.c\
	inntobbs.c bbslib.c receive_article.c port.c
MOBJS=	makedbz.o bbslib.o file.o dbz.o closeonexec.o 
HOBJS=	mkhistory.o bbslib.o file.o his.o dbz.o port.o closeonexec.o
NNRPOBJS=bbsnnrp.o pmain.o bbslib.o connectsock.o file.o 
LINKOBJS=	bbslink.o pmain.o inntobbs.o echobbslib.o connectsock.o\
		file.o port.o
CTLOBJS=ctlinnbbsd.o pmain.o bbslib.o connectsock.o file.o 

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $*.c


all: $(TARGET)

echobbslib.o: echobbslib.c
	$(CC) $(CFLAGS) -DWITH_ECHOMAIL -c echobbslib.c

bbsnnrp: $(NNRPOBJS)
	$(CC) $(LDFLAGS) -o bbsnnrp $(NNRPOBJS)

bbslink: $(LINKOBJS)
	$(CC) $(LDFLAGS) -o bbslink $(LINKOBJS)

ctlinnbbsd: $(CTLOBJS)
	$(CC) $(LDFLAGS) -o ctlinnbbsd $(CTLOBJS)

mkhistory: $(HOBJS)
	$(CC) $(LDFLAGS) -o mkhistory $(HOBJS)

innbbsd: $(OBJS)
	$(CC) $(LDFLAGS) -o innbbsd $(OBJS)

install: $(TARGET)
	install -d $(BBSHOME)/innd/
	install -c -m 755 $(TARGET) $(BBSHOME)/innd/

depend:
	env MKDEP_CPP_OPTS=-MM mkdep $(CFLAGS) $(SRCS)

clean:
	rm -f *.o $(TARGET)
