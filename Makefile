SUBDIR=libptt mbbsd util innbbsd
BBSHOME?=$(HOME)

all install clean depend:
	@for i in $(SUBDIR); do\
		cd $$i;\
		make BBSHOME=$(BBSHOME) OSTYPE=$(OSTYPE) $@;\
		cd ..;\
	done
