MAINTAINERCLEANFILES	= Makefile.in
INCLUDES		= -I$(includedir) \
			  -I$(top_srcdir)/src/bin
			  @ecore_cflags@ \
			  @evas_cflags@

lib_LTLIBRARIES		= libe.la

include_HEADERS		= E.h

libe_la_SOURCES= \
e_main.c \
e.h \
../bin/e_ipc_codec.c \
../bin/e_ipc_codec.h \
e_private.h

libe_la_LIBADD		= @ecore_libs@ @evas_libs@
libe_la_DEPENDENCIES	= $(top_builddir)/config.h
