MAINTAINERCLEANFILES = Makefile.in

EET_EET = @eet_eet@

EXTRA_DIST = \
	e.src \
	module.battery.src \
	module.dropshadow.src \
	module.illume.src \
	module.mixer.src \
	module.pager.src

filesdir = $(datadir)/enlightenment/data/config/illume
files_DATA = \
        profile.desktop \
        icon.png \
	e.cfg \
	module.battery.cfg \
	module.dropshadow.cfg \
	module.illume.cfg \
	module.mixer.cfg \
	module.pager.cfg

%.cfg: %.src
	$(EET_EET) -e \
	$(top_builddir)/config/illume/$@ config \
	$< 1

clean-local:
	rm -rf *.cfg *~
	
