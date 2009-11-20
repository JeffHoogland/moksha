#include "e.h"
#include "e_mod_main.h"
#include "e_kbd.h"

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume Keyboard" };
const char *mod_dir = NULL;

EAPI void *
e_modapi_init(E_Module *m) 
{
   if (!e_kbd_init(m)) return NULL;
   mod_dir = eina_stringshare_add(m->dir);
   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m) 
{
   if (mod_dir) eina_stringshare_del(mod_dir);
   e_kbd_shutdown();
   return 1;
}

EAPI int 
e_modapi_save(E_Module *m) 
{
   return 1;
}
