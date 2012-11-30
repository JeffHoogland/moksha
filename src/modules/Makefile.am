MAINTAINERCLEANFILES = Makefile.in
AUTOMAKE_OPTIONS = subdir-objects

SUBDIRS =
CLEANFILES =
EXTRA_DIST =
INSTALL_DATA_HOOKS =

MDIR = $(libdir)/enlightenment/modules
AM_LDFLAGS = -module -avoid-version
AM_CPPFLAGS = -I. \
	      -I$(top_srcdir) \
	      -I$(top_srcdir)/src/bin \
	      -I$(top_builddir)/src/bin \
	      -I$(top_srcdir)/src/modules
AM_CFLAGS = @e_cflags@
LIBS += @e_libs@ @dlopen_libs@

EDJE_CC = $(EDJE_CC_$(V))
EDJE_CC_ = $(EDJE_CC_$(AM_DEFAULT_VERBOSITY))
EDJE_CC_0 = @echo "  EDJE_CC " $@; @edje_cc@
EDJE_CC_1 = @edje_cc@

EDJE_FLAGS_VERBOSE_ = $(EDJE_FLAGS_VERBOSE_$(AM_DEFAULT_VERBOSITY))
EDJE_FLAGS_VERBOSE_0 =
EDJE_FLAGS_VERBOSE_1 = -v
EDJE_FLAGS = $(EDJE_FLAGS_VERBOSE_$(V)) @EDJE_DEF@

if USE_MODULE_CONNMAN
include Makefile_connman.am
endif

if USE_MODULE_IBAR
include Makefile_ibar.am
endif

if USE_MODULE_DROPSHADOW
include Makefile_dropshadow.am
endif

if USE_MODULE_CLOCK
include Makefile_clock.am
endif

if USE_MODULE_PAGER
include Makefile_pager.am
endif

if USE_MODULE_BATTERY
include Makefile_battery.am
endif

if USE_MODULE_TEMPERATURE
include Makefile_temperature.am
endif

if USE_MODULE_NOTIFICATION
include Makefile_notification.am
endif

if USE_MODULE_CPUFREQ
include Makefile_cpufreq.am
endif

if USE_MODULE_IBOX
include Makefile_ibox.am
endif

if USE_MODULE_START
include Makefile_start.am
endif

if USE_MODULE_WINLIST
include Makefile_winlist.am
endif

if USE_MODULE_FILEMAN
include Makefile_fileman.am
endif

if USE_MODULE_FILEMAN_OPINFO
include Makefile_fileman_opinfo.am
endif

if USE_MODULE_WIZARD
include Makefile_wizard.am
endif

if USE_MODULE_CONF
include Makefile_conf.am
endif

if USE_MODULE_CONF_WALLPAPER2
include Makefile_conf_wallpaper2.am
endif

if USE_MODULE_CONF_THEME
include Makefile_conf_theme.am
endif

if USE_MODULE_CONF_INTL
include Makefile_conf_intl.am
endif

if USE_MODULE_MSGBUS
include Makefile_msgbus.am
endif

if USE_MODULE_CONF_APPLICATIONS
include Makefile_conf_applications.am
endif

if USE_MODULE_CONF_DISPLAY
include Makefile_conf_display.am
endif

if USE_MODULE_CONF_SHELVES
include Makefile_conf_shelves.am
endif

if USE_MODULE_CONF_KEYBINDINGS
include Makefile_conf_keybindings.am
endif

if USE_MODULE_CONF_EDGEBINDINGS
include Makefile_conf_edgebindings.am
endif

if USE_MODULE_CONF_WINDOW_REMEMBERS
include Makefile_conf_window_remembers.am
endif

if USE_MODULE_CONF_WINDOW_MANIPULATION
include Makefile_conf_window_manipulation.am
endif

if USE_MODULE_CONF_MENUS
include Makefile_conf_menus.am
endif

if USE_MODULE_CONF_DIALOGS
include Makefile_conf_dialogs.am
endif

if USE_MODULE_CONF_PERFORMANCE
include Makefile_conf_performance.am
endif

if USE_MODULE_CONF_PATHS
include Makefile_conf_paths.am
endif

if USE_MODULE_CONF_INTERACTION
include Makefile_conf_interaction.am
endif

if USE_MODULE_CONF_RANDR
include Makefile_conf_randr.am
endif

if USE_MODULE_GADMAN
include Makefile_gadman.am
endif

if USE_MODULE_MIXER
include Makefile_mixer.am
endif

if USE_MODULE_ILLUME2
include Makefile_illume2.am
include Makefile_illume-home.am
include Makefile_illume-home-toggle.am
include Makefile_illume-softkey.am
include Makefile_illume-keyboard.am
include Makefile_illume-indicator.am
include Makefile_illume-kbd-toggle.am
include Makefile_illume-mode-toggle.am
include Makefile_illume-bluetooth.am
endif

if USE_MODULE_SYSCON
include Makefile_syscon.am
endif

if USE_MODULE_EVERYTHING
include Makefile_everything.am
endif

if USE_MODULE_SYSTRAY
include Makefile_systray.am
endif

if USE_MODULE_COMP
include Makefile_comp.am
endif

if USE_MODULE_PHYSICS
include Makefile_physics.am
endif

if USE_MODULE_QUICKACCESS
include Makefile_quickaccess.am
endif

if USE_MODULE_SHOT
include Makefile_shot.am
endif

if USE_MODULE_BACKLIGHT
include Makefile_backlight.am
endif

if USE_MODULE_TASKS
include Makefile_tasks.am
endif

if USE_MODULE_XKBSWITCH
include Makefile_xkbswitch.am
endif

if USE_MODULE_TILING
include Makefile_tiling.am
endif

if USE_MODULE_ACCESS
include Makefile_access.am
endif

#if HAVE_WAYLAND_DRM
#include Makefile_wl_drm.am
#endif

#if HAVE_WAYLAND_SHELL
#include Makefile_wl_shell.am
#endif

#if HAVE_WAYLAND_SCREENSHOT
#include Makefile_wl_screenshot.am
#endif

install-data-hook: $(INSTALL_DATA_HOOKS)
