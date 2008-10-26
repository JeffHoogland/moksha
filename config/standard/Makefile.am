MAINTAINERCLEANFILES = Makefile.in

EET_EET = @eet_eet@

EXTRA_DIST = \
	e.src \
	module.battery.src \
	module.cpufreq.src \
	module.dropshadow.src \
	module.fileman.src \
	module.ibar.src \
	module.ibox.src \
	module.pager.src \
	module.temperature.src

filesdir = $(datadir)/enlightenment/data/config/standard
files_DATA = \
        profile.desktop \
        icon.png \
	e.cfg \
	module.battery.cfg \
	module.cpufreq.cfg \
	module.dropshadow.cfg \
	module.fileman.cfg \
	module.ibar.cfg \
	module.ibox.cfg \
	module.pager.cfg \
	module.temperature.cfg

%.cfg: %.src
	$(EET_EET) -e \
	$(top_builddir)/config/standard/$@ config \
	$< 1

clean-local:
	rm -rf *.cfg *~
	
