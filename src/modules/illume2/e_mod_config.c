#include "e_illume_private.h"
#include "e_mod_config.h"
#include "e_mod_config_animation.h"
#include "e_mod_config_windows.h"
#include "e_mod_config_policy.h"

/* local function prototypes */
static void _e_mod_illume_config_free(void);
static void _e_mod_illume_config_new(void);

/* local variables */
static E_Config_DD *_il_conf_edd = NULL;
static E_Config_DD *_il_conf_zone_edd = NULL;

/* external variables */
E_Illume_Config *_e_illume_cfg = NULL;

int 
e_mod_illume_config_init(void) 
{
   /* create config structure for zones */
   _il_conf_zone_edd = E_CONFIG_DD_NEW("Illume_Config_Zone", E_Illume_Config_Zone);
#undef T
#undef D
#define T E_Illume_Config_Zone
#define D _il_conf_zone_edd
   E_CONFIG_VAL(D, T, id, INT);
   E_CONFIG_VAL(D, T, mode.dual, INT);
   E_CONFIG_VAL(D, T, mode.side, INT);

   /* create config structure for module */
   _il_conf_edd = E_CONFIG_DD_NEW("Illume_Config", E_Illume_Config);
#undef T
#undef D
#define T E_Illume_Config
#define D _il_conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, animation.vkbd.duration, INT);
   E_CONFIG_VAL(D, T, animation.vkbd.resize_before, INT);
   E_CONFIG_VAL(D, T, animation.quickpanel.duration, INT);
   E_CONFIG_VAL(D, T, policy.name, STR);
   E_CONFIG_VAL(D, T, policy.vkbd.class, STR);
   E_CONFIG_VAL(D, T, policy.vkbd.name, STR);
   E_CONFIG_VAL(D, T, policy.vkbd.title, STR);
   E_CONFIG_VAL(D, T, policy.vkbd.type, INT);
   E_CONFIG_VAL(D, T, policy.vkbd.match.class, INT);
   E_CONFIG_VAL(D, T, policy.vkbd.match.name, INT);
   E_CONFIG_VAL(D, T, policy.vkbd.match.title, INT);
   E_CONFIG_VAL(D, T, policy.vkbd.match.type, INT);
   E_CONFIG_VAL(D, T, policy.indicator.class, STR);
   E_CONFIG_VAL(D, T, policy.indicator.name, STR);
   E_CONFIG_VAL(D, T, policy.indicator.title, STR);
   E_CONFIG_VAL(D, T, policy.indicator.type, INT);
   E_CONFIG_VAL(D, T, policy.indicator.match.class, INT);
   E_CONFIG_VAL(D, T, policy.indicator.match.name, INT);
   E_CONFIG_VAL(D, T, policy.indicator.match.title, INT);
   E_CONFIG_VAL(D, T, policy.indicator.match.type, INT);
   E_CONFIG_VAL(D, T, policy.softkey.class, STR);
   E_CONFIG_VAL(D, T, policy.softkey.name, STR);
   E_CONFIG_VAL(D, T, policy.softkey.title, STR);
   E_CONFIG_VAL(D, T, policy.softkey.type, INT);
   E_CONFIG_VAL(D, T, policy.softkey.match.class, INT);
   E_CONFIG_VAL(D, T, policy.softkey.match.name, INT);
   E_CONFIG_VAL(D, T, policy.softkey.match.title, INT);
   E_CONFIG_VAL(D, T, policy.softkey.match.type, INT);
   E_CONFIG_VAL(D, T, policy.home.class, STR);
   E_CONFIG_VAL(D, T, policy.home.name, STR);
   E_CONFIG_VAL(D, T, policy.home.title, STR);
   E_CONFIG_VAL(D, T, policy.home.type, INT);
   E_CONFIG_VAL(D, T, policy.home.match.class, INT);
   E_CONFIG_VAL(D, T, policy.home.match.name, INT);
   E_CONFIG_VAL(D, T, policy.home.match.title, INT);
   E_CONFIG_VAL(D, T, policy.home.match.type, INT);
   E_CONFIG_LIST(D, T, policy.zones, _il_conf_zone_edd);

   /* attempt to load existing configuration */
   _e_illume_cfg = e_config_domain_load("module.illume2", _il_conf_edd);

   /* check version */
   if ((_e_illume_cfg) && ((_e_illume_cfg->version >> 16) < IL_CONFIG_MAJOR))
     _e_mod_illume_config_free();

   /* create new config if we need to */
   if (!_e_illume_cfg) _e_mod_illume_config_new();

   /* setup category for config panel */
   e_configure_registry_category_add("illume", 0, _("Illume"), NULL, "preferences-illume");

   /* add config items to category */
   e_configure_registry_generic_item_add("illume/policy", 0, _("Policy"), 
                                         NULL, "preferences-profiles", 
                                         e_mod_illume_config_policy_show);
   e_configure_registry_generic_item_add("illume/animation", 0, _("Animation"), 
                                         NULL, "preferences-transitions", 
                                         e_mod_illume_config_animation_show);
   e_configure_registry_generic_item_add("illume/windows", 0, _("Windows"), 
                                         NULL, "preferences-winlist", 
                                         e_mod_illume_config_windows_show);

   return 1;
}

int 
e_mod_illume_config_shutdown(void) 
{
   /* destroy config item entries */
   e_configure_registry_item_del("illume/windows");
   e_configure_registry_item_del("illume/animation");
   e_configure_registry_item_del("illume/policy");

   /* destroy config category */
   e_configure_registry_category_del("illume");

   /* free config structure */
   _e_mod_illume_config_free();

   /* free data descriptors */
   E_CONFIG_DD_FREE(_il_conf_zone_edd);
   E_CONFIG_DD_FREE(_il_conf_edd);

   return 1;
}

int 
e_mod_illume_config_save(void) 
{
   return e_config_domain_save("module.illume2", _il_conf_edd, _e_illume_cfg);
}

/* local functions */
static void 
_e_mod_illume_config_free(void) 
{
   E_Illume_Config_Zone *cz;

   /* check for config */
   if (!_e_illume_cfg) return;

   /* cleanup any stringshares */
   if (_e_illume_cfg->policy.name) 
     eina_stringshare_del(_e_illume_cfg->policy.name);
   _e_illume_cfg->policy.name = NULL;

   if (_e_illume_cfg->policy.vkbd.class) 
     eina_stringshare_del(_e_illume_cfg->policy.vkbd.class);
   _e_illume_cfg->policy.vkbd.class = NULL;
   if (_e_illume_cfg->policy.vkbd.name) 
     eina_stringshare_del(_e_illume_cfg->policy.vkbd.name);
   _e_illume_cfg->policy.vkbd.name = NULL;
   if (_e_illume_cfg->policy.vkbd.title) 
     eina_stringshare_del(_e_illume_cfg->policy.vkbd.title);
   _e_illume_cfg->policy.vkbd.title = NULL;

   if (_e_illume_cfg->policy.indicator.class) 
     eina_stringshare_del(_e_illume_cfg->policy.indicator.class);
   _e_illume_cfg->policy.indicator.class = NULL;
   if (_e_illume_cfg->policy.indicator.name) 
     eina_stringshare_del(_e_illume_cfg->policy.indicator.name);
   _e_illume_cfg->policy.indicator.name = NULL;
   if (_e_illume_cfg->policy.indicator.title) 
     eina_stringshare_del(_e_illume_cfg->policy.indicator.title);
   _e_illume_cfg->policy.indicator.title = NULL;

   if (_e_illume_cfg->policy.softkey.class) 
     eina_stringshare_del(_e_illume_cfg->policy.softkey.class);
   _e_illume_cfg->policy.softkey.class = NULL;
   if (_e_illume_cfg->policy.softkey.name) 
     eina_stringshare_del(_e_illume_cfg->policy.softkey.name);
   _e_illume_cfg->policy.softkey.name = NULL;
   if (_e_illume_cfg->policy.softkey.title) 
     eina_stringshare_del(_e_illume_cfg->policy.softkey.title);
   _e_illume_cfg->policy.softkey.title = NULL;

   if (_e_illume_cfg->policy.home.class) 
     eina_stringshare_del(_e_illume_cfg->policy.home.class);
   _e_illume_cfg->policy.home.class = NULL;
   if (_e_illume_cfg->policy.home.name) 
     eina_stringshare_del(_e_illume_cfg->policy.home.name);
   _e_illume_cfg->policy.home.name = NULL;
   if (_e_illume_cfg->policy.home.title) 
     eina_stringshare_del(_e_illume_cfg->policy.home.title);
   _e_illume_cfg->policy.home.title = NULL;

   /* free configured zones */
   EINA_LIST_FREE(_e_illume_cfg->policy.zones, cz)
     E_FREE(cz);

   /* free config structure */
   E_FREE(_e_illume_cfg);
}

static void 
_e_mod_illume_config_new(void) 
{
   E_Illume_Config_Zone *cz;

   /* create initial config */
   _e_illume_cfg = E_NEW(E_Illume_Config, 1);
   _e_illume_cfg->version = 0;
   _e_illume_cfg->animation.vkbd.duration = 1000;
   _e_illume_cfg->animation.vkbd.resize_before = 1;
   _e_illume_cfg->animation.quickpanel.duration = 1000;
   _e_illume_cfg->policy.name = eina_stringshare_add("illume");

   _e_illume_cfg->policy.vkbd.class = eina_stringshare_add("Virtual-Keyboard");
   _e_illume_cfg->policy.vkbd.name = eina_stringshare_add("Virtual-Keyboard");
   _e_illume_cfg->policy.vkbd.title = eina_stringshare_add("Virtual Keyboard");
   _e_illume_cfg->policy.vkbd.type = ECORE_X_WINDOW_TYPE_NORMAL;
   _e_illume_cfg->policy.vkbd.match.class = 0;
   _e_illume_cfg->policy.vkbd.match.name = 1;
   _e_illume_cfg->policy.vkbd.match.title = 1;
   _e_illume_cfg->policy.vkbd.match.type = 0;

   _e_illume_cfg->policy.indicator.class = 
     eina_stringshare_add("Illume-Indicator");
   _e_illume_cfg->policy.indicator.name = 
     eina_stringshare_add("Illume-Indicator");
   _e_illume_cfg->policy.indicator.title = 
     eina_stringshare_add("Illume Indicator");
   _e_illume_cfg->policy.indicator.type = ECORE_X_WINDOW_TYPE_DOCK;
   _e_illume_cfg->policy.indicator.match.class = 0;
   _e_illume_cfg->policy.indicator.match.name = 1;
   _e_illume_cfg->policy.indicator.match.title = 1;
   _e_illume_cfg->policy.indicator.match.type = 0;

   _e_illume_cfg->policy.softkey.class = 
     eina_stringshare_add("Illume-Softkey");
   _e_illume_cfg->policy.softkey.name = 
     eina_stringshare_add("Illume-Softkey");
   _e_illume_cfg->policy.softkey.title = 
     eina_stringshare_add("Illume Softkey");
   _e_illume_cfg->policy.softkey.type = ECORE_X_WINDOW_TYPE_DOCK;
   _e_illume_cfg->policy.softkey.match.class = 0;
   _e_illume_cfg->policy.softkey.match.name = 1;
   _e_illume_cfg->policy.softkey.match.title = 1;
   _e_illume_cfg->policy.softkey.match.type = 0;

   _e_illume_cfg->policy.home.class = eina_stringshare_add("Illume-Home");
   _e_illume_cfg->policy.home.name = eina_stringshare_add("Illume-Home");
   _e_illume_cfg->policy.home.title = eina_stringshare_add("Illume Home");
   _e_illume_cfg->policy.home.type = ECORE_X_WINDOW_TYPE_NORMAL;
   _e_illume_cfg->policy.home.match.class = 0;
   _e_illume_cfg->policy.home.match.name = 1;
   _e_illume_cfg->policy.home.match.title = 1;
   _e_illume_cfg->policy.home.match.type = 0;

   /* create config for initial zone */
   cz = E_NEW(E_Illume_Config_Zone, 1);
   cz->id = 0;
   cz->mode.dual = 0;
   cz->mode.side = 0;

   /* add zone config to main config structure */
   _e_illume_cfg->policy.zones = 
     eina_list_append(_e_illume_cfg->policy.zones, cz);

   /* add any new config variables here */
   /* if ((_e_illume_cfg->version & 0xffff) < 1) */

   _e_illume_cfg->version = ((IL_CONFIG_MAJOR << 16) | IL_CONFIG_MINOR);
}
