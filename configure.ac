AC_PREREQ([2.61])
AC_INIT([jel], [1.0], [iam@csl.sri.com])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_AUX_DIR([config-aux])

AM_INIT_AUTOMAKE([foreign  nostdinc silent-rules subdir-objects])

AC_CANONICAL_BUILD

AC_PROG_CC    

AC_PROG_RANLIB

AC_CHECK_HEADER(jpeglib.h)

AC_CHECK_LIB(jpeg, main)

AC_CHECK_LIB(m, main)

AC_CONFIG_FILES([Makefile jel.pc])

AC_OUTPUT

