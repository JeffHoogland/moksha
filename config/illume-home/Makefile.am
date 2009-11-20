MAINTAINERCLEANFILES = Makefile.in

EET_EET = @eet_eet@

EXTRA_DIST = \
        profile.desktop \
        icon.png \
	module.battery.src \
	module.dropshadow.src \
	module.pager.src \
	module.illume-home.src

filesdir = $(datadir)/enlightenment/data/config/illume-home
files_DATA = \
        profile.desktop \
        icon.png \
	e.cfg \
	module.battery.cfg \
	module.dropshadow.cfg \
	module.pager.cfg \
	module.illume-home.cfg

%.cfg: %.src
	$(EET_EET) -e \
	$(top_builddir)/config/illume-home/$@ config \
	$< 1

clean-local:
	rm -rf *.cfg *~
	
