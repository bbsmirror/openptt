SUBDIR=	etc innd

all:

install:
	@for i in $(SUBDIR); do\
		cd $$i;\
		make $@;\
		cd ..;\
	done
