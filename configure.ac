AC_PREREQ(2.60)
AC_INIT([huayra-accesibility-settings], [0.1], [mati86dl@gmail.com])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.9.6 -Wall -Werror dist-bzip2])

AC_PROG_CC
# Compiling sources with per-target flags requires AM_PROG_CC_C_O
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(GTK, [gtk+-3.0 >= 3.0])
PKG_CHECK_MODULES(XCURSOR, [xcursor >= 1.0])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
        Makefile
        src/Makefile
])
AC_OUTPUT
