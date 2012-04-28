#include "e.h"
#include "e_mod_main.h"
#include "e_mod_parse.h"
#include "e_mod_keybindings.h"

/* Static functions
 * The static functions specific to the current code unit.
 */

/* GADCON */

static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);

static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient  (E_Gadcon_Client *gcc, E_Gadcon_Orient orient);

static const char *_gc_label (E_Gadcon_Client_Class *client_class);
static const char *_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__);

static Evas_Object *_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas);

/* CONFIG */

static void _e_xkb_cfg_new (void);
static void _e_xkb_cfg_free(void);

static Eina_Bool _e_xkb_cfg_timer(void *data);

/* EVENTS */

static void _e_xkb_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event);

static void _e_xkb_cb_menu_post(void *data, E_Menu *menu __UNUSED__);
static void _e_xkb_cb_menu_configure(void *data, E_Menu *mn, E_Menu_Item *mi __UNUSED__);

static void _e_xkb_cb_lmenu_post(void *data, E_Menu *menu __UNUSED__);
static void _e_xkb_cb_lmenu_set(void *data, E_Menu *mn __UNUSED__, E_Menu_Item *mi __UNUSED__);

/* Static variables
 * The static variables specific to the current code unit.
 */

/* GADGET INSTANCE */

typedef struct _Instance 
{
   E_Gadcon_Client *gcc;
   
   Evas_Object *o_xkbswitch;
   Evas_Object *o_xkbflag;
   
   E_Menu *menu;
   E_Menu *lmenu;
} Instance;

/* LIST OF INSTANCES */
static Eina_List *instances = NULL;

/* EET STRUCTURES */

static E_Config_DD *e_xkb_cfg_edd        = NULL;
static E_Config_DD *e_xkb_cfg_layout_edd = NULL;
static E_Config_DD *e_xkb_cfg_option_edd = NULL;

/* Global variables
 * Global variables shared across the module.
 */

/* CONFIG STRUCTURE */
E_XKB_Config *e_xkb_cfg = NULL;

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
                                 _("XKB Switcher"),  NULL,
                                 "preferences-desktop-locale", 
                                 e_xkb_cfg_dialog);
   e_xkb_cfg_layout_edd = E_CONFIG_DD_NEW("E_XKB_Config_Layout", E_XKB_Config_Layout);
#undef T
#undef D
#define T E_XKB_Config_Layout
#define D e_xkb_cfg_layout_edd
   E_CONFIG_VAL(D, T, name,    STR);
   E_CONFIG_VAL(D, T, model,   STR);
   E_CONFIG_VAL(D, T, variant, STR);
   
   e_xkb_cfg_option_edd = E_CONFIG_DD_NEW("E_XKB_Config_Option", E_XKB_Config_Option);
#undef T
#undef D
#define T E_XKB_Config_Option
#define D e_xkb_cfg_option_edd
   E_CONFIG_VAL(D, T, name, STR);
   
   e_xkb_cfg_edd = E_CONFIG_DD_NEW("e_xkb_cfg", E_XKB_Config);
#undef T
#undef D
#define T E_XKB_Config
#define D e_xkb_cfg_edd
   E_CONFIG_LIST(D, T, used_layouts, e_xkb_cfg_layout_edd);
   E_CONFIG_LIST(D, T, used_options, e_xkb_cfg_option_edd);
   E_CONFIG_VAL(D, T, layout_next_key.context, INT);
   E_CONFIG_VAL(D, T, layout_next_key.modifiers, INT);
   E_CONFIG_VAL(D, T, layout_next_key.key, STR);
   E_CONFIG_VAL(D, T, layout_next_key.action, STR);
   E_CONFIG_VAL(D, T, layout_next_key.params, STR);
   E_CONFIG_VAL(D, T, layout_next_key.any_mod, UCHAR);
   E_CONFIG_VAL(D, T, layout_prev_key.context, INT);
   E_CONFIG_VAL(D, T, layout_prev_key.modifiers, INT);
   E_CONFIG_VAL(D, T, layout_prev_key.key, STR);
   E_CONFIG_VAL(D, T, layout_prev_key.action, STR);
   E_CONFIG_VAL(D, T, layout_prev_key.params, STR);
   E_CONFIG_VAL(D, T, layout_prev_key.any_mod, UCHAR);
   E_CONFIG_VAL(D, T, default_model, STR);
   E_CONFIG_VAL(D, T, only_label, INT);
   E_CONFIG_VAL(D, T, version, INT);
   
   /* Version check */
   e_xkb_cfg = e_config_domain_load("module.xkbswitch", e_xkb_cfg_edd);
   if (e_xkb_cfg) 
     {
        /* Check config version */
        if ((e_xkb_cfg->version >> 16) < MOD_CONFIG_FILE_EPOCH) 
          {
             /* config too old */
             _e_xkb_cfg_free();
             ecore_timer_add(1.0, _e_xkb_cfg_timer,
                             _("XKB Switcher Module Configuration data needed "
                               "upgrading. Your old configuration<br> has been"
                               " wiped and a new set of defaults initialized. "
                               "This<br>will happen regularly during "
                               "development, so don't report a<br>bug. "
                               "This simply means the module needs "
                               "new configuration<br>data by default for "
                               "usable functionality that your old<br>"
                               "configuration simply lacks. This new set of "
                               "defaults will fix<br>that by adding it in. "
                               "You can re-configure things now to your<br>"
                               "liking. Sorry for the inconvenience.<br>"));
          }
        /* Ardvarks */
        else if (e_xkb_cfg->version > MOD_CONFIG_FILE_VERSION) 
          {
             /* config too new...wtf ? */
             _e_xkb_cfg_free();
             ecore_timer_add(1.0, _e_xkb_cfg_timer, 
                             _("Your XKB Switcher Module configuration is NEWER "
                               "than the module version. This is "
                               "very<br>strange. This should not happen unless"
                               " you downgraded<br>the module or "
                               "copied the configuration from a place where"
                               "<br>a newer version of the module "
                               "was running. This is bad and<br>as a "
                               "precaution your configuration has been now "
                               "restored to<br>defaults. Sorry for the "
                               "inconvenience.<br>"));
          }
     }
   
   if (!e_xkb_cfg) _e_xkb_cfg_new();
   e_xkb_cfg->module = m;
   /* Rules */
   find_rules();
   /* Update the layout - can't update icon, gadgets are not there yet */
   e_xkb_update_layout();
   /* Gadcon */
   e_gadcon_provider_register(&_gc_class);
   /* Bindings */
   e_xkb_register_module_actions();
   e_xkb_register_module_keybindings();
   
   return m;
}

/* Module shutdown
 * Called when the module gets unloaded. Deregisters the menu state
 * and frees up the config.
 */
EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   E_XKB_Config_Layout *cl;
   E_XKB_Config_Option *op;
   
   e_configure_registry_item_del("keyboard_and_mouse/xkbswitch");
   e_configure_registry_category_del("keyboard_and_mouse");
   
   if (e_xkb_cfg->cfd) e_object_del(E_OBJECT(e_xkb_cfg->cfd));
   
   e_xkb_cfg->cfd    = NULL;
   e_xkb_cfg->module = NULL;
   
   e_gadcon_provider_unregister(&_gc_class);
   
   e_xkb_unregister_module_actions();
   e_xkb_unregister_module_keybindings();
   
   EINA_LIST_FREE(e_xkb_cfg->used_layouts, cl)
     {
        eina_stringshare_del(cl->name);
        eina_stringshare_del(cl->model);
        eina_stringshare_del(cl->variant);
        
        E_FREE(cl);
     }
   
   EINA_LIST_FREE(e_xkb_cfg->used_options, op)
     {
        eina_stringshare_del(op->name);
        E_FREE(op);
     }
   
   if (e_xkb_cfg->default_model)
     eina_stringshare_del(e_xkb_cfg->default_model);
   
   E_FREE(e_xkb_cfg);
   
   E_CONFIG_DD_FREE(e_xkb_cfg_layout_edd);
   E_CONFIG_DD_FREE(e_xkb_cfg_option_edd);
   E_CONFIG_DD_FREE(e_xkb_cfg_edd);
   
   clear_rules();
   
   return 1;
}

/* Module state save
 * Used to save the configuration file.
 */
EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   e_config_domain_save("module.xkbswitch", e_xkb_cfg_edd, e_xkb_cfg);
   return 1;
}

/* Updates icons on all available xkbswitch gadgets to reflect the
 * current layout state.
 */
void
e_xkb_update_icon(void)
{
   Instance  *inst;
   Eina_List *l;
   
   if (!e_xkb_cfg->used_layouts) return;
   const char *name = ((E_XKB_Config_Layout*)eina_list_data_get(e_xkb_cfg->used_layouts))->name;

   if ((name) && (strchr(name, '/'))) name = strchr(name, '/') + 1;
   
   if (e_xkb_cfg->only_label)
     {
        EINA_LIST_FOREACH(instances, l, inst)
          {
             if (inst->o_xkbflag)
               {
                  evas_object_del(inst->o_xkbflag);
                  inst->o_xkbflag = NULL;
               }
             
             e_theme_edje_object_set(inst->o_xkbswitch, 
                                     "base/theme/modules/xkbswitch", 
                                     "modules/xkbswitch/noflag");
             edje_object_part_text_set(inst->o_xkbswitch, "label", name);
          }
     }
   else
     {
        char buf[PATH_MAX];
        
        snprintf(buf, sizeof(buf), "%s/data/flags/%s_flag.png",
                 e_prefix_data_get(), name);
        
        EINA_LIST_FOREACH(instances, l, inst)
          {
             if (!inst->o_xkbflag)
               {
                  inst->o_xkbflag = e_icon_add(inst->gcc->gadcon->evas);
                  e_icon_file_set(inst->o_xkbflag, buf);
                  edje_object_part_swallow(inst->o_xkbswitch, "flag",
                                           inst->o_xkbflag);
               }
             else
               e_icon_file_set(inst->o_xkbflag, buf);
             edje_object_part_text_set(inst->o_xkbswitch, "label", name);
          }
     }
}

void
e_xkb_update_layout(void)
{
   E_XKB_Config_Layout *cl;
   E_XKB_Config_Option *op;
   Eina_List *l;
   char buf[PATH_MAX];
   
   if (!e_xkb_cfg->used_layouts) return;

   /* We put an empty -option here in order to override all previously
    * set options.
    */
 
   // XXX: this is unsafe. doesn't keep into account size of buf
   snprintf(buf, sizeof(buf), "setxkbmap ");
   EINA_LIST_FOREACH(e_xkb_cfg->used_layouts, l, cl)
     {
        strcat(buf, cl->name);
        break;
        if (l->next) strcat(buf, ",");
     }
   
   strcat(buf, " -variant ");
   EINA_LIST_FOREACH(e_xkb_cfg->used_layouts, l, cl)
     {
        strcat(buf, cl->variant);
        strcat(buf, ",");
        break;
     }
   
   strcat(buf, " -model ");
   cl = eina_list_data_get(e_xkb_cfg->used_layouts);
   
   if (strcmp(cl->model, "default"))
     strcat(buf, cl->model);
   else if (strcmp(e_xkb_cfg->default_model, "default"))
     strcat(buf, e_xkb_cfg->default_model);
   else
     strcat(buf, "default");
   
   EINA_LIST_FOREACH(e_xkb_cfg->used_options, l, op)
     {
        strcat(buf, " -option ");
        strcat(buf, op->name);
        break;
     }
   printf("RUN: '%s'\n", buf);
   ecore_exe_run(buf, NULL);
}

void
e_xkb_layout_next(void)
{
   void *odata, *ndata;
   Eina_List *l;
   
   odata = eina_list_data_get(e_xkb_cfg->used_layouts);
   
   EINA_LIST_FOREACH(eina_list_next(e_xkb_cfg->used_layouts), l, ndata)
     {
        eina_list_data_set(eina_list_prev(l), ndata);
     }
   
   eina_list_data_set(eina_list_last(e_xkb_cfg->used_layouts), odata);
   e_xkb_update_icon();
   e_xkb_update_layout();
}

void
e_xkb_layout_prev(void)
{
   void *odata, *ndata;
   Eina_List *l;
   
   odata = eina_list_data_get(eina_list_last(e_xkb_cfg->used_layouts));
   
   for (l = e_xkb_cfg->used_layouts, ndata = eina_list_data_get(l);
        l; l = eina_list_next(l))
     {
        if (eina_list_next(l))
          ndata = eina_list_data_set(eina_list_next(l), ndata);
     }
   
   eina_list_data_set(e_xkb_cfg->used_layouts, odata);
   e_xkb_update_icon();
   e_xkb_update_layout();
}

/* LOCAL STATIC FUNCTIONS */

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *gcname, const char *id, const char *style) 
{
   Instance   *inst;
   const char *name;
   
   char buf[PATH_MAX];
   
   if (e_xkb_cfg->used_layouts)
     name = ((E_XKB_Config_Layout*)eina_list_data_get(e_xkb_cfg->used_layouts))->name;
   else name = NULL;

   if ((name) && (strchr(name, '/'))) name = strchr(name, '/') + 1;
   
   /* The instance */
   inst = E_NEW(Instance, 1);
   /* The gadget */
   inst->o_xkbswitch = edje_object_add(gc->evas);
   //XXX add to theme
   e_theme_edje_object_set(inst->o_xkbswitch,
                           "base/theme/modules/xkbswitch", 
                           "modules/xkbswitch/main");
   if (name) edje_object_part_text_set(inst->o_xkbswitch, "label", name);
   /* The gadcon client */
   inst->gcc = e_gadcon_client_new(gc, gcname, id, style, inst->o_xkbswitch);
   inst->gcc->data = inst;
   /* The flag icon */
   if (!e_xkb_cfg->only_label)
     {
        inst->o_xkbflag = e_icon_add(gc->evas);
        snprintf(buf, sizeof(buf), "%s/data/flags/%s_flag.png",
                 e_prefix_data_get(), name ? name : "unknown");
        e_icon_file_set(inst->o_xkbflag, buf);
        /* The icon is part of the gadget. */
        edje_object_part_swallow(inst->o_xkbswitch, "flag", inst->o_xkbflag);
     }
   else inst->o_xkbflag = NULL;
   
   /* Hook some menus */
   evas_object_event_callback_add(inst->o_xkbswitch, EVAS_CALLBACK_MOUSE_DOWN,
                                  _e_xkb_cb_mouse_down, inst);
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
   
   if (inst->menu) 
     {
        e_menu_post_deactivate_callback_set(inst->menu, NULL, NULL);
        e_object_del(E_OBJECT(inst->menu));
        inst->menu = NULL;
     }
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
_gc_label(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("XKB Switcher");
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__) 
{
   return _gc_class.name;
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas) 
{
   Evas_Object *o;
   char buf[PATH_MAX];
   
   snprintf(buf, sizeof(buf), "%s/e-module-xkbswitch.edj",
            e_xkb_cfg->module->dir);
   o = edje_object_add(evas);
   edje_object_file_set(o, buf, "icon");
   return o;
}

static void
_e_xkb_cfg_new(void) 
{
   e_xkb_cfg = E_NEW(E_XKB_Config, 1);
   
   e_xkb_cfg->used_layouts = NULL;
   e_xkb_cfg->used_options = NULL;
   e_xkb_cfg->version = MOD_CONFIG_FILE_VERSION;
   e_xkb_cfg->default_model = eina_stringshare_add("default");
   
#define BIND(act, actname) \
   e_xkb_cfg->layout_##act##_key.context = E_BINDING_CONTEXT_ANY; \
   e_xkb_cfg->layout_##act##_key.key = eina_stringshare_add("comma"); \
   e_xkb_cfg->layout_##act##_key.modifiers = E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT; \
   e_xkb_cfg->layout_##act##_key.any_mod = 0; \
   e_xkb_cfg->layout_##act##_key.action = eina_stringshare_add(actname); \
   e_xkb_cfg->layout_##act##_key.params = NULL
   
   BIND(next, E_XKB_NEXT_ACTION);
   BIND(prev, E_XKB_PREV_ACTION);
#undef BIND
   
   e_config_save_queue();
}

static void
_e_xkb_cfg_free(void) 
{
   E_XKB_Config_Layout *cl;
   E_XKB_Config_Option *op;
   
   EINA_LIST_FREE(e_xkb_cfg->used_layouts, cl)
     {
        eina_stringshare_del(cl->name);
        eina_stringshare_del(cl->model);
        eina_stringshare_del(cl->variant);
        E_FREE(cl);
     }
   
   EINA_LIST_FREE(e_xkb_cfg->used_options, op)
     {
        eina_stringshare_del(op->name);
        E_FREE(op);
     }
   
   if (e_xkb_cfg->default_model)
     eina_stringshare_del(e_xkb_cfg->default_model);
   E_FREE(e_xkb_cfg);
}

static Eina_Bool
_e_xkb_cfg_timer(void *data) 
{
   e_util_dialog_internal( _("XKB Switcher Configuration Updated"), data);
   return EINA_FALSE;
}

static void
_e_xkb_cb_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event) 
{
   Evas_Event_Mouse_Down *ev = event;
   Instance *inst = data;
   
   if (!inst) return;
   
   if ((ev->button == 3) && (!inst->menu)) /* Right-click utility menu */
     {
        int x, y;
        E_Menu_Item *mi;
        
        /* The menu and menu item */
        inst->menu = e_menu_new();
        mi = e_menu_item_new(inst->menu);
        /* Menu item specifics */
        e_menu_item_label_set(mi, _("Settings"));
        e_util_menu_item_theme_icon_set(mi, "preferences-system");
        e_menu_item_callback_set(mi, _e_xkb_cb_menu_configure, NULL);
        /* Append into the util menu */
        inst->menu = e_gadcon_client_util_menu_items_append(inst->gcc, 
                                                            inst->menu, 0);
        /* Callback */
        e_menu_post_deactivate_callback_set(inst->menu, _e_xkb_cb_menu_post,
                                            inst);
        /* Coords */
        e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y,
                                          NULL, NULL);
        /* Activate - we show the menu relative to the gadget */
        e_menu_activate_mouse(inst->menu,
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
             E_XKB_Config_Layout *cl;
             E_Menu_Item *mi;
             Eina_List *l;
             int dir;
             char buf[PATH_MAX];
             
             mi = e_menu_item_new(inst->lmenu);
             
             e_menu_item_label_set(mi, _("Settings"));
             e_util_menu_item_theme_icon_set(mi, "preferences-system");
             e_menu_item_callback_set(mi, _e_xkb_cb_menu_configure, NULL);

             mi = e_menu_item_new(inst->lmenu);
             e_menu_item_separator_set(mi, 1);
             
             /* Append all the layouts */
             EINA_LIST_FOREACH(e_xkb_cfg->used_layouts, l, cl)
               {
                  const char *name = cl->name;
                  
                  mi = e_menu_item_new(inst->lmenu);
                  
                  e_menu_item_radio_set(mi, 1);
                  e_menu_item_radio_group_set(mi, 1);
                  e_menu_item_toggle_set(mi, 
                                         (l == e_xkb_cfg->used_layouts) ? 1 : 0);
                  if (strchr(name, '/')) name = strchr(name, '/') + 1;
                  snprintf(buf, sizeof(buf), "%s/data/flags/%s_flag.png",
                           e_prefix_data_get(), name);
                  if (!ecore_file_exists(buf))
                    snprintf(buf, sizeof(buf), "%s/data/flags/unknown_flag.png",
                             e_prefix_data_get());
                  e_menu_item_icon_file_set(mi, buf);
                  snprintf(buf, sizeof(buf), "%s (%s, %s)", cl->name, 
                           cl->model, cl->variant);
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
_e_xkb_cb_menu_post(void *data, E_Menu *menu __UNUSED__) 
{
   Instance *inst = data;
   
   if (!(inst) || !inst->menu) return;
   inst->menu = NULL;
}

static void
_e_xkb_cb_lmenu_post(void *data, E_Menu *menu __UNUSED__) 
{
   Instance *inst = data;

   if (!(inst) || !inst->lmenu) return;
   inst->lmenu = NULL;
}

static void
_e_xkb_cb_menu_configure(void *data __UNUSED__, E_Menu *mn, E_Menu_Item *mi __UNUSED__)
{
   if (!e_xkb_cfg || e_xkb_cfg->cfd) return;
   e_xkb_cfg_dialog(mn->zone->container, NULL);
}

static void
_e_xkb_cb_lmenu_set(void *data, E_Menu *mn __UNUSED__, E_Menu_Item *mi __UNUSED__) 
{
   Eina_List *l;
   void *ndata;
   void *odata = eina_list_data_get(e_xkb_cfg->used_layouts);

   EINA_LIST_FOREACH(e_xkb_cfg->used_layouts, l, ndata)
     {
        if (ndata == data) eina_list_data_set(l, odata);
     }
   
   eina_list_data_set(e_xkb_cfg->used_layouts, data);
   
   e_xkb_update_icon();
   e_xkb_update_layout();
}
