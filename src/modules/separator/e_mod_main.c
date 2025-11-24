#include "config.h"
#include <e.h>
#include "e_mod_main.h"

typedef struct _Instance Instance;
typedef struct _Separator Separator;

struct _Instance 
{
   E_Gadcon_Client *gcc;
   Separator *sep;
   Config_Item *ci;
};

struct _Separator
{
   Instance *inst;
   Evas_Object *o_icon;
};

static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char *_gc_label(const E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas);
static const char *_gc_id_new(const E_Gadcon_Client_Class *client_class);
static Config_Item *_config_item_get(const char *id);
static void _button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _eval_instance_position(Instance *inst);


static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_item_edd = NULL;

Config *sep_conf = NULL;
static int blank;

static Eina_Bool
_separator_site_is_safe(E_Gadcon_Site site)
{
   /* There is nothing logical to have separator on the desktop */

   if (e_gadcon_site_is_desktop(site))
     return EINA_FALSE;
   else if (e_gadcon_site_is_any_toolbar(site))
     return EINA_FALSE;
   return EINA_TRUE;
}

static const E_Gadcon_Client_Class _gc_class = 
{
   GADCON_CLIENT_CLASS_VERSION, "separator", 
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL,
      _separator_site_is_safe
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style) 
{
   Separator *sep;
   Instance *inst;
   E_Gadcon_Client *gcc;

   inst = E_NEW(Instance, 1);   
   inst->ci = _config_item_get(id);

   sep = E_NEW(Separator, 1);
   sep->inst = inst;

   sep->o_icon = edje_object_add(gc->evas);
   e_theme_edje_object_set(sep->o_icon, "base/theme/modules/separator",
                                             "modules/separator/main");
   evas_object_show(sep->o_icon);

   gcc = e_gadcon_client_new(gc, name, id, style, sep->o_icon);
   gcc->data = inst;
   inst->gcc = gcc;
   inst->sep = sep;

   sep_conf->instances = eina_list_append(sep_conf->instances, inst);
   _eval_instance_position(inst);

   evas_object_event_callback_add(sep->o_icon, EVAS_CALLBACK_MOUSE_DOWN,
               _button_cb_mouse_down, inst);

   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc) 
{
   Instance *inst;
   Separator *sep;

   inst = gcc->data;
   sep = inst->sep;

   if (sep->o_icon) evas_object_del(sep->o_icon);

   sep_conf->instances = eina_list_remove(sep_conf->instances, inst);
   E_FREE(sep);
   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__) 
{
   Instance *inst;
   Evas_Coord mw, mh;

   if (!(inst = gcc->data)) return;
   edje_object_size_min_calc(inst->sep->o_icon, &mw, &mh);
   e_gadcon_client_min_size_set(gcc, mw + 5, mh + 5);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("Separator");
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas) 
{
   Evas_Object *o;
   char buf[PATH_MAX];

   if (!sep_conf->module) return NULL;

   snprintf(buf, sizeof(buf), "%s/e-module-separator.edj", 
            e_module_dir_get(sep_conf->module));

   o = edje_object_add(evas);
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   Config_Item *ci;

   ci = _config_item_get(NULL);
   return ci->id;
}

static Config_Item *
_config_item_get(const char *id)
{
   Eina_List *l = NULL;
   Config_Item *ci;
   char buf[128];
   int num = 0;
   const char *p;

   if (!id)
     {
        /* Create id */
       if (sep_conf->items)
          {
             ci = eina_list_last(sep_conf->items)->data;
             p = strrchr(ci->id, '.');
             if (p) num = atoi(p + 1) + 1;
          }
        snprintf(buf, sizeof(buf), "%s.%d", _gc_class.name, num);
        id = buf;
     }
   else
     {
        for (l = sep_conf->items; l; l = l->next)
          {
             ci = l->data;
             if (!ci->id) continue;
             if (!strcmp(ci->id, id))
               {
                  blank = ci->blank;
                  return ci;
               }
          }
     }

   ci = E_NEW(Config_Item, 1);
   ci->id = eina_stringshare_add(id);
   ci->blank = 0;

   sep_conf->items = eina_list_append(sep_conf->items, ci);
   return ci;
}

static void
_eval_instance_position(Instance *inst)
{
   if (!inst) return;
   Eina_Bool horiz;

   if (inst->ci->blank)
     {
       edje_object_signal_emit(inst->sep->o_icon, "hide_separator", "");
       return;
     }

   switch (inst->gcc->gadcon->orient)
     {
       case E_GADCON_ORIENT_TOP:
       case E_GADCON_ORIENT_CORNER_TL:
       case E_GADCON_ORIENT_CORNER_TR:
       case E_GADCON_ORIENT_BOTTOM:
       case E_GADCON_ORIENT_CORNER_BL:
       case E_GADCON_ORIENT_CORNER_BR:
       case E_GADCON_ORIENT_HORIZ:
         horiz = EINA_TRUE;
        break;

       case E_GADCON_ORIENT_LEFT:
       case E_GADCON_ORIENT_CORNER_LB:
       case E_GADCON_ORIENT_CORNER_LT:
       case E_GADCON_ORIENT_RIGHT:
       case E_GADCON_ORIENT_CORNER_RB:
       case E_GADCON_ORIENT_CORNER_RT:
       case E_GADCON_ORIENT_VERT:
         horiz = EINA_FALSE;
        break;

       default:
         horiz = EINA_TRUE;
     }

   if (inst->gcc->gadcon->shelf)
     {
       if (horiz)
         edje_object_signal_emit(inst->sep->o_icon, "horizontal_shelf", "");
       else
         edje_object_signal_emit(inst->sep->o_icon, "vertical_shelf", "");
     }
}

static void
_sep_menu_blank_set(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi)
{
   Instance *inst;

   inst = data;
   inst->ci->blank = e_menu_item_toggle_get(mi);
   _eval_instance_position(inst);
   e_config_save_queue();
}


static void
_button_cb_mouse_down(void *data, Evas *e __UNUSED__, 
                      Evas_Object *obj __UNUSED__, void *event_info)
{
   Instance *inst;
   Evas_Event_Mouse_Down *ev;

   inst = data;
   ev = event_info;
   if (ev->button == 3)
     {
        E_Menu *m;
        E_Menu_Item *mi;
        int cx, cy, cw, ch;

        m = e_menu_new();

        mi = e_menu_item_new(m);
        e_menu_item_label_set(mi, _("Transparent"));
        e_menu_item_check_set(mi, 1);
        if (inst->ci->blank) e_menu_item_toggle_set(mi, 1);
        e_menu_item_callback_set(mi, _sep_menu_blank_set, inst);

        m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);

        e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &cx, &cy, &cw, &ch);
        e_menu_activate_mouse(m,
                              e_util_zone_current_get(e_manager_current_get()),
                              cx + ev->output.x, cy + ev->output.y, 1, 1,
                              E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
        evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
                              EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
}


EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION, "Separator"
};

EAPI void *
e_modapi_init(E_Module *m) 
{
   conf_item_edd = E_CONFIG_DD_NEW("Separator_Config_Item", Config_Item);
   conf_edd = E_CONFIG_DD_NEW("Separator_Config", Config);

   #undef T
   #define T Config_Item
   #undef D
   #define D conf_item_edd
   E_CONFIG_VAL(D, T, id, STR);
   E_CONFIG_VAL(D, T, blank, INT);

   #undef T
   #define T Config
   #undef D
   #define D conf_edd
   E_CONFIG_LIST(D, T, items, conf_item_edd);

   sep_conf = e_config_domain_load("module.separator", conf_edd);
   if (!sep_conf) 
     {
        Config_Item *ci;

        sep_conf = E_NEW(Config, 1);
        ci = E_NEW(Config_Item, 1);
        ci->id = eina_stringshare_add("0");
        ci->blank = 0;

        sep_conf->items = eina_list_append(sep_conf->items, ci);
     }

   sep_conf->module = m;
   e_gadcon_provider_register(&_gc_class);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   sep_conf->module = NULL;
   e_gadcon_provider_unregister(&_gc_class);

   while (sep_conf->items) 
     {
        Config_Item *ci;

        ci = sep_conf->items->data;
        if (ci->id) eina_stringshare_del(ci->id);
        sep_conf->items = eina_list_remove_list(sep_conf->items, sep_conf->items);
        E_FREE(ci);
     }

   E_FREE(sep_conf);
   E_CONFIG_DD_FREE(conf_item_edd);
   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   e_config_domain_save("module.separator", conf_edd, sep_conf);
   return 1;
}
