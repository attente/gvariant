include $(srcdir)/gtester.mk

noinst_PROGRAMS = link-test $(TEST_PROGS)
LIBS            = $(glib_LIBS)
AM_CFLAGS       = $(glib_CFLAGS) -I$(srcdir)/.. -I$(top_srcdir) -g
AM_LDFLAGS      = ../libgvariant.la

TEST_PROGS     += gvariant-endian
TEST_PROGS     += gvariant-markup
TEST_PROGS     += gvariant-random
TEST_PROGS     += gvariant-serialiser
TEST_PROGS     += gvariant-varargs
