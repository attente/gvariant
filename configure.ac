AC_INIT(gvariant, alpha)
AM_INIT_AUTOMAKE

AC_PROG_LIBTOOL
GTK_DOC_CHECK([1.0])
AC_PROG_CC

PKG_CHECK_MODULES(glib, glib-2.0 >= 2.8)

AC_OUTPUT([
  docs/reference/glib/Makefile
  docs/reference/Makefile
  docs/Makefile

  glib/gvariant-tests/Makefile
  glib/gvariant.pc
  glib/Makefile

  tools/Makefile

  Makefile
])
