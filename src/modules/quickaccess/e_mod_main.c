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

static Eina_Bool
_e_mod_cb_config_timer(void *data)
{
   e_util_dialog_show(_("Quickaccess Settings Updated"), "%s", (char *)data);
   return ECORE_CALLBACK_CANCEL;
}

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
        if ((qa_config->config_version >> 16) < MOD_CONFIG_FILE_EPOCH)
          {
             e_qa_config_free(qa_config);
             qa_config = NULL;
             ecore_timer_add(1.0, _e_mod_cb_config_timer,
                             _("Quickaccess module settings data needed upgrading. Your old configuration<br>"
                               "has been wiped and a new set of defaults initialized. This<br>"
                               "will happen regularly during development, so don't report a<br>"
                               "bug. This means Quickaccess module needs new configuration<br>"
                               "data by default for usable functionality that your old<br>"
                               "configuration lacked. This new set of defaults will fix<br>"
                               "that by adding it in. You can re-configure things now to your<br>"
                               "liking. Sorry for the hiccup in your configuration.<br>"));
          }
        else if (qa_config->config_version > MOD_CONFIG_FILE_VERSION)
          {
             e_qa_config_free(qa_config);
             qa_config = NULL;
             ecore_timer_add(1.0, _e_mod_cb_config_timer,
                             _("Your Quickaccess module configuration is NEWER than Quickaccess module version. This is very<br>"
                               "strange. This should not happen unless you downgraded<br>"
                               "the Quickaccess Module or copied the configuration from a place where<br>"
                               "a newer version of the Quickaccess Module was running. This is bad and<br>"
                               "as a precaution your configuration has been now restored to<br>"
                               "defaults. Sorry for the inconvenience.<br>"));
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

