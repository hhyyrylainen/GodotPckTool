#
# Process this file with autoconf to produce a configure script.
#
AC_REVISION($Revision: 0.8 $)
AC_INIT(MD5, 0.8, [https://github.com/CommanderBubble/MD5])

AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(config)
AC_CONFIG_SRCDIR(./src)
AC_CONFIG_HEADER(src/conf.h)

AC_MSG_NOTICE([Configurations for the MD5 library])

##############################################################################
# arguments

AC_PROG_CXX

AC_PROG_INSTALL
AC_PROG_RANLIB

# we need this for various settings
AC_HEADER_STDC

###############################################################################
#
# check for unsigned-ness
#

AC_MSG_CHECKING([endian-ness])
AC_RUN_IFELSE(
	[AC_LANG_SOURCE([
		int main() { int x = 1; char *s = (char *)&x; if (*s == '\001') return 0; else return 1; }
	])],
	[AC_DEFINE(MD5_BIG_ENDIAN, 0, [Are we a big- or little-endian machine])],
	[AC_DEFINE(MD5_BIG_ENDIAN, 1, [Are we a big- or little-endian machine])],
	[AC_DEFINE(MD5_BIG_ENDIAN, 0, [Are we a big- or little-endian machine])]
)

# HACK: we run the test because AC_RUN_IFELSE wont take multiple actions that i can figure out...
AC_RUN_IFELSE(
	[AC_LANG_SOURCE([
		int main() { int x = 1; char *s = (char *)&x; if (*s == '\001') return 0; else return 1; }
	])],
	[AC_MSG_RESULT([little])],
	[AC_MSG_RESULT([big])],
	[AC_MSG_RESULT([unknown, assume little])]
)

##############################################################################
#
# check if we are enabling debig printing
#

AC_ARG_ENABLE([transform-debugging],
	[AS_HELP_STRING([--enable-transform-debugging],
					[Enable debugging output from internal transforms])]
)

AC_MSG_CHECKING([for transform debugging])
if test "x$enable_transform_debugging" = "xyes"; then
	AC_DEFINE(MD5_DEBUG, 1, [Enable debugging output from internal transforms])
	AC_MSG_RESULT([enabled])
else
	AC_DEFINE(MD5_DEBUG, 0, [Disable debugging output from internal transforms])
	AC_MSG_RESULT([disabled])
fi

AC_CONFIG_FILES(Makefile)
AC_OUTPUT

AC_MSG_NOTICE([])
AC_MSG_NOTICE([To run the library tests, type in:])
AC_MSG_NOTICE([  make check])
AC_MSG_NOTICE([])
