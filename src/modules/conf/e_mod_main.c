#include "e.h"
#include "e_mod_main.h"

typedef struct _Instance Instance;
struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_toggle;
};

/* actual module specifics */

static void             _e_mod_action_conf_cb(E_Object *obj, const char *params);
static void             _e_mod_conf_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void             _e_mod_menu_add(void *data, E_Menu *m);
static void             _e_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void             _config_pre_activate_cb(void *data, E_Menu *m);

/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void             _gc_shutdown(E_Gadcon_Client *gcc);
static void             _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char      *_gc_label(const E_Gadcon_Client_Class *client_class);
static Evas_Object     *_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas);
static const char      *_gc_id_new(const E_Gadcon_Client_Class *client_class);
static void             _cb_action_conf(void *data, Evas_Object *obj, const char *emission, const char *source);

static void             _conf_new(void);
static void             _conf_free(void);

static E_Module *conf_module = NULL;
static E_Action *act = NULL;
static E_Int_Menu_Augmentation *maug = NULL;
static E_Config_DD *conf_edd = NULL;
Config *conf = NULL;

static Eina_List *instances = NULL;

/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION, "configuration",
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL,
      e_gadcon_site_is_not_toolbar
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Instance *inst;

   inst = E_NEW(Instance, 1);
   inst->o_toggle = edje_object_add(gc->evas);
   e_theme_edje_object_set(inst->o_toggle,
                           "base/theme/modules/conf",
                           "e/modules/conf/main");

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->o_toggle);
   inst->gcc->data = inst;

   edje_object_signal_callback_add(inst->o_toggle, "e,action,conf", "",
                                   _cb_action_conf, inst);

   instances = eina_list_append(instances, inst);
   e_gadcon_client_util_menu_attach(inst->gcc);

   return inst->gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

   if (!(inst = gcc->data)) return;
   instances = eina_list_remove(instances, inst);
   if (inst->o_toggle) evas_object_del(inst->o_toggle);
   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
   Evas_Coord mw, mh;

   edje_object_size_min_get(gcc->o_base, &mw, &mh);
   if ((mw < 1) || (mh < 1))
     edje_object_size_min_calc(gcc->o_base, &mw, &mh);
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   e_gadcon_client_aspect_set(gcc, mw, mh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("Settings");
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;
   char buf[PATH_MAX];

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-conf.edj",
            e_module_dir_get(conf_module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _gadcon_class.name;
}

/*
   static void
   _cb_button_click(void *data __UNUSED__, void *data2 __UNUSED__)
   {
   E_Action *a;

   a = e_action_find("configuration");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
   }
 */

static void
_cb_action_conf(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Instance *inst;
   E_Action *a;

   if (!(inst = data)) return;
   a = e_action_find("configuration");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
}

static void
_e_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   Eina_List *l;
   E_Configure_Cat *ecat;

   EINA_LIST_FOREACH(e_configure_registry, l, ecat)
     {
        if ((ecat->pri >= 0) && (ecat->items))
          {
             E_Configure_It *eci;
             Eina_List *ll;

             EINA_LIST_FOREACH(ecat->items, ll, eci)
               {
                  char buf[1024];

                  if ((eci->pri >= 0) && (eci == data))
                    {
                       snprintf(buf, sizeof(buf), "%s/%s", ecat->cat, eci->item);
                       e_configure_registry_call(buf, m->zone->container, NULL);
                    }
               }
          }
     }
}

static void
_config_pre_activate_cb(void *data, E_Menu *m)
{
   E_Configure_Cat *ecat = data;
   E_Configure_It *eci;
   Eina_List *l;
   E_Menu_Item *mi;

   e_menu_pre_activate_callback_set(m, NULL, NULL);

   EINA_LIST_FOREACH(ecat->items, l, eci)
     {
        if (eci->pri >= 0)
          {
             mi = e_menu_item_new(m);
             e_menu_item_label_set(mi, eci->label);
             if (eci->icon)
               {
                  if (eci->icon_file)
                    e_menu_item_icon_edje_set(mi, eci->icon_file, eci->icon);
                  else
                    e_util_menu_item_theme_icon_set(mi, eci->icon);
               }
             e_menu_item_callback_set(mi, _e_mod_run_cb, eci);
          }
     }
}

static void
_config_item_activate_cb(void *data, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   E_Configure_Cat *ecat = data;
   e_configure_show(m->zone->container, ecat->cat);
}

static void
_config_all_pre_activate_cb(void *data __UNUSED__, E_Menu *m)
{
   const Eina_List *l;
   E_Configure_Cat *ecat;

   e_menu_pre_activate_callback_set(m, NULL, NULL);

   EINA_LIST_FOREACH(e_configure_registry, l, ecat)
     {
        E_Menu_Item *mi;
        E_Menu *sub;

        if ((ecat->pri < 0) || (!ecat->items)) continue;

        mi = e_menu_item_new(m);
        e_menu_item_label_set(mi, ecat->label);
        if (ecat->icon)
          {
             if (ecat->icon_file)
               e_menu_item_icon_edje_set(mi, ecat->icon_file, ecat->icon);
             else
               e_util_menu_item_theme_icon_set(mi, ecat->icon);
          }
        e_menu_item_callback_set(mi, _config_item_activate_cb, ecat);
        sub = e_menu_new();
        e_menu_item_submenu_set(mi, sub);
        e_menu_pre_activate_callback_set(sub, _config_pre_activate_cb, ecat);
     }
}

/* menu item add hook */
void
e_mod_config_menu_add(void *data __UNUSED__, E_Menu *m)
{
   E_Menu_Item *mi;
   E_Menu *sub;

   e_menu_pre_activate_callback_set(m, NULL, NULL);

   sub = e_menu_new();
   e_menu_pre_activate_callback_set(sub, _config_all_pre_activate_cb, NULL);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("All"));
   e_menu_item_submenu_set(mi, sub);
}

/* module setup */
EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Conf" };

EAPI void *
e_modapi_init(E_Module *m)
{
   char buf[PATH_MAX];

   conf_module = m;

   /* add module supplied action */
   act = e_action_add("configuration");
   if (act)
     {
        act->func.go = _e_mod_action_conf_cb;
        e_action_predef_name_set(_("Launch"), _("Settings Panel"),
                                 "configuration", NULL, NULL, 0);
     }
   maug =
     e_int_menus_menu_augmentation_add_sorted("config/0", _("Settings Panel"),
                                              _e_mod_menu_add, NULL, NULL, NULL);
   e_module_delayed_set(m, 1);

   snprintf(buf, sizeof(buf), "%s/e-module-conf.edj",
            e_module_dir_get(conf_module));

   e_configure_registry_category_add("advanced", 80, _("Advanced"),
                                     NULL, "preferences-advanced");
   e_configure_registry_item_add("advanced/conf", 110, _("Configuration Panel"),
                                 NULL, buf, e_int_config_conf_module);

   conf_edd = E_CONFIG_DD_NEW("Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, menu_augmentation, INT);

   conf = e_config_domain_load("module.conf", conf_edd);
   if (conf)
     {
        if (!e_util_module_config_check("Configuration Panel", conf->version, MOD_CONFIG_FILE_VERSION))
          _conf_free();
     }

   if (!conf) _conf_new();
   conf->module = m;

   if (conf->menu_augmentation)
     {
        conf->aug =
          e_int_menus_menu_augmentation_add
            ("config/2", e_mod_config_menu_add, NULL, NULL, NULL);
     }

   e_gadcon_provider_register(&_gadcon_class);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   e_configure_del();

   e_configure_registry_item_del("advanced/conf");
   e_configure_registry_category_del("advanced");

   if (conf->cfd) e_object_del(E_OBJECT(conf->cfd));
   conf->cfd = NULL;

   e_gadcon_provider_unregister(&_gadcon_class);

   /* remove module-supplied menu additions */
   if (maug)
     {
        e_int_menus_menu_augmentation_del("config/0", maug);
        maug = NULL;
     }
   if (conf->aug)
     {
        e_int_menus_menu_augmentation_del("config/2", conf->aug);
        conf->aug = NULL;
     }

   /* remove module-supplied action */
   if (act)
     {
        e_action_predef_name_del(_("Launch"), _("Settings Panel"));
        e_action_del("configuration");
        act = NULL;
     }
   conf_module = NULL;

   E_FREE(conf);
   E_CONFIG_DD_FREE(conf_edd);

   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   e_config_domain_save("module.conf", conf_edd, conf);
   return 1;
}

/* action callback */
static void
_e_mod_action_conf_cb(E_Object *obj, const char *params)
{
   E_Zone *zone = NULL;

   if (obj)
     {
        if (obj->type == E_MANAGER_TYPE)
          zone = e_util_zone_current_get((E_Manager *)obj);
        else if (obj->type == E_CONTAINER_TYPE)
          zone = e_util_zone_current_get(((E_Container *)obj)->manager);
        else if (obj->type == E_ZONE_TYPE)
          zone = ((E_Zone *)obj);
        else
          zone = e_util_zone_current_get(e_manager_current_get());
     }
   if (!zone) zone = e_util_zone_current_get(e_manager_current_get());
   if ((zone) && (params))
     e_configure_registry_call(params, zone->container, params);
   else if (zone)
     e_configure_show(zone->container, params);
}

/* menu item callback(s) */
static void
_e_mod_conf_cb(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   e_configure_show(m->zone->container, NULL);
}

static void
_e_mod_mode_presentation_toggle(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi)
{
   e_config->mode.presentation = !e_config->mode.presentation;
   e_menu_item_toggle_set(mi, e_config->mode.presentation);
   e_config_mode_changed();
   e_config_save_queue();
}

static void
_e_mod_mode_offline_toggle(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi)
{
   e_config->mode.offline = !e_config->mode.offline;
   e_menu_item_toggle_set(mi, e_config->mode.offline);
   e_config_mode_changed();
   e_config_save_queue();
}

static void
_e_mod_submenu_modes_fill(void *data __UNUSED__, E_Menu *m)
{
   E_Menu_Item *mi;

   mi = e_menu_item_new(m);
   e_menu_item_check_set(mi, 1);
   e_menu_item_toggle_set(mi, e_config->mode.presentation);
   e_menu_item_label_set(mi, _("Presentation"));
   e_util_menu_item_theme_icon_set(mi, "preferences-modes-presentation");
   e_menu_item_callback_set(mi, _e_mod_mode_presentation_toggle, NULL);

   mi = e_menu_item_new(m);
   e_menu_item_check_set(mi, 1);
   e_menu_item_toggle_set(mi, e_config->mode.offline);
   e_menu_item_label_set(mi, _("Offline"));
   e_util_menu_item_theme_icon_set(mi, "preferences-modes-offline");
   e_menu_item_callback_set(mi, _e_mod_mode_offline_toggle, NULL);

   e_menu_pre_activate_callback_set(m, NULL, NULL);
}

static E_Menu *
_e_mod_submenu_modes_get(void)
{
   E_Menu *m;

   if (!(m = e_menu_new())) return NULL;
   e_menu_pre_activate_callback_set(m, _e_mod_submenu_modes_fill, NULL);
   return m;
}

/* menu item add hook */
static void
_e_mod_menu_add(void *data __UNUSED__, E_Menu *m)
{
   E_Menu_Item *mi;

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Settings Panel"));
   e_util_menu_item_theme_icon_set(mi, "preferences-system");
   e_menu_item_callback_set(mi, _e_mod_conf_cb, NULL);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Modes"));
   e_util_menu_item_theme_icon_set(mi, "preferences-modes");
   e_menu_item_submenu_set(mi, _e_mod_submenu_modes_get());
}

static void
_conf_new(void)
{
   conf = E_NEW(Config, 1);
   conf->version = (MOD_CONFIG_FILE_EPOCH << 16);

#define IFMODCFG(v) if ((conf->version & 0xffff) < v) {
#define IFMODCFGEND }

    IFMODCFG(0x008d);
    conf->menu_augmentation = 1;
    IFMODCFGEND;

    conf->version = MOD_CONFIG_FILE_VERSION;
    e_config_save_queue();
}

static void
_conf_free(void)
{
   E_FREE(conf);
}
