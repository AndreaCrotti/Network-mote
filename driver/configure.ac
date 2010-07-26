#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.66])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([struct_manage.h])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE()

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# TODO: do we need any libraries? Maybe TOSROOT compiled code

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h syslog.h termios.h unistd.h linux/if_tun.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([bzero gethostbyname inet_ntoa memset select socket])

AC_CONFIG_FILES([Makefile_auto])
AC_OUTPUT
