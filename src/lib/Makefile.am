MAINTAINERCLEANFILES	= Makefile.in
INCLUDES		= -I/usr/local/include \
			  -I$(includedir) \
			  -I../bin/
			  @ecore_cflags@

lib_LTLIBRARIES		= libe.la

include_HEADERS		= E.h

libe_la_SOURCES= \
e_main.c \
e_private.h

libe_la_LIBADD		= @ecore_libs@
libe_la_DEPENDENCIES	= $(top_builddir)/config.h
