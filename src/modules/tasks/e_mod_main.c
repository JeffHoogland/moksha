#include "e.h"
#include "e_mod_main.h"

/***************************************************************************/
 /**/
/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *client_class);

/* and actually define the gadcon class that this module provides (just 1) */
static E_Gadcon_Client_Class _gadcon_class = {
   GADCON_CLIENT_CLASS_VERSION,
   "tasks",
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, NULL
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};
/**/
/***************************************************************************/
/***************************************************************************/
/**/
/* actual module specifics */

typedef struct _Tasks Tasks;
typedef struct _Tasks_Item Tasks_Item;

struct _Tasks
{
   E_Gadcon_Client *gcc;        // The gadcon client
   Evas_Object *o_items;        // Table of items
   Eina_List *items;            // List of items
   E_Zone *zone;                // Current Zone
   Config_Item *config;         // Configuration
   int horizontal;
};

struct _Tasks_Item
{
   Tasks *tasks;            // Parent tasks
   E_Border *border;            // The border this item points to
   Evas_Object *o_item;         // The edje theme object
   Evas_Object *o_icon;         // The icon
};

static Tasks *_tasks_new(Evas *evas, E_Zone *zone, const char *id);
static void _tasks_free(Tasks *tasks);
static void _tasks_refill(Tasks *tasks);
static void _tasks_refill_all();
static void _tasks_refill_border(E_Border *border);
static void _tasks_signal_emit(E_Border *border, char *sig, char *src);

static Tasks_Item *_tasks_item_find(Tasks *tasks, E_Border *border);
static Tasks_Item *_tasks_item_new(Tasks *tasks, E_Border *border);

static int _tasks_item_check_add(Tasks *tasks, E_Border *border);
static void _tasks_item_add(Tasks *tasks, E_Border *border);
static void _tasks_item_remove(Tasks_Item *item);
static void _tasks_item_refill(Tasks_Item *item);
static void _tasks_item_fill(Tasks_Item *item);
static void _tasks_item_free(Tasks_Item *item);
static void _tasks_item_signal_emit(Tasks_Item *item, char *sig, char *src);

static Config_Item *_tasks_config_item_get(const char *id);

static void _tasks_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi);
static void _tasks_cb_item_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _tasks_cb_item_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);

static Eina_Bool _tasks_cb_event_border_add(void *data, int type, void *event);
static Eina_Bool _tasks_cb_event_border_remove(void *data, int type, void *event);
static Eina_Bool _tasks_cb_event_border_iconify(void *data, int type, void *event);
static Eina_Bool _tasks_cb_event_border_uniconify(void *data, int type, void *event);
static Eina_Bool _tasks_cb_event_border_icon_change(void *data, int type, void *event);
static Eina_Bool _tasks_cb_event_border_zone_set(void *data, int type, void *event);
static Eina_Bool _tasks_cb_event_border_desk_set(void *data, int type, void *event);
static Eina_Bool _tasks_cb_window_focus_in(void *data, int type, void *event);
static Eina_Bool _tasks_cb_window_focus_out(void *data, int type, void *event);
static Eina_Bool _tasks_cb_event_border_property(void *data, int type, void *event);
static Eina_Bool _tasks_cb_event_desk_show(void *data, int type, void *event);
static Eina_Bool _tasks_cb_event_border_urgent_change(void *data, int type, void *event);

static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_item_edd = NULL;

Config *tasks_config = NULL;

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Tasks"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   conf_item_edd = E_CONFIG_DD_NEW("Tasks_Config_Item", Config_Item);

#undef T
#undef D
#define T Config_Item
#define D conf_item_edd
   E_CONFIG_VAL(D, T, id, STR);
   E_CONFIG_VAL(D, T, show_all, INT);
   E_CONFIG_VAL(D, T, minw, INT);
   E_CONFIG_VAL(D, T, minh, INT);

   conf_edd = E_CONFIG_DD_NEW("Tasks_Config", Config);

#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_LIST(D, T, items, conf_item_edd);

   tasks_config = e_config_domain_load("module.tasks", conf_edd);
   if (!tasks_config)
     {
        Config_Item *config;
        
        tasks_config = E_NEW(Config, 1);
        config = E_NEW(Config_Item, 1);
        config->id = eina_stringshare_add("0");
        config->show_all = 0;
        config->minw = 100;
        config->minh = 32;
        tasks_config->items = eina_list_append(tasks_config->items, config);
     }

   tasks_config->module = m;

   tasks_config->handlers = eina_list_append
     (tasks_config->handlers, ecore_event_handler_add
         (E_EVENT_BORDER_ADD, _tasks_cb_event_border_add, NULL));
   tasks_config->handlers = eina_list_append
     (tasks_config->handlers, ecore_event_handler_add
         (E_EVENT_BORDER_REMOVE, _tasks_cb_event_border_remove, NULL));
   tasks_config->handlers = eina_list_append
     (tasks_config->handlers, ecore_event_handler_add
         (E_EVENT_BORDER_ICONIFY, _tasks_cb_event_border_iconify, NULL));
   tasks_config->handlers = eina_list_append
     (tasks_config->handlers, ecore_event_handler_add
         (E_EVENT_BORDER_UNICONIFY, _tasks_cb_event_border_uniconify, NULL));
   tasks_config->handlers = eina_list_append
     (tasks_config->handlers, ecore_event_handler_add
         (E_EVENT_BORDER_ICON_CHANGE, _tasks_cb_event_border_icon_change, NULL));
   tasks_config->handlers = eina_list_append
     (tasks_config->handlers, ecore_event_handler_add
         (E_EVENT_BORDER_DESK_SET, _tasks_cb_event_border_desk_set, NULL));
   tasks_config->handlers = eina_list_append
     (tasks_config->handlers, ecore_event_handler_add
         (E_EVENT_BORDER_ZONE_SET, _tasks_cb_event_border_zone_set, NULL));
   tasks_config->handlers = eina_list_append
     (tasks_config->handlers, ecore_event_handler_add
         (E_EVENT_BORDER_FOCUS_IN, _tasks_cb_window_focus_in, NULL));
   tasks_config->handlers = eina_list_append
     (tasks_config->handlers, ecore_event_handler_add
         (E_EVENT_BORDER_FOCUS_OUT, _tasks_cb_window_focus_out, NULL));
   tasks_config->handlers = eina_list_append
     (tasks_config->handlers, ecore_event_handler_add
         (E_EVENT_BORDER_PROPERTY, _tasks_cb_event_border_property, NULL));
   tasks_config->handlers = eina_list_append
     (tasks_config->handlers, ecore_event_handler_add
         (E_EVENT_DESK_SHOW, _tasks_cb_event_desk_show, NULL));
   tasks_config->handlers = eina_list_append
     (tasks_config->handlers, ecore_event_handler_add
         (E_EVENT_BORDER_URGENT_CHANGE, _tasks_cb_event_border_urgent_change, NULL));
   
   tasks_config->borders = eina_list_clone(e_border_client_list());
   
   e_gadcon_provider_register(&_gadcon_class);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   Ecore_Event_Handler *eh;
   Tasks *tasks;

   e_gadcon_provider_unregister(&_gadcon_class);

   EINA_LIST_FREE(tasks_config->tasks, tasks)
     {
        _tasks_free(tasks);
     }

   if (tasks_config->config_dialog)
     e_object_del(E_OBJECT(tasks_config->config_dialog));

   EINA_LIST_FREE(tasks_config->handlers, eh)
     {
        ecore_event_handler_del(eh);
     }

   eina_list_free(tasks_config->borders);

   free(tasks_config);
   tasks_config = NULL;
   E_CONFIG_DD_FREE(conf_item_edd);
   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   e_config_domain_save("module.tasks", conf_edd, tasks_config);
   return 1;
}

/**************************************************************/

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Tasks *tasks;
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Evas_Coord x, y, w, h;
   int cx, cy, cw, ch;

   tasks = _tasks_new(gc->evas, gc->zone, id);
   
   o = tasks->o_items;
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = tasks;
   tasks->gcc = gcc;
   
   e_gadcon_canvas_zone_geometry_get(gcc->gadcon, &cx, &cy, &cw, &ch);
   evas_object_geometry_get(o, &x, &y, &w, &h);
   
   tasks_config->tasks = eina_list_append(tasks_config->tasks, tasks);

   // Fill on initial config
   _tasks_config_updated(tasks->config);

   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Tasks *tasks;
   
   tasks = (Tasks *)gcc->data;
   tasks_config->tasks = eina_list_remove(tasks_config->tasks, tasks);
   _tasks_free(tasks);
}

/* TODO */
static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   Tasks *tasks;

   tasks = (Tasks *)gcc->data;

   switch (orient)
     {
      case E_GADCON_ORIENT_FLOAT:
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_BOTTOM:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_BR:
	if (!tasks->horizontal)
	  {
             tasks->horizontal = 1;
             e_box_orientation_set(tasks->o_items, tasks->horizontal);
             _tasks_refill(tasks);
	  }
	break;
      case E_GADCON_ORIENT_VERT:
      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_RIGHT:
      case E_GADCON_ORIENT_CORNER_LT:
      case E_GADCON_ORIENT_CORNER_RT:
      case E_GADCON_ORIENT_CORNER_LB:
      case E_GADCON_ORIENT_CORNER_RB:
	if (tasks->horizontal)
	  {
             tasks->horizontal = 0;
             e_box_orientation_set(tasks->o_items, tasks->horizontal);
             _tasks_refill(tasks);
	  }
	break;
      default:
	break;
     }
   e_box_align_set(tasks->o_items, 0.5, 0.5);
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("Tasks");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;
   char buf[4096];

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-tasks.edj",
            e_module_dir_get(tasks_config->module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   Config_Item *config;

   config = _tasks_config_item_get(NULL);
   return config->id;
}

/***************************************************************************/

static Tasks *
_tasks_new(Evas *evas, E_Zone *zone, const char *id)
{
   Tasks *tasks;

   tasks = E_NEW(Tasks, 1);
   tasks->config = _tasks_config_item_get(id);
   tasks->o_items = e_box_add(evas);
   tasks->horizontal = 1;

   e_box_homogenous_set(tasks->o_items, 1);
   e_box_orientation_set(tasks->o_items, tasks->horizontal);
   e_box_align_set(tasks->o_items, 0.5, 0.5);
   tasks->zone = zone;
   return tasks;
}

static void
_tasks_free(Tasks *tasks)
{
   Tasks_Item *item;
   EINA_LIST_FREE(tasks->items, item) _tasks_item_free(item);
   evas_object_del(tasks->o_items);
   free(tasks);
}

static void 
_tasks_refill(Tasks *tasks)
{
   Eina_List *l;
   E_Border *border;
   Tasks_Item *item;
   Evas_Coord w, h;

   while (tasks->items)
     {
        item = tasks->items->data;
        _tasks_item_remove(item);
     }
   EINA_LIST_FOREACH(tasks_config->borders, l, border)
     {
        _tasks_item_check_add(tasks, border);
     }
   if (tasks->items)
     {
        item = tasks->items->data;
	edje_object_size_min_calc(item->o_item, &w, &h);
        if (tasks->horizontal)
          {
             if (w < tasks->config->minw) w = tasks->config->minw;
          }
        else
          {
             if (h < tasks->config->minh) h = tasks->config->minh;
          }
	if (!tasks->gcc->resizable)
	  {
	     if (tasks->horizontal)
               e_gadcon_client_min_size_set(tasks->gcc, 
                                            w * eina_list_count(tasks->items),
                                            h);
	     else
               e_gadcon_client_min_size_set(tasks->gcc, 
                                            w,
                                            h * eina_list_count(tasks->items));
          }
     }
}

static void
_tasks_refill_all(void)
{
   const Eina_List *l;
   Tasks *tasks;

   EINA_LIST_FOREACH(tasks_config->tasks, l, tasks)
     {
        _tasks_refill(tasks);
     }
}


static void
_tasks_refill_border(E_Border *border)
{
   const Eina_List *l;
   Tasks *tasks;
   
   EINA_LIST_FOREACH(tasks_config->tasks, l, tasks)
     {
	const Eina_List *m;
	Tasks_Item *item;
	EINA_LIST_FOREACH(tasks->items, m, item)
          {
             if (item->border == border) _tasks_item_refill(item);
          }
     }
}

static void 
_tasks_signal_emit(E_Border *border, char *sig, char *src)
{
   const Eina_List *l;
   Tasks *tasks;
   
   EINA_LIST_FOREACH(tasks_config->tasks, l, tasks)
     {
	const Eina_List *m;
	Tasks_Item *item;
        
	EINA_LIST_FOREACH(tasks->items, m, item)
          {
             if (item->border == border)
               _tasks_item_signal_emit(item, sig, src);
          }
     }
}

static Tasks_Item *
_tasks_item_find(Tasks *tasks, E_Border *border)
{
   const Eina_List *l;
   Tasks_Item *item;

   EINA_LIST_FOREACH(tasks->items, l, item)
     {
        if (item->border == border) return item;
     }
   return NULL;
}

static Tasks_Item *
_tasks_item_new(Tasks *tasks, E_Border *border)
{
   Tasks_Item *item;

   item = E_NEW(Tasks_Item, 1);
   e_object_ref(E_OBJECT(border));
   item->tasks = tasks;
   item->border = border;
   item->o_item = edje_object_add(evas_object_evas_get(tasks->o_items));
   if (tasks->horizontal)
     e_theme_edje_object_set(item->o_item, 
                             "base/theme/modules/tasks", 
                             "modules/tasks/item");
   else
     {
        if (!e_theme_edje_object_set(item->o_item, 
                                     "base/theme/modules/tasks", 
                                     "modules/tasks/item_vert"))
          e_theme_edje_object_set(item->o_item, 
                                  "base/theme/modules/tasks", 
                                  "modules/tasks/item");
     }
   evas_object_event_callback_add(item->o_item, EVAS_CALLBACK_MOUSE_DOWN,
                                  _tasks_cb_item_mouse_down, item);
   evas_object_event_callback_add(item->o_item, EVAS_CALLBACK_MOUSE_UP,
                                  _tasks_cb_item_mouse_up, item);
   evas_object_show(item->o_item);
   
   _tasks_item_fill(item);
   return item;
}

static int
_tasks_item_check_add(Tasks *tasks, E_Border *border)
{
   if (border->user_skip_winlist) return 1;
   if (border->client.netwm.state.skip_taskbar) return 1;
   if (_tasks_item_find(tasks, border)) return 1;
   if (!tasks->config) return 1;
   if (!(tasks->config->show_all))
     {
        if (border->zone != tasks->zone) return 1;
        if ((border->desk != e_desk_current_get(border->zone)) && 
            (!border->sticky))
          return 1;
     }
   _tasks_item_add(tasks, border);
   return 0;
}

static void
_tasks_item_add(Tasks *tasks, E_Border *border)
{
   Tasks_Item *item;

   item = _tasks_item_new(tasks, border);
   e_box_pack_end(tasks->o_items, item->o_item);
   e_box_pack_options_set(item->o_item,
                          1, 1, /* fill */
                          1, 1, /* expand */
                          0.5, 0.5, /* align */
                          1, 1, /* min */
                          9999, 9999 /* max */
                         );
   tasks->items = eina_list_append(tasks->items, item);
}

static void
_tasks_item_remove(Tasks_Item *item)
{
   item->tasks->items = eina_list_remove(item->tasks->items, item);
   e_box_unpack(item->o_item);
   evas_object_del(item->o_item);
   _tasks_item_free(item);
}

static void
_tasks_item_free(Tasks_Item *item)
{
   if (item->o_icon) evas_object_del(item->o_icon);
   e_object_unref(E_OBJECT(item->border));
   evas_object_del(item->o_item);
   free(item);
}

static void
_tasks_item_refill(Tasks_Item *item)
{
   if (item->o_icon) evas_object_del(item->o_icon);
   _tasks_item_fill(item);
}

static void
_tasks_item_fill(Tasks_Item *item)
{
   const char *label;
   
   item->o_icon = e_border_icon_add(item->border, evas_object_evas_get(item->tasks->o_items));
   edje_object_part_swallow(item->o_item, "e.swallow.icon", item->o_icon);
   evas_object_pass_events_set(item->o_icon, 1);
   evas_object_show(item->o_icon);
   
   label = e_border_name_get(item->border);
   edje_object_part_text_set(item->o_item, "e.text.label", label);
   
   if (item->border->iconic)
     _tasks_item_signal_emit(item, "e,state,iconified", "e");
   else
     _tasks_item_signal_emit(item, "e,state,uniconified", "e");
   if (item->border->focused)
     _tasks_item_signal_emit(item, "e,state,focused", "e");
   else
     _tasks_item_signal_emit(item, "e,state,unfocused", "e");
   if (item->border->client.icccm.urgent)
     _tasks_item_signal_emit(item, "e,state,urgent", "e");
   else
     _tasks_item_signal_emit(item, "e,state,not_urgent", "e");
}

static void
_tasks_item_signal_emit(Tasks_Item *item, char *sig, char *src)
{
   if (item->o_item) edje_object_signal_emit(item->o_item, sig, src);
   if (item->o_icon) edje_object_signal_emit(item->o_icon, sig, src);
}

static Config_Item *
_tasks_config_item_get(const char *id)
{
   Eina_List *l;
   Config_Item *config;
   char buf[128];
   
   if (!id)
     {
        int num = 0;
        
        /* Create id */
        if (tasks_config->items)
          {
             const char *p;
             
             config = eina_list_last(tasks_config->items)->data;
             p = strrchr(config->id, '.');
             if (p) num = atoi(p + 1) + 1;
          }
        snprintf(buf, sizeof(buf), "%s.%d", _gadcon_class.name, num);
        id = buf;
     }
   else
     {
	EINA_LIST_FOREACH(tasks_config->items, l, config)
          {
             if (!config->id) continue;
             if (!strcmp(config->id, id)) return config;
          }
     }

   config = E_NEW(Config_Item, 1);
   config->id = eina_stringshare_add(id);
   config->show_all = 0;

   tasks_config->items = eina_list_append(tasks_config->items, config);

   return config;
}

void
_tasks_config_updated(Config_Item *config)
{
   const Eina_List *l;
   Tasks *tasks;
   
   if (!tasks_config) return;
   EINA_LIST_FOREACH(tasks_config->tasks, l, tasks)
     {
        if (tasks->config == config) _tasks_refill(tasks);
     }
}

static void
_tasks_cb_menu_configure(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   Tasks *tasks;

   tasks = (Tasks *) data;
   _config_tasks_module(tasks->config);
}

static void
_tasks_cb_item_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Tasks_Item *item;
   
   item = (Tasks_Item *) data;
   ev = event_info;
   if (ev->button == 3) 
     {
        E_Menu *m;
	E_Menu_Item *mi;
        int cx, cy, cw, ch;
        
        e_gadcon_canvas_zone_geometry_get(item->tasks->gcc->gadcon, &cx, &cy, &cw, &ch);
        
	e_int_border_menu_create(item->border);
        
	mi = e_menu_item_new(item->border->border_menu);
	e_menu_item_separator_set(mi, 1);
        
	m = e_menu_new();
        mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Settings"));
	e_util_menu_item_theme_icon_set(mi, "preferences-system");
	e_menu_item_callback_set(mi, _tasks_cb_menu_configure, item->tasks);
        
	m = e_gadcon_client_util_menu_items_append(item->tasks->gcc, m, 0);
        
        mi = e_menu_item_new(item->border->border_menu);
        e_menu_item_label_set(mi, _("Tasks"));
        e_menu_item_submenu_set(mi, m);
	e_util_menu_item_theme_icon_set(mi, "preferences-system");
        
	e_menu_activate_mouse(item->border->border_menu,
                              e_util_zone_current_get(e_manager_current_get()),
                              cx + ev->output.x, cy + ev->output.y, 1, 1,
                              E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
        evas_event_feed_mouse_up(item->tasks->gcc->gadcon->evas, ev->button,
                                 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
   
}

static void
_tasks_cb_item_mouse_up(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   Tasks_Item *item;
   
   ev = event_info;
   item = data;
   if (ev->button == 1)
     {
        if (!item->border->sticky && item->tasks->config->show_all)
          e_desk_show(item->border->desk);
        if (evas_key_modifier_is_set(ev->modifiers, "Alt"))
          {
             if (item->border->iconic)
               e_border_uniconify(item->border);
             else
               e_border_iconify(item->border);
          }
        else if (evas_key_modifier_is_set(ev->modifiers, "Control"))
          {
             if (item->border->maximized)
               e_border_unmaximize(item->border, e_config->maximize_policy);
             else
               e_border_maximize(item->border, e_config->maximize_policy);
          }
        else if (evas_key_modifier_is_set(ev->modifiers, "Shift"))
          {
             if (item->border->shaded)
               e_border_unshade(item->border, item->border->shade.dir);
             else
               e_border_shade(item->border, item->border->shade.dir);
          }
        else if (evas_key_modifier_is_set(ev->modifiers, "Super"))
          {
             e_border_act_close_begin(item->border);
          }
        else
          {
             if (item->border->iconic)
               {
                  e_border_uniconify(item->border);
                  e_border_focus_set(item->border, 1, 1);
               }
             else
               {
                  if (item->border->focused)
                    {
                       e_border_iconify(item->border);
                    }
                  else
                    {
                       e_border_raise(item->border);
                       e_border_focus_set(item->border, 1, 1);
                    }
               }
          }
     }
   else if (ev->button == 2)
     {
        if (!item->border->sticky && item->tasks->config->show_all)
          e_desk_show(item->border->desk);
        e_border_raise(item->border);
        e_border_focus_set(item->border, 1, 1);
        if (item->border->maximized)
          e_border_unmaximize(item->border, e_config->maximize_policy);
        else
          e_border_maximize(item->border, e_config->maximize_policy);
     }
}

/************ BORDER CALLBACKS *********************/

static Eina_Bool
_tasks_cb_event_border_add(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Add *ev;

   ev = event;
   tasks_config->borders = eina_list_append(tasks_config->borders, ev->border);
   _tasks_refill_all();
   return EINA_TRUE;
}

static Eina_Bool
_tasks_cb_event_border_remove(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Remove *ev;

   ev = event;
   tasks_config->borders = eina_list_remove(tasks_config->borders, ev->border);
   _tasks_refill_all();
   return EINA_TRUE;
}

static Eina_Bool
_tasks_cb_event_border_iconify(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Iconify *ev;

   ev = event;
   _tasks_signal_emit(ev->border, "e,state,iconified", "e");
   return EINA_TRUE;
}

static Eina_Bool
_tasks_cb_event_border_uniconify(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Uniconify *ev;

   ev = event;
   _tasks_signal_emit(ev->border, "e,state,uniconified", "e");
   return EINA_TRUE;
}

static Eina_Bool
_tasks_cb_window_focus_in(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Focus_In *ev;

   ev = event;
   _tasks_signal_emit(ev->border, "e,state,focused", "e");
   return EINA_TRUE;
}

static Eina_Bool
_tasks_cb_window_focus_out(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Focus_Out *ev;

   ev = event;
   _tasks_signal_emit(ev->border, "e,state,unfocused", "e");
   return EINA_TRUE;
}

static Eina_Bool
_tasks_cb_event_border_urgent_change(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Urgent_Change *ev;

   ev = event;
   if (ev->border->client.icccm.urgent)
     _tasks_signal_emit(ev->border, "e,state,urgent", "e");
   else
     _tasks_signal_emit(ev->border, "e,state,not_urgent", "e");
   return EINA_TRUE;
}

static Eina_Bool
_tasks_cb_event_border_property(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Property *ev;
   E_Border *border;

   ev = event;
   border = ev->border;
   if (border) _tasks_refill_border(border);
   return EINA_TRUE;
}

static Eina_Bool
_tasks_cb_event_border_icon_change(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Icon_Change *ev;
   E_Border *border;
   
   ev = event;
   border = ev->border;
   if (border) _tasks_refill_border(border);
   return EINA_TRUE;
}

static Eina_Bool
_tasks_cb_event_border_zone_set(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   _tasks_refill_all();
   return EINA_TRUE;
}

static Eina_Bool
_tasks_cb_event_border_desk_set(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   _tasks_refill_all();
   return EINA_TRUE;
}

static Eina_Bool
_tasks_cb_event_desk_show(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   _tasks_refill_all();
   return EINA_TRUE;
}

/***************************************************************************/
 /**/

 /**/
/***************************************************************************/
