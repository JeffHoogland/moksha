MAINTAINERCLEANFILES = Makefile.in
MODULE = illume2

# the module .so file
INCLUDES = -I.. \
           -I$(top_srcdir) \
           -I$(top_srcdir)/src/modules/$(MODULE) \
           -I$(top_srcdir)/src/modules/$(MODULE)/policies/illume \
           -I$(top_srcdir)/src/bin \
           -I$(top_srcdir)/src/lib \
           -I$(top_srcdir)/src/modules \
           @e_cflags@

plugindir = $(libdir)/enlightenment/modules/$(MODULE)/policies

illumedir = $(plugindir)
illume_LTLIBRARIES = illume.la
illume_la_SOURCES = illume.c illume.h layout.c layout.h
illume_la_LIBADD = @e_libs@
illume_la_LDFLAGS = -no-undefined -module -avoid-version
illume_la_LIBTOOLFLAGS = --tag=disable-static
