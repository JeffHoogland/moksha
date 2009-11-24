#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_layout.h"
#include "e_kbd.h"

static E_Kbd *kbd = NULL;

/* this is needed to advertise a label for the module IN the code (not just
 * the .desktop file) but more specifically the api version it was compiled
 * for so E can skip modules that are compiled for an incorrect API version
 * safely) */
EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume2" };

/* called first thing when E inits the module */
EAPI void *
e_modapi_init(E_Module *m) 
{
   /* init the config system */
   if (!il_config_init(m)) return NULL;

   /* set up the virtual keyboard */
   e_kbd_init(m);

   /* init the layout system */
   e_mod_layout_init(m);

   kbd = e_kbd_new(e_util_container_zone_number_get(0, 0), 
                   m->dir, m->dir, m->dir);
   e_kbd_show(kbd);

   return m; /* return NULL on failure, anything else on success. the pointer
	      * returned will be set as m->data for convenience tracking */
}

/* called on module shutdown - should clean up EVERYTHING or we leak */
EAPI int
e_modapi_shutdown(E_Module *m) 
{
   e_object_del(E_OBJECT(kbd));
   kbd = NULL;

   e_mod_layout_shutdown();
   e_kbd_shutdown();
   il_config_shutdown();
   return 1; /* 1 for success, 0 for failure */
}

/* called by E when it thinks this module shoudl go save any config it has */
EAPI int
e_modapi_save(E_Module *m) 
{
   return il_config_save();
}
