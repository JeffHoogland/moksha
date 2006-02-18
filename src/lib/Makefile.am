MAINTAINERCLEANFILES	= Makefile.in
INCLUDES		= -I$(top_srcdir)/src/bin \
			  @e_cflags@

lib_LTLIBRARIES		= libe.la

include_HEADERS		= E_Lib.h

libe_la_SOURCES= \
e_main.c \
E_Lib.h \
e_private.h

libe_la_LIBADD		= @ecore_libs@ @evas_libs@
libe_la_DEPENDENCIES	= $(top_builddir)/config.h
