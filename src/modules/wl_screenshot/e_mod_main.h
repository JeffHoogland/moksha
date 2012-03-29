#ifndef E_MOD_MAIN_H
# define E_MOD_MAIN_H

# include <wayland-client.h>
# include "e_screenshooter_client_protocol.h"

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

#endif
