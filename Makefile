SUBDIR=	mbbsd util innbbsd

all install clean depend:
	@for i in $(SUBDIR); do\
		cd $$i;\
		make $@;\
		cd ..;\
	done
