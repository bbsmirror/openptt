# Process this file with autoconf to produce a configure script.
AC_INIT("PttBBS", "1.1.0", "me@davidyu.org")
AC_CONFIG_SRCDIR([include/common.h])
AC_PREFIX_DEFAULT([/usr/local/pttbbs])

AC_SUBST(CFLAGS)
CFLAGS="-pipe -Wall -g -O3"

AC_SUBST(LDFLAGS)
LDFLAGS="-pipe -Wall -g -O3"

echo "Please enter the following questions..."
echo

AC_SUBST(bbsuser)
AC_SUBST(bbsuid)
AC_SUBST(bbsgid)
echo -n "BBS User Account: "; read bbsuser
bbsuid=`id -u $bbsuser`
bbsgid=`id -g $bbsuser`

AC_SUBST(bbsname)
echo -n "BBS Name: "; read bbsname

AC_SUBST(myhostname)
echo -n "Hostname: "; read myhostname

AC_SUBST(myip)
echo -n "IP: "; read myip

AC_SUBST(max_users)
echo -n "Max Users: "; read max_users

echo

AC_CANONICAL_TARGET
AC_SUBST(OSDEFS)
case "$target_os" in
freebsd*)
	OSDEFS="-DFreeBSD"
	;;
linux*)
	OSDEFS="-DLinux"
esac

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.
AC_SEARCH_LIBS(setproctitle, util, AC_DEFINE(HAVE_SETPROCTITLE))
AC_SEARCH_LIBS(crypt, crypt)

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_GETLOADAVG

AC_CONFIG_FILES([Makefile
		 mbbsd/Makefile
		 util/Makefile
		 innbbsd/Makefile])
AC_OUTPUT
