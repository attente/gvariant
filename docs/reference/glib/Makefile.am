DOC_MODULE = gvariant

DOC_MAIN_SGML_FILE = gvariant-docs.xml
DOC_SOURCE_DIR = $(top_srcdir)/glib

IGNORE_HFILES = gsignature-iterate.h gvariant-private.h gsvariant.h gsvhelper.h
HFILE_GLOB = $(top_srcdir)/glib/*.h
CFILE_GLOB = $(top_srcdir)/glib/*.c
MKDB_OPTIONS = --output-format=xml --sgml-mode

include ../gtk-doc.make

CLEANFILES += \
	gvariant-overrides.txt \
	gvariant-decl-list.txt \
	gvariant-undocumented.txt \
	gvariant-decl.txt \
	-r html tmpl xml
