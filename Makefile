# $Id$
SUBDIR=libptt mbbsd util
BBSHOME?=$(HOME)

all install depend clean depclean:
	@for i in $(SUBDIR); do\
		cd $$i;\
		make BBSHOME=$(BBSHOME) OSTYPE=$(OSTYPE) $@;\
		cd ..;\
	done
