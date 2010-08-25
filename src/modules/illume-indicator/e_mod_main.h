#ifndef E_MOD_MAIN_H
# define E_MOD_MAIN_H

# ifdef HAVE_ENOTIFY
#  include <E_Notification_Daemon.h>
# endif

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

extern const char *_ind_mod_dir;

#endif
