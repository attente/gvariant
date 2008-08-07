include $(srcdir)/gtester.mk

noinst_PROGRAMS = link-test $(TEST_PROGS)
LIBS            = $(glib_LIBS)
AM_CFLAGS       = -I$(top_srcdir) $(glib_CFLAGS) -g
AM_LDFLAGS      = ../libgvariant.la

TEST_PROGS     += gvariant-big
TEST_PROGS     += gvariant-endian
TEST_PROGS     += gvariant-markup
TEST_PROGS     += gvariant-objpath
TEST_PROGS     += gvariant-random
TEST_PROGS     += gvariant-serialiser
TEST_PROGS     += gvariant-varargs
