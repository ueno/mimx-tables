AC_PREREQ(2.63)
AC_CONFIG_MACRO_DIR([m4])

AC_INIT([mimx-tables], [0.0.0], [ueno@unixuser.org])

dnl Init automake
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE([enable])
AC_GNU_SOURCE

dnl Support silent build
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

dnl Check for programs
AC_PROG_CC
LT_INIT

PKG_CHECK_MODULES([M17N], [m17n-shell], ,
  [AC_MSG_ERROR([m17n-lib not found])])
M17N_MODULE_DIR=`pkg-config --variable=moduledir m17n-shell`
AC_SUBST(M17N_MODULE_DIR)

PKG_CHECK_MODULES([SQLITE3], [sqlite3], ,
  [AC_MSG_ERROR([sqlite3 not found])])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

