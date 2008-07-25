SUBDIRS = . gvariant-tests

lib_LTLIBRARIES = libgvariant.la

AM_LDFLAGS = -O1
AM_CFLAGS = -Wall -Wwrite-strings -Wmissing-prototypes \
	      -fvisibility=hidden -I$(top_srcdir) $(glib_CFLAGS)

libgvariant_la_LIBADD = $(glib_LIBS)
libgvariant_la_SOURCES = \
	gvarianttype.c		\
	gvarianttypeinfo.c	\
	gvariant-serialiser.c	\
	gvariant-core.c		\
	gvariant-util.c		\
	gvariant-valist.c	\
	gvariant-markup.c

glibincludedir = $(includedir)/glib-2.0/glib
glibinclude_HEADERS = \
	gvarianttype.h		\
	gvariant-loadstore.h	\
	gvariant.h

noinst_HEADERS = \
	gvarianttypeinfo.h	\
	gvariant-serialiser.h	\
	gvariant-private.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gvariant.pc

# automake bug workaround
tags: ctags
