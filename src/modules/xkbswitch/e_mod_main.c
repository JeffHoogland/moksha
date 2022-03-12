#include "e.h"
#include "e_mod_main.h"
#include "e_mod_parse.h"

/* GADCON */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void             _gc_shutdown(E_Gadcon_Client *gcc);
static void             _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char      *_gc_label(const E_Gadcon_Client_Class *client_class);
static const char      *_gc_id_new(const E_Gadcon_Client_Class *client_class __UNUSED__);
static Evas_Object     *_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas);

/* EVENTS */
static Eina_Bool        _xkb_changed_state(void *data __UNUSED__, int type __UNUSED__, void *event);
static void             _e_xkb_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event);
static void             _e_xkb_cb_mouse_wheel(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event);
static void             _e_xkb_cb_menu_configure(void *data, E_Menu *mn, E_Menu_Item *mi __UNUSED__);
static void             _e_xkb_cb_lmenu_post(void *data, E_Menu *menu __UNUSED__);
static void             _e_xkb_cb_lmenu_set(void *data, E_Menu *mn __UNUSED__, E_Menu_Item *mi __UNUSED__);

/* Static variables
 * The static variables specific to the current code unit.
 */

/* GADGET INSTANCE */

typedef struct _Instance
{
   E_Gadcon_Client *gcc;

   Evas_Object     *o_xkbswitch;
   Evas_Object     *o_xkbflag;
   E_Config_XKB_Layout *layout;

   E_Menu          *lmenu;
} Instance;

/* LIST OF INSTANCES */
static Eina_List *instances = NULL;

/* Global variables
 * Global variables shared across the module.
 */

/* CONFIG STRUCTURE */
Xkb _xkb = { NULL, NULL, NULL };

static const E_Gadcon_Client_Class _gc_class =
{
   GADCON_CLIENT_CLASS_VERSION,
   "xkbswitch",
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, NULL
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "XKB Switcher"
};

/* Module initializer
 * Initializes the configuration file, checks its versions, populates
 * menus, finds the rules file, initializes gadget icon.
 */
EAPI void *
e_modapi_init(E_Module *m)
{
   /* Menus and dialogs */
   e_configure_registry_category_add("keyboard_and_mouse", 80, _("Input"),
                                     NULL, "preferences-behavior");
   e_configure_registry_item_add("keyboard_and_mouse/xkbswitch", 110,
                                 _("Keyboard"), NULL,
                                 "preferences-desktop-keyboard",
                                 _xkb_cfg_dialog);
   _xkb.module = m;
   //~ ecore_event_handler_add(ECORE_X_EVENT_XKB_STATE_NOTIFY, _xkb_changed_state, NULL);
   ecore_event_handler_add(E_EVENT_XKB_CHANGED, _xkb_changed_state, NULL);
   /* Gadcon */
   e_gadcon_provider_register(&_gc_class);
   return m;
}

/* Module shutdown
 * Called when the module gets unloaded. Deregisters the menu state
 * and frees up the config.
 */
EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   e_configure_registry_item_del("keyboard_and_mouse/xkbswitch");
   e_configure_registry_category_del("keyboard_and_mouse");

   if (_xkb.evh) ecore_event_handler_del(_xkb.evh);
   if (_xkb.cfd) e_object_del(E_OBJECT(_xkb.cfd));
   _xkb.cfd = NULL;
   _xkb.module = NULL;
   e_gadcon_provider_unregister(&_gc_class);

   return 1;
}

/* Module state save
 * Used to save the configuration file.
 */
EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}

/* Updates icons on all available xkbswitch gadgets to reflect the
 * current layout state.
 */
void
_xkb_update_icon(int cur_group)
{
   Instance *inst;
   Eina_List *l;
   E_Config_XKB_Layout *cl;

   EINA_SAFETY_ON_NULL_RETURN(e_config->xkb.used_layouts);
   //INF("update icon ui: %d", cur_group);
   cl = eina_list_nth(e_config->xkb.used_layouts, cur_group);
   EINA_SAFETY_ON_NULL_RETURN(cl);
   if (!e_config_xkb_layout_eq(cl, e_config->xkb.current_layout))
     {
        e_config_xkb_layout_free(e_config->xkb.current_layout);
        e_config->xkb.current_layout = e_config_xkb_layout_dup(cl);
     }

   if (e_config->xkb.only_label)
     {
        EINA_LIST_FOREACH(instances, l, inst)
          {
             if (!e_config_xkb_layout_eq(e_config->xkb.current_layout, inst->layout))
               inst->layout = e_config_xkb_layout_dup(e_config->xkb.current_layout);
             E_FREE_FUNC(inst->o_xkbflag, evas_object_del);
             e_theme_edje_object_set(inst->o_xkbswitch,
                                     "base/theme/modules/xkbswitch",
                                     "e/modules/xkbswitch/noflag");
             edje_object_part_text_set(inst->o_xkbswitch,
                                       "e.text.label", cl->name);
          }
     }
   else
     {
        EINA_LIST_FOREACH(instances, l, inst)
          {
             if (!e_config_xkb_layout_eq(e_config->xkb.current_layout, inst->layout))
               inst->layout = e_config_xkb_layout_dup(e_config->xkb.current_layout);
             if (!inst->o_xkbflag)
               inst->o_xkbflag = e_icon_add(inst->gcc->gadcon->evas);
             e_theme_edje_object_set(inst->o_xkbswitch,
                                     "base/theme/modules/xkbswitch",
                                     "e/modules/xkbswitch/main");
             e_xkb_e_icon_flag_setup(inst->o_xkbflag, cl->name);
             edje_object_part_swallow(inst->o_xkbswitch, "e.swallow.flag",
                                      inst->o_xkbflag);
             edje_object_part_text_set(inst->o_xkbswitch, "e.text.label",
                                       e_xkb_layout_name_reduce(cl->name));
          }
     }
}

/* LOCAL STATIC FUNCTIONS */

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *gcname, const char *id, const char *style)
{
   Instance *inst;

   /* The instance */
   inst = E_NEW(Instance, 1);
   /* The gadget */
   inst->o_xkbswitch = edje_object_add(gc->evas);
   inst->layout = e_xkb_layout_get();
   
#if 0
   /* inst->layout is potentially null due to Bodhi changes in the wizard module
    *   specifically page_011 is never run and keyboard stuff is never initialized.
    *   The attempt below causes issues: heap-use-after-free. */
   if (!inst->layout)
      {
          inst->layout = E_NEW(E_Config_XKB_Layout, 1);
          // Initialize to defaults
          inst->layout->name = eina_stringshare_ref("us");
          inst->layout->variant = eina_stringshare_add("basic");
          inst->layout->model = eina_stringshare_add("default");
          e_config->xkb.used_layouts = eina_list_prepend(e_config->xkb.used_layouts, inst->layout);
          e_xkb_update(-1);
      }
#endif
   if (e_config->xkb.only_label)
     e_theme_edje_object_set(inst->o_xkbswitch,
                             "base/theme/modules/xkbswitch",
                             "e/modules/xkbswitch/noflag");
   else
     e_theme_edje_object_set(inst->o_xkbswitch,
                             "base/theme/modules/xkbswitch",
                             "e/modules/xkbswitch/main");
   if (inst->layout) 
   edje_object_part_text_set(inst->o_xkbswitch, "e.text.label",
                             e_xkb_layout_name_reduce(inst->layout->name));
   /* The gadcon client */
   inst->gcc = e_gadcon_client_new(gc, gcname, id, style, inst->o_xkbswitch);
   inst->gcc->data = inst;
   /* The flag icon */
   if (!e_config->xkb.only_label)
     {
        inst->o_xkbflag = e_icon_add(gc->evas);
        if (inst->layout)                             
          e_xkb_e_icon_flag_setup(inst->o_xkbflag, inst->layout->name);
        else
          e_xkb_e_icon_flag_setup(inst->o_xkbflag, "us");
          
        /* The icon is part of the gadget. */
        edje_object_part_swallow(inst->o_xkbswitch, "e.swallow.flag",
                                 inst->o_xkbflag);
     }
   else inst->o_xkbflag = NULL;
   /* e_config->xkb.used_layout */
   /* Hook some menus */
   evas_object_event_callback_add(inst->o_xkbswitch, EVAS_CALLBACK_MOUSE_DOWN,
                                  _e_xkb_cb_mouse_down, inst);
   evas_object_event_callback_add(inst->o_xkbswitch, EVAS_CALLBACK_MOUSE_WHEEL,
                                  _e_xkb_cb_mouse_wheel, inst);                                       
   /* Make the list know about the instance */
   instances = eina_list_append(instances, inst);

   return inst->gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

   if (!(inst = gcc->data)) return;
   instances = eina_list_remove(instances, inst);

   if (inst->lmenu)
     {
        e_menu_post_deactivate_callback_set(inst->lmenu, NULL, NULL);
        e_object_del(E_OBJECT(inst->lmenu));
        inst->lmenu = NULL;
     }
   if (inst->o_xkbswitch)
     {
        evas_object_event_callback_del(inst->o_xkbswitch,
                                       EVAS_CALLBACK_MOUSE_DOWN,
                                       _e_xkb_cb_mouse_down);
        evas_object_event_callback_del(inst->o_xkbswitch,
                                       EVAS_CALLBACK_MOUSE_WHEEL,
                                       _e_xkb_cb_mouse_wheel);                                 
        evas_object_del(inst->o_xkbswitch);
        evas_object_del(inst->o_xkbflag);
     }
   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("Keyboard");
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class)
{
   static char buf[4096];

   snprintf(buf, sizeof(buf), "%s.%d", client_class->name,
            eina_list_count(instances) + 1);
   return buf;
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf), "%s/e-module-xkbswitch.edj", _xkb.module->dir);
   o = edje_object_add(evas);
   edje_object_file_set(o, buf, "icon");
   return o;
}

static Eina_Bool
_xkb_changed_state(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   //~ Ecore_X_Event_Xkb *ev = (Ecore_X_Event_Xkb *)event;

   //INF("xkb group %d", ev->group);
   //~ e_config->xkb.cur_group = ev->group;
   _xkb_update_icon(e_config->xkb.cur_group);
   return ECORE_CALLBACK_PASS_ON;
}

#if 0
static int
_xkb_menu_items_sort(const void *data1, const void *data2)
{
   const E_Config_XKB_Layout *cl1 = data1;
   const E_Config_XKB_Layout *cl2 = data2;
   int v;

   v = strcmp(cl1->name, cl2->name);
   if (!v) v = strcmp(cl1->model, cl2->model);
   if (!v) v = strcmp(cl1->variant, cl2->variant);
   return v;
}

#endif

static void
_e_xkb_cb_mouse_wheel(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
{
Evas_Event_Mouse_Wheel *ev = event;

Instance *inst = data;
if (!inst) return;
  
if (ev->z == -1) // up
     {
        e_xkb_layout_next();
     }
   else if (ev->z == 1) // down
     {
        e_xkb_layout_prev();
     }
}

static void
_e_xkb_cb_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
{
   Evas_Event_Mouse_Down *ev = event;
   Instance *inst = data;
   E_Menu *m;

   if (!inst) return;

   if (ev->button == 3) /* Right-click utility menu */
     {
        int x, y;
        E_Menu_Item *mi;

        /* The menu and menu item */
        m = e_menu_new();
        mi = e_menu_item_new(m);
        /* Menu item specifics */
        e_menu_item_label_set(mi, _("Settings"));
        e_util_menu_item_theme_icon_set(mi, "preferences-system");
        e_menu_item_callback_set(mi, _e_xkb_cb_menu_configure, NULL);
        /* Append into the util menu */
        m = e_gadcon_client_util_menu_items_append(inst->gcc,
                                                   m, 0);
        /* Coords */
        e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y,
                                          NULL, NULL);
        /* Activate - we show the menu relative to the gadget */
        e_menu_activate_mouse(m,
                              e_util_zone_current_get(e_manager_current_get()),
                              (x + ev->output.x), (y + ev->output.y), 1, 1,
                              E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
        evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
                                 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
   else if ((ev->button == 1) && (!inst->lmenu)) /* Left-click layout menu */
     {
        Evas_Coord x, y, w, h;
        int cx, cy;

        /* Coordinates and sizing */
        evas_object_geometry_get(inst->o_xkbswitch, &x, &y, &w, &h);
        e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &cx, &cy,
                                          NULL, NULL);
        x += cx;
        y += cy;

        if (!inst->lmenu) inst->lmenu = e_menu_new();

        if (inst->lmenu)
          {
             E_Config_XKB_Layout *cl, *cur;
             E_Menu_Item *mi;
             Eina_List *l;
             int dir;
             char buf[4096];

             mi = e_menu_item_new(inst->lmenu);

             e_menu_item_label_set(mi, _("Settings"));
             e_util_menu_item_theme_icon_set(mi, "preferences-system");
             e_menu_item_callback_set(mi, _e_xkb_cb_menu_configure, NULL);

             mi = e_menu_item_new(inst->lmenu);
             e_menu_item_separator_set(mi, 1);
             cur = e_xkb_layout_get();

             /* Append all the layouts */
             EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, cl)
               {
                  const char *name = cl->name;

                  mi = e_menu_item_new(inst->lmenu);

                  e_menu_item_radio_set(mi, 1);
                  e_menu_item_radio_group_set(mi, 1);
                  if (e_config_xkb_layout_eq(cur, cl))
                    e_menu_item_toggle_set(mi, 1);
                  e_xkb_flag_file_get(buf, sizeof(buf), name);
                  e_menu_item_icon_file_set(mi, buf);
                  if (cl->variant)
                    snprintf(buf, sizeof(buf), "%s (%s, %s)", cl->name, cl->model, cl->variant);
                  else
                    snprintf(buf, sizeof(buf), "%s (%s)", cl->name, cl->model);
                  e_menu_item_label_set(mi, buf);
                  e_menu_item_callback_set(mi, _e_xkb_cb_lmenu_set, cl);
               }

             /* Deactivate callback */
             e_menu_post_deactivate_callback_set(inst->lmenu,
                                                 _e_xkb_cb_lmenu_post, inst);
             /* Proper menu orientation */
             switch (inst->gcc->gadcon->orient)
               {
                case E_GADCON_ORIENT_TOP:
                  dir = E_MENU_POP_DIRECTION_DOWN;
                  break;

                case E_GADCON_ORIENT_BOTTOM:
                  dir = E_MENU_POP_DIRECTION_UP;
                  break;

                case E_GADCON_ORIENT_LEFT:
                  dir = E_MENU_POP_DIRECTION_RIGHT;
                  break;

                case E_GADCON_ORIENT_RIGHT:
                  dir = E_MENU_POP_DIRECTION_LEFT;
                  break;

                case E_GADCON_ORIENT_CORNER_TL:
                  dir = E_MENU_POP_DIRECTION_DOWN;
                  break;

                case E_GADCON_ORIENT_CORNER_TR:
                  dir = E_MENU_POP_DIRECTION_DOWN;
                  break;

                case E_GADCON_ORIENT_CORNER_BL:
                  dir = E_MENU_POP_DIRECTION_UP;
                  break;

                case E_GADCON_ORIENT_CORNER_BR:
                  dir = E_MENU_POP_DIRECTION_UP;
                  break;

                case E_GADCON_ORIENT_CORNER_LT:
                  dir = E_MENU_POP_DIRECTION_RIGHT;
                  break;

                case E_GADCON_ORIENT_CORNER_RT:
                  dir = E_MENU_POP_DIRECTION_LEFT;
                  break;

                case E_GADCON_ORIENT_CORNER_LB:
                  dir = E_MENU_POP_DIRECTION_RIGHT;
                  break;

                case E_GADCON_ORIENT_CORNER_RB:
                  dir = E_MENU_POP_DIRECTION_LEFT;
                  break;

                case E_GADCON_ORIENT_FLOAT:
                case E_GADCON_ORIENT_HORIZ:
                case E_GADCON_ORIENT_VERT:
                default:
                  dir = E_MENU_POP_DIRECTION_AUTO;
                  break;
               }

             e_gadcon_locked_set(inst->gcc->gadcon, 1);

             /* We display not relatively to the gadget, but similarly to
              * the start menu - thus the need for direction etc.
              */
             e_menu_activate_mouse(inst->lmenu,
                                   e_util_zone_current_get
                                     (e_manager_current_get()),
                                   x, y, w, h, dir, ev->timestamp);
          }
     }
   else if (ev->button == 2) /* Middle click */
     e_xkb_layout_next();
}

static void
_e_xkb_cb_lmenu_post(void *data, E_Menu *menu __UNUSED__)
{
   Instance *inst = data;

   if (!inst) return;
   e_gadcon_locked_set(inst->gcc->gadcon, 0);
   inst->lmenu = NULL;
}

static void
_e_xkb_cb_menu_configure(void *data __UNUSED__, E_Menu *mn, E_Menu_Item *mi __UNUSED__)
{
   if (_xkb.cfd) return;
   _xkb_cfg_dialog(mn->zone->container, NULL);
}

static void
_e_xkb_cb_lmenu_set(void *data, E_Menu *mn __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   Eina_List *l;
   int cur_group = -1, grp = -1;
   E_Config_XKB_Layout *cl2, *cl = data;

   EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, cl2)
     {
        grp++;
        if (cl2 == cl) cur_group = grp;
     }
   if (cur_group == -1) return;
   if (e_config_xkb_layout_eq(cl, e_xkb_layout_get())) return;
   e_xkb_layout_set(cl);
   e_config_xkb_layout_free(e_config->xkb.sel_layout);
   e_config->xkb.sel_layout = e_config_xkb_layout_dup(cl);
   (void)cur_group;
   //~ _xkb_update_icon(cur_group);
}

