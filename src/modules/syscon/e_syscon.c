#include "e.h"
#include "e_mod_main.h"

/* local subsystem functions */
static Eina_Bool _cb_key_down(void *data, int type, void *event);
static Eina_Bool _cb_mouse_down(void *data, int type, void *event);
static Eina_Bool _cb_mouse_up(void *data, int type, void *event);
static Eina_Bool _cb_mouse_move(void *data, int type, void *event);
static Eina_Bool _cb_mouse_wheel(void *data, int type, void *event);
static void      _cb_signal_close(void *data, Evas_Object *obj, const char *emission, const char *source);
static void      _cb_signal_syscon(void *data, Evas_Object *obj, const char *emission, const char *source);
static void      _cb_signal_action(void *data, Evas_Object *obj, const char *emission, const char *source);
static void      _cb_signal_action_extra(void *data, Evas_Object *obj, const char *emission, const char *source);
static Eina_Bool _cb_timeout_defaction(void *data);

/* local subsystem globals */
static E_Popup *popup = NULL;
static Ecore_X_Window input_window = 0;
static const char *do_defact = NULL;
static Eina_List *handlers = NULL;
static Evas_Object *o_bg = NULL;
static Evas_Object *o_flow_main = NULL;
static Evas_Object *o_flow_secondary = NULL;
static Evas_Object *o_flow_extra = NULL;
static Evas_Object *o_selected_flow = NULL;
static Evas_Object *o_selected = NULL;
static int inevas = 0;
static Ecore_Timer *deftimer = NULL;
static double show_time = 0.0;
static int act_count = 0;

/* externally accessible functions */
int
e_syscon_init(void)
{
   return 1;
}

int
e_syscon_shutdown(void)
{
   e_syscon_hide();
   return 1;
}

int
e_syscon_show(E_Zone *zone, const char *defact)
{
   Evas_Object *o, *o2;
   Evas_Coord mw, mh;
   int x, y, w, h, zx, zy, zw, zh;
   int iw, ih;
   Eina_List *l;
   double t;

   t = ecore_loop_time_get();
   if (popup)
     {
        if ((t - show_time) > 0.5)
          {
             for (l = e_config->syscon.actions; l; l = l->next)
               {
                  E_Config_Syscon_Action *sca;
                  E_Action *a;

                  if (!(sca = l->data)) continue;
                  if (!sca->action) continue;
                  a = e_action_find(sca->action);
                  if (!a) continue;
                  if (sca->is_main == 2)
                    {
                       a->func.go(NULL, sca->params);
                       e_syscon_hide();
                       break;
                    }
               }
          }
        return 0;
     }

   input_window = ecore_x_window_input_new(zone->container->win, zone->x,
                                           zone->y, zone->w, zone->h);
   ecore_x_window_show(input_window);
   if (!e_grabinput_get(input_window, 1, input_window))
     {
        ecore_x_window_free(input_window);
        input_window = 0;
        return 0;
     }

   popup = e_popup_new(zone, 0, 0, 1, 1);
   if (!popup)
     {
        e_grabinput_release(input_window, input_window);
        ecore_x_window_free(input_window);
        input_window = 0;
        return 0;
     }
   evas_event_freeze(popup->evas);
   e_popup_layer_set(popup, E_LAYER_POPUP);

   handlers = eina_list_append
       (handlers, ecore_event_handler_add
         (ECORE_EVENT_KEY_DOWN, _cb_key_down, NULL));
   handlers = eina_list_append
       (handlers, ecore_event_handler_add
         (ECORE_EVENT_MOUSE_BUTTON_DOWN, _cb_mouse_down, NULL));
   handlers = eina_list_append
       (handlers, ecore_event_handler_add
         (ECORE_EVENT_MOUSE_BUTTON_UP, _cb_mouse_up, NULL));
   handlers = eina_list_append
       (handlers, ecore_event_handler_add
         (ECORE_EVENT_MOUSE_MOVE, _cb_mouse_move, NULL));
   handlers = eina_list_append
       (handlers, ecore_event_handler_add
         (ECORE_EVENT_MOUSE_WHEEL, _cb_mouse_wheel, NULL));

   o = edje_object_add(popup->evas);
   o_bg = o;
   e_theme_edje_object_set(o, "base/theme/syscon",
                           "e/widgets/syscon/main");
   edje_object_part_text_set(o, "e.text.label", _("Cancel"));
   edje_object_signal_callback_add(o, "e,action,close", "",
                                   _cb_signal_close, NULL);
   edje_object_signal_callback_add(o, "e,action,syscon", "*",
                                   _cb_signal_syscon, NULL);

   act_count = 0;
   show_time = t;

   // main (default):
   //  halt | suspend | desk_lock
   // secondary (default):
   //  reboot | hibernate | logout
   // extra (example for illume):
   //  home | close | kill

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

        if (!(sca = l->data)) continue;
        if (!sca->action) continue;
        a = e_action_find(sca->action);
        if (!a) continue;
        disabled = 0;
        if ((!strcmp(sca->action, "logout")) &&
            (!e_sys_action_possible_get(E_SYS_LOGOUT))) disabled = 1;
        else if ((!strcmp(sca->action, "halt")) &&
                 (!e_sys_action_possible_get(E_SYS_HALT)))
          disabled = 1;
        else if ((!strcmp(sca->action, "halt_now")) &&
                 (!e_sys_action_possible_get(E_SYS_HALT_NOW)))
          disabled = 1;
        else if ((!strcmp(sca->action, "reboot")) &&
                 (!e_sys_action_possible_get(E_SYS_REBOOT)))
          disabled = 1;
        else if ((!strcmp(sca->action, "suspend")) &&
                 (!e_sys_action_possible_get(E_SYS_SUSPEND)))
          disabled = 1;
        else if ((!strcmp(sca->action, "hibernate")) &&
                 (!e_sys_action_possible_get(E_SYS_HIBERNATE)))
          disabled = 1;
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
                                  _(e_action_predef_label_get(sca->action, sca->params)));
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

   e_zone_useful_geometry_get(zone, &zx, &zy, &zw, &zh);
   evas_object_resize(o_bg, zw, zh);
   edje_object_calc_force(o_bg);

   e_flowlayout_size_min_get(o_flow_main, &mw, &mh);
   evas_object_size_hint_min_set(o_flow_main, mw, mh);
   edje_object_part_swallow(o_bg, "e.swallow.main", o_flow_main);
   e_flowlayout_size_min_get(o_flow_secondary, &mw, &mh);
   evas_object_size_hint_min_set(o_flow_secondary, mw, mh);
   edje_object_part_swallow(o_bg, "e.swallow.secondary", o_flow_secondary);
   e_flowlayout_size_min_get(o_flow_extra, &mw, &mh);
   evas_object_size_hint_min_set(o_flow_extra, mw, mh);
   edje_object_part_swallow(o_bg, "e.swallow.extra", o_flow_extra);

   edje_object_size_min_calc(o_bg, &mw, &mh);

   w = mw;
   if (w > zw) w = zw;
   x = zx - zone->x + (zw - w) / 2;
   h = mh;
   if (h > zh) h = zh;
   y = zy - zone->y + (zh - h) / 2;

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

void
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
   ecore_x_window_free(input_window);
   input_window = 0;
   o_selected_flow = o_selected = o_flow_extra = o_flow_main = o_flow_secondary = NULL;
}

/* local subsystem functions */
static Eina_Bool
_cb_key_down(__UNUSED__ void *data, __UNUSED__ int type, void *event)
{
   Ecore_Event_Key *ev;

   ev = event;
   if (ev->event_window != input_window) return ECORE_CALLBACK_PASS_ON;
   if (!strcmp(ev->key, "Escape"))
     e_syscon_hide();
   else if ((!strcmp(ev->key, "Left")) || (!strcmp(ev->key, "Up")))
     {
        if (!o_selected)
          {
             if (e_flowlayout_pack_count_get(o_flow_extra))
               o_selected_flow = o_flow_extra, o_selected = e_flowlayout_pack_object_last(o_flow_extra);
             else if (e_flowlayout_pack_count_get(o_flow_secondary))
               o_selected_flow = o_flow_secondary, o_selected = e_flowlayout_pack_object_last(o_flow_secondary);
             else
               o_selected_flow = o_flow_main, o_selected = e_flowlayout_pack_object_last(o_flow_main);
          }
        else
          {
             edje_object_signal_emit(o_selected, "e,state,focused", "e");
             o_selected = e_flowlayout_pack_object_prev(o_selected_flow, o_selected);
             if (!o_selected)
               {
                  if (o_selected_flow == o_flow_extra)
                    {
                       if (e_flowlayout_pack_count_get(o_flow_secondary))
                         o_selected_flow = o_flow_secondary, o_selected = e_flowlayout_pack_object_last(o_flow_secondary);
                       else
                         o_selected_flow = o_flow_main, o_selected = e_flowlayout_pack_object_last(o_flow_main);
                    }
                  else if (o_selected_flow == o_flow_secondary)
                    o_selected_flow = o_flow_main, o_selected = e_flowlayout_pack_object_last(o_flow_main);
                  else
                    {
                       if (e_flowlayout_pack_count_get(o_flow_extra))
                         o_selected_flow = o_flow_extra, o_selected = e_flowlayout_pack_object_last(o_flow_extra);
                       else if (e_flowlayout_pack_count_get(o_flow_secondary))
                         o_selected_flow = o_flow_secondary, o_selected = e_flowlayout_pack_object_last(o_flow_secondary);
                       else
                         o_selected_flow = o_flow_main, o_selected = e_flowlayout_pack_object_last(o_flow_main);
                    }
               }
          }
        edje_object_signal_emit(o_selected, "e,state,unfocused", "e");
     }
   else if ((!strcmp(ev->key, "Right")) || (!strcmp(ev->key, "Down")) || (!strcmp(ev->key, "Tab")))
     {
        if (!o_selected)
          o_selected_flow = o_flow_main, o_selected = e_flowlayout_pack_object_first(o_flow_main);
        else
          {
             edje_object_signal_emit(o_selected, "e,state,focused", "e");
             o_selected = e_flowlayout_pack_object_next(o_selected_flow, o_selected);
             if (!o_selected)
               {
                  if (o_selected_flow == o_flow_extra)
                    o_selected_flow = o_flow_main, o_selected = e_flowlayout_pack_object_first(o_flow_main);
                  else if (o_selected_flow == o_flow_secondary)
                    {
                       if (e_flowlayout_pack_count_get(o_flow_extra))
                         o_selected_flow = o_flow_extra, o_selected = e_flowlayout_pack_object_first(o_flow_extra);
                       else
                         o_selected_flow = o_flow_main, o_selected = e_flowlayout_pack_object_first(o_flow_main);
                    }
                  else
                    {
                       if (e_flowlayout_pack_count_get(o_flow_secondary))
                         o_selected_flow = o_flow_secondary, o_selected = e_flowlayout_pack_object_first(o_flow_secondary);
                       else if (e_flowlayout_pack_count_get(o_flow_extra))
                         o_selected_flow = o_flow_extra, o_selected = e_flowlayout_pack_object_first(o_flow_extra);
                       else
                         o_selected_flow = o_flow_main, o_selected = e_flowlayout_pack_object_first(o_flow_main);
                    }
               }
          }
        edje_object_signal_emit(o_selected, "e,state,unfocused", "e");
     }
   else if ((!strcmp(ev->keyname, "KP_Enter")) || (!strcmp(ev->keyname, "Return")))
     {
        if (!o_selected) return ECORE_CALLBACK_RENEW;
        edje_object_signal_emit(o_selected, "e,state,focused", "e");
        edje_object_signal_emit(o_selected, "e,action,click", "");
        o_selected = o_selected_flow = NULL;
     }
   else
     {
        E_Action *act;
        double t;

        t = ecore_loop_time_get();
        if (t - show_time > 0.5)
          {
             act = e_bindings_key_down_event_find(E_BINDING_CONTEXT_ANY, ev);
             if ((act) && (act->name))
               {
                  if (!strcmp(act->name, "syscon"))
                    {
                       if (popup)
                         {
                            e_syscon_show(popup->zone, do_defact);
                         }
                    }
                  else
                    {
                       Eina_List *l;

                       for (l = e_config->syscon.actions; l; l = l->next)
                         {
                            E_Config_Syscon_Action *sca;

                            if (!(sca = l->data)) continue;
                            if (!sca->action) continue;
                            if (!strcmp(sca->action, act->name))
                              {
                                 act_count++;
                                 if (act_count > 2)
                                   {
                                      act->func.go(NULL, sca->params);
                                      e_syscon_hide();
                                      break;
                                   }
                              }
                         }
                    }
               }
          }
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_mouse_down(__UNUSED__ void *data, __UNUSED__ int type, void *event)
{
   Ecore_Event_Mouse_Button *ev;
   Evas_Button_Flags flags = EVAS_BUTTON_NONE;

   ev = event;
   if (ev->event_window != input_window) return ECORE_CALLBACK_PASS_ON;
   if (ev->double_click) flags |= EVAS_BUTTON_DOUBLE_CLICK;
   if (ev->triple_click) flags |= EVAS_BUTTON_TRIPLE_CLICK;
   if ((ev->x < popup->x) || (ev->x >= (popup->x + popup->w)) ||
       (ev->y < popup->y) || (ev->y >= (popup->y + popup->h)))
     {
        e_syscon_hide();
        return ECORE_CALLBACK_PASS_ON;
     }
   evas_event_feed_mouse_down(popup->evas, ev->buttons, flags, ev->timestamp, NULL);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_mouse_up(__UNUSED__ void *data, __UNUSED__ int type, void *event)
{
   Ecore_Event_Mouse_Button *ev;

   ev = event;
   if (ev->event_window != input_window) return ECORE_CALLBACK_PASS_ON;
   evas_event_feed_mouse_up(popup->evas, ev->buttons, EVAS_BUTTON_NONE,
                            ev->timestamp, NULL);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_mouse_move(__UNUSED__ void *data, __UNUSED__ int type, void *event)
{
   Ecore_Event_Mouse_Move *ev;

   ev = event;
   if (ev->event_window != input_window) return ECORE_CALLBACK_PASS_ON;
   if (!inevas)
     {
        evas_event_feed_mouse_in(popup->evas, ev->timestamp, NULL);
        inevas = 1;
     }
   evas_event_feed_mouse_move(popup->evas, ev->x - popup->x, ev->y - popup->y,
                              ev->timestamp, NULL);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_mouse_wheel(__UNUSED__ void *data, __UNUSED__ int type, void *event)
{
   Ecore_Event_Mouse_Wheel *ev;

   ev = event;
   if (ev->event_window != input_window) return ECORE_CALLBACK_PASS_ON;
   evas_event_feed_mouse_wheel(popup->evas, ev->direction, ev->z,
                               ev->timestamp, NULL);
   return ECORE_CALLBACK_PASS_ON;
}

static void
_cb_signal_close(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
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
_cb_signal_syscon(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source)
{
   e_syscon_hide();
   _do_action_name(source);
}

static void
_cb_signal_action(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Config_Syscon_Action *sca;
   E_Action *a;

   e_syscon_hide();
   sca = data;
   if (!sca) return;
   a = e_action_find(sca->action);
   if (!a) return;
   a->func.go(NULL, sca->params);
}

static void
_cb_signal_action_extra(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Sys_Con_Action *sca;

   e_syscon_hide();
   sca = data;
   if (!sca) return;
   if (sca->func) sca->func((void *)sca->data);
}

static Eina_Bool
_cb_timeout_defaction(void *data __UNUSED__)
{
   const char *defact = NULL;
   deftimer = NULL;
   if (!do_defact) return ECORE_CALLBACK_CANCEL;
   defact = eina_stringshare_add(do_defact);
   e_syscon_hide();
   if (defact)
     {
        _do_action_name(defact);
        eina_stringshare_del(defact);
     }
   return ECORE_CALLBACK_CANCEL;
}

