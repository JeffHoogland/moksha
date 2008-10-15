#include <e.h>
#include "config.h"
#include "e_mod_main.h"
#include "e_mod_gadman.h"
#include "e_mod_config.h"

/* local protos */
static void _gadman_maug_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void _gadman_maug_add(void *data, E_Menu *m);
static void _gadman_action_cb(E_Object *obj, const char *params);

/* public module routines. all modules must have these */
EAPI E_Module_Api e_modapi = {
   E_MODULE_API_VERSION,
   "Gadman"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   char buf[4096];

   /* Set up a new configuration panel */
   snprintf(buf, sizeof(buf), "%s/e-module-gadman.edj", m->dir);
   e_configure_registry_category_add("extensions", 90, _("Extensions"), NULL, 
                                     "enlightenment/extensions");
   e_configure_registry_item_add("extensions/gadman", 150, _("Gadgets"), NULL, 
                                 buf, _config_gadman_module);

   /* Set this module to be loaded after all other modules, or we don't see
    modules loaded after this */
   e_module_priority_set(m, -100);

   gadman_init(m);

   //Configuration values
   Man->conf_edd = E_CONFIG_DD_NEW("Gadman_Config", Config);
#undef T
#undef D
#define T Config
#define D Man->conf_edd
   E_CONFIG_VAL(D, T, bg_type, INT);
   E_CONFIG_VAL(D, T, color_r, INT);
   E_CONFIG_VAL(D, T, color_g, INT);
   E_CONFIG_VAL(D, T, color_b, INT);
   E_CONFIG_VAL(D, T, color_a, INT);
   E_CONFIG_VAL(D, T, anim_bg, INT);
   E_CONFIG_VAL(D, T, anim_gad, INT);
   E_CONFIG_VAL(D, T, custom_bg, STR);
   
   Man->conf = e_config_domain_load("module.gadman", Man->conf_edd);
   if (!Man->conf)
     {
	Man->conf = E_NEW(Config, 1);
	Man->conf->bg_type = 0;
	Man->conf->color_r = 255;
	Man->conf->color_g = 255;
	Man->conf->color_b = 255;
	Man->conf->color_a = 255;
	Man->conf->anim_bg = 1;
	Man->conf->anim_gad = 1;
	Man->conf->custom_bg = NULL;
     }
   E_CONFIG_LIMIT(Man->conf->bg_type, 0, 5);
   E_CONFIG_LIMIT(Man->conf->color_r, 0, 255);
   E_CONFIG_LIMIT(Man->conf->color_g, 0, 255);
   E_CONFIG_LIMIT(Man->conf->color_b, 0, 255);
   E_CONFIG_LIMIT(Man->conf->color_a, 0, 255);
   E_CONFIG_LIMIT(Man->conf->anim_bg, 0, 1);
   E_CONFIG_LIMIT(Man->conf->anim_gad, 0, 1);

   /* Menu augmentation */
   Man->icon_name = eina_stringshare_add(buf);
   Man->maug = NULL;
   Man->maug = 
     e_int_menus_menu_augmentation_add("config/1", _gadman_maug_add,
                                       (void *)Man->icon_name, NULL, NULL);
   /* Create toggle action */
   Man->action = e_action_add("gadman_toggle");
   if (Man->action)
     {
	Man->action->func.go = _gadman_action_cb;
	e_action_predef_name_set(_("Gadgets"), _("Show/hide gadgets"),
				 "gadman_toggle", NULL, NULL, 0);
     }

   /* Create a binding for the action (if not exists) */
   if (!e_bindings_key_get("gadman_toggle"))
     {
        e_managers_keys_ungrab();
        e_bindings_key_add(E_BINDING_CONTEXT_ANY, "g", E_BINDING_MODIFIER_CTRL|E_BINDING_MODIFIER_ALT,
                           0, "gadman_toggle", NULL);

        e_managers_keys_grab();
        e_config_save_queue();
     }

   gadman_update_bg();

   return Man;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   if (Man->maug)
     e_int_menus_menu_augmentation_del("config/1", Man->maug);

   e_configure_registry_item_del("extensions/deskman");
   e_configure_registry_category_del("extensions");

   if (Man->config_dialog)
     {
        e_object_del(E_OBJECT(Man->config_dialog));
        Man->config_dialog = NULL;
     }
   if (Man->action)
     {
	e_action_predef_name_del(_("Gadgets"), _("Show/hide gadgets"));
	e_action_del("gadman_toggle");
	Man->action = NULL;
     }

   gadman_shutdown();

   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   e_config_domain_save("module.gadman", Man->conf_edd, Man->conf);
   return 1;
}

static void 
_gadman_maug_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   e_configure_registry_call("extensions/gadman", m->zone->container, NULL);
}

static void
_gadman_maug_add(void *data, E_Menu *m)
{
   E_Menu_Item *mi;

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Gadgets"));
   e_menu_item_icon_edje_set(mi, (char *)data, "icon");
   e_menu_item_callback_set(mi, _gadman_maug_cb, NULL);
}

static void
_gadman_action_cb(E_Object *obj, const char *params)
{
   gadman_gadgets_toggle();
}
