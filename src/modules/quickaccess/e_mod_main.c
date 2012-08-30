#include "e_mod_main.h"

EINTERN int _e_quick_access_log_dom = -1;
static E_Config_DD *conf_edd = NULL;
Mod *qa_mod = NULL;
Config *qa_config = NULL;


/**
 * in priority order:
 *
 * @todo config (see e_mod_config.c)
 *
 * @todo custom border based on E_Quick_Access_Entry_Mode/E_Gadcon_Orient
 *
 * @todo show/hide effects:
 *        - fullscreen
 *        - centered
 *        - slide from top, bottom, left or right
 *
 * @todo match more than one, doing tabs (my idea is to do another
 *       tabbing module first, experiment with that, maybe use/reuse
 *       it here)
 */

EAPI E_Module_Api e_modapi = {E_MODULE_API_VERSION, "Quickaccess"};

//////////////////////////////
EAPI void *
e_modapi_init(E_Module *m)
{
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf), "%s/e-module-quickaccess.edj", e_module_dir_get(m));
   e_configure_registry_category_add("launcher", 80, _("Launcher"), NULL,
                                     "preferences-extensions");
   e_configure_registry_item_add("launcher/quickaccess", 1, _("Quickaccess"), NULL,
                                 buf, e_int_config_qa_module);

   qa_mod = E_NEW(Mod, 1);
   qa_mod->module = m;
   m->data = qa_mod;
   conf_edd = e_qa_config_dd_new();
   qa_config = e_config_domain_load("module.quickaccess", conf_edd);
   if (qa_config)
     {
        if (!e_util_module_config_check("Quickaccess", qa_config->config_version, MOD_CONFIG_FILE_VERSION))
          {
             e_qa_config_free(qa_config);
             qa_config = NULL;
          }
     }

   if (!qa_config) qa_config = e_qa_config_new();
   qa_config->config_version = MOD_CONFIG_FILE_VERSION;

   _e_quick_access_log_dom = eina_log_domain_register("quickaccess", EINA_COLOR_ORANGE);
   eina_log_domain_level_set("quickaccess", EINA_LOG_LEVEL_DBG);

   if (!e_qa_init())
     {
        eina_log_domain_unregister(_e_quick_access_log_dom);
        _e_quick_access_log_dom = -1;
        return NULL;
     }

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   e_qa_shutdown();

   conf_edd = e_qa_config_dd_free(conf_edd);
   eina_log_domain_unregister(_e_quick_access_log_dom);
   _e_quick_access_log_dom = -1;
   e_qa_config_free(qa_config);
   free(qa_mod);
   qa_config = NULL;
   qa_mod = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   e_config_domain_save("module.quickaccess", conf_edd, qa_config);
   return 1;
}

