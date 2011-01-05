MAINTAINERCLEANFILES = Makefile.in
MODULE = illume2
POLICY = illume

# the module .so file
INCLUDES = -I.. \
           -I$(top_srcdir) \
           -I$(top_srcdir)/src/modules/$(MODULE) \
           -I$(top_srcdir)/src/modules/$(MODULE)/policies/$(POLICY) \
           -I$(top_srcdir)/src/bin \
           -I$(top_builddir)/src/bin \
           -I$(top_srcdir)/src/modules \
           @e_cflags@

plugindir = $(libdir)/enlightenment/modules/$(MODULE)/policies

illumedir = $(plugindir)
illume_LTLIBRARIES = illume.la
illume_la_SOURCES = \
                  policy.h \
                  policy.c \
                  illume.h \
                  illume.c

illume_la_LIBADD = @e_libs@
illume_la_LDFLAGS = -module -avoid-version
illume_la_LIBTOOLFLAGS = --tag=disable-static
