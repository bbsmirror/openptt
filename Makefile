# $Id$
SUBDIR=libptt mbbsd util
BBSHOME?=$(HOME)

depend all install clean depclean:
	@for i in $(SUBDIR); do\
		cd $$i;\
		make BBSHOME=$(BBSHOME) OSTYPE=$(OSTYPE) $@;\
		cd ..;\
	done
