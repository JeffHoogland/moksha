#ifndef E_MOD_MAIN_H
# define E_MOD_MAIN_H

# define SLOGFNS 1

# ifdef SLOGFNS
#  include <stdio.h>
#  define SLOGFN(fl, ln, fn) printf("-E-SHELL: %25s: %5i - %s\n", fl, ln, fn);
# else
#  define SLOGFN(fl, ln, fn)
# endif

# include <wayland-server.h>

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

#endif
