#include "e_mod_main.h"

/* Notification box protos */
static Notification_Box *_notification_box_new(const char *id,
                                               Evas       *evas);
static void              _notification_box_free(Notification_Box *b);
static void              _notification_box_evas_set(Notification_Box *b,
                                                    Evas             *evas);
static void              _notification_box_empty(Notification_Box *b);
static void              _notification_box_resize_handle(Notification_Box *b);
static void              _notification_box_empty_handle(Notification_Box *b);
static Eina_List        *_notification_box_find(E_Notification_Urgency urgency);

/* Notification box icons protos */
static Notification_Box_Icon *_notification_box_icon_new(Notification_Box *b,
                                                         E_Notification   *n,
                                                         E_Border         *bd,
                                                         unsigned int      id);
static void                   _notification_box_icon_free(Notification_Box_Icon *ic);
static void                   _notification_box_icon_fill(Notification_Box_Icon *ic,
                                                          E_Notification        *n);
static void                   _notification_box_icon_fill_label(Notification_Box_Icon *ic);
static void                   _notification_box_icon_empty(Notification_Box_Icon *ic);
static Notification_Box_Icon *_notification_box_icon_find(Notification_Box *b,
                                                          E_Border         *bd,
                                                          unsigned int      n_id);
static void _notification_box_icon_signal_emit(Notification_Box_Icon *ic,
                                               char                  *sig,
                                               char                  *src);

/* Utils */
static E_Border *_notification_find_source_border(E_Notification *n);

/* Notification box callbacks */
static void _notification_box_cb_menu_post(void   *data,
                                           E_Menu *m);

void
notification_box_notify(E_Notification *n,
                        unsigned int    replaces_id,
                        unsigned int    id)
{
   Eina_List *n_box;
   E_Border *bd;
   Notification_Box *b;
   Notification_Box_Icon *ic = NULL;

   bd = _notification_find_source_border(n);

   n_box = _notification_box_find(e_notification_hint_urgency_get(n));
   EINA_LIST_FREE(n_box, b)
     {
        if (bd || replaces_id)
          ic = _notification_box_icon_find(b, bd, replaces_id);
        if (ic)
          {
             e_notification_unref(ic->notif);
             e_notification_ref(n);
             ic->notif = n;
             ic->n_id = id;
             _notification_box_icon_empty(ic);
             _notification_box_icon_fill(ic, n);
          }
        else
          {
             ic = _notification_box_icon_new(b, n, bd, id);
             if (!ic) continue;
             b->icons = eina_list_append(b->icons, ic);
             e_box_pack_end(b->o_box, ic->o_holder);
          }
        _notification_box_empty_handle(b);
        _notification_box_resize_handle(b);
        _gc_orient(b->inst->gcc, b->inst->gcc->gadcon->orient);
     }
}

void
notification_box_shutdown(void)
{
   Notification_Box *b;

   EINA_LIST_FREE(notification_cfg->n_box, b)
     {
        if (b) _notification_box_free(b);
     }
}

void
notification_box_del(const char *id)
{
   Eina_List *l;
   Notification_Box *b;

   /* Find old config */
   EINA_LIST_FOREACH(notification_cfg->n_box, l, b)
     {
        if (b->id == id)
          {
             _notification_box_free(b);
             notification_cfg->n_box = eina_list_remove(notification_cfg->n_box, b);
             return;
          }
     }
}

void
notification_box_visible_set(Notification_Box *b, Eina_Bool visible)
{
   Eina_List *l;
   Notification_Box_Icon *ic;
   Ecore_Cb cb = (Ecore_Cb)(visible ? evas_object_show : evas_object_hide);

   cb(b->o_box);
   if (b->o_empty) cb(b->o_empty);
   EINA_LIST_FOREACH(b->icons, l, ic)
     {
        if (!ic) continue;
        cb(ic->o_holder);
        cb(ic->o_holder2);
        cb(ic->o_icon);
        cb(ic->o_icon2);
     }
}

Notification_Box *
notification_box_get(const char *id,
                     Evas       *evas)
{
   Eina_List *l;
   Notification_Box *b;

   /* Find old config */
   EINA_LIST_FOREACH(notification_cfg->n_box, l, b)
     {
        if (b->id == id)
          {
             _notification_box_evas_set(b, evas);
             notification_box_visible_set(b, EINA_TRUE);
             return b;
          }
     }

   b = _notification_box_new(id, evas);
   notification_cfg->n_box = eina_list_append(notification_cfg->n_box, b);
   return b;
}

Config_Item *
notification_box_config_item_get(const char *id)
{
   Config_Item *ci;

   GADCON_CLIENT_CONFIG_GET(Config_Item, notification_cfg->items, _gc_class, id);

   ci = E_NEW(Config_Item, 1);
   ci->id = eina_stringshare_ref(id);
   ci->show_label = 1;
   ci->show_popup = 1;
   ci->focus_window = 1;
   ci->store_low = 1;
   ci->store_normal = 1;
   ci->store_critical = 0;
   notification_cfg->items = eina_list_append(notification_cfg->items, ci);

   return ci;
}

void
notification_box_orient_set(Notification_Box *b,
                            int               horizontal)
{
   e_box_orientation_set(b->o_box, horizontal);
   e_box_align_set(b->o_box, 0.5, 0.5);
}

void
notification_box_cb_obj_moveresize(void        *data,
                                   Evas        *e __UNUSED__,
                                   Evas_Object *obj __UNUSED__,
                                   void        *event_info __UNUSED__)
{
   Instance *inst;

   inst = data;
   _notification_box_resize_handle(inst->n_box);
}

Eina_Bool
notification_box_cb_border_remove(void *data __UNUSED__,
                                  int   type __UNUSED__,
                                  E_Event_Border_Remove *ev)
{
   Notification_Box_Icon *ic;
   Eina_List *l;
   Instance *inst;

   EINA_LIST_FOREACH(notification_cfg->instances, l, inst)
     {
        Notification_Box *b;

        if (!inst) continue;
        b = inst->n_box;

        ic = _notification_box_icon_find(b, ev->border, 0);
        if (!ic) continue;
        b->icons = eina_list_remove(b->icons, ic);
        _notification_box_icon_free(ic);
        _notification_box_empty_handle(b);
        _notification_box_resize_handle(b);
        _gc_orient(inst->gcc, inst->gcc->gadcon->orient);
     }
   return ECORE_CALLBACK_RENEW;
}

static Notification_Box *
_notification_box_new(const char *id,
                      Evas       *evas)
{
   Notification_Box *b;

   b = E_NEW(Notification_Box, 1);
   b->id = eina_stringshare_ref(id);
   b->o_box = e_box_add(evas);
   e_box_homogenous_set(b->o_box, 1);
   e_box_orientation_set(b->o_box, 1);
   e_box_align_set(b->o_box, 0.5, 0.5);
   _notification_box_empty(b);
   return b;
}

static void
_notification_box_free(Notification_Box *b)
{
   _notification_box_empty(b);
   eina_stringshare_del(b->id);
   evas_object_del(b->o_box);
   if (b->o_empty) evas_object_del(b->o_empty);
   b->o_empty = NULL;
   free(b);
}

static void
_notification_box_evas_set(Notification_Box *b,
                           Evas             *evas)
{
   Eina_List *new_icons = NULL;
   Notification_Box_Icon *ic, *new_ic;

   evas_object_del(b->o_box);
   if (b->o_empty) evas_object_del(b->o_empty);
   b->o_empty = NULL;
   b->o_box = e_box_add(evas);

   e_box_homogenous_set(b->o_box, 1);
   e_box_orientation_set(b->o_box, 1);
   e_box_align_set(b->o_box, 0.5, 0.5);

   EINA_LIST_FREE(b->icons, ic)
     {
        if (!ic) continue;

        new_ic = _notification_box_icon_new(b, ic->notif, ic->border, ic->n_id);
        _notification_box_icon_free(ic);
        new_icons = eina_list_append(new_icons, new_ic);

        e_box_pack_end(b->o_box, new_ic->o_holder);
     }
   b->icons = new_icons;
   _notification_box_empty_handle(b);
   _notification_box_resize_handle(b);
}

static void
_notification_box_empty(Notification_Box *b)
{
   Notification_Box_Icon *ic;
   EINA_LIST_FREE(b->icons, ic)
     _notification_box_icon_free(b->icons->data);
   _notification_box_empty_handle(b);
}

static void
_notification_box_resize_handle(Notification_Box *b)
{
   Notification_Box_Icon *ic;
   Evas_Coord w, h;

   evas_object_geometry_get(b->o_box, NULL, NULL, &w, &h);
   if (e_box_orientation_get(b->o_box))
     w = h;
   else
     h = w;
   e_box_freeze(b->o_box);
   EINA_LIST_FREE(b->icons, ic)
     e_box_pack_options_set(ic->o_holder, 1, 1, 0, 0, 0.5, 0.5, w, h, w, h);
   e_box_thaw(b->o_box);
}

static Eina_List *
_notification_box_find(E_Notification_Urgency urgency)
{
   Eina_List *l, *n_box = NULL;
   Instance *inst;

   EINA_LIST_FOREACH(notification_cfg->instances, l, inst)
     {
        if ((urgency == E_NOTIFICATION_URGENCY_LOW) && (!inst->ci->store_low))
          continue;
        if ((urgency == E_NOTIFICATION_URGENCY_NORMAL) && (!inst->ci->store_normal))
          continue;
        if ((urgency == E_NOTIFICATION_URGENCY_CRITICAL) && (!inst->ci->store_critical))
          continue;
        n_box = eina_list_append(n_box, inst->n_box);
     }
   return n_box;
}

static void
_notification_box_icon_free(Notification_Box_Icon *ic)
{
   if (notification_cfg->menu)
     {
        e_menu_post_deactivate_callback_set(notification_cfg->menu, NULL, NULL);
        e_object_del(E_OBJECT(notification_cfg->menu));
        notification_cfg->menu = NULL;
     }
   _notification_box_icon_empty(ic);
   evas_object_del(ic->o_holder);
   evas_object_del(ic->o_holder2);
   if (ic->border) e_object_unref(E_OBJECT(ic->border));
   if (ic->notif) e_notification_unref(ic->notif);
   free(ic);
}

static void
_notification_box_icon_fill(Notification_Box_Icon *ic,
                            E_Notification        *n)
{
   void *img;
   const char *icon_path;
   Evas_Object *app_icon;
   Evas_Object *dummy = NULL;
   int w, h = 0;

   if ((icon_path = e_notification_app_icon_get(n)) && *icon_path)
     {
        if (!memcmp(icon_path, "file://", 7)) icon_path += 7;
        app_icon = evas_object_image_add(evas_object_evas_get(ic->n_box->o_box));
        evas_object_image_load_scale_down_set(app_icon, 1);
        evas_object_image_load_size_set(app_icon, 80, 80);
        evas_object_image_file_set(app_icon, icon_path, NULL);
        evas_object_image_fill_set(app_icon, 0, 0, 80, 80);
     }
   else if ((img = e_notification_hint_icon_data_get(n)))
     {
        app_icon = e_notification_image_evas_object_add(evas_object_evas_get(ic->n_box->o_box), img);
     }
   else
     {
        char buf[PATH_MAX];

        snprintf(buf, sizeof(buf), "%s/e-module-notification.edj", notification_mod->dir);
        dummy = edje_object_add(evas_object_evas_get(ic->n_box->o_box));
        if (!e_theme_edje_object_set(dummy, "base/theme/modules/notification",
                                     "modules/notification/logo"))
          edje_object_file_set(dummy, buf, "modules/notification/logo");
        evas_object_resize(dummy, 80, 80);
        app_icon = (Evas_Object*)edje_object_part_object_get(dummy, "image");
     }
   evas_object_image_size_get(app_icon, &w, &h);

   ic->o_icon = e_icon_add(evas_object_evas_get(ic->n_box->o_box));
   e_icon_alpha_set(ic->o_icon, 1);
   e_icon_data_set(ic->o_icon, evas_object_image_data_get(app_icon, 0), w, h);
   edje_object_part_swallow(ic->o_holder, "e.swallow.content", ic->o_icon);
   evas_object_pass_events_set(ic->o_icon, 1);
   evas_object_show(ic->o_icon);

   ic->o_icon2 = e_icon_add(evas_object_evas_get(ic->n_box->o_box));
   e_icon_alpha_set(ic->o_icon2, 1);
   e_icon_data_set(ic->o_icon2, evas_object_image_data_get(app_icon, 0), w, h);
   edje_object_part_swallow(ic->o_holder2, "e.swallow.content", ic->o_icon2);
   evas_object_pass_events_set(ic->o_icon2, 1);
   evas_object_show(ic->o_icon2);

   if (dummy) evas_object_del(dummy);
   evas_object_del(app_icon);
   _notification_box_icon_fill_label(ic);
}

static void
_notification_box_icon_fill_label(Notification_Box_Icon *ic)
{
   const char *label = NULL;

   if (ic->border)
     label = ic->border->client.netwm.name;

   if (!label) label = e_notification_app_name_get(ic->notif);
   edje_object_part_text_set(ic->o_holder, "e.text.label", label);
   edje_object_part_text_set(ic->o_holder2, "e.text.label", label);
}

static void
_notification_box_icon_empty(Notification_Box_Icon *ic)
{
   if (ic->o_icon) evas_object_del(ic->o_icon);
   if (ic->o_icon2) evas_object_del(ic->o_icon2);
   ic->o_icon2 = ic->o_icon = NULL;
}

static Notification_Box_Icon *
_notification_box_icon_find(Notification_Box *b,
                            E_Border         *bd,
                            unsigned int      n_id)
{
   Eina_List *l;
   Notification_Box_Icon *ic;

   EINA_LIST_FOREACH(b->icons, l, ic)
     {
        if (!ic) continue;
        if ((ic->border == bd) || (ic->n_id == n_id))
          return ic;
     }

   return NULL;
}

static void
_notification_box_icon_signal_emit(Notification_Box_Icon *ic,
                                   char                  *sig,
                                   char                  *src)
{
   if (ic->o_holder)
     edje_object_signal_emit(ic->o_holder, sig, src);
   if (ic->o_icon)
     edje_object_signal_emit(ic->o_icon, sig, src);
   if (ic->o_holder2)
     edje_object_signal_emit(ic->o_holder2, sig, src);
   if (ic->o_icon2)
     edje_object_signal_emit(ic->o_icon2, sig, src);
}

static E_Border *
_notification_find_source_border(E_Notification *n)
{
   const char *app_name;
   Eina_List *l;
   E_Border *bd;

   if (!(app_name = e_notification_app_name_get(n))) return NULL;

   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        size_t app, test;

        if ((!bd) || ((!bd->client.icccm.name) && (!bd->client.icccm.class))) continue;
        /* We can't be sure that the app_name really match the application name.
         * Some plugin put their name instead. But this search gives some good
         * results.
         */
        app = strlen(app_name);
        if (bd->client.icccm.name)
          {
             test = eina_strlen_bounded(bd->client.icccm.name, app + 1);
             if (!strncasecmp(bd->client.icccm.name, app_name, (app < test) ? app : test))
               return bd;
          }
        if (bd->client.icccm.class)
          {
             test = eina_strlen_bounded(bd->client.icccm.class, app + 1);
             if (!strncasecmp(bd->client.icccm.class, app_name, (app < test) ? app : test))
               return bd;
          }
     }
   return NULL;
}

static void
_notification_box_cb_menu_post(void   *data __UNUSED__,
                               E_Menu *m __UNUSED__)
{
   if (!notification_cfg->menu) return;
   e_object_del(E_OBJECT(notification_cfg->menu));
   notification_cfg->menu = NULL;
}

static void
_notification_box_cb_menu_configuration(Notification_Box *b,
                                        E_Menu      *m __UNUSED__,
                                        E_Menu_Item *mi __UNUSED__)
{
   Eina_List *l;
   E_Config_Dialog *cfd;

   EINA_LIST_FOREACH(notification_cfg->config_dialog, l, cfd)
     {
        if (cfd->data == b->inst->ci) return;
     }
   config_notification_box_module(b->inst->ci);
}

static void
_notification_box_cb_empty_mouse_down(Notification_Box *b,
                                      Evas        *e __UNUSED__,
                                      Evas_Object *obj __UNUSED__,
                                      Evas_Event_Mouse_Down *ev)
{
   E_Menu *m;
   E_Menu_Item *mi;
   int cx, cy, cw, ch;

   if (notification_cfg->menu) return;
   m = e_menu_new();
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Settings"));
   e_util_menu_item_theme_icon_set(mi, "preferences-system");
   e_menu_item_callback_set(mi, (E_Menu_Cb)_notification_box_cb_menu_configuration, b);

   notification_cfg->menu = m = e_gadcon_client_util_menu_items_append(b->inst->gcc, m, 0);
   e_menu_post_deactivate_callback_set(m, _notification_box_cb_menu_post, NULL);

   e_gadcon_canvas_zone_geometry_get(b->inst->gcc->gadcon,
                                     &cx, &cy, &cw, &ch);
   e_menu_activate_mouse(m,
                         e_util_zone_current_get(e_manager_current_get()),
                         cx + ev->output.x, cy + ev->output.y, 1, 1,
                         E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
}

static void
_notification_box_cb_icon_move(Notification_Box_Icon *ic,
                               Evas        *e __UNUSED__,
                               Evas_Object *obj __UNUSED__,
                               void        *event_info __UNUSED__)
{
   Evas_Coord x, y;

   evas_object_geometry_get(ic->o_holder, &x, &y, NULL, NULL);
   evas_object_move(ic->o_holder2, x, y);
   evas_object_raise(ic->o_holder2);
}

static void
_notification_box_cb_icon_resize(Notification_Box_Icon *ic,
                                 Evas        *e __UNUSED__,
                                 Evas_Object *obj __UNUSED__,
                                 void        *event_info __UNUSED__)
{
   Evas_Coord w, h;

   evas_object_geometry_get(ic->o_holder, NULL, NULL, &w, &h);
   evas_object_resize(ic->o_holder2, w, h);
   evas_object_raise(ic->o_holder2);
}


static Eina_Bool
_notification_box_cb_icon_mouse_still_in(Notification_Box_Icon *ic)
{
   e_notification_timeout_set(ic->notif, 0);
   e_notification_hint_urgency_set(ic->notif, 4);
   ic->popup = notification_popup_notify(ic->notif,
                                         e_notification_id_get(ic->notif),
                                         e_notification_app_name_get(ic->notif));
   ecore_timer_del(ic->mouse_in_timer);
   ic->mouse_in_timer = NULL;
   return EINA_FALSE;
}


static void
_notification_box_cb_icon_mouse_in(Notification_Box_Icon *ic,
                                   Evas        *e __UNUSED__,
                                   Evas_Object *obj __UNUSED__,
                                   void        *event_info __UNUSED__)
{
   Config_Item *ci;

   if ((!ic) || !ic->n_box || !ic->n_box->inst) return;
   if (!(ci = ic->n_box->inst->ci)) return;

   _notification_box_icon_signal_emit(ic, "e,state,focused", "e");
   if (ci->show_label)
     {
        _notification_box_icon_fill_label(ic);
        _notification_box_icon_signal_emit(ic, "e,action,show,label", "e");
     }
   if (ci->show_popup && !ic->popup && !ic->mouse_in_timer)
     ic->mouse_in_timer = ecore_timer_add(0.5, (Ecore_Task_Cb)_notification_box_cb_icon_mouse_still_in, ic);
}

static void
_notification_box_cb_icon_mouse_out(Notification_Box_Icon *ic,
                                    Evas        *e __UNUSED__,
                                    Evas_Object *obj __UNUSED__,
                                    void        *event_info __UNUSED__)
{
   _notification_box_icon_signal_emit(ic, "e,state,unfocused", "e");
   if (ic->n_box->inst->ci->show_label)
     _notification_box_icon_signal_emit(ic, "e,action,hide,label", "e");

   if (ic->mouse_in_timer)
     {
        ecore_timer_del(ic->mouse_in_timer);
        ic->mouse_in_timer = NULL;
     }
   if (ic->popup)
     {
        notification_popup_close(e_notification_id_get(ic->notif));
        ic->popup = 0;
     }
}

static void
_notification_box_cb_icon_mouse_up(Notification_Box_Icon *ic,
                                   Evas        *e __UNUSED__,
                                   Evas_Object *obj __UNUSED__,
                                   Evas_Event_Mouse_Up *ev)
{
   Notification_Box *b;

   b = ic->n_box;
   if (ev->button != 1) return;

   if (b->inst->ci->focus_window && ic->border)
     {
        e_border_uniconify(ic->border);
        e_desk_show(ic->border->desk);
        e_border_show(ic->border);
        e_border_raise(ic->border);
        e_border_focus_set(ic->border, 1, 1);
     }
   b->icons = eina_list_remove(b->icons, ic);
   _notification_box_icon_free(ic);
   _notification_box_empty_handle(b);
   _notification_box_resize_handle(b);
   _gc_orient(b->inst->gcc, b->inst->gcc->gadcon->orient);
}

static void
_notification_box_cb_icon_mouse_down(Notification_Box_Icon *ic,
                                     Evas        *e __UNUSED__,
                                     Evas_Object *obj __UNUSED__,
                                     Evas_Event_Mouse_Down *ev)
{
   E_Menu *m;
   E_Menu_Item *mi;
   int cx, cy, cw, ch;

   if (notification_cfg->menu || (ev->button != 3)) return;

   m = e_menu_new();
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Settings"));
   e_util_menu_item_theme_icon_set(mi, "preferences-system");
   e_menu_item_callback_set(mi, (E_Menu_Cb)_notification_box_cb_menu_configuration, ic->n_box);

   m = e_gadcon_client_util_menu_items_append(ic->n_box->inst->gcc, m, 0);
   e_menu_post_deactivate_callback_set(m, _notification_box_cb_menu_post, NULL);
   notification_cfg->menu = m;

   e_gadcon_canvas_zone_geometry_get(ic->n_box->inst->gcc->gadcon,
                                     &cx, &cy, &cw, &ch);
   e_menu_activate_mouse(m,
                         e_util_zone_current_get(e_manager_current_get()),
                         cx + ev->output.x, cy + ev->output.y, 1, 1,
                         E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
}

static void
_notification_box_empty_handle(Notification_Box *b)
{
   if (!b->icons)
     {
        Evas_Coord w, h;
        if (b->o_empty) return;

        b->o_empty = evas_object_rectangle_add(evas_object_evas_get(b->o_box));
        evas_object_event_callback_add(b->o_empty, EVAS_CALLBACK_MOUSE_DOWN,
                                       (Evas_Object_Event_Cb)_notification_box_cb_empty_mouse_down, b);
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
   else if (b->o_empty)
     {
        evas_object_del(b->o_empty);
        b->o_empty = NULL;
     }
}

static Notification_Box_Icon *
_notification_box_icon_new(Notification_Box *b,
                           E_Notification   *n,
                           E_Border         *bd,
                           unsigned int      id)
{
   Notification_Box_Icon *ic;

   ic = E_NEW(Notification_Box_Icon, 1);
   if (bd) e_object_ref(E_OBJECT(bd));
   e_notification_ref(n);
   ic->label = e_notification_app_name_get(n);
   ic->n_box = b;
   ic->n_id = id;
   ic->border = bd;
   ic->notif = n;
   ic->o_holder = edje_object_add(evas_object_evas_get(b->o_box));
   e_theme_edje_object_set(ic->o_holder, "base/theme/modules/ibox",
                           "e/modules/ibox/icon");
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_IN,
                                  (Evas_Object_Event_Cb)_notification_box_cb_icon_mouse_in, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_OUT,
                                  (Evas_Object_Event_Cb)_notification_box_cb_icon_mouse_out, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_DOWN,
                                  (Evas_Object_Event_Cb)_notification_box_cb_icon_mouse_down, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_UP,
                                  (Evas_Object_Event_Cb)_notification_box_cb_icon_mouse_up, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOVE,
                                  (Evas_Object_Event_Cb)_notification_box_cb_icon_move, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_RESIZE,
                                  (Evas_Object_Event_Cb)_notification_box_cb_icon_resize, ic);
   evas_object_show(ic->o_holder);

   ic->o_holder2 = edje_object_add(evas_object_evas_get(b->o_box));
   e_theme_edje_object_set(ic->o_holder2, "base/theme/modules/ibox",
                           "e/modules/ibox/icon_overlay");
   evas_object_layer_set(ic->o_holder2, 9999);
   evas_object_pass_events_set(ic->o_holder2, 1);
   evas_object_show(ic->o_holder2);

   _notification_box_icon_fill(ic, n);
   return ic;
}
