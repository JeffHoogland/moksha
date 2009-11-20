MAINTAINERCLEANFILES = Makefile.in

EET_EET = @eet_eet@

EXTRA_DIST = \
        profile.desktop \
        icon.png \
	module.battery.src \
	module.dropshadow.src \
	module.pager.src

filesdir = $(datadir)/enlightenment/data/config/illume
files_DATA = \
        profile.desktop \
        icon.png \
	e.cfg \
	module.battery.cfg \
	module.dropshadow.cfg \
	module.pager.cfg

%.cfg: %.src
	$(EET_EET) -e \
	$(top_builddir)/config/illume/$@ config \
	$< 1

clean-local:
	rm -rf *.cfg *~
	
