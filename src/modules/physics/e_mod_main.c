#include "e.h"
#include "e_mod_main.h"
#include "e_mod_physics.h"
#include "e_mod_config.h"
#include <EPhysics.h>

/* module private routines */
Mod *_physics_mod = NULL;

/* public module routines. all modules must have these */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Physics"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   Mod *mod;
   char buf[4096];

   {
      Eina_List *l;
      E_Module *m2;
      EINA_LIST_FOREACH(e_module_list(), l, m2)
        {
           if (m2->enabled && (!strcmp(m2->name, "tiling")))
             {
                e_util_dialog_internal(_("Physics"),
                                       _("Cowardly refusing to battle<br>"
                                         "with the Tiling module for control<br>"
                                         "of your windows. There can be only one!"));
                return NULL;
             }
        }
   }

   if (!ephysics_init()) return NULL;
   mod = calloc(1, sizeof(Mod));
   m->data = mod;

   mod->module = m;

   snprintf(buf, sizeof(buf), "%s/e-module-physics.edj", e_module_dir_get(m));
   e_configure_registry_category_add("appearance", 10, _("Look"), NULL,
                                     "preferences-look");
   e_configure_registry_item_add("appearance/physics", 120, _("Physics"), NULL,
                                 buf, e_int_config_physics_module);
   mod->conf_edd = e_mod_physics_cfdata_edd_init();

   mod->conf = e_config_domain_load("module.physics", mod->conf_edd);
   if (mod->conf)
     {
        if (!e_util_module_config_check("Physics", mod->conf->config_version, MOD_CONFIG_FILE_VERSION))
          {
             e_mod_cfdata_config_free(mod->conf);
             mod->conf = NULL;
          }
     }
   if (mod->conf)
     mod->conf->config_version = MOD_CONFIG_FILE_VERSION;
   else
     _e_mod_config_new(m);
   _physics_mod = mod;

   if (!e_mod_physics_init())
     {
        e_util_dialog_internal(_("Physics Error"), _("The physics module could not be started"));
        e_modapi_shutdown(mod->module);
        return NULL;
     }

   e_module_delayed_set(m, 0);
   e_module_priority_set(m, -1000);
   return mod;
}

void
_e_mod_config_new(E_Module *m)
{
   Mod *mod = m->data;

   mod->conf = e_mod_physics_cfdata_config_new();
}

void
_e_mod_config_free(E_Module *m)
{
   Mod *mod = m->data;

   e_mod_cfdata_config_free(mod->conf);
   mod->conf = NULL;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   Mod *mod = m->data;

   e_mod_physics_shutdown();

   e_configure_registry_item_del("appearance/physics");
   e_configure_registry_category_del("appearance");

   if (mod->config_dialog)
     {
        e_object_del(E_OBJECT(mod->config_dialog));
        mod->config_dialog = NULL;
     }
   _e_mod_config_free(m);

   E_CONFIG_DD_FREE(mod->conf_edd);
   free(mod);

   if (mod == _physics_mod) _physics_mod = NULL;

   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   Mod *mod = m->data;
   e_config_domain_save("module.physics", mod->conf_edd, mod->conf);
   return 1;
}

