MAINTAINERCLEANFILES = Makefile.in
MODULE = cpufreq

# data files for the module
filesdir = $(libdir)/enlightenment/modules/$(MODULE)
files_DATA = \
module_icon.png
freqsetdir = $(libdir)/enlightenment/modules/$(MODULE)
freqset_DATA = \
freqset$(EXEEXT)

EXTRA_DIST = $(files_DATA)

# the module .so file
INCLUDES               = -I. \
			 -I$(top_srcdir) \
			 -I$(includedir) \
		         -I$(top_srcdir)$(MODULE) \
		         -I$(top_srcdir)/src/bin \
			 -I$(top_srcdir)/src/lib \
		         -I$(top_srcdir)/src/modules \
			 @e_cflags@
pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la
module_la_SOURCES      = e_mod_main.c \
		         e_mod_main.h
module_la_LIBADD       = @e_libs@ @dlopen_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

all-local: freqset$(EXEEXT)
installed_su = $(pkgdir)/freqset$(EXEEXT)
setuid_root_mode = a=rx,u+s

INSTALL_SU = \
  echo "$(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install install -m $(setuid_root_mode) freqset$(EXEEXT) $(installed_su)"; \
  mkdir $(pkgdir) -p; \
  $(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install install -m $(setuid_root_mode) freqset$(EXEEXT) $(installed_su); \
  #echo " chown root $(installed_su)"; \
  #chown root $(installed_su); \
  #echo " chmod $(setuid_root_mode) $(installed_su)"; \
  #chmod $(setuid_root_mode) $(installed_su)

install-freqsetDATA: freqset$(EXEEXT)
	@$(INSTALL_SU)

noinst_PROGRAMS        = freqset
freqset_SOURCES        = freqset.c
