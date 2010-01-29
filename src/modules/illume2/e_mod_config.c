#include "E_Illume.h"
#include "e_mod_config.h"
#include "e_mod_policy.h"
#include "e_mod_animation.h"
#include "e_mod_windows.h"

/* local variables */
static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_zone_edd = NULL;

/* public variables */
EAPI E_Illume_Config*il_cfg = NULL;

int 
e_mod_config_init(E_Module *m) 
{
   conf_zone_edd = E_CONFIG_DD_NEW("Illume_Cfg_Zone", E_Illume_Config_Zone);
   E_CONFIG_VAL(conf_zone_edd, E_Illume_Config_Zone, id, INT);
   E_CONFIG_VAL(conf_zone_edd, E_Illume_Config_Zone, mode.dual, INT);
   E_CONFIG_VAL(conf_zone_edd, E_Illume_Config_Zone, mode.side, INT);

   conf_edd = E_CONFIG_DD_NEW("Illume_Cfg", E_Illume_Config);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, version, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, sliding.kbd.duration, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, sliding.softkey.duration, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, sliding.quickpanel.duration, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.name, STR);

   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.vkbd.class, STR);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.vkbd.name, STR);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.vkbd.title, STR);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.vkbd.win_type, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.vkbd.match.class, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.vkbd.match.name, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.vkbd.match.title, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.vkbd.match.win_type, INT);

   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.softkey.class, STR);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.softkey.name, STR);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.softkey.title, STR);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.softkey.win_type, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.softkey.match.class, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.softkey.match.name, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.softkey.match.title, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.softkey.match.win_type, INT);

   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.home.class, STR);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.home.name, STR);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.home.title, STR);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.home.win_type, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.home.match.class, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.home.match.name, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.home.match.title, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.home.match.win_type, INT);

   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.indicator.class, STR);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.indicator.name, STR);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.indicator.title, STR);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.indicator.win_type, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.indicator.match.class, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.indicator.match.name, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.indicator.match.title, INT);
   E_CONFIG_VAL(conf_edd, E_Illume_Config, policy.indicator.match.win_type, INT);

   E_CONFIG_LIST(conf_edd, E_Illume_Config, policy.zones, conf_zone_edd);

   /* load the config */
   il_cfg = e_config_domain_load("module.illume2", conf_edd);
   if ((il_cfg) && ((il_cfg->version >> 16) < IL_CONFIG_MAJ)) 
     E_FREE(il_cfg);

   if (!il_cfg) 
     {
        E_Illume_Config_Zone *cz;

        cz = E_NEW(E_Illume_Config_Zone, 1);
        cz->id = 0;
        cz->mode.dual = 0;
        cz->mode.side = 0;

        il_cfg = E_NEW(E_Illume_Config, 1);
        il_cfg->version = 0;
        il_cfg->sliding.kbd.duration = 1000;
        il_cfg->sliding.softkey.duration = 1000;
        il_cfg->sliding.quickpanel.duration = 1000;
        il_cfg->policy.name = eina_stringshare_add("illume");
        il_cfg->policy.vkbd.class = 
          eina_stringshare_add("Virtual-Keyboard");
        il_cfg->policy.vkbd.name = 
          eina_stringshare_add("Virtual-Keyboard");
        il_cfg->policy.vkbd.title = 
          eina_stringshare_add("Virtual Keyboard");
        il_cfg->policy.vkbd.win_type = ECORE_X_WINDOW_TYPE_NORMAL;
        il_cfg->policy.vkbd.match.class = 0;
        il_cfg->policy.vkbd.match.name = 1;
        il_cfg->policy.vkbd.match.title = 1;
        il_cfg->policy.vkbd.match.win_type = 0;
        il_cfg->policy.softkey.class = 
          eina_stringshare_add("Illume-Softkey");
        il_cfg->policy.softkey.name = 
          eina_stringshare_add("Illume-Softkey");
        il_cfg->policy.softkey.title = 
          eina_stringshare_add("Illume Softkey");
        il_cfg->policy.softkey.win_type = ECORE_X_WINDOW_TYPE_DOCK;
        il_cfg->policy.softkey.match.class = 0;
        il_cfg->policy.softkey.match.name = 1;
        il_cfg->policy.softkey.match.title = 1;
        il_cfg->policy.softkey.match.win_type = 0;
        il_cfg->policy.home.class = 
          eina_stringshare_add("Illume-Home");
        il_cfg->policy.home.name = 
          eina_stringshare_add("Illume-Home");
        il_cfg->policy.home.title = 
          eina_stringshare_add("Illume Home");
        il_cfg->policy.home.win_type = ECORE_X_WINDOW_TYPE_NORMAL;
        il_cfg->policy.home.match.class = 0;
        il_cfg->policy.home.match.name = 1;
        il_cfg->policy.home.match.title = 1;
        il_cfg->policy.home.match.win_type = 0;
        il_cfg->policy.indicator.class = 
          eina_stringshare_add("Illume-Indicator");
        il_cfg->policy.indicator.name = 
          eina_stringshare_add("Illume-Indicator");
        il_cfg->policy.indicator.title = 
          eina_stringshare_add("Illume Indicator");
        il_cfg->policy.indicator.win_type = ECORE_X_WINDOW_TYPE_DOCK;
        il_cfg->policy.indicator.match.class = 0;
        il_cfg->policy.indicator.match.name = 1;
        il_cfg->policy.indicator.match.title = 1;
        il_cfg->policy.indicator.match.win_type = 0;
        il_cfg->policy.zones = eina_list_append(il_cfg->policy.zones, cz);
     }
   if (il_cfg) 
     {
        /* Add new config variables here */
        /* if ((il_cfg->version & 0xffff) < 1) */
        il_cfg->version = (IL_CONFIG_MAJ << 16) | IL_CONFIG_MIN;
     }
   il_cfg->mod_dir = eina_stringshare_add(m->dir);

   e_configure_registry_category_add("illume", 0, _("Illume"), 
                                     NULL, "enlightenment/display");
   e_configure_registry_generic_item_add("illume/animation", 0, _("Animation"), 
                                         NULL, "enlightenment/animation", 
                                         il_config_animation_show);
   e_configure_registry_generic_item_add("illume/policy", 0, _("Policy"), 
                                         NULL, "enlightenment/policy", 
                                         il_config_policy_show);
   e_configure_registry_generic_item_add("illume/windows", 0, _("Windows"), 
                                         NULL, "enlightenment/windows", 
                                         il_config_windows_show);

   return 1;
}

int 
e_mod_config_shutdown(void) 
{
   E_Illume_Config_Zone *cz;

   e_configure_registry_item_del("illume/windows");
   e_configure_registry_item_del("illume/policy");
   e_configure_registry_item_del("illume/animation");
   e_configure_registry_category_del("illume");

   if (il_cfg->mod_dir) eina_stringshare_del(il_cfg->mod_dir);
   il_cfg->mod_dir = NULL;

   if (il_cfg->policy.name) eina_stringshare_del(il_cfg->policy.name);

   if (il_cfg->policy.vkbd.class) 
     eina_stringshare_del(il_cfg->policy.vkbd.class);
   if (il_cfg->policy.vkbd.name) 
     eina_stringshare_del(il_cfg->policy.vkbd.name);
   if (il_cfg->policy.vkbd.title) 
     eina_stringshare_del(il_cfg->policy.vkbd.title);

   if (il_cfg->policy.softkey.class) 
     eina_stringshare_del(il_cfg->policy.softkey.class);
   if (il_cfg->policy.softkey.name) 
     eina_stringshare_del(il_cfg->policy.softkey.name);
   if (il_cfg->policy.softkey.title) 
     eina_stringshare_del(il_cfg->policy.softkey.title);

   if (il_cfg->policy.home.class) 
     eina_stringshare_del(il_cfg->policy.home.class);
   if (il_cfg->policy.home.name) 
     eina_stringshare_del(il_cfg->policy.home.name);
   if (il_cfg->policy.home.title) 
     eina_stringshare_del(il_cfg->policy.home.title);

   if (il_cfg->policy.indicator.class) 
     eina_stringshare_del(il_cfg->policy.indicator.class);
   if (il_cfg->policy.indicator.name) 
     eina_stringshare_del(il_cfg->policy.indicator.name);
   if (il_cfg->policy.indicator.title) 
     eina_stringshare_del(il_cfg->policy.indicator.title);

   EINA_LIST_FREE(il_cfg->policy.zones, cz)
     E_FREE(cz);

   E_FREE(il_cfg);

   E_CONFIG_DD_FREE(conf_zone_edd);
   E_CONFIG_DD_FREE(conf_edd);

   return 1;
}

int 
e_mod_config_save(void) 
{
   e_config_domain_save("module.illume2", conf_edd, il_cfg);
   return 1;
}

EAPI E_Illume_Config_Zone *
e_illume_zone_config_get(int id) 
{
   Eina_List *l;
   E_Illume_Config_Zone *cz = NULL;

   EINA_LIST_FOREACH(il_cfg->policy.zones, l, cz) 
     {
        if (cz->id != id) continue;
        return cz;
     }

   /* didn't find any existing config. Create new one as fallback */
   cz = E_NEW(E_Illume_Config_Zone, 1);
   cz->id = id;
   cz->mode.dual = 0;
   cz->mode.side = 0;
   il_cfg->policy.zones = eina_list_append(il_cfg->policy.zones, cz);
   e_mod_config_save();
   return cz;
}
