MAINTAINERCLEANFILES = Makefile.in
MODULE = illume2

# the module .so file
INCLUDES = -I.. \
           -I$(top_srcdir) \
           -I$(top_srcdir)/src/modules/$(MODULE) \
           -I$(top_srcdir)/src/bin \
           -I$(top_srcdir)/src/lib \
           -I$(top_srcdir)/src/modules \
           @e_cflags@

plugindir = $(libdir)/enlightenment/modules/$(MODULE)/policies

samsungdir = $(plugindir)
samsung_LTLIBRARIES = samsung.la
samsung_la_SOURCES = samsung.c layout.c
samsung_la_LIBADD = @e_libs@
samsung_la_LDFLAGS = -no-undefined -module -avoid-version
samsung_la_LIBTOOLFLAGS = --tag=disable-static
