SUBDIR=	mbbsd util
BBSHOME?=$(HOME)

all install clean:
	@for i in $(SUBDIR); do\
		cd $$i;\
		make BBSHOME=$(BBSHOME) $@;\
		cd ..;\
	done
