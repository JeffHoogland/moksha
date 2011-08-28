MAINTAINERCLEANFILES = Makefile.in
MODULE = illume2
POLICY = tablet

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

tabletdir = $(plugindir)
tablet_LTLIBRARIES = tablet.la
tablet_la_SOURCES = \
                  policy.h \
                  policy.c \
		  tablet.h \
                  tablet.c

tablet_la_LIBADD = @e_libs@
tablet_la_LDFLAGS = -module -avoid-version
tablet_la_LIBTOOLFLAGS = --tag=disable-static
