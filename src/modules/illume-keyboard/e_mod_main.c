#include "e.h"
#include "e_mod_main.h"
#include "e_kbd_int.h"

/* local variables */
const char *mod_dir = NULL;
static E_Kbd_Int *ki = NULL;

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume Keyboard" };

EAPI void *
e_modapi_init(E_Module *m) 
{
   mod_dir = eina_stringshare_add(m->dir);
   ki = e_kbd_int_new(mod_dir, mod_dir, mod_dir);
   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m) 
{
   if (ki) 
     {
        e_kbd_int_free(ki);
        ki = NULL;
     }
   if (mod_dir) eina_stringshare_del(mod_dir);
   return 1;
}

EAPI int 
e_modapi_save(E_Module *m) 
{
   return 1;
}
