SUBDIR=	mbbsd util

all install clean:
	@for i in $(SUBDIR); do\
		cd $$i;\
		make $@;\
		cd ..;\
	done
