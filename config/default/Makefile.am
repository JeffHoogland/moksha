MAINTAINERCLEANFILES = Makefile.in

EET_EET = @eet_eet@

EXTRA_DIST = \
        profile.desktop \
        icon.png \
	e.src

filesdir = $(datadir)/enlightenment/data/config/default
files_DATA = \
        profile.desktop \
        icon.png \
	e.cfg

%.cfg: %.src
	$(EET_EET) -e \
	$(top_builddir)/config/default/$@ config \
	$< 1

clean-local:
	rm -rf *.cfg *~
	
