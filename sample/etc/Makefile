SUBDIR=
BBSHOME?=$(HOME)
TARGET=$(BBSHOME)/etc/
FILES=	Welcome Welcome_login goodbye register registered ve.hlp Logout\
	Welcome_birth feast today_boring

all:

install_sub:
	@for i in $(SUBDIR); do\
		cd $$i;\
		make BBSHOME=$(BBSHOME) OSTYPE=$(OSTYPE) install;\
		cd ..;\
	done

install: install_sub
	install -d $(TARGET)
	install -c -m 644 $(FILES) $(TARGET)
