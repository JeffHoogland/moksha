MAINTAINERCLEANFILES = Makefile.in

EET_EET = @eet_eet@

EXTRA_DIST = \
		profile.desktop \
		icon.png \
		e_randr.src \
		e_remember_restart.src \
		e.src \
		exehist.src \
		module.battery.src \
		module.clock.src \
		module.comp.src \
		module.conf.src \
		module.everything-files.src \
		module.everything.src \
		module.fileman.src \
		module.gadman.src \
		module.ibar.src \
		module.mixer.src \
		module.notification.src \
		module.pager.src \
		module.places.src \
		module.syscon.src \
		module.tasks.src

filesdir = $(datadir)/enlightenment/data/config/bodhi
files_DATA = \
		profile.desktop \
		icon.png \
		e_randr.cfg \
		e_remember_restart.cfg \
		e.cfg \
		exehist.cfg \
		module.battery.cfg \
		module.clock.cfg \
		module.comp.cfg \
		module.conf.cfg \
		module.everything-files.cfg \
		module.everything.cfg \
		module.fileman.cfg \
		module.gadman.cfg \
		module.ibar.cfg \
		module.mixer.cfg \
		module.notification.cfg \
		module.pager.cfg \
		module.places.cfg \
		module.syscon.cfg \
		module.tasks.cfg

SUFFIXES = .cfg

.src.cfg:
	$(EET_EET) -e \
	$(top_builddir)/config/bodhi/$@ config \
	$< 1

clean-local:
	rm -rf *.cfg *~
