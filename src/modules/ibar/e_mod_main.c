#include "e.h"
#include "e_mod_main.h"

/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void             _gc_shutdown(E_Gadcon_Client *gcc);
static void             _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char      *_gc_label(const E_Gadcon_Client_Class *client_class);
static Evas_Object     *_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas);
static const char      *_gc_id_new(const E_Gadcon_Client_Class *client_class);
static void             _gc_id_del(const E_Gadcon_Client_Class *client_class, const char *id);

/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
   "ibar",
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, _gc_id_del,
      e_gadcon_site_is_not_toolbar
   },
   E_GADCON_CLIENT_STYLE_INSET
};

/* actual module specifics */
typedef struct _Instance  Instance;
typedef struct _IBar      IBar;
typedef struct _IBar_Icon IBar_Icon;

struct _Instance
{
   E_Gadcon_Client *gcc;
   IBar            *ibar;
   E_Drop_Handler  *drop_handler;
   Config_Item     *ci;
   E_Gadcon_Orient  orient;
};

typedef struct
{
  E_Order *eo;
  Eina_Inlist *bars;
} IBar_Order;

struct _IBar
{
   EINA_INLIST;
   Instance    *inst;
   Ecore_Job *resize_job;
   Evas_Object *o_outerbox;
   Evas_Object *o_box, *o_drop;
   Evas_Object *o_drop_over, *o_empty;
   Evas_Object *o_sep;
   unsigned int not_in_order_count;
   IBar_Icon   *ic_drop_before;
   int          drop_before;
   Eina_Hash    *icon_hash;
   Eina_Inlist  *icons;
   IBar_Order  *io;
   Evas_Coord   dnd_x, dnd_y;
   IBar_Icon   *menu_icon;
   Eina_Bool    focused : 1;
};

struct _IBar_Icon
{
   EINA_INLIST;
   IBar            *ibar;
   Evas_Object     *o_holder, *o_icon;
   Evas_Object     *o_holder2, *o_icon2;
   Efreet_Desktop  *app;
   Ecore_Timer     *reset_timer;
   Ecore_Timer     *show_timer; //for menu
   Ecore_Timer     *timer;
   Ecore_Timer     *hide_timer;
   E_Exec_Instance *exe_inst;
   Eina_List       *exes; //all instances
   Eina_List       *exe_current;
   E_Gadcon_Popup  *menu;
   const char      *hashname;
   int              mouse_down;
   struct
   {
      unsigned char start : 1;
      unsigned char dnd : 1;
      int           x, y;
   } drag;
   Eina_Bool       focused : 1;
   Eina_Bool       not_in_order : 1;
   Eina_Bool       menu_grabbed : 1;
   Eina_Bool       starting : 1;
};

static IBar        *_ibar_new(Evas *evas, Instance *inst);
static void         _ibar_free(IBar *b);
static void         _ibar_cb_empty_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void         _ibar_empty_handle(IBar *b);
static void         _ibar_instance_watch(void *data, E_Exec_Instance *inst, E_Exec_Watch_Type type);
static void         _ibar_fill(IBar *b);
static void         _ibar_empty(IBar *b);
static void         _ibar_orient_set(IBar *b, int horizontal);
static void         _ibar_resize_handle(IBar *b);
static void         _ibar_instance_drop_zone_recalc(Instance *inst);
static Config_Item *_ibar_config_item_get(const char *id);
static IBar_Icon   *_ibar_icon_at_coord(IBar *b, Evas_Coord x, Evas_Coord y);
static IBar_Icon   *_ibar_icon_new(IBar *b, Efreet_Desktop *desktop, Eina_Bool notinorder);
static IBar_Icon   *_ibar_icon_notinorder_new(IBar *b, E_Exec_Instance *exe);
static void         _ibar_icon_free(IBar_Icon *ic);
static void         _ibar_icon_fill(IBar_Icon *ic);
static void         _ibar_icon_empty(IBar_Icon *ic);
static void         _ibar_sep_create(IBar *b);
static void         _ibar_icon_signal_emit(IBar_Icon *ic, char *sig, char *src);
static void         _ibar_cb_app_change(void *data, E_Order *eo);
static void         _ibar_cb_obj_moveresize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void         _ibar_cb_menu_icon_new(void *data, E_Menu *m, E_Menu_Item *mi);
static void         _ibar_cb_menu_icon_add(void *data, E_Menu *m, E_Menu_Item *mi);
static void         _ibar_cb_menu_icon_properties(void *data, E_Menu *m, E_Menu_Item *mi);
static void         _ibar_cb_menu_icon_remove(void *data, E_Menu *m, E_Menu_Item *mi);
static void         _ibar_cb_menu_configuration(void *data, E_Menu *m, E_Menu_Item *mi);
static void         _ibar_cb_icon_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void         _ibar_cb_icon_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void         _ibar_cb_icon_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void         _ibar_cb_icon_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void         _ibar_cb_icon_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void         _ibar_cb_icon_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void         _ibar_cb_icon_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void         _ibar_cb_icon_wheel(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void         _ibar_inst_cb_enter(void *data, const char *type, void *event_info);
static void         _ibar_inst_cb_move(void *data, const char *type, void *event_info);
static void         _ibar_inst_cb_leave(void *data, const char *type, void *event_info);
static void         _ibar_inst_cb_drop(void *data, const char *type, void *event_info);
static void         _ibar_cb_drag_finished(E_Drag *data, int dropped);
static void         _ibar_drop_position_update(Instance *inst, Evas_Coord x, Evas_Coord y);
static void         _ibar_inst_cb_scroll(void *data);
static Eina_Bool    _ibar_cb_config_icons(void *data, int ev_type, void *ev);
static void         _ibar_cb_icon_menu_img_del(void *data, Evas *e, Evas_Object *obj, void *event_info);

static Eina_Bool    _ibar_cb_out_hide_delay(void *data);
static void         _ibar_icon_menu_show(IBar_Icon *ic, Eina_Bool grab);
static void         _ibar_icon_menu_hide(IBar_Icon *ic, Eina_Bool grab);

static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_item_edd = NULL;

static Eina_Hash *ibar_orders = NULL;
static Eina_List *ibars = NULL;

Config *ibar_config = NULL;

static inline const char *
_desktop_name_get(const Efreet_Desktop *desktop)
{
   if (!desktop) return NULL;
   return desktop->orig_path ?: desktop->name;
}

static IBar_Order *
_ibar_order_new(IBar *b, const char *path)
{
   IBar_Order *io;

   io = eina_hash_find(ibar_orders, path);
   if (io)
     {
        io->bars = eina_inlist_append(io->bars, EINA_INLIST_GET(b));
        return io;
     }
   io = E_NEW(IBar_Order, 1);
   io->eo = e_order_new(path);
   e_order_update_callback_set(io->eo, _ibar_cb_app_change, io);
   eina_hash_add(ibar_orders, path, io);
   io->bars = eina_inlist_append(io->bars, EINA_INLIST_GET(b));
   return io;
}

static void
_ibar_order_del(IBar *b)
{
   IBar_Order *io;
   if (!b->io) return;
   io = b->io;
   io->bars = eina_inlist_remove(io->bars, EINA_INLIST_GET(b));
   b->io = NULL;
   if (io->bars) return;
   eina_hash_del_by_key(ibar_orders, io->eo->path);
   e_order_update_callback_set(io->eo, NULL, NULL);
   e_object_del(E_OBJECT(io->eo));
   free(io);
}


static void
_ibar_order_refresh(IBar *b, const char *path)
{
   IBar_Order *io;
   IBar *bar;

   io = eina_hash_find(ibar_orders, path);
   if (io)
     {
        /* different order, remove/refresh */
        if (io != b->io)
          {
             if (b->io) _ibar_order_del(b);
             io->bars = eina_inlist_append(io->bars, EINA_INLIST_GET(b));
             b->io = io;
          }
        /* else same order, refresh all users */
     }
   else
      {
        _ibar_order_del(b);
        io = b->io = _ibar_order_new(b, path);
     }
   EINA_INLIST_FOREACH(io->bars, bar)
     {
        _ibar_empty(bar);
        _ibar_fill(bar);
     }
}

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   IBar *b;
   E_Gadcon_Client *gcc;
   Instance *inst;
   Evas_Coord x, y, w, h;
   const char *drop[] = { "enlightenment/desktop", "enlightenment/border", "text/uri-list" };
   Config_Item *ci;

   inst = E_NEW(Instance, 1);

   ci = _ibar_config_item_get(id);
   inst->ci = ci;
   if (!ci->dir) ci->dir = eina_stringshare_add("default");
   b = _ibar_new(gc->o_container, inst);
   gcc = e_gadcon_client_new(gc, name, id, style, b->o_outerbox);
   e_gadcon_client_min_size_set(gcc, 16, 16);
   e_gadcon_client_autoscroll_toggle_disabled_set(gcc, !ci->dont_add_nonorder);
   gcc->data = inst;

   inst->gcc = gcc;
   inst->orient = E_GADCON_ORIENT_HORIZ;

   evas_object_geometry_get(b->o_box, &x, &y, &w, &h);
   inst->drop_handler =
     e_drop_handler_add(E_OBJECT(inst->gcc), inst,
                        _ibar_inst_cb_enter, _ibar_inst_cb_move,
                        _ibar_inst_cb_leave, _ibar_inst_cb_drop,
                        drop, 3, x, y, w, h);
   evas_object_event_callback_add(b->o_outerbox, EVAS_CALLBACK_MOVE,
                                  _ibar_cb_obj_moveresize, inst);
   evas_object_event_callback_add(b->o_outerbox, EVAS_CALLBACK_RESIZE,
                                  _ibar_cb_obj_moveresize, inst);
   ibar_config->instances = eina_list_append(ibar_config->instances, inst);
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

   inst = gcc->data;
   ibar_config->instances = eina_list_remove(ibar_config->instances, inst);
   e_drop_handler_del(inst->drop_handler);
   _ibar_free(inst->ibar);
   E_FREE(inst);
}

static Eina_Bool
_gc_vertical(Instance *inst)
{
   switch (inst->orient)
     {
      case E_GADCON_ORIENT_FLOAT:
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_BOTTOM:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_BR:
        return EINA_FALSE;
        break;

      case E_GADCON_ORIENT_VERT:
      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_RIGHT:
      case E_GADCON_ORIENT_CORNER_LT:
      case E_GADCON_ORIENT_CORNER_RT:
      case E_GADCON_ORIENT_CORNER_LB:
      case E_GADCON_ORIENT_CORNER_RB:
      default:
        break;
     }
   return EINA_TRUE;
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   Instance *inst;

   inst = gcc->data;
   if ((int)orient != -1) inst->orient = orient;

   if (_gc_vertical(inst))
     {
        _ibar_orient_set(inst->ibar, 0);
     }
   else
     {
        _ibar_orient_set(inst->ibar, 1);
     }
}

static const char *
_gc_label(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("IBar");
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;
   char buf[PATH_MAX];

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-ibar.edj",
            e_module_dir_get(ibar_config->module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   Config_Item *ci;

   ci = _ibar_config_item_get(NULL);
   return ci->id;
}

static void
_gc_id_del(const E_Gadcon_Client_Class *client_class __UNUSED__, const char *id __UNUSED__)
{
/* yes - don't do this. on shutdown gadgets are deleted and this means config
 * for them is deleted - that means empty config is saved. keep them around
 * as if u add a gadget back it can pick up its old config again
 *
   Config_Item *ci;

   ci = _ibar_config_item_get(id);
   if (ci)
     {
        if (ci->id) eina_stringshare_del(ci->id);
        ibar_config->items = eina_list_remove(ibar_config->items, ci);
     }
 */
}

static IBar *
_ibar_new(Evas *evas, Instance *inst)
{
   IBar *b;
   char buf[PATH_MAX];
   int w, h;

   b = E_NEW(IBar, 1);
   inst->ibar = b;
   b->inst = inst;
   b->icon_hash = eina_hash_string_superfast_new(NULL);
   b->o_outerbox = e_box_add(evas);
   e_box_orientation_set(b->o_outerbox, 1);
   e_box_align_set(b->o_outerbox, 0.5, 0.5);
   b->o_box = e_box_add(evas);
   e_box_homogenous_set(b->o_box, 1);
   e_box_orientation_set(b->o_box, 1);
   e_box_align_set(b->o_box, 0.5, 0.5);
   if (inst->ci->dir[0] != '/')
     e_user_dir_snprintf(buf, sizeof(buf), "applications/bar/%s/.order",
                         inst->ci->dir);
   else
     eina_strlcpy(buf, inst->ci->dir, sizeof(buf));
   b->io = _ibar_order_new(b, buf);
   e_box_pack_end(b->o_outerbox, b->o_box);
   _ibar_fill(b);
   e_box_size_min_get(b->o_box, &w, &h);
   e_box_pack_options_set(b->o_box,
                          1, 1, /* fill */
                          1, 1, /* expand */
                          0.5, 0.5, /* align */
                          w, h, /* min */
                          -1, -1 /* max */
                          );
   evas_object_show(b->o_box);
   ibars = eina_list_append(ibars, b);
   return b;
}

static void
_ibar_free(IBar *b)
{
   _ibar_empty(b);
   evas_object_del(b->o_outerbox);
   evas_object_del(b->o_box);
   if (b->o_drop) evas_object_del(b->o_drop);
   if (b->o_drop_over) evas_object_del(b->o_drop_over);
   if (b->o_empty) evas_object_del(b->o_empty);
   E_FREE_FUNC(b->resize_job, ecore_job_del);
   eina_hash_free(b->icon_hash);
   _ibar_order_del(b);
   ibars = eina_list_remove(ibars, b);
   free(b);
}

static void
_ibar_cb_empty_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   IBar *b;
   E_Menu *m;
   E_Menu_Item *mi;
   int cx, cy, cw, ch;

   ev = event_info;
   b = data;
   if (ev->button != 3) return;

   m = e_menu_new();

   if (e_configure_registry_exists("applications/new_application"))
     {
        mi = e_menu_item_new(m);
        e_menu_item_label_set(mi, _("Create new Icon"));
        e_util_menu_item_theme_icon_set(mi, "document-new");
        e_menu_item_callback_set(mi, _ibar_cb_menu_icon_new, NULL);

        mi = e_menu_item_new(m);
        e_menu_item_separator_set(mi, 1);
     }

   if (e_configure_registry_exists("applications/ibar_applications"))
     {
        mi = e_menu_item_new(m);
        e_menu_item_label_set(mi, _("Contents"));
        e_util_menu_item_theme_icon_set(mi, "list-add");
        e_menu_item_callback_set(mi, _ibar_cb_menu_icon_add, b);
     }

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Settings"));
   e_util_menu_item_theme_icon_set(mi, "configure");
   e_menu_item_callback_set(mi, _ibar_cb_menu_configuration, b);

   m = e_gadcon_client_util_menu_items_append(b->inst->gcc, m, 0);

   e_gadcon_canvas_zone_geometry_get(b->inst->gcc->gadcon,
                                     &cx, &cy, &cw, &ch);
   e_menu_activate_mouse(m,
                         e_util_zone_current_get(e_manager_current_get()),
                         cx + ev->output.x, cy + ev->output.y, 1, 1,
                         E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
   evas_event_feed_mouse_up(b->inst->gcc->gadcon->evas, ev->button,
                            EVAS_BUTTON_NONE, ev->timestamp, NULL);
}

static void
_ibar_empty_handle(IBar *b)
{
   if (!b->icons)
     {
        if (!b->o_empty)
          {
             Evas_Coord w, h;

             b->o_empty = evas_object_rectangle_add(evas_object_evas_get(b->o_box));
             evas_object_event_callback_add(b->o_empty,
                                            EVAS_CALLBACK_MOUSE_DOWN,
                                            _ibar_cb_empty_mouse_down, b);
             evas_object_color_set(b->o_empty, 0, 0, 0, 0);
             evas_object_show(b->o_empty);
             e_box_pack_end(b->o_box, b->o_empty);
             evas_object_geometry_get(b->o_box, NULL, NULL, &w, &h);
             if (e_box_orientation_get(b->o_box))
               w = h;
             else
               h = w;
             e_box_pack_options_set(b->o_empty,
                                    1, 1, /* fill */
                                    1, 1, /* expand */
                                    0.5, 0.5, /* align */
                                    w, h, /* min */
                                    9999, 9999 /* max */
                                    );
          }
     }
   else if (b->o_empty)
     {
        evas_object_del(b->o_empty);
        b->o_empty = NULL;
     }
}

static void
_ibar_fill(IBar *b)
{
   IBar_Icon *ic;
   int w, h;

   if (b->io->eo)
     {
        Efreet_Desktop *desktop;
        const Eina_List *l;

        EINA_LIST_FOREACH(b->io->eo->desktops, l, desktop)
          {
             const Eina_List *ll;
             ic = _ibar_icon_new(b, desktop, 0);
             ll = e_exec_desktop_instances_find(desktop);
             if (ll)
               {
                  ic->exes = eina_list_clone(ll);
                  _ibar_icon_signal_emit(ic, "e,state,on", "e");
               }
          }
     }
   if (!b->inst->ci->dont_add_nonorder)
     {
        const Eina_Hash *execs = e_exec_instances_get();
        Eina_Iterator *it;
        const Eina_List *l, *ll;
        E_Exec_Instance *exe;

        it = eina_hash_iterator_data_new(execs);
        EINA_ITERATOR_FOREACH(it, l)
          {
             EINA_LIST_FOREACH(l, ll, exe)
               {
                  E_Border *bd;
                  Eina_List *lll;
                  Eina_Bool skip = EINA_TRUE;

                  if (!exe->desktop) continue;
                  EINA_LIST_FOREACH(exe->borders, lll, bd)
                    if (!bd->client.netwm.state.skip_taskbar)
                      {
                         skip = EINA_FALSE;
                         break;
                      }
                  if (skip) continue;
                  ic = eina_hash_find(b->icon_hash, _desktop_name_get(exe->desktop));
                  if (ic)
                    {
                       if (!eina_list_data_find(ic->exes, exe))
                         ic->exes = eina_list_append(ic->exes, exe);
                       continue;
                    }
                  _ibar_sep_create(b);
                  _ibar_icon_notinorder_new(b, exe);
               }
          }
        eina_iterator_free(it);
     }
   
   _ibar_empty_handle(b);
   _ibar_resize_handle(b);
   if (!b->inst->gcc) return;
   evas_object_size_hint_min_get(b->o_box, &w, &h);
   evas_object_size_hint_max_set(b->o_box, w, h);
}

static void
_ibar_empty(IBar *b)
{
   while (b->icons)
     _ibar_icon_free((IBar_Icon*)b->icons);

   E_FREE_FUNC(b->o_sep, evas_object_del);
   _ibar_empty_handle(b);
}

static void
_ibar_orient_set(IBar *b, int horizontal)
{
   e_box_orientation_set(b->o_box, horizontal);
   e_box_align_set(b->o_box, 0.5, 0.5);
   e_box_orientation_set(b->o_outerbox, horizontal);
   e_box_align_set(b->o_outerbox, 0.5, 0.5);
}

static void
_ibar_resize_handle(IBar *b)
{
   IBar_Icon *ic;
   Evas_Coord w, h;

   evas_object_geometry_get(b->o_box, NULL, NULL, &w, &h);
   if (b->inst->gcc)
     {
        if (b->inst->gcc->max.w)
          w = MIN(w, b->inst->gcc->max.w);
        if (b->inst->gcc->max.h)
          h = MIN(h, b->inst->gcc->max.h);
     }
   if (e_box_orientation_get(b->o_box))
     w = h;
   else
     h = w;
   e_box_freeze(b->o_outerbox);
   e_box_freeze(b->o_box);
   EINA_INLIST_FOREACH(b->icons, ic)
     {
        e_box_pack_options_set(ic->o_holder,
                               1, 1, /* fill */
                               0, 0, /* expand */
                               0.5, 0.5, /* align */
                               w, h, /* min */
                               w, h /* max */
                               );
     }
   if (b->o_sep)
     {
        if (_gc_vertical(b->inst))
          h = 16 * e_scale;
        else
          w = 16 * e_scale;
        e_box_pack_options_set(b->o_sep,
                               1, 1, /* fill */
                               0, 0, /* expand */
                               0.5, 0.5, /* align */
                               8, 8, /* min */
                               w, h /* max */
                               );
     }
   e_box_thaw(b->o_box);
   e_box_thaw(b->o_outerbox);
   e_box_size_min_get(b->o_box, &w, &h);
   e_box_pack_options_set(b->o_box,
                          1, 1, /* fill */
                          1, 1, /* expand */
                          0.5, 0.5, /* align */
                          w, h, /* min */
                          -1, -1 /* max */
                          );
   if (!b->inst->gcc) return;
   e_box_size_min_get(b->o_outerbox, &w, &h);
   e_gadcon_client_aspect_set(b->inst->gcc, w, h);
}

static void
_ibar_instance_drop_zone_recalc(Instance *inst)
{
   Evas_Coord x, y, w, h;

   e_gadcon_client_viewport_geometry_get(inst->gcc, &x, &y, &w, &h);
   e_drop_handler_geometry_set(inst->drop_handler, x, y, w, h);
}

static Config_Item *
_ibar_config_item_get(const char *id)
{
   Config_Item *ci;

   GADCON_CLIENT_CONFIG_GET(Config_Item, ibar_config->items, _gadcon_class, id);

   ci = E_NEW(Config_Item, 1);
   ci->id = eina_stringshare_add(id);
   ci->dir = eina_stringshare_add("default");
   ci->show_label = 1;
   ci->eap_label = 0;
   ci->lock_move = 0;
   ci->dont_add_nonorder = 0;
   ci->dont_track_launch = 0;
   ci->focus_flash = 1;
   ci->dont_icon_menu_mouseover = 0;
   ibar_config->items = eina_list_append(ibar_config->items, ci);
   return ci;
}

void
_ibar_config_update(Config_Item *ci)
{
   const Eina_List *l;
   Instance *inst;
   IBar_Icon *ic;

   EINA_LIST_FOREACH(ibar_config->instances, l, inst)
     {
        char buf[PATH_MAX];

        if (inst->ci != ci) continue;

        if (inst->ci->dir[0] != '/')
          e_user_dir_snprintf(buf, sizeof(buf), "applications/bar/%s/.order",
                              inst->ci->dir);
        else
          eina_strlcpy(buf, inst->ci->dir, sizeof(buf));
        _ibar_order_refresh(inst->ibar, buf);
        _ibar_resize_handle(inst->ibar);
        _gc_orient(inst->gcc, -1);
     }
   EINA_LIST_FOREACH(ibar_config->instances, l, inst)
     EINA_INLIST_FOREACH(inst->ibar->icons, ic)
          {
             switch (ci->eap_label)
               {
                case 0:
                  edje_object_part_text_set(ic->o_holder2, "e.text.label",
                                            ic->app->name);
                  break;

                case 1:
                  edje_object_part_text_set(ic->o_holder2, "e.text.label",
                                            ic->app->comment);
                  break;

                case 2:
                  edje_object_part_text_set(ic->o_holder2, "e.text.label",
                                            ic->app->generic_name);
                  break;
               }
          }
}

static void
_ibar_sep_create(IBar *b)
{
   if (b->o_sep) return;
   b->o_sep = edje_object_add(evas_object_evas_get(b->o_box));
   if (_gc_vertical(b->inst))
     e_theme_edje_object_set(b->o_sep, "base/theme/modules/ibar", "e/modules/ibar/separator/horizontal");
   else
     e_theme_edje_object_set(b->o_sep, "base/theme/modules/ibar", "e/modules/ibar/separator/default");
   evas_object_show(b->o_sep);
   e_box_pack_end(b->o_outerbox, b->o_sep);
}

static IBar_Icon *
_ibar_icon_at_coord(IBar *b, Evas_Coord x, Evas_Coord y)
{
   IBar_Icon *ic;

   EINA_INLIST_FOREACH(b->icons, ic)
     {
        Evas_Coord dx, dy, dw, dh;

        /* block drops in the non-order section */
        if (ic->not_in_order) continue;
        evas_object_geometry_get(ic->o_holder, &dx, &dy, &dw, &dh);
        if (E_INSIDE(x, y, dx, dy, dw, dh))
          return ic;
     }
   return NULL;
}

static IBar_Icon *
_ibar_icon_new(IBar *b, Efreet_Desktop *desktop, Eina_Bool notinorder)
{
   IBar_Icon *ic;

   ic = E_NEW(IBar_Icon, 1);
   ic->ibar = b;
   ic->app = desktop;
   efreet_desktop_ref(desktop);
   ic->o_holder = edje_object_add(evas_object_evas_get(b->o_box));
   e_theme_edje_object_set(ic->o_holder, "base/theme/modules/ibar",
                           "e/modules/ibar/icon");
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_IN,
                                  _ibar_cb_icon_mouse_in, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_OUT,
                                  _ibar_cb_icon_mouse_out, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_DOWN,
                                  _ibar_cb_icon_mouse_down, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_UP,
                                  _ibar_cb_icon_mouse_up, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_MOVE,
                                  _ibar_cb_icon_mouse_move, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_WHEEL,
                                  _ibar_cb_icon_wheel, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOVE,
                                  _ibar_cb_icon_move, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_RESIZE,
                                  _ibar_cb_icon_resize, ic);
   evas_object_show(ic->o_holder);

   ic->o_holder2 = edje_object_add(evas_object_evas_get(b->o_box));
   e_theme_edje_object_set(ic->o_holder2, "base/theme/modules/ibar",
                           "e/modules/ibar/icon_overlay");
   evas_object_layer_set(ic->o_holder2, 9999);
   evas_object_pass_events_set(ic->o_holder2, 1);
   evas_object_show(ic->o_holder2);

   _ibar_icon_fill(ic);
   b->icons = eina_inlist_append(b->icons, EINA_INLIST_GET(ic));
   if (eina_hash_find(b->icon_hash, _desktop_name_get(ic->app)))
     {
        char buf[PATH_MAX];

        ERR("Ibar - Unexpected: icon with same desktop path created twice");
        snprintf(buf, sizeof(buf), "%s::%1.20f",
                 _desktop_name_get(ic->app), ecore_time_get());
        ic->hashname = eina_stringshare_add(buf);
     }
   else ic->hashname = eina_stringshare_add(_desktop_name_get(ic->app));
   eina_hash_add(b->icon_hash, ic->hashname, ic);
   if (notinorder)
     {
        ic->not_in_order = 1;
        b->not_in_order_count++;
        e_box_pack_end(b->o_outerbox, ic->o_holder);
     }
   else
     e_box_pack_end(b->o_box, ic->o_holder);
   return ic;
}

static IBar_Icon *
_ibar_icon_notinorder_new(IBar *b, E_Exec_Instance *exe)
{
   IBar_Icon *ic;

   ic = _ibar_icon_new(b, exe->desktop, 1);
   ic->exes = eina_list_append(ic->exes, exe);
   _ibar_icon_signal_emit(ic, "e,state,on", "e");
   return ic;
}

static void
_ibar_cb_icon_menu_client_menu_del(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   IBar *b = data;

   evas_object_event_callback_del(obj, EVAS_CALLBACK_HIDE, _ibar_cb_icon_menu_client_menu_del);
   if (!b->menu_icon) return;
   if (b->menu_icon->hide_timer)
     ecore_timer_loop_reset(b->menu_icon->hide_timer);
   else
     b->menu_icon->hide_timer = ecore_timer_loop_add(0.5, _ibar_cb_out_hide_delay, b->menu_icon);
}

static void
_ibar_icon_free(IBar_Icon *ic)
{
   E_Exec_Instance *inst;

   if (ic->ibar->menu_icon == ic) ic->ibar->menu_icon = NULL;
   if (ic->ibar->ic_drop_before == ic) ic->ibar->ic_drop_before = NULL;
   if (ic->menu) e_object_data_set(E_OBJECT(ic->menu), NULL);
   E_FREE_FUNC(ic->menu, e_object_del);
   E_FREE_FUNC(ic->timer, ecore_timer_del);
   E_FREE_FUNC(ic->hide_timer, ecore_timer_del);
   E_FREE_FUNC(ic->show_timer, ecore_timer_del);

   ic->ibar->icons = eina_inlist_remove(ic->ibar->icons, EINA_INLIST_GET(ic));
   eina_hash_del_by_key(ic->ibar->icon_hash, ic->hashname);
   E_FREE_FUNC(ic->hashname, eina_stringshare_del);
   E_FREE_FUNC(ic->reset_timer, ecore_timer_del);
   if (ic->app) efreet_desktop_unref(ic->app);
   evas_object_event_callback_del_full(ic->o_holder, EVAS_CALLBACK_MOUSE_IN,
                                  _ibar_cb_icon_mouse_in, ic);
   evas_object_event_callback_del_full(ic->o_holder, EVAS_CALLBACK_MOUSE_OUT,
                                  _ibar_cb_icon_mouse_out, ic);
   evas_object_event_callback_del_full(ic->o_holder, EVAS_CALLBACK_MOUSE_DOWN,
                                  _ibar_cb_icon_mouse_down, ic);
   evas_object_event_callback_del_full(ic->o_holder, EVAS_CALLBACK_MOUSE_UP,
                                  _ibar_cb_icon_mouse_up, ic);
   evas_object_event_callback_del_full(ic->o_holder, EVAS_CALLBACK_MOUSE_MOVE,
                                  _ibar_cb_icon_mouse_move, ic);
   evas_object_event_callback_del_full(ic->o_holder, EVAS_CALLBACK_MOUSE_WHEEL,
                                  _ibar_cb_icon_wheel, ic);
   evas_object_event_callback_del_full(ic->o_holder, EVAS_CALLBACK_MOVE,
                                  _ibar_cb_icon_move, ic);
   evas_object_event_callback_del_full(ic->o_holder, EVAS_CALLBACK_RESIZE,
                                  _ibar_cb_icon_resize, ic);
   ic->ibar->not_in_order_count -= ic->not_in_order;
   if (ic->ibar->ic_drop_before == ic)
     ic->ibar->ic_drop_before = NULL;
   _ibar_icon_empty(ic);
    EINA_LIST_FREE(ic->exes, inst)
     {
        E_Border *bd;
        Eina_List *ll;

        if (!ic->not_in_order)
          e_exec_instance_watcher_del(inst, _ibar_instance_watch, ic);
        EINA_LIST_FOREACH(inst->borders, ll, bd)
          if (bd->border_menu)
            evas_object_event_callback_del(bd->border_menu->bg_object, EVAS_CALLBACK_HIDE, _ibar_cb_icon_menu_client_menu_del);
     }
   evas_object_del(ic->o_holder);
   evas_object_del(ic->o_holder2);
   if (ic->exe_inst)
     {
        e_exec_instance_watcher_del(ic->exe_inst, _ibar_instance_watch, ic);
        ic->exe_inst = NULL;
     }
   free(ic);
}

static void
_ibar_icon_fill(IBar_Icon *ic)
{
   if (ic->o_icon) evas_object_del(ic->o_icon);
   ic->o_icon = e_icon_add(evas_object_evas_get(ic->ibar->o_box));
   evas_object_name_set(ic->o_icon, "icon");
   e_icon_fdo_icon_set(ic->o_icon, ic->app->icon);
   edje_object_part_swallow(ic->o_holder, "e.swallow.content", ic->o_icon);
   evas_object_show(ic->o_icon);
   if (ic->o_icon2) evas_object_del(ic->o_icon2);
   ic->o_icon2 = e_icon_add(evas_object_evas_get(ic->ibar->o_box));
   e_icon_fdo_icon_set(ic->o_icon2, ic->app->icon);
   edje_object_part_swallow(ic->o_holder2, "e.swallow.content", ic->o_icon2);
   evas_object_show(ic->o_icon2);

   switch (ic->ibar->inst->ci->eap_label)
     {
      case 0: /* Eap Name */
        edje_object_part_text_set(ic->o_holder2, "e.text.label", ic->app->name);
        break;

      case 1: /* Eap Comment */
        edje_object_part_text_set(ic->o_holder2, "e.text.label", ic->app->comment);
        break;

      case 2: /* Eap Generic */
        edje_object_part_text_set(ic->o_holder2, "e.text.label", ic->app->generic_name);
        break;
     }
}

static void
_ibar_icon_empty(IBar_Icon *ic)
{
   if (ic->o_icon) evas_object_del(ic->o_icon);
   if (ic->o_icon2) evas_object_del(ic->o_icon2);
   ic->o_icon = NULL;
   ic->o_icon2 = NULL;
}

static void
_ibar_icon_signal_emit(IBar_Icon *ic, char *sig, char *src)
{
   if (ic->o_holder)
     edje_object_signal_emit(ic->o_holder, sig, src);
   if (ic->o_icon && e_icon_edje_get(ic->o_icon))
     edje_object_signal_emit(ic->o_icon, sig, src);
   if (ic->o_holder2)
     edje_object_signal_emit(ic->o_holder2, sig, src);
   if (ic->o_icon2 && e_icon_edje_get(ic->o_icon))
     edje_object_signal_emit(ic->o_icon2, sig, src);
}

static void
_ibar_cb_app_change(void *data, E_Order *eo __UNUSED__)
{
   IBar *b;
   IBar_Order *io = data;

   EINA_INLIST_FOREACH(io->bars, b)
     {
        _ibar_empty(b);
        if (b->inst)
          {
             _ibar_fill(b);
             if (b->inst->gcc) _gc_orient(b->inst->gcc, -1);
          }
     }
}

static void
_ibar_cb_resize_job(void *data)
{
   Instance *inst = data;
   _ibar_resize_handle(inst->ibar);
   _ibar_instance_drop_zone_recalc(inst);
   inst->ibar->resize_job = NULL;
}

static void
_ibar_cb_obj_moveresize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Instance *inst = data;

   if (inst->ibar->resize_job) return;
   inst->ibar->resize_job = ecore_job_add((Ecore_Cb)_ibar_cb_resize_job, inst);
}

static void
_ibar_cb_menu_icon_action_exec(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   Efreet_Desktop_Action *action = (Efreet_Desktop_Action*)data;
   e_exec(NULL, NULL, action->exec, NULL, "ibar");
}

static void
_ibar_cb_menu_icon_new(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Container *con;

   if (!e_configure_registry_exists("applications/new_application")) return;
   con = e_container_current_get(e_manager_current_get());
   e_configure_registry_call("applications/new_application", con, NULL);
}

static void
_ibar_cb_menu_icon_add(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   char path[PATH_MAX];
   IBar *b;

   b = data;
   e_user_dir_snprintf(path, sizeof(path), "applications/bar/%s/.order",
                       b->inst->ci->dir);
   e_configure_registry_call("internal/ibar_other",
                             e_container_current_get(e_manager_current_get()),
                             path);
}

static void
_ibar_cb_menu_icon_properties(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   IBar_Icon *ic;

   ic = data;
   e_desktop_edit(ic->ibar->inst->gcc->gadcon->zone->container, ic->app);
}

static void
_ibar_cb_menu_icon_stick(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   IBar_Icon *ic = data;

   e_order_append(ic->ibar->io->eo, ic->app);
   _ibar_icon_free(ic);
}

static void
_ibar_cb_menu_icon_remove(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   IBar_Icon *ic = data;
   IBar *i;

   i = ic->ibar;
   e_order_remove(i->io->eo, ic->app);
   _ibar_icon_free(ic);
   _ibar_resize_handle(i);
}

static void
_ibar_cb_menu_configuration(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   IBar *b;

   b = data;
   _config_ibar_module(b->inst->ci);
}

static void
_ibar_cb_icon_menu_hide_begin(IBar_Icon *ic)
{
   if (!ic->menu) return;
   evas_object_pass_events_set(ic->menu->o_bg, 1);
   edje_object_signal_emit(ic->menu->o_bg, "e,action,hide", "e");
}

static void
_ibar_cb_icon_menu_mouse_up(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info)
{
   IBar_Icon *ic;
   E_Border *bd = data;
   Evas_Event_Mouse_Up *ev = event_info;

   ic = evas_object_data_get(obj, "ibar_icon");
   if (!ic) return;
   if (ev->button == 3)
     {
        e_int_border_menu_show(bd, ev->canvas.x, ev->canvas.y, 0, ev->timestamp);
        evas_object_event_callback_add(bd->border_menu->bg_object, EVAS_CALLBACK_HIDE, _ibar_cb_icon_menu_client_menu_del, ic->ibar);
        return;
     }

   if (bd->iconic)
     {
        e_border_uniconify(bd);
        e_border_focus_set(bd, 1, 1);
     }
   else
     {
       if (bd->focused)
         {
            e_border_iconify(bd);
         }
       else
         {
            e_border_raise(bd);
            e_border_focus_set(bd, 1, 1);
         }
     }
   //~ e_border_activate(bd, 1);
   if (ic)
     _ibar_cb_icon_menu_hide_begin(ic);
}

static void
_ibar_cb_icon_menu_del(void *obj)
{
   IBar_Icon *ic = e_object_data_get(obj);

   if (!ic) return;
   ic->menu = NULL;
}

static void
_ibar_cb_icon_menu_autodel(void *data, void *pop __UNUSED__)
{
   _ibar_cb_icon_menu_hide_begin(data);
}

static void
_ibar_cb_icon_menu_shown(void *data, Evas_Object *obj __UNUSED__, const char *sig __UNUSED__, const char *src __UNUSED__)
{
   IBar_Icon *ic = data;

   evas_object_pass_events_set(ic->menu->o_bg, 0);
}

static void
_ibar_cb_icon_menu_hidden(void *data, Evas_Object *obj __UNUSED__, const char *sig __UNUSED__, const char *src __UNUSED__)
{
   IBar_Icon *ic = data;
   
   E_OBJECT_DEL_SET(ic->menu, NULL);
   E_FREE_FUNC(ic->menu, e_object_del);
   E_FREE_FUNC(ic->hide_timer, ecore_timer_del);
}

static void
_ibar_cb_icon_menu_img_del(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   int w, h;
   IBar_Icon *ic;

   ic = evas_object_data_del(obj, "ibar_icon");
   if (!ic) return; //menu is closing

   evas_object_data_del(obj, "ibar_icon");
   if (!ic->menu) return; //who knows
   edje_object_part_box_remove(ic->menu->o_bg, "e.box", data);

   evas_object_del(data);
   if (eina_list_count(ic->exes) <= 1)
     {
        E_Exec_Instance *inst = eina_list_data_get(ic->exes);
        
        if ((!inst) || (!inst->borders))
          {
            _ibar_cb_icon_menu_hide_begin(ic); 
            return;
          }
     }
   edje_object_calc_force(ic->menu->o_bg);
   edje_object_size_min_calc(ic->menu->o_bg, &w, &h);
   evas_object_size_hint_min_set(ic->menu->o_bg, w, h);
   if (e_box_orientation_get(ic->ibar->o_box))
    {
        int cx, cy, cw, ch, ny;
        E_Zone *zone;

        evas_object_geometry_get(ic->menu->o_bg, &cx, &cy, &cw, &ch);
        zone = e_gadcon_zone_get(ic->ibar->inst->gcc->gadcon);
        if (cy > (zone->h / 2))
          ny = cy - (h - ch);
        else
          ny = cy;
        evas_object_geometry_set(ic->menu->o_bg, cx, ny, w, h);
     }
   else
      evas_object_resize(ic->menu->o_bg, w, h);
}

static void
_ibar_icon_menu_mouse_in(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   IBar_Icon *ic = data;
   E_FREE_FUNC(ic->hide_timer, ecore_timer_del);
}

static void
_ibar_icon_menu_mouse_out(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   IBar_Icon *ic = data;

   if (e_menu_grab_window_get()) return;
   if (ic->hide_timer)
     ecore_timer_loop_reset(ic->hide_timer);
   else
     ic->hide_timer = ecore_timer_loop_add(0.5, _ibar_cb_out_hide_delay, ic);
}

static void
_ibar_icon_menu(IBar_Icon *ic, Eina_Bool grab)
{
   Evas_Object *o, *it;
   Eina_List *l;
   E_Exec_Instance *exe;
   Evas *e;
   int w, h;
   E_Zone *zone;

   if (!ic->exes) return; //FIXME
   ic->menu = e_gadcon_popup_new(ic->ibar->inst->gcc);
   e_popup_name_set(ic->menu->win, "noshadow-ibarmenu");
   e_object_data_set(E_OBJECT(ic->menu), ic);
   E_OBJECT_DEL_SET(ic->menu, _ibar_cb_icon_menu_del);
   
   zone = ic->ibar->inst->gcc->gadcon->zone;
   e = ic->menu->win->evas;
   o = edje_object_add(e);
   e_theme_edje_object_set(o, "base/theme/modules/ibar",
                           "e/modules/ibar/menu");
   EINA_LIST_FOREACH(ic->exes, l, exe)
     {
        Evas_Object *img;
        const char *txt;
        Eina_List *ll;
        E_Border *bd;

        EINA_LIST_FOREACH(exe->borders, ll, bd)
          {
             if (bd->client.netwm.state.skip_taskbar) continue;
             it = edje_object_add(e);
             
             e_theme_edje_object_set(it, "base/theme/modules/ibar",
                                     "e/modules/ibar/menu/item");
             e_popup_object_add(ic->menu->win, it);
             img = e_border_icon_add(bd, evas_object_evas_get(it));
             evas_object_data_set(img, "ibar_icon", ic);

             evas_object_event_callback_add(img, EVAS_CALLBACK_DEL,
                                            _ibar_cb_icon_menu_img_del, it);
             txt = e_border_name_get(bd);
             w = h = 48;
             evas_object_show(img);
             e_popup_object_add(ic->menu->win, img);
             evas_object_size_hint_aspect_set(img, EVAS_ASPECT_CONTROL_BOTH, w, h);
             
             edje_object_part_swallow(it, "e.swallow.icon", img);
             edje_object_part_text_set(it, "e.text.title", txt);
             if (bd->focused)
               edje_object_signal_emit(it, "e,state,focused", "e");
             if (bd->sticky)
               {
                 if (bd->zone != ic->ibar->inst->gcc->gadcon->zone)
                   edje_object_signal_emit(it, "e,state,otherscreen", "e");
               }
             else
               {
                 if (bd->zone != ic->ibar->inst->gcc->gadcon->zone)
                   edje_object_signal_emit(it, "e,state,otherscreen", "e");
                 else if (bd->desk != e_desk_current_get(ic->ibar->inst->gcc->gadcon->zone))
                   edje_object_signal_emit(it, "e,state,otherdesk", "e");
               }
             edje_object_calc_force(it);
             edje_object_size_min_calc(it, &w, &h);
             evas_object_size_hint_min_set(it, w, h);
             evas_object_show(it);
             evas_object_event_callback_add(it, EVAS_CALLBACK_MOUSE_UP,
               _ibar_cb_icon_menu_mouse_up, bd);
             evas_object_data_set(it, "ibar_icon", ic);
             edje_object_part_box_append(o, "e.box", it);
          }
     }
   if (!ic->menu->win->objects)
     {
        /* something crazy happened */
        evas_object_del(o);
        e_object_del(E_OBJECT(ic->menu));
        return;
     }
   if (!grab)
     {
        evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN, _ibar_icon_menu_mouse_in, ic);
        evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_OUT, _ibar_icon_menu_mouse_out, ic);
     }
   edje_object_calc_force(o);
   edje_object_size_min_calc(o, &w, &h);
   evas_object_size_hint_min_set(o, w, h);
   /* gadcon popups don't really prevent this,
    * so away we go!
    */
   evas_object_del(ic->menu->o_bg);
   ic->menu->o_bg = o;
   ic->menu->w = w, ic->menu->h = h;
   edje_object_signal_callback_add(o, "e,action,show,done", "*",
                                   _ibar_cb_icon_menu_shown, ic);
   edje_object_signal_callback_add(o, "e,action,hide,done", "*",
                                   _ibar_cb_icon_menu_hidden, ic);
   e_popup_resize(ic->menu->win, w, h);
   evas_object_resize(o, w, h);
   evas_object_show(o);
   edje_object_signal_emit(o, "e,state,hidden", "e");
   edje_object_message_signal_process(o);
   e_gadcon_popup_show(ic->menu);
   ic->ibar->menu_icon = ic;
   
   {
      Evas_Coord x, y, iw, ih, ox, oy;
      evas_object_geometry_get(ic->o_holder, &x, &y, &iw, &ih);
      x -= ic->menu->win->zone->x;
      y -= ic->menu->win->zone->y;
      ox = ic->menu->win->x, oy = ic->menu->win->y;
      if (e_box_orientation_get(ic->ibar->o_box))
        ox = (x + (iw / 2)) - (w / 2);
      else
        oy = (y + (ih / 2)) - (h / 2);
     
      ox = E_CLAMP(ox, zone->x, zone->x + zone->w - w);
      e_popup_move(ic->menu->win, ox, oy);
   }
   evas_object_pass_events_set(o, 1);
   edje_object_signal_emit(o, "e,action,show", "e");
   ic->menu_grabbed = grab;
   if (grab)
     {
       _ibar_cb_icon_menu_autodel(ic, NULL);
       e_popup_hide(ic->menu->win);
     }
}

static void
_ibar_icon_menu_show(IBar_Icon *ic, Eina_Bool grab)
{
   if (ic->ibar->menu_icon && (ic->ibar->menu_icon != ic))
     _ibar_icon_menu_hide(ic->ibar->menu_icon, ic->ibar->menu_icon->menu_grabbed);
   if (ic->menu)
     {
       if (ic->ibar->menu_icon != ic)
          {
             edje_object_signal_emit(ic->menu->o_bg, "e,action,show", "e");
             ic->ibar->menu_icon = ic;
          }
        return;
     }
   ic->drag.start = 0;
   ic->drag.dnd = 0;
   ic->mouse_down = 0;
   _ibar_icon_menu(ic, grab);
}

static void
_ibar_icon_menu_hide(IBar_Icon *ic, Eina_Bool grab)
{
   if (!ic->menu) return;
   if (ic->menu_grabbed != grab) return;
   if (ic->ibar && (ic->ibar->menu_icon == ic))
     ic->ibar->menu_icon = NULL;
   E_FREE_FUNC(ic->hide_timer, ecore_timer_del);
   ic->menu_grabbed = EINA_FALSE;
   _ibar_cb_icon_menu_hide_begin(ic);
}

static Eina_Bool
_ibar_icon_mouse_in_timer(void *data)
{
   IBar_Icon *ic = data;

   ic->show_timer = NULL;
   _ibar_icon_menu_show(ic, EINA_FALSE);
   return EINA_FALSE;
}

static void
_ibar_cb_icon_mouse_in(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
  IBar_Icon *ic;

   ic = data;
   E_FREE_FUNC(ic->reset_timer, ecore_timer_del);
   ic->focused = EINA_TRUE;
   if (ic->ibar->inst->ci->focus_flash)
      _ibar_icon_signal_emit(ic, "e,state,focused", "e");
   if (ic->ibar->inst->ci->show_label)
     _ibar_icon_signal_emit(ic, "e,action,show,label", "e");
   E_FREE_FUNC(ic->hide_timer, ecore_timer_del);
   if (!ic->ibar->inst->ci->dont_icon_menu_mouseover)
     {
        E_FREE_FUNC(ic->show_timer, ecore_timer_del);
        ic->show_timer = ecore_timer_loop_add(0.2, _ibar_icon_mouse_in_timer, ic);
     }
}

static Eina_Bool
_ibar_cb_out_hide_delay(void *data)
{
   IBar_Icon *ic = data;

   ic->hide_timer = NULL;
   _ibar_icon_menu_hide(ic, EINA_FALSE);
   return EINA_FALSE;
}

static void
_ibar_cb_icon_mouse_out(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   IBar_Icon *ic;

   ic = data;
   E_FREE_FUNC(ic->reset_timer, ecore_timer_del);
   E_FREE_FUNC(ic->show_timer, ecore_timer_del);
   ic->focused = EINA_FALSE;
   _ibar_icon_signal_emit(ic, "e,state,unfocused", "e");
   if (ic->ibar->inst->ci->show_label)
     _ibar_icon_signal_emit(ic, "e,action,hide,label", "e");
   if (!ic->ibar->inst->ci->dont_icon_menu_mouseover)
     {
        if (ic->hide_timer)
          ecore_timer_loop_reset(ic->hide_timer);
        else
          ic->hide_timer = ecore_timer_loop_add(0.75, _ibar_cb_out_hide_delay, ic);
     }
}

static Eina_Bool
_ibar_cb_icon_menu_cb(void *data)
{
   IBar_Icon *ic = data;

   ic->timer = NULL;
   _ibar_icon_menu_show(ic, EINA_TRUE);
   return EINA_FALSE;
}

static void
_ibar_cb_icon_menu_cb_void(void *data)
{
   _ibar_cb_icon_menu_cb(data);
}

static void
_ibar_cb_icon_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   IBar_Icon *ic;

   ev = event_info;
   ic = data;
   if (ev->button == 1)
     {
        ic->drag.x = ev->output.x;
        ic->drag.y = ev->output.y;
        ic->drag.start = 1;
        ic->drag.dnd = 0;
        ic->mouse_down = 1;
        if (!ic->timer)
          ic->timer = ecore_timer_loop_add(0.35, _ibar_cb_icon_menu_cb, ic);
     }
   else if (ev->button == 2)
     {
        E_FREE_FUNC(ic->show_timer, ecore_timer_del);
        E_FREE_FUNC(ic->hide_timer, ecore_timer_del);
        E_FREE_FUNC(ic->timer, ecore_timer_del);
        _ibar_icon_menu_show(ic, EINA_TRUE);
     }
   else if (ev->button == 3)
     {
        Eina_List *it; 
        E_Menu *m, *mo;
        E_Menu_Item *mi;
        Efreet_Desktop_Action *action;
        char buf[256];
        int cx, cy;
        
        E_FREE_FUNC(ic->show_timer, ecore_timer_del);
        E_FREE_FUNC(ic->hide_timer, ecore_timer_del);
        E_FREE_FUNC(ic->timer, ecore_timer_del);
        
        if (ic->menu)
          _ibar_icon_menu_hide(ic, ic->menu_grabbed);
        m = e_menu_new();

        /* FIXME: other icon options go here too */
        mo = e_menu_new();

        if (e_configure_registry_exists("applications/new_application"))
          {
             mi = e_menu_item_new(m);
             e_menu_item_label_set(mi, _("Create new Icon"));
             e_util_menu_item_theme_icon_set(mi, "document-new");
             e_menu_item_callback_set(mi, _ibar_cb_menu_icon_new, NULL);

             mi = e_menu_item_new(m);
             e_menu_item_separator_set(mi, 1);
          }

        if (e_configure_registry_exists("applications/ibar_applications"))
          {
             mi = e_menu_item_new(m);
             e_menu_item_label_set(mi, _("Contents"));
             e_util_menu_item_theme_icon_set(mi, "list-add");
             e_menu_item_callback_set(mi, _ibar_cb_menu_icon_add, ic->ibar);
          }

        mi = e_menu_item_new(m);
        e_menu_item_label_set(mi, _("Settings"));
        e_util_menu_item_theme_icon_set(mi, "configure");
        e_menu_item_callback_set(mi, _ibar_cb_menu_configuration, ic->ibar);

        m = e_gadcon_client_util_menu_items_append(ic->ibar->inst->gcc, m, 0);

        mi = e_menu_item_new(mo);
        e_menu_item_label_set(mi, _("Properties"));
        e_util_menu_item_theme_icon_set(mi, "configure");
        e_menu_item_callback_set(mi, _ibar_cb_menu_icon_properties, ic);

        mi = e_menu_item_new(mo);
        if (ic->not_in_order)
          {
             e_menu_item_label_set(mi, _("Add to bar"));
             e_util_menu_item_theme_icon_set(mi, "list-add");
             e_menu_item_callback_set(mi, _ibar_cb_menu_icon_stick, ic);
          }
        else
          {
             e_menu_item_label_set(mi, _("Remove from bar"));
             e_util_menu_item_theme_icon_set(mi, "list-remove");
             e_menu_item_callback_set(mi, _ibar_cb_menu_icon_remove, ic);
          }

        mi = e_menu_item_new_relative(m, NULL);
        snprintf(buf, sizeof(buf), _("Icon %s"), ic->app->name);
        e_menu_item_label_set(mi, buf);
        e_util_desktop_menu_item_icon_add(ic->app,
                                          e_util_icon_size_normalize(24 * e_scale),
                                          mi);
        e_menu_item_submenu_set(mi, mo);
        e_object_unref(E_OBJECT(mo));
        
        if (ic->app->actions)
          {
             mi = NULL;
             EINA_LIST_FOREACH(ic->app->actions, it, action)
               {
                  mi = e_menu_item_new_relative(m, mi);
                  e_menu_item_label_set(mi, action->name);
                  e_util_menu_item_theme_icon_set(mi, action->icon);
                  e_menu_item_callback_set(mi, _ibar_cb_menu_icon_action_exec, action);
               }
             mi = e_menu_item_new_relative(m, mi);
             e_menu_item_separator_set(mi, 1);
          }
        e_gadcon_client_menu_set(ic->ibar->inst->gcc, m);

        e_gadcon_canvas_zone_geometry_get(ic->ibar->inst->gcc->gadcon,
                                          &cx, &cy, NULL, NULL);
        e_menu_activate_mouse(m,
                              e_util_zone_current_get(e_manager_current_get()),
                              cx + ev->output.x, cy + ev->output.y, 1, 1,
                              E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
     }
}

static Eina_Bool
_ibar_cb_icon_reset(void *data)
{
   IBar_Icon *ic = data;
   
   if (ic->focused)
     {
        if (ic->ibar->inst->ci->focus_flash)
          _ibar_icon_signal_emit(ic, "e,state,focused", "e");
        if (ic->ibar->inst->ci->show_label)
          _ibar_icon_signal_emit(ic, "e,action,show,label", "e");
     }
   ic->reset_timer = NULL;
   return EINA_FALSE;
}

static void
_ibar_cb_icon_wheel(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Wheel *ev = event_info;
   E_Exec_Instance *exe;
   IBar_Icon *ic = data;
   E_Border *cur, *sel = NULL;
   Eina_List *l, *exe_current = NULL;

   if (!ic->exes) return;

   cur = e_border_focused_get();
   //~ cur = e_border_stack_bottom_get(cur);
   if (cur && cur->exe_inst)
     {
        EINA_LIST_FOREACH(ic->exes, l, exe)
          if (cur->exe_inst == exe)
            {
               exe_current = l;
               break;
            }
     }
   if (!exe_current)
     exe_current = ic->exes;

   exe = eina_list_data_get(exe_current);
   if (ev->z < 0)
     {
        if (cur && (cur->exe_inst == exe))
          {
             l = eina_list_data_find_list(exe->borders, cur);
             if (l) sel = eina_list_data_get(eina_list_next(l));
          }
        if (!sel)
          {
             exe_current = eina_list_next(exe_current);
             if (!exe_current)
               exe_current = ic->exes;
          }
     }
   else if (ev->z > 0)
     {
        if (cur && (cur->exe_inst == exe))
          {
             l = eina_list_data_find_list(exe->borders, cur);
             if (l) sel = eina_list_data_get(eina_list_prev(l));
          }
        if (!sel)
          {
             exe_current = eina_list_prev(exe_current);
             if (!exe_current)
               exe_current = eina_list_last(ic->exes);
          }
     }

   if (!sel)
     {
        exe = eina_list_data_get(exe_current);
        sel = eina_list_data_get(exe->borders);
        if (sel == cur)
          sel = eina_list_data_get(eina_list_next(exe->borders));
     }

   if (sel)
     e_border_activate(sel, 1);
}

static void
_ibar_instance_watch(void *data, E_Exec_Instance *inst, E_Exec_Watch_Type type)
{
   IBar_Icon *ic = data;

   switch (type)
     {
      case E_EXEC_WATCH_STARTED:
        if (ic->starting) _ibar_icon_signal_emit(ic, "e,state,started", "e");
        ic->starting = EINA_FALSE;
        if (!ic->exes) _ibar_icon_signal_emit(ic, "e,state,on", "e");
        if (ic->exe_inst == inst) ic->exe_inst = NULL;
        if (!eina_list_data_find(ic->exes, inst))
          ic->exes = eina_list_append(ic->exes, inst);
        break;
      default:
        break;
     }
}

static void
_ibar_icon_go(IBar_Icon *ic, Eina_Bool keep_going)
{  
   if (ic->not_in_order)
     {
        Eina_List *l, *ll;
        E_Exec_Instance *exe;
        E_Border *bd, *bdlast = NULL;
        unsigned int count = 0; 
       
        EINA_LIST_FOREACH(ic->exes, l, exe)
          {
             EINA_LIST_FOREACH(exe->borders, ll, bd)
               {
                  count++;
                  if (count > 1)
                    {
                       ecore_job_add((Ecore_Cb)_ibar_cb_icon_menu_cb_void, ic); 
                       return;
                    }
                  bdlast = bd;
               }
          }
        if (bdlast)
          e_border_activate(bdlast, 1);
        return;
     }
   if (ic->app->type == EFREET_DESKTOP_TYPE_APPLICATION)
     {
        if (ic->ibar->inst->ci->dont_track_launch)
          e_exec(ic->ibar->inst->gcc->gadcon->zone,
                 ic->app, NULL, NULL, "ibar");
        else
          {
             E_Exec_Instance *einst;
             
             if (ic->exe_inst) return;
             einst = e_exec(ic->ibar->inst->gcc->gadcon->zone,
                            ic->app, NULL, NULL, "ibar");
             
             if (einst)
               {
                  ic->exe_inst = einst;
                  e_exec_instance_watcher_add(einst, _ibar_instance_watch, ic);
                  if (!ic->starting) _ibar_icon_signal_emit(ic, "e,state,starting", "e");
                  ic->starting = EINA_TRUE;
               }
          }
     }
   else if (ic->app->type == EFREET_DESKTOP_TYPE_LINK)
     {
        if (!strncasecmp(ic->app->url, "file:", 5))
          {
             E_Action *act;
             
             act = e_action_find("fileman");
             if (act) act->func.go(NULL, ic->app->url + 5);
          }
     }
   _ibar_icon_signal_emit(ic, "e,action,exec", "e");
   if (keep_going)
     ic->reset_timer = ecore_timer_add(1.0, _ibar_cb_icon_reset, ic);
}

static void
_ibar_cb_icon_mouse_up(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   IBar_Icon *ic;

   ev = event_info;
   ic = data;

   if ((ev->button == 1) && (ic->mouse_down == 1))
     {
        if (!ic->drag.dnd) _ibar_icon_go(ic, EINA_FALSE);
        ic->drag.start = 0;
        ic->drag.dnd = 0;
        ic->mouse_down = 0;
        E_FREE_FUNC(ic->timer, ecore_timer_del);
     }
}

static void
_ibar_cb_icon_mouse_move(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   IBar_Icon *ic = data;
   int dx, dy;

   E_FREE_FUNC(ic->timer, ecore_timer_del);
   if (!ic->drag.start) return;

   dx = ev->cur.output.x - ic->drag.x;
   dy = ev->cur.output.y - ic->drag.y;
   if (((dx * dx) + (dy * dy)) >
       (e_config->drag_resist * e_config->drag_resist))
     {
        E_Drag *d;
        Evas_Object *o;
        Evas_Coord x, y, w, h;
        unsigned int size;
        IBar *i;
        const char *drag_types[] = { "enlightenment/desktop" };

        ic->drag.dnd = 1;
        ic->drag.start = 0;

        if (ic->ibar->inst->ci->lock_move) return;

        evas_object_geometry_get(ic->o_icon, &x, &y, &w, &h);
        d = e_drag_new(ic->ibar->inst->gcc->gadcon->zone->container,
                       x, y, drag_types, 1,
                       ic->app, -1, NULL, _ibar_cb_drag_finished);
        efreet_desktop_ref(ic->app);
        size = MAX(w, h);
        o = e_util_desktop_icon_add(ic->app, size, e_drag_evas_get(d));
        e_drag_object_set(d, o);

        e_drag_resize(d, w, h);
        e_drag_start(d, ic->drag.x, ic->drag.y);
        i = ic->ibar;
        e_object_data_set(E_OBJECT(d), i);
        if (!ic->not_in_order)
          e_order_remove(i->io->eo, ic->app);
        _ibar_icon_free(ic);
        _ibar_resize_handle(i);
        _gc_orient(i->inst->gcc, -1);
     }
}

static void
_ibar_cb_icon_move(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   IBar_Icon *ic;
   Evas_Coord x, y;

   ic = data;
   evas_object_geometry_get(ic->o_holder, &x, &y, NULL, NULL);
   evas_object_move(ic->o_holder2, x, y);
   evas_object_raise(ic->o_holder2);
}

static void
_ibar_cb_icon_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   IBar_Icon *ic;
   Evas_Coord w, h;

   ic = data;
   evas_object_geometry_get(ic->o_holder, NULL, NULL, &w, &h);
   evas_object_resize(ic->o_holder2, w, h);
   evas_object_raise(ic->o_holder2);
}

static void
_ibar_cb_drop_move(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   IBar *b;
   Evas_Coord x, y;

   b = data;
   evas_object_geometry_get(b->o_drop, &x, &y, NULL, NULL);
   evas_object_move(b->o_drop_over, x, y);
}

static void
_ibar_cb_drop_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   IBar *b;
   Evas_Coord w, h;

   b = data;
   evas_object_geometry_get(b->o_drop, NULL, NULL, &w, &h);
   evas_object_resize(b->o_drop_over, w, h);
}

static void
_ibar_cb_drag_finished(E_Drag *drag, int dropped)
{
   IBar *i = e_object_data_get(E_OBJECT(drag));

   efreet_desktop_unref(drag->data);
   if (!i) return;
   if (!dropped)
     {
        _ibar_empty(i);
        _ibar_fill(i);
        _ibar_resize_handle(i);
     }
}

static void
_ibar_inst_cb_scroll(void *data)
{
   Instance *inst;

   /* Update the position of the dnd to handle for autoscrolling
    * gadgets. */
   inst = data;
   _ibar_drop_position_update(inst, inst->ibar->dnd_x, inst->ibar->dnd_y);
}

static void
_ibar_drop_position_update(Instance *inst, Evas_Coord x, Evas_Coord y)
{
   IBar_Icon *ic;

   inst->ibar->dnd_x = x;
   inst->ibar->dnd_y = y;

   if (inst->ibar->o_drop) e_box_unpack(inst->ibar->o_drop);
   ic = _ibar_icon_at_coord(inst->ibar, x, y);

   inst->ibar->ic_drop_before = ic;
   if (ic)
     {
        Evas_Coord ix, iy, iw, ih;
        int before = 0;

        evas_object_geometry_get(ic->o_holder, &ix, &iy, &iw, &ih);
        if (e_box_orientation_get(inst->ibar->o_box))
          {
             if (x < (ix + (iw / 2))) before = 1;
          }
        else
          {
             if (y < (iy + (ih / 2))) before = 1;
          }
        if (before)
          e_box_pack_before(inst->ibar->o_box, inst->ibar->o_drop, ic->o_holder);
        else
          e_box_pack_after(inst->ibar->o_box, inst->ibar->o_drop, ic->o_holder);
        inst->ibar->drop_before = before;
     }
   else e_box_pack_end(inst->ibar->o_box, inst->ibar->o_drop);
   e_box_pack_options_set(inst->ibar->o_drop,
                          1, 1, /* fill */
                          1, 1, /* expand */
                          0.5, 0.5, /* align */
                          1, 1, /* min */
                          -1, -1 /* max */
                          );
   _ibar_resize_handle(inst->ibar);
   _gc_orient(inst->gcc, -1);
}

static void
_ibar_inst_cb_enter(void *data, const char *type __UNUSED__, void *event_info)
{
   E_Event_Dnd_Enter *ev;
   Instance *inst;
   Evas_Object *o, *o2;

   ev = event_info;
   inst = data;
   o = edje_object_add(evas_object_evas_get(inst->ibar->o_box));
   inst->ibar->o_drop = o;
   o2 = edje_object_add(evas_object_evas_get(inst->ibar->o_box));
   inst->ibar->o_drop_over = o2;
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOVE,
                                  _ibar_cb_drop_move, inst->ibar);
   evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE,
                                  _ibar_cb_drop_resize, inst->ibar);
   e_theme_edje_object_set(o, "base/theme/modules/ibar",
                           "e/modules/ibar/drop");
   e_theme_edje_object_set(o2, "base/theme/modules/ibar",
                           "e/modules/ibar/drop_overlay");
   evas_object_layer_set(o2, 19999);
   evas_object_show(o);
   evas_object_show(o2);

   _ibar_drop_position_update(inst, ev->x, ev->y);
   e_gadcon_client_autoscroll_cb_set(inst->gcc, _ibar_inst_cb_scroll, inst);
   e_gadcon_client_autoscroll_update(inst->gcc, ev->x, ev->y);
}

static void
_ibar_inst_cb_move(void *data, const char *type __UNUSED__, void *event_info)
{
   E_Event_Dnd_Move *ev;
   Instance *inst;
   int x,y;

   ev = event_info;
   inst = data;
   _ibar_drop_position_update(inst, ev->x, ev->y);
   //~ e_gadcon_client_autoscroll_update(inst->gcc, ev->x, ev->y);
   evas_object_geometry_get(inst->ibar->o_outerbox, &x, &y, NULL, NULL);
   e_gadcon_client_autoscroll_update(inst->gcc, ev->x - x, ev->y - y);
}

static void
_ibar_inst_cb_leave(void *data, const char *type __UNUSED__, void *event_info __UNUSED__)
{
   Instance *inst;

   inst = data;
   inst->ibar->ic_drop_before = NULL;
   evas_object_del(inst->ibar->o_drop);
   inst->ibar->o_drop = NULL;
   evas_object_del(inst->ibar->o_drop_over);
   inst->ibar->o_drop_over = NULL;
   _ibar_resize_handle(inst->ibar);
   e_gadcon_client_autoscroll_cb_set(inst->gcc, NULL, NULL);
   _gc_orient(inst->gcc, -1);
}

static void
_ibar_inst_cb_drop(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Drop *ev;
   Instance *inst;
   Efreet_Desktop *app = NULL;
   Eina_List *fl = NULL;
   IBar_Icon *ic;

   ev = event_info;
   inst = data;

   if (!strcmp(type, "enlightenment/desktop"))
     app = ev->data;
   else if (!strcmp(type, "enlightenment/border"))
     {
        E_Border *bd;

        bd = ev->data;
        app = bd->desktop;
        if (!app)
          {
             app = e_desktop_border_create(bd);
             efreet_desktop_save(app);
             e_desktop_edit(e_container_current_get(e_manager_current_get()),
                            app);
          }
     }
   else if (!strcmp(type, "text/uri-list"))
     fl = ev->data;

   ic = inst->ibar->ic_drop_before;
   if (ic)
     {
        /* Add new eapp before this icon */
        if (!inst->ibar->drop_before)
          {
             IBar_Icon *ic2;

             EINA_INLIST_FOREACH(inst->ibar->icons, ic2)
               {
                  if (ic2 == ic)
                    {
                       if (EINA_INLIST_GET(ic2)->next)
                         ic = (IBar_Icon*)EINA_INLIST_GET(ic2)->next;
                       else
                         ic = NULL;
                       break;
                    }
               }
          }
        if (!ic) goto atend;
        if (app)
          e_order_prepend_relative(ic->ibar->io->eo, app, ic->app);
        else if (fl)
          e_order_files_prepend_relative(ic->ibar->io->eo, fl, ic->app);
     }
   else
     {
atend:
        if (inst->ibar->io->eo)
          {
             if (app)
               e_order_append(inst->ibar->io->eo, app);
             else if (fl)
               e_order_files_append(inst->ibar->io->eo, fl);
          }
     }
   evas_object_del(inst->ibar->o_drop);
   inst->ibar->o_drop = NULL;
   evas_object_del(inst->ibar->o_drop_over);
   inst->ibar->o_drop_over = NULL;
   e_gadcon_client_autoscroll_cb_set(inst->gcc, NULL, NULL);
   _ibar_empty_handle(inst->ibar);
   _ibar_resize_handle(inst->ibar);
   _gc_orient(inst->gcc, -1);
}

static E_Action *act_ibar_focus = NULL;
static Ecore_X_Window _ibar_focus_win = 0;
static Ecore_Event_Handler *_ibar_key_down_handler = NULL;

static void _ibar_go_unfocus(void);

static IBar *
_ibar_manager_find(E_Manager *man)
{
   E_Zone *z = e_util_zone_current_get(man);
   IBar *b;
   Eina_List *l;
   
   if (!z) return NULL;
   // find iubar on current zone
   EINA_LIST_FOREACH(ibars, l, b)
     {
        if ((!b->inst) || (!b->inst->gcc) || (!b->inst->gcc->gadcon)) continue;
        if (b->inst->gcc->gadcon->zone == z) return b;
     }
   // no ibars on current zone - return any old ibar
   EINA_LIST_FOREACH(ibars, l, b)
     {
        return b;
     }
   // no ibars. null.
   return NULL;
}

static void
_ibar_icon_unfocus_focus(IBar_Icon *ic1, IBar_Icon *ic2)
{
   if (ic1)
     {
        ic1->focused = EINA_FALSE;
        _ibar_icon_signal_emit(ic1, "e,state,unfocused", "e");
        if (ic1->ibar->inst->ci->show_label)
          _ibar_icon_signal_emit(ic1, "e,action,hide,label", "e");
     }
   if (ic2)
     {
        ic2->focused = EINA_TRUE;
        if (ic2->ibar->inst->ci->focus_flash)
          _ibar_icon_signal_emit(ic2, "e,state,focused", "e");
        if (ic2->ibar->inst->ci->show_label)
          _ibar_icon_signal_emit(ic2, "e,action,show,label", "e");
     }
}

static IBar *
_ibar_focused_find(void)
{
   IBar *b;
   Eina_List *l;
   
   EINA_LIST_FOREACH(ibars, l, b)
     {
        if (b->focused) return b;
     }
   return NULL;
}

static int
_ibar_cb_sort(IBar *b1, IBar *b2)
{
   E_Zone *z1 = NULL, *z2 = NULL;
   
   if ((b1) && (b1->inst) && (b1->inst->gcc) && (b1->inst->gcc->gadcon))
     z1 = b1->inst->gcc->gadcon->zone;
   if ((b2) && (b2->inst) && (b2->inst->gcc) && (b2->inst->gcc->gadcon))
     z2 = b2->inst->gcc->gadcon->zone;
   if ((z1) && (!z2)) return -1;
   else if ((!z1) && (z2)) return 1;
   else if ((!z1) && (!z2)) return 0;
   else
     {
        int id1, id2;
        
        id1 = z1->id + (z1->container->num * 100) + (z1->container->manager->num * 10000);
        id2 = z2->id + (z2->container->num * 100) + (z2->container->manager->num * 10000);
        return id2 - id1;
     }
   return 0;
}

static IBar *
_ibar_focused_next_find(void)
{
   IBar *b, *bn = NULL;
   Eina_List *l;
   Eina_List *tmpl = NULL;
  
   EINA_LIST_FOREACH(ibars, l, b)
     {
        if (!b->icons) continue;
        tmpl = eina_list_sorted_insert
          (tmpl, EINA_COMPARE_CB(_ibar_cb_sort), b);
     }
   if (!tmpl) tmpl = ibars;
   EINA_LIST_FOREACH(tmpl, l, b)
     {
        if (b->focused)
          {
             if (l->next)
               {
                  bn = l->next->data;
                  break;
               }
             else
               {
                  bn = tmpl->data;
                  break;
               }
          }
     }
   if (tmpl != ibars) eina_list_free(tmpl);
   return bn;
}

static IBar *
_ibar_focused_prev_find(void)
{
   IBar *b, *bn = NULL;
   Eina_List *l;
   Eina_List *tmpl = NULL;
  
   EINA_LIST_FOREACH(ibars, l, b)
     {
        if (!b->icons) continue;
        tmpl = eina_list_sorted_insert
          (tmpl, EINA_COMPARE_CB(_ibar_cb_sort), b);
     }
   if (!tmpl) tmpl = ibars;
   EINA_LIST_FOREACH(tmpl, l, b)
     {
        if (b->focused)
          {
             if (l->prev)
               {
                  bn = l->prev->data;
                  break;
               }
             else
               {
                  bn = eina_list_last_data_get(tmpl);
                  break;
               }
          }
     }
   if (tmpl != ibars) eina_list_free(tmpl);
   return bn;
}

static void
_ibar_focus(IBar *b)
{
   IBar_Icon *ic;
   
   if (b->focused) return;
   b->focused = EINA_TRUE;
   EINA_INLIST_FOREACH(b->icons, ic)
     {
        if (ic->focused)
          {
             _ibar_icon_unfocus_focus(ic, NULL);
             break;
          }
     }
   if (b->icons)
     _ibar_icon_unfocus_focus(NULL, (IBar_Icon*)b->icons);
}

static void
_ibar_unfocus(IBar *b)
{
   IBar_Icon *ic;

   if (!b->focused) return;
   b->focused = EINA_FALSE;
   EINA_INLIST_FOREACH(b->icons, ic)
     {
        if (ic->focused)
          {
             _ibar_icon_unfocus_focus(ic, NULL);
             break;
          }
     }
}

static void
_ibar_focus_next(IBar *b)
{
   IBar_Icon *ic, *ic1 = NULL, *ic2 = NULL;
   
   if (!b->focused) return;
   if (!b->icons) return;
   EINA_INLIST_FOREACH(b->icons, ic)
     {
        if (!ic1)
          {
             if (ic->focused) ic1 = ic;
          }
        else
          {
             ic2 = ic;
             break;
          }
     }
   // wrap to start
   if ((ic1) && (!ic2)) ic2 = (IBar_Icon*)b->icons;
   if ((ic1) && (ic2) && (ic1 != ic2))
     _ibar_icon_unfocus_focus(ic1, ic2);
}

static void
_ibar_focus_prev(IBar *b)
{
   IBar_Icon *ic, *ic1 = NULL, *ic2 = NULL;
   
   if (!b->focused) return;
   if (!b->icons) return;
   EINA_INLIST_FOREACH(b->icons, ic)
     {
        if (ic->focused)
          {
             ic1 = ic;
             break;
          }
        ic2 = ic;
     }
   // wrap to end
   if ((ic1) && (!ic2)) ic2 = (IBar_Icon*)b->icons;
   if ((ic1) && (ic2) && (ic1 != ic2))
     _ibar_icon_unfocus_focus(ic1, ic2);
}

static void
_ibar_focus_launch(IBar *b)
{
   IBar_Icon *ic;
   
   if (!b->focused) return;
   EINA_INLIST_FOREACH(b->icons, ic)
     {
        if (ic->focused)
          {
             _ibar_icon_go(ic, EINA_TRUE);
             break;
          }
     }
}

static Eina_Bool
_ibar_focus_cb_key_down(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Event_Key *ev;
   IBar *b, *b2;
   
   ev = event;
   if (ev->window != _ibar_focus_win) return ECORE_CALLBACK_PASS_ON;
   b = _ibar_focused_find();
   if (!b) return ECORE_CALLBACK_PASS_ON;
   if (!strcmp(ev->key, "Up"))
     {
        if (b->inst)
          {  
             switch (b->inst->orient)
               {
                case E_GADCON_ORIENT_VERT:
                case E_GADCON_ORIENT_LEFT:
                case E_GADCON_ORIENT_RIGHT:
                case E_GADCON_ORIENT_CORNER_LT:
                case E_GADCON_ORIENT_CORNER_RT:
                case E_GADCON_ORIENT_CORNER_LB:
                case E_GADCON_ORIENT_CORNER_RB:
                   _ibar_focus_prev(b);
                  break;
                default:
                  break;
               }
          }
     }
   else if (!strcmp(ev->key, "Down"))
     {
        if (b->inst)
          {  
             switch (b->inst->orient)
               {
                case E_GADCON_ORIENT_VERT:
                case E_GADCON_ORIENT_LEFT:
                case E_GADCON_ORIENT_RIGHT:
                case E_GADCON_ORIENT_CORNER_LT:
                case E_GADCON_ORIENT_CORNER_RT:
                case E_GADCON_ORIENT_CORNER_LB:
                case E_GADCON_ORIENT_CORNER_RB:
                   _ibar_focus_next(b);
                  break;
                default:
                  break;
               }
          }
     }
   else if (!strcmp(ev->key, "Left"))
     {
        if (b->inst)
          {  
             switch (b->inst->orient)
               {
                case E_GADCON_ORIENT_FLOAT:
                case E_GADCON_ORIENT_HORIZ:
                case E_GADCON_ORIENT_TOP:
                case E_GADCON_ORIENT_BOTTOM:
                case E_GADCON_ORIENT_CORNER_TL:
                case E_GADCON_ORIENT_CORNER_TR:
                case E_GADCON_ORIENT_CORNER_BL:
                case E_GADCON_ORIENT_CORNER_BR:
                  _ibar_focus_prev(b);
                  break;
                default:
                  break;
               }
          }
     }
   else if (!strcmp(ev->key, "Right"))
     {
        if (b->inst)
          {  
             switch (b->inst->orient)
               {
                case E_GADCON_ORIENT_FLOAT:
                case E_GADCON_ORIENT_HORIZ:
                case E_GADCON_ORIENT_TOP:
                case E_GADCON_ORIENT_BOTTOM:
                case E_GADCON_ORIENT_CORNER_TL:
                case E_GADCON_ORIENT_CORNER_TR:
                case E_GADCON_ORIENT_CORNER_BL:
                case E_GADCON_ORIENT_CORNER_BR:
                  _ibar_focus_next(b);
                  break;
                default:
                  break;
               }
          }
     }
   else if (!strcmp(ev->key, "space"))
     {
        _ibar_focus_launch(b);
     }
   else if ((!strcmp(ev->key, "Return")) ||
            (!strcmp(ev->key, "KP_Enter")))
     {
        _ibar_focus_launch(b);
        _ibar_go_unfocus();
     }
   else if (!strcmp(ev->key, "Escape"))
     {
        _ibar_go_unfocus();
     }
   else if (!strcmp(ev->key, "Tab"))
     {
        if (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)
          {
             b2 = _ibar_focused_prev_find();
             if ((b) && (b2) && (b != b2))
               {
                  _ibar_unfocus(b);
                  _ibar_focus(b2);
               }
          }
        else
          {
             b2 = _ibar_focused_next_find();
             if ((b) && (b2) && (b != b2))
               {
                  _ibar_unfocus(b);
                  _ibar_focus(b2);
               }
          }
     }
   else if (!strcmp(ev->key, "ISO_Left_Tab"))
     {
        b2 = _ibar_focused_prev_find();
        if ((b) && (b2) && (b != b2))
          {
             _ibar_unfocus(b);
             _ibar_focus(b2);
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static void
_ibar_go_focus(void)
{
   E_Manager *man;
   IBar *b;
   
   if (_ibar_focus_win) return;
   man = e_manager_current_get();
   if (!man) return;
   _ibar_focus_win = ecore_x_window_input_new(man->root, -10, -20, 1, 1);
    ecore_x_window_show(_ibar_focus_win);
   //~ _ibar_focus_win = e_comp_get(man)->ee_win;
   if (!e_grabinput_get(0, 0, _ibar_focus_win))
     {
        _ibar_focus_win = 0;
        return;
     }
   _ibar_key_down_handler = ecore_event_handler_add
     (ECORE_EVENT_KEY_DOWN, _ibar_focus_cb_key_down, NULL);
   if (!_ibar_key_down_handler) goto err;
   b = _ibar_manager_find(man);
   if (!b) goto err;
   _ibar_focus(b);
   return;
err:
   if (_ibar_key_down_handler) ecore_event_handler_del(_ibar_key_down_handler);
   _ibar_key_down_handler = NULL;
   if (_ibar_focus_win)
     {
        e_grabinput_release(0, _ibar_focus_win);
     }
   _ibar_focus_win = 0;
}

static void
_ibar_go_unfocus(void)
{
   IBar *b;
   
   if (!_ibar_focus_win) return;
   b = _ibar_focused_find();
   if (b) _ibar_unfocus(b);
   e_grabinput_release(0, _ibar_focus_win);
   _ibar_focus_win = 0;
   ecore_event_handler_del(_ibar_key_down_handler);
   _ibar_key_down_handler = NULL;
}
   
static void
_ibar_cb_action_focus(E_Object *obj __UNUSED__, const char *params __UNUSED__, Ecore_Event_Key *ev __UNUSED__)
{
   _ibar_go_focus();
}

static Eina_Bool
_ibar_cb_bd_prop(void *d __UNUSED__, int t __UNUSED__, E_Event_Border_Property *ev)
{
   IBar *b;
   Eina_List *l;
   E_Border *bd;
   Eina_Bool skip = EINA_TRUE;

   if ((!ev->border->exe_inst) || (!ev->border->exe_inst->desktop)) return ECORE_CALLBACK_RENEW;
   EINA_LIST_FOREACH(ev->border->exe_inst->borders, l, bd)
     if (!bd->client.netwm.state.skip_taskbar)
       {
          skip = EINA_FALSE;
          break;
       }
   EINA_LIST_FOREACH(ibars, l, b)
     {
        IBar_Icon *ic;

        ic = eina_hash_find(b->icon_hash, _desktop_name_get(ev->border->exe_inst->desktop));
        if (skip && (!ic)) continue;
        if (!skip)
          {
             if (ic)
               {
                  if (ic->starting) _ibar_icon_signal_emit(ic, "e,state,started", "e");
                  ic->starting = EINA_FALSE;
                  if (!ic->exes) _ibar_icon_signal_emit(ic, "e,state,on", "e");
                  if (!eina_list_data_find(ic->exes, ev->border->exe_inst))
                    ic->exes = eina_list_append(ic->exes, ev->border->exe_inst);
               }
            else if (!b->inst->ci->dont_add_nonorder)
              {
                 _ibar_sep_create(b);
                 ic = _ibar_icon_notinorder_new(b, ev->border->exe_inst);
                 _ibar_resize_handle(b);
              }
          }
        else
          {
             ic->exes = eina_list_remove(ic->exes, ev->border->exe_inst);
             if (ic->exe_inst == ev->border->exe_inst) ic->exe_inst = NULL;
             if (!ic->exes)
               {
                  if (ic->not_in_order)
                    {
                       _ibar_icon_free(ic);
                       if (!b->not_in_order_count)
                         {
                            E_FREE_FUNC(b->o_sep, evas_object_del);
                         }
                       _ibar_resize_handle(b);
                    }
                  else
                    _ibar_icon_signal_emit(ic, "e,state,off", "e");
               }
          }
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_ibar_cb_exec_del(void *d __UNUSED__, int t __UNUSED__, E_Exec_Instance *exe)
{
   IBar *b;
   Eina_List *l;

   if (!exe->desktop) return ECORE_CALLBACK_RENEW; //can't do anything here :(
   EINA_LIST_FOREACH(ibars, l, b)
     {
        IBar_Icon *ic;

        ic = eina_hash_find(b->icon_hash, _desktop_name_get(exe->desktop));
        if (ic)
          {
             if (ic->starting) _ibar_icon_signal_emit(ic, "e,state,started", "e");
             ic->starting = EINA_FALSE;
             ic->exes = eina_list_remove(ic->exes, exe);
             if (ic->exe_inst == exe) ic->exe_inst = NULL;
             if (!ic->exes)
               {
                  if (ic->not_in_order)
                    {
                       _ibar_icon_free(ic);
                       if (!b->not_in_order_count)
                         {
                            E_FREE_FUNC(b->o_sep, evas_object_del);
                         }
                       _ibar_resize_handle(b);
                    }
                  else
                    _ibar_icon_signal_emit(ic, "e,state,off", "e");
               }
          }
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_ibar_cb_exec_new_client(void *d __UNUSED__, int t __UNUSED__, E_Exec_Instance *exe)
{
   IBar *b;
   E_Border *bd;
   Eina_List *l;
   Eina_Bool skip;

   if (!exe->desktop) return ECORE_CALLBACK_RENEW; //can't do anything here :(
   if (!exe->desktop->icon) return ECORE_CALLBACK_RENEW;
   bd = eina_list_last_data_get(exe->borders); //only care about last (new) one
   skip = bd->client.netwm.state.skip_taskbar;
   EINA_LIST_FOREACH(ibars, l, b)
     {
        IBar_Icon *ic;

        ic = eina_hash_find(b->icon_hash, _desktop_name_get(exe->desktop));
        if (ic)
          {
             if (ic->starting) _ibar_icon_signal_emit(ic, "e,state,started", "e");
             ic->starting = EINA_FALSE;
             if (!ic->exes) _ibar_icon_signal_emit(ic, "e,state,on", "e");
             if (skip) continue;
             if (!eina_list_data_find(ic->exes, exe))
               ic->exes = eina_list_append(ic->exes, exe);
          }
        else if (!b->inst->ci->dont_add_nonorder)
          {
             if (skip) continue;
             _ibar_sep_create(b);
             ic = _ibar_icon_notinorder_new(b, exe);
             _ibar_resize_handle(b);
          }
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_ibar_cb_exec_new(void *d __UNUSED__, int t __UNUSED__, E_Exec_Instance *exe)
{
   IBar *b;
   E_Border *bd;
   Eina_List *l;
   Eina_Bool skip = EINA_TRUE;

   if (!exe->desktop) return ECORE_CALLBACK_RENEW; //can't do anything here :(
   if (!exe->desktop->icon) return ECORE_CALLBACK_RENEW;
   EINA_LIST_FOREACH(exe->borders, l, bd)
     if (!bd->client.netwm.state.skip_taskbar)
        {
          skip = EINA_FALSE;
          break;
       }
   EINA_LIST_FOREACH(ibars, l, b)
     {
        IBar_Icon *ic;

        ic = eina_hash_find(b->icon_hash, _desktop_name_get(exe->desktop));
        if (ic)
          {
             _ibar_icon_signal_emit(ic, "e,state,started", "e");
             if (!ic->exes) _ibar_icon_signal_emit(ic, "e,state,on", "e");
             if (skip) continue;
             if (!eina_list_data_find(ic->exes, exe))
               ic->exes = eina_list_append(ic->exes, exe);
          }
        else if (!b->inst->ci->dont_add_nonorder)
          {
             if (skip) continue;
             _ibar_sep_create(b);
             ic = _ibar_icon_notinorder_new(b, exe);
             _ibar_resize_handle(b);
          }
     }
   return ECORE_CALLBACK_RENEW;
}

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION, "IBar"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   conf_item_edd = E_CONFIG_DD_NEW("IBar_Config_Item", Config_Item);
#undef T
#undef D
#define T Config_Item
#define D conf_item_edd
   E_CONFIG_VAL(D, T, id, STR);
   E_CONFIG_VAL(D, T, dir, STR);
   E_CONFIG_VAL(D, T, show_label, INT);
   E_CONFIG_VAL(D, T, eap_label, INT);
   E_CONFIG_VAL(D, T, lock_move, INT);
   E_CONFIG_VAL(D, T, focus_flash, INT);
   E_CONFIG_VAL(D, T, dont_add_nonorder, INT);
   E_CONFIG_VAL(D, T, dont_track_launch, UCHAR);
   E_CONFIG_VAL(D, T, dont_icon_menu_mouseover, UCHAR);
   
   conf_edd = E_CONFIG_DD_NEW("IBar_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_LIST(D, T, items, conf_item_edd);

   ibar_config = e_config_domain_load("module.ibar", conf_edd);

   if (!ibar_config)
     {
        Config_Item *ci;

        ibar_config = E_NEW(Config, 1);

        ci = E_NEW(Config_Item, 1);
        ci->id = eina_stringshare_add("ibar.1");
        ci->dir = eina_stringshare_add("default");
        ci->show_label = 1;
        ci->eap_label = 0;
        ci->lock_move = 0;
        ci->dont_add_nonorder = 0;
        ci->dont_track_launch = 0;
        ci->focus_flash = 1;
        ci->dont_icon_menu_mouseover = 0;
        ibar_config->items = eina_list_append(ibar_config->items, ci);
     }

   ibar_config->module = m;

   E_LIST_HANDLER_APPEND(ibar_config->handlers, E_EVENT_CONFIG_ICON_THEME,
                         _ibar_cb_config_icons, NULL);
   E_LIST_HANDLER_APPEND(ibar_config->handlers, EFREET_EVENT_ICON_CACHE_UPDATE,
                         _ibar_cb_config_icons, NULL);
   E_LIST_HANDLER_APPEND(ibar_config->handlers, E_EVENT_EXEC_NEW,
                         _ibar_cb_exec_new, NULL);
   E_LIST_HANDLER_APPEND(ibar_config->handlers, E_EVENT_EXEC_NEW_CLIENT,
                         _ibar_cb_exec_new_client, NULL);
   E_LIST_HANDLER_APPEND(ibar_config->handlers, E_EVENT_EXEC_DEL,
                         _ibar_cb_exec_del, NULL);
   E_LIST_HANDLER_APPEND(ibar_config->handlers, E_EVENT_BORDER_PROPERTY,
                         _ibar_cb_bd_prop, NULL);

   e_gadcon_provider_register(&_gadcon_class);
   ibar_orders = eina_hash_string_superfast_new(NULL);
   
   act_ibar_focus = e_action_add("ibar_focus");
   if (act_ibar_focus)
     {
        act_ibar_focus->func.go_key = _ibar_cb_action_focus;
        e_action_predef_name_set(N_("IBar"), N_("Focus IBar"),
                                 "ibar_focus", "<none>", NULL, 0);
     }
   
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   Ecore_Event_Handler *eh;
   Config_Item *ci;

   _ibar_go_unfocus();
   
   e_action_del("ibar_focus");
   e_action_predef_name_del("IBar", "Focus IBar");
   act_ibar_focus = NULL;
   
   e_gadcon_provider_unregister(&_gadcon_class);

   if (ibar_config->config_dialog)
     e_object_del(E_OBJECT(ibar_config->config_dialog));

   EINA_LIST_FREE(ibar_config->handlers, eh)
     ecore_event_handler_del(eh);

   EINA_LIST_FREE(ibar_config->items, ci)
     {
        if (ci->id) eina_stringshare_del(ci->id);
        if (ci->dir) eina_stringshare_del(ci->dir);
        E_FREE(ci);
     }
   E_FREE(ibar_config);
   ibar_config = NULL;
   eina_hash_free(ibar_orders);
   ibar_orders = NULL;
   E_CONFIG_DD_FREE(conf_item_edd);
   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   e_config_domain_save("module.ibar", conf_edd, ibar_config);
   return 1;
}

static Eina_Bool
_ibar_cb_config_icons(__UNUSED__ void *data, __UNUSED__ int ev_type, __UNUSED__ void *ev)
{
   const Eina_List *l;
   Instance *inst;

   EINA_LIST_FOREACH(ibar_config->instances, l, inst)
     {
        IBar_Icon *icon;

        EINA_INLIST_FOREACH(inst->ibar->icons, icon)
          _ibar_icon_fill(icon);
     }
   return ECORE_CALLBACK_PASS_ON;
}

