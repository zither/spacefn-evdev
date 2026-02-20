#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([spacefn], [0.1], [ilorn.mc@gmail.com])
AM_INIT_AUTOMAKE([foreign])
PKG_PROG_PKG_CONFIG
EXTERNAL_CFLAGS="$CFLAGS"

AC_CONFIG_SRCDIR([src/spacefn.c])
#AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h libconfig.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
#AC_CHECK_FUNCS([select strerror])

PKG_CHECK_MODULES([EVDEV], [libevdev], [], [echo "Did not find libevdev"; exit -1])
PKG_CHECK_MODULES([CONFIG], [libconfig], [], [echo "Did not find libconfig"; exit -1])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
