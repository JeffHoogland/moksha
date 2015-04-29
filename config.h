/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */


#ifndef EFL_CONFIG_H__
#define EFL_CONFIG_H__


/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* enable Files menu item */
#define ENABLE_FILES 1

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
#define ENABLE_NLS 1

/* "This define can be used to wrap internal E stuff */
#define E_INTERNAL 1

/* Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
#define HAVE_ALLOCA_H 1

/* Define if the ALSA output plugin should be built */
#define HAVE_ALSA 1

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Package BATTERY ( ecore >= $efl_version ecore-file >= $efl_version
   ecore-con >= $efl_version eina >= $efl_version ) found. */
#define HAVE_BATTERY 1

/* Define to 1 if you have the <CFBase.h> header file. */
/* #undef HAVE_CFBASE_H */

/* Define to 1 if you have the MacOS X function CFLocaleCopyCurrent in the
   CoreFoundation framework. */
/* #undef HAVE_CFLOCALECOPYCURRENT */

/* Define to 1 if you have the MacOS X function CFPreferencesCopyAppValue in
   the CoreFoundation framework. */
/* #undef HAVE_CFPREFERENCESCOPYAPPVALUE */

/* Define to 1 if you have the `clearenv' function. */
#define HAVE_CLEARENV 1

/* Define if the GNU dcgettext() function is already present or preinstalled.
   */
#define HAVE_DCGETTEXT 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Package ECONNMAN ( edbus >= $efl_version ) found. */
#define HAVE_ECONNMAN 1

/* Package ECORE_IMF ( ecore-imf >= ${efl_version} ecore-imf-evas >=
   ${efl_version} ) found. */
#define HAVE_ECORE_IMF 1

/* enable udev support */
#define HAVE_EEZE 1

/* enable eeze mounting */
/* #undef HAVE_EEZE_MOUNT */

/* "Have Elementary support" */
#define HAVE_ELEMENTARY 1

/* "Have emotion support" */
#define HAVE_EMOTION 1

/* Package ENOTIFY ( edbus >= $efl_version enotify >= $efl_version ) found. */
#define HAVE_ENOTIFY 1

/* Have environ var */
#define HAVE_ENVIRON 1

/* Package EPHYSICS ( ephysics ) found. */
#define HAVE_EPHYSICS 1

/* Define to 1 if you have the <execinfo.h> header file. */
#define HAVE_EXECINFO_H 1

/* Define to 1 if you have the <features.h> header file. */
#define HAVE_FEATURES_H 1

/* Define to 1 if you have the `fnmatch' function. */
#define HAVE_FNMATCH 1

/* Define to 1 if you have the <fnmatch.h> header file. */
#define HAVE_FNMATCH_H 1

/* Define if the GNU gettext() function is already present or preinstalled. */
#define HAVE_GETTEXT 1

/* enable HAL support */
/* #undef HAVE_HAL */

/* enable HAL mounting */
#define HAVE_HAL_MOUNT 1

/* Define if you have the iconv() function and it works. */
/* #undef HAVE_ICONV */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* PAM Authentication Support */
#define HAVE_PAM 1

/* Define to 1 if you have the <security/pam_appl.h> header file. */
#define HAVE_SECURITY_PAM_APPL_H 1

/* Define to 1 if you have the `setenv' function. */
#define HAVE_SETENV 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/ptrace.h> header file. */
#define HAVE_SYS_PTRACE_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/timerfd.h> header file. */
#define HAVE_SYS_TIMERFD_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Package TEMPERATURE ( ecore >= $efl_version ecore-file >= $efl_version eina
   >= $efl_version ) found. */
#define HAVE_TEMPERATURE 1

/* enable Udisks mounting */
#define HAVE_UDISKS_MOUNT 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `unsetenv' function. */
#define HAVE_UNSETENV 1

/* enable wayland client support */
/* #undef HAVE_WAYLAND_CLIENTS */

/* Define to 1 if your compiler has __attribute__ */
#define HAVE___ATTRIBUTE__ 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* "Module architecture" */
#define MODULE_ARCH "linux-gnu-x86_64-0.17.6"

/* Name of package */
#define PACKAGE "enlightenment"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "enlightenment-devel@lists.sourceforge.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "enlightenment"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "enlightenment 0.17.6"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "enlightenment"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.17.6"

/* default value since PATH_MAX is not defined */
/* #undef PATH_MAX */

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at runtime.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Use module access */
/* #undef USE_MODULE_ACCESS */

/* Use module backlight */
#define USE_MODULE_BACKLIGHT 1

/* Use module battery */
#define USE_MODULE_BATTERY 1

/* Use module clock */
#define USE_MODULE_CLOCK 1

/* Use module comp */
#define USE_MODULE_COMP 1

/* Use module conf */
#define USE_MODULE_CONF 1

/* Use module conf-applications */
#define USE_MODULE_CONF_APPLICATIONS 1

/* Use module conf-dialogs */
#define USE_MODULE_CONF_DIALOGS 1

/* Use module conf-display */
#define USE_MODULE_CONF_DISPLAY 1

/* Use module conf-edgebindings */
#define USE_MODULE_CONF_EDGEBINDINGS 1

/* Use module conf-interaction */
#define USE_MODULE_CONF_INTERACTION 1

/* Use module conf-intl */
#define USE_MODULE_CONF_INTL 1

/* Use module conf-keybindings */
#define USE_MODULE_CONF_KEYBINDINGS 1

/* Use module conf-menus */
#define USE_MODULE_CONF_MENUS 1

/* Use module conf-paths */
#define USE_MODULE_CONF_PATHS 1

/* Use module conf-performance */
#define USE_MODULE_CONF_PERFORMANCE 1

/* Use module conf-randr */
#define USE_MODULE_CONF_RANDR 1

/* Use module conf-shelves */
#define USE_MODULE_CONF_SHELVES 1

/* Use module conf-theme */
#define USE_MODULE_CONF_THEME 1

/* Use module conf-wallpaper2 */
/* #undef USE_MODULE_CONF_WALLPAPER2 */

/* Use module conf-window-manipulation */
#define USE_MODULE_CONF_WINDOW_MANIPULATION 1

/* Use module conf-window-remembers */
#define USE_MODULE_CONF_WINDOW_REMEMBERS 1

/* Use module connman */
#define USE_MODULE_CONNMAN 1

/* Use module cpufreq */
#define USE_MODULE_CPUFREQ 1

/* Use module dropshadow */
#define USE_MODULE_DROPSHADOW 1

/* Use module everything */
#define USE_MODULE_EVERYTHING 1

/* Use module fileman */
#define USE_MODULE_FILEMAN 1

/* Use module fileman-opinfo */
#define USE_MODULE_FILEMAN_OPINFO 1

/* Use module gadman */
#define USE_MODULE_GADMAN 1

/* Use module ibar */
#define USE_MODULE_IBAR 1

/* Use module ibox */
#define USE_MODULE_IBOX 1

/* Use module illume2 */
#define USE_MODULE_ILLUME2 1

/* Use module mixer */
#define USE_MODULE_MIXER 1

/* Use module msgbus */
#define USE_MODULE_MSGBUS 1

/* Use module notification */
#define USE_MODULE_NOTIFICATION 1

/* Use module pager */
#define USE_MODULE_PAGER 1

/* Use module physics */
/* #undef USE_MODULE_PHYSICS */

/* Use module quickaccess */
#define USE_MODULE_QUICKACCESS 1

/* Use module shot */
#define USE_MODULE_SHOT 1

/* Use module start */
#define USE_MODULE_START 1

/* Use module syscon */
#define USE_MODULE_SYSCON 1

/* Use module systray */
#define USE_MODULE_SYSTRAY 1

/* Use module tasks */
#define USE_MODULE_TASKS 1

/* Use module temperature */
#define USE_MODULE_TEMPERATURE 1

/* Use module tiling */
#define USE_MODULE_TILING 1

/* Use module winlist */
#define USE_MODULE_WINLIST 1

/* Use module wizard */
#define USE_MODULE_WIZARD 1

/* Use module xkbswitch */
#define USE_MODULE_XKBSWITCH 1

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif


/* Version number of package */
#define VERSION "0.17.6"

/* Major version */
#define VMAJ 0

/* Micro version */
#define VMIC 6

/* Minor version */
#define VMIN 17

/* Revison */
#define VREV 0

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */

/* Macro declaring a function argument to be unused */
#define __UNUSED__ __attribute__((unused))

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* "apple doesn't follow POSIX in this case." */
/* #undef environ */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */


#endif /* EFL_CONFIG_H__ */

