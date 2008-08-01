include $(srcdir)/gtester.mk

noinst_PROGRAMS = link-test $(TEST_PROGS)
LIBS            = $(glib_LIBS)
AM_CFLAGS       = $(glib_CFLAGS) -I$(srcdir)/.. -I$(top_srcdir) -g
AM_LDFLAGS      = ../libgvariant.la

TEST_PROGS     += varargs
