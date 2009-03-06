/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/* local subsystem functions */
static int _cb_key_down(void *data, int type, void *event);
static int _cb_mouse_down(void *data, int type, void *event);
static int _cb_mouse_up(void *data, int type, void *event);
static int _cb_mouse_move(void *data, int type, void *event);
static int _cb_mouse_wheel(void *data, int type, void *event);
static void _cb_signal_close(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _cb_signal_syscon(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _cb_signal_action(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _cb_signal_action_extra(void *data, Evas_Object *obj, const char *emission, const char *source);
static int _cb_timeout_defaction(void *data);

/* local subsystem globals */
static E_Popup *popup = NULL;
static Ecore_X_Window input_window = 0;
static const char *do_defact = NULL;
static Eina_List *handlers = NULL;
static Evas_Object *o_bg = NULL;
static Evas_Object *o_flow_main = NULL;
static Evas_Object *o_flow_secondary = NULL;
static Evas_Object *o_flow_extra = NULL;
static int inevas = 0;
static Ecore_Timer *deftimer = NULL;

/* externally accessible functions */
EAPI int
e_syscon_init(void)
{
   return 1;
}

EAPI int
e_syscon_shutdown(void)
{
   e_syscon_hide();
   return 1;
}

EAPI int
e_syscon_show(E_Zone *zone, const char *defact)
{
   Evas_Object *o, *o2;
   Evas_Coord mw, mh;
   int x, y, w, h;
   int iw, ih;
   Eina_List *l;

   if (popup) return 0;

   input_window = ecore_x_window_input_new(zone->container->win, zone->x,
                                           zone->y, zone->w, zone->h);
   ecore_x_window_show(input_window);
   if (!e_grabinput_get(input_window, 1, input_window))
     {
        ecore_x_window_del(input_window);
        input_window = 0;
        return 0;
     }

   popup = e_popup_new(zone, 0, 0, 1, 1);
   if (!popup)
     {
        e_grabinput_release(input_window, input_window);
        ecore_x_window_del(input_window);
        input_window = 0;
        return 0;
     }
   evas_event_freeze(popup->evas);
   e_popup_layer_set(popup, 500);

   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_X_EVENT_KEY_DOWN, _cb_key_down, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_X_EVENT_MOUSE_BUTTON_DOWN, _cb_mouse_down, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_X_EVENT_MOUSE_BUTTON_UP, _cb_mouse_up, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_X_EVENT_MOUSE_MOVE, _cb_mouse_move, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_X_EVENT_MOUSE_WHEEL, _cb_mouse_wheel, NULL));

   o = edje_object_add(popup->evas);
   o_bg = o;
   e_theme_edje_object_set(o, "base/theme/syscon",
                           "e/widgets/syscon/main");
   edje_object_part_text_set(o, "e.text.label", _("Cancel"));
   edje_object_signal_callback_add(o, "e,action,close", "", 
                                   _cb_signal_close, NULL);
   edje_object_signal_callback_add(o, "e,action,syscon", "*", 
                                   _cb_signal_syscon, NULL);

   // main (default):
   //  halt | suspend | desk_lock
   // secondary (default):
   //  reboot | hibernate | logout
   // extra (example for illume):
   //  home | close | kill

   iw = 64 * e_scale;
   ih = 64 * e_scale;

   o = e_flowlayout_add(popup->evas);
   o_flow_main = o;
   e_flowlayout_orientation_set(o, 1);
   e_flowlayout_flowdirection_set(o, 1, 1);
   e_flowlayout_homogenous_set(o, 1);

   o = e_flowlayout_add(popup->evas);
   o_flow_secondary = o;
   e_flowlayout_orientation_set(o, 1);
   e_flowlayout_flowdirection_set(o, 1, 1);
   e_flowlayout_homogenous_set(o, 1);

   o = e_flowlayout_add(popup->evas);
   o_flow_extra = o;
   e_flowlayout_orientation_set(o, 1);
   e_flowlayout_flowdirection_set(o, 1, 1);
   e_flowlayout_homogenous_set(o, 1);

   for (l = e_config->syscon.actions; l; l = l->next)
     {
        E_Config_Syscon_Action *sca;
        char buf[1024];
        E_Action *a;
        int disabled;

        sca = l->data;
        if (!sca->action) continue;
        a = e_action_find(sca->action);
        if (!a) continue;
        disabled = 0;
        if ((!strcmp(sca->action, "logout")) &&
            (!e_sys_action_possible_get(E_SYS_LOGOUT))) disabled = 1;
        else if ((!strcmp(sca->action, "halt")) &&
            (!e_sys_action_possible_get(E_SYS_HALT))) disabled = 1;
        else if ((!strcmp(sca->action, "halt_now")) &&
            (!e_sys_action_possible_get(E_SYS_HALT_NOW))) disabled = 1;
        else if ((!strcmp(sca->action, "reboot")) &&
            (!e_sys_action_possible_get(E_SYS_REBOOT))) disabled = 1;
        else if ((!strcmp(sca->action, "suspend")) &&
            (!e_sys_action_possible_get(E_SYS_SUSPEND))) disabled = 1;
        else if ((!strcmp(sca->action, "hibernate")) &&
            (!e_sys_action_possible_get(E_SYS_HIBERNATE))) disabled = 1;
        o = edje_object_add(popup->evas);
        edje_object_signal_callback_add(o, "e,action,click", "", 
                                        _cb_signal_action, sca);
        if (sca->button)
          {
             snprintf(buf, sizeof(buf), "e/widgets/syscon/item/%s",
                      sca->button);
             e_theme_edje_object_set(o, "base/theme/widgets", buf);
          }
        else
          e_theme_edje_object_set(o, "base/theme/widgets",
                                  "e/widgets/syscon/item/button");
        edje_object_part_text_set(o, "e.text.label", 
                                  e_action_predef_label_get(sca->action, sca->params));
        if (sca->icon)
          {
             o2 = e_icon_add(popup->evas);
             e_util_icon_theme_set(o2, sca->icon);
             edje_object_part_swallow(o, "e.swallow.icon", o2);
             evas_object_show(o2);
             if (disabled)
               edje_object_signal_emit(o2, "e,state,disabled", "e");
          }
        if (disabled)
          edje_object_signal_emit(o, "e,state,disabled", "e");
        if (sca->is_main)
          {
             e_flowlayout_pack_end(o_flow_main, o);
             iw = ih = e_config->syscon.main.icon_size * e_scale;
          }
        else
          {
             e_flowlayout_pack_end(o_flow_secondary, o);
             iw = ih = e_config->syscon.secondary.icon_size * e_scale;
          }
        edje_object_message_signal_process(o);
        edje_object_size_min_calc(o, &mw, &mh);
        if (mw > iw) iw = mw;
        if (mh > ih) ih = mh;
        e_flowlayout_pack_options_set(o, 1, 1, 0, 0, 0.5, 0.5, 
                                      iw, ih, iw, ih);
        evas_object_show(o);
     }

   for (l = (Eina_List *)e_sys_con_extra_action_list_get(); l; l = l->next)
     {
        E_Sys_Con_Action *sca;
        char buf[1024];
        
        sca = l->data;
        o = edje_object_add(popup->evas);
        edje_object_signal_callback_add(o, "e,action,click", "", _cb_signal_action_extra, sca);
        if (sca->button_name)
          {
             snprintf(buf, sizeof(buf), "e/widgets/syscon/item/%s",
                      sca->button_name);
             e_theme_edje_object_set(o, "base/theme/widgets", buf);
          }
        else
          e_theme_edje_object_set(o, "base/theme/widgets",
                                  "e/widgets/syscon/item/button");
        edje_object_part_text_set(o, "e.text.label", sca->label);
        if (sca->icon_group)
          {
             o2 = edje_object_add(popup->evas);
             e_util_edje_icon_set(o2, sca->icon_group);
             edje_object_part_swallow(o, "e.swallow.icon", o2);
             evas_object_show(o2);
             if (sca->disabled)
               edje_object_signal_emit(o2, "e,state,disabled", "e");
          }
        if (sca->disabled)
          edje_object_signal_emit(o, "e,state,disabled", "e");
        e_flowlayout_pack_end(o_flow_extra, o);
        iw = ih = e_config->syscon.extra.icon_size * e_scale;
        e_flowlayout_pack_options_set(o, 1, 1, 0, 0, 0.5, 0.5, 
                                      iw, ih, iw, ih);
        evas_object_show(o);
     }

   e_flowlayout_fill_set(o_flow_main, 1);
   edje_object_part_swallow(o_bg, "e.swallow.main", o_flow_main);
   e_flowlayout_fill_set(o_flow_secondary, 1);
   edje_object_part_swallow(o_bg, "e.swallow.secondary", o_flow_secondary);
   e_flowlayout_fill_set(o_flow_extra, 1);
   edje_object_part_swallow(o_bg, "e.swallow.extra", o_flow_extra);

   evas_object_resize(o_bg, zone->w, zone->h);
   edje_object_calc_force(o_bg);

   e_flowlayout_min_size_get(o_flow_main, &mw, &mh);
   edje_extern_object_min_size_set(o_flow_main, mw, mh); 
   edje_object_part_swallow(o_bg, "e.swallow.main", o_flow_main);
   e_flowlayout_min_size_get(o_flow_secondary, &mw, &mh);
   edje_extern_object_min_size_set(o_flow_secondary, mw, mh); 
   edje_object_part_swallow(o_bg, "e.swallow.secondary", o_flow_secondary);
   e_flowlayout_min_size_get(o_flow_extra, &mw, &mh);
   edje_extern_object_min_size_set(o_flow_extra, mw, mh); 
   edje_object_part_swallow(o_bg, "e.swallow.extra", o_flow_extra);

   edje_object_size_min_calc(o_bg, &mw, &mh);
   w = mw;
   if (w > zone->w) w = zone->w;
   x = (zone->w - w) / 2;
   h = mh;
   if (h > zone->h) h = zone->h;
   y = (zone->h - h) / 2;

   e_popup_move_resize(popup, x, y, w, h);
   evas_object_move(o_bg, 0, 0);
   evas_object_resize(o_bg, w, h);
   evas_object_show(o_bg);
   e_popup_edje_bg_object_set(popup, o_bg);

   if (e_config->syscon.do_input)
     {
        deftimer = ecore_timer_add(e_config->syscon.timeout, 
                                   _cb_timeout_defaction, NULL);
        if (defact) do_defact = eina_stringshare_add(defact);
     }

   evas_event_thaw(popup->evas);
   inevas = 0;
   e_popup_show(popup);
   return 1;
}

EAPI void
e_syscon_hide(void)
{
   if (!popup) return;

   if (deftimer)
     {
        ecore_timer_del(deftimer);
        deftimer = NULL;
     }
   if (do_defact) eina_stringshare_del(do_defact);
   do_defact = NULL;
   while (handlers)
     {
        ecore_event_handler_del(handlers->data);
        handlers = eina_list_remove_list(handlers, handlers);
     }
   e_popup_hide(popup);
   e_object_del(E_OBJECT(popup));
   popup = NULL;
   e_grabinput_release(input_window, input_window);
   ecore_x_window_del(input_window);
   input_window = 0;
}

/* local subsystem functions */
static int
_cb_key_down(void *data, int type, void *event)
{
   Ecore_X_Event_Key_Down *ev;

   ev = event;
   if (ev->event_win != input_window) return 1;
   if (!strcmp(ev->keysymbol, "Escape"))
     e_syscon_hide();
   else if (!strcmp(ev->keysymbol, "Up"))
     {
        // FIXME: implement focus and key control... eventually
     }

   return 1;
}

static int
_cb_mouse_down(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Down *ev;
   Evas_Button_Flags flags = EVAS_BUTTON_NONE;

   ev = event;
   if (ev->event_win != input_window) return 1;
   if (ev->double_click) flags |= EVAS_BUTTON_DOUBLE_CLICK;
   if (ev->triple_click) flags |= EVAS_BUTTON_TRIPLE_CLICK;
   if ((ev->x < popup->x) || (ev->x >= (popup->x + popup->w)) ||
       (ev->y < popup->y) || (ev->y >= (popup->y + popup->h)))
     {
        e_syscon_hide();
        return 1;
     }
   evas_event_feed_mouse_down(popup->evas, ev->button, flags, ev->time, NULL);
   return 1;
}

static int
_cb_mouse_up(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Up *ev;

   ev = event;
   if (ev->event_win != input_window) return 1;
   evas_event_feed_mouse_up(popup->evas, ev->button, EVAS_BUTTON_NONE,
                            ev->time, NULL);
   return 1;
}

static int
_cb_mouse_move(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Move *ev;

   ev = event;
   if (ev->event_win != input_window) return 1;
   if (!inevas)
     {
        evas_event_feed_mouse_in(popup->evas, ev->time, NULL);
        inevas = 1;
     }
   evas_event_feed_mouse_move(popup->evas, ev->x - popup->x, ev->y - popup->y,
                              ev->time, NULL);
   return 1;
}

static int
_cb_mouse_wheel(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Wheel *ev;

   ev = event;
   if (ev->event_win != input_window) return 1;
   evas_event_feed_mouse_wheel(popup->evas, ev->direction, ev->z, 
                               ev->time, NULL);
   return 1;
}

static void
_cb_signal_close(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   e_syscon_hide();
}

static void
_do_action_name(const char *action)
{
   Eina_List *l;

   for (l = e_config->syscon.actions; l; l = l->next)
     {
        E_Config_Syscon_Action *sca;
        E_Action *a;

        sca = l->data;
        if (!sca->action) continue;
        if (!strcmp(sca->action, action))
          {
             a = e_action_find(sca->action);
             if (!a) break;
             if (a) a->func.go(NULL, sca->params);
             break;
          }
     }
}

static void
_cb_signal_syscon(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   e_syscon_hide();
   _do_action_name(source);
}

static void
_cb_signal_action(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Config_Syscon_Action *sca;
   E_Action *a;

   e_syscon_hide();
   sca = data;
   if (!sca) return;
   a = e_action_find(sca->action);
   if (!a) return;
   if (a) a->func.go(NULL, sca->params);
}

static void
_cb_signal_action_extra(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Sys_Con_Action *sca;
  
   e_syscon_hide();
   sca = data;
   if (!sca) return;
   if (sca->func) sca->func((void *)sca->data);
}

static int
_cb_timeout_defaction(void *data)
{
   deftimer = NULL;
   if (!do_defact) return 0;
   e_syscon_hide();
   _do_action_name(do_defact);
   return 0;
}
