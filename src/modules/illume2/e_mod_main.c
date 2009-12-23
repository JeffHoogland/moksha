#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_layout.h"
#include "e_kbd.h"

/* local variables */
static E_Kbd *kbd = NULL;

/* this is needed to advertise a label for the module IN the code (not just
 * the .desktop file) but more specifically the api version it was compiled
 * for so E can skip modules that are compiled for an incorrect API version
 * safely */
EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume2" };

/* called first thing when E inits the module */
EAPI void *
e_modapi_init(E_Module *m) 
{
   E_Container *con;

   /* init the config subsystem */
   if (!il_config_init(m)) return NULL;

   /* set up the virtual keyboard */
   e_kbd_init(m);

   /* init the layout subsystem */
   e_mod_layout_init(m);

   /* create a new keyboard */
   con = e_container_current_get(e_manager_current_get());
   kbd = 
     e_kbd_new(e_zone_current_get(con), m->dir, m->dir, m->dir);

   /* show the keyboard if needed */
   e_kbd_show(kbd);

   /* return NULL on failure, anything else on success. the pointer
    * returned will be set as m->data for convenience tracking */
   return m;
}

/* called on module shutdown - should clean up EVERYTHING or we leak */
EAPI int
e_modapi_shutdown(E_Module *m) 
{
   /* cleanup the keyboard */
   e_object_del(E_OBJECT(kbd));
   kbd = NULL;

   /* run any layout shutdown code */
   e_mod_layout_shutdown();

   /* shutdown the kbd subsystem */
   e_kbd_shutdown();

   /* shutdown the config subsystem */
   il_config_shutdown();

   /* 1 for success, 0 for failure */
   return 1;
}

/* called by E when it thinks this module should go save any config it has */
EAPI int
e_modapi_save(E_Module *m) 
{
   return il_config_save();
}
