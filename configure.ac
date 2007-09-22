dnl
dnl Copyright (c) 2006 Claes Nästén <me@pekdon.net>
dnl
dnl Permission is hereby granted, free of charge, to any person
dnl obtaining a copy of this software and associated documentation
dnl files (the "Software"), to deal in the Software without
dnl restriction, including without limitation the rights to use, copy,
dnl modify, merge, publish, distribute, sublicense, and/or sell copies
dnl of the Software, and to permit persons to whom the Software is
dnl furnished to do so, subject to the following conditions:
dnl
dnl The above copyright notice and this permission notice shall be
dnl included in all copies or substantial portions of the Software.
dnl
dnl THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
dnl EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
dnl MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
dnl NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
dnl BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
dnl ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
dnl CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
dnl SOFTWARE.
dnl

dnl $Id$

dnl init autoconf / automake
AC_INIT(geh, 0.2.0)
AC_CONFIG_SRCDIR(src/main.c)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

dnl search for programs
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

dnl search for headers
AC_STDC_HEADERS
AC_HAVE_HEADERS(sys/types.h sys/stat.h unistd.h)

dnl check required libraries
AM_PATH_GLIB_2_0(2.0.0,,AC_MSG_ERROR(GLIB >= 2.0.0 not installed.))
AM_PATH_GTK_2_0(2.8.0,,AC_MSG_ERROR(GTK+2 >= 2.8.0 not installed.),gthread)

dnl output makefiles
AC_OUTPUT(Makefile doc/Makefile src/Makefile)
