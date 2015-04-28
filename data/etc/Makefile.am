MAINTAINERCLEANFILES = Makefile.in
filesdir = $(sysconfdir)/enlightenment
files_DATA =

if INSTALL_SYSACTIONS
files_DATA += sysactions.conf
endif

menusdir = $(sysconfdir)/xdg/menus
menus_DATA =

if INSTALL_ENLIGHTENMENT_MENU
menus_DATA += e-applications.menu
endif

EXTRA_DIST = $(files_DATA) $(menus_DATA)
