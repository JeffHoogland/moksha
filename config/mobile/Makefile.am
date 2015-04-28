MAINTAINERCLEANFILES = Makefile.in

EET_EET = @eet_eet@

EXTRA_DIST = \
        profile.desktop \
        enlightenment-mobile.png \
        e.src \
	module.battery.src \
	module.illume-home.src \
	module.illume-keyboard.src \
	module.illume-indicator.src \
	module.illume-softkey.src \
        module.illume2.src \
        module.conf.src

filesdir = $(datadir)/enlightenment/data/config/mobile
files_DATA = \
        profile.desktop \
        enlightenment-mobile.png \
	e.cfg \
	module.battery.cfg \
	module.illume-home.cfg \
        module.illume-keyboard.cfg \
        module.illume-indicator.cfg \
        module.illume-softkey.cfg \
        module.illume2.cfg \
        module.conf.cfg

SUFFIXES = .cfg

.src.cfg:
	$(EET_EET) -e \
	$(top_builddir)/config/mobile/$@ config \
	$< 1

clean-local:
	rm -rf *.cfg *~
