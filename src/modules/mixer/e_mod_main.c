#include <e.h>
#include <Eina.h>
#include <Ecore.h>
#include "lib/common.h"
#include "lib/epulse.h"
#include "e_mod_main.h"
#ifdef HAVE_ENOTIFY
#include <E_Notify.h>
#endif

//~ #define VOLUME_STEP (PA_VOLUME_NORM / BASE_VOLUME_STEP)
#define VOLUME_STEP (PA_VOLUME_NORM / 100 * 5) // volume step set up to 5%

/* module requirements */
EAPI E_Module_Api e_modapi =
   {
      E_MODULE_API_VERSION,
      "Pulse Mixer"
   };

/* necessary forward declaration */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name,
                                 const char *id, const char *style);
static void             _gc_shutdown(E_Gadcon_Client *gcc);
static void             _gc_orient(E_Gadcon_Client *gcc,
                                   E_Gadcon_Orient orient EINA_UNUSED);
static const char      *_gc_label(const E_Gadcon_Client_Class *client_class);
static Evas_Object     *_gc_icon(const E_Gadcon_Client_Class *client_class,
                                 Evas *evas);
static const char      *_gc_id_new(const E_Gadcon_Client_Class *client_class);
static                 E_Module *epulse_module = NULL;


static const E_Gadcon_Client_Class _gadcon_class =
   {
      GADCON_CLIENT_CLASS_VERSION,
      "pulse_mixer",
      {
         _gc_init, _gc_shutdown,
         _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL,
         e_gadcon_site_is_not_toolbar
      },
      E_GADCON_CLIENT_STYLE_PLAIN
   };

typedef struct _Sink Sink;
struct _Sink {
   int index;
   pa_cvolume volume;
   int mute;
   char *name;
};

typedef struct _Context Context;
struct _Context
{
   char *theme;
   Ecore_Exe *epulse;
   Ecore_Event_Handler *disconnected_handler;
   Ecore_Event_Handler *epulse_event_handler;
   Ecore_Event_Handler *sink_default_handler;
   Ecore_Event_Handler *sink_changed_handler;
   Ecore_Event_Handler *sink_added_handler;
   Ecore_Event_Handler *sink_removed_handler;
   Sink *sink_default;
   E_Module *module;
   Eina_List *instances;
   Eina_List *sinks;
   E_Menu *menu;
   unsigned int notification_id;

   struct {
      E_Action *incr;
      E_Action *decr;
      E_Action *mute;
   } actions;
};

typedef struct _Instance Instance;
struct _Instance
{
   E_Gadcon_Client *gcc;

   E_Gadcon_Popup *popup;
   Evas_Object *gadget;
   Evas_Object *list;
   Evas_Object *slider;
   Evas_Object *check;

   int mute;

    struct
   {
      Evas_Object *gadget;
      Evas_Object *label;
      Evas_Object *left;
      Evas_Object *right;
      Evas_Object *mute;
      Evas_Object *table;
      Evas_Object *button;
      struct
      {
         Ecore_X_Window win;
         Ecore_Event_Handler *mouse_up;
         Ecore_Event_Handler *key_down;
         Ecore_Event_Handler *wheel;
      } input;
   } ui;

};

static void     _mixer_popup_input_window_create(Instance *inst);
static void     _mixer_popup_input_window_destroy(Instance *inst);
static Context  *mixer_context = NULL;

static void
_notify(const int val)
{
   if (val < 0)
     return;
#ifdef HAVE_ENOTIFY
   static E_Notification *n;
   char *icon, buf[56];
   int ret;

   ret = snprintf(buf, (sizeof(buf) - 1), "%s: %d%%", _("New volume"), val);
   if ((ret < 0) || ((unsigned int)ret > sizeof(buf)))
     return;
   //Names are taken from FDO icon naming scheme
   if (val == 0)
     icon = "audio-volume-muted";
   else if ((val > 33) && (val < 66))
     icon = "audio-volume-medium";
   else if (val < 34)
     icon = "audio-volume-low";
   else
     icon = "audio-volume-high";
   if (n) return;
   n = e_notification_full_new(_("EPulse"), 0, icon, _("Volume Changed"), buf, 2000);
   e_notification_replaces_id_set(n, EINA_TRUE);
   e_notification_send(n, NULL, NULL);
   e_notification_unref(n);
   n = NULL;
#endif
}

static void
_mixer_popup_update(Instance *inst, int mute, int vol)
{
   e_widget_check_checked_set(inst->check, mute);
   e_slider_value_set(inst->slider, vol);
}

static void _popup_del(Instance *inst);

static void
_mixer_gadget_update(void)
{
   Edje_Message_Int_Set *msg;
   Instance *inst;
   Eina_List*l;

   EINA_LIST_FOREACH(mixer_context->instances, l, inst)
     {
        msg = alloca(sizeof(Edje_Message_Int_Set) + (2 * sizeof(int)));
        msg->count = 3;

        if (!mixer_context->sink_default)
          {
             msg->val[0] = EINA_FALSE;
             msg->val[1] = 0;
             msg->val[2] = 0;
             if (inst->popup)
               _popup_del(inst);
          }
        else
          {
             pa_volume_t vol =
                pa_cvolume_avg(&mixer_context->sink_default->volume);
             msg->val[0] = mixer_context->sink_default->mute;
             msg->val[1] = PA_VOLUME_TO_INT(vol);
             msg->val[2] = msg->val[1];
             if (inst->popup)
               _mixer_popup_update(inst, mixer_context->sink_default->mute,
                                    msg->val[1]);
          }
        edje_object_message_send(inst->gadget, EDJE_MESSAGE_INT_SET, 0, msg);
        edje_object_signal_emit(inst->gadget, "e,action,volume,change", "e");
     }
}

static void
_volume_increase_cb(E_Object *obj EINA_UNUSED, const char *params EINA_UNUSED)
{
   EINA_SAFETY_ON_NULL_RETURN(mixer_context->sink_default);

   Sink *s = mixer_context->sink_default;
   pa_cvolume v = s->volume;
   pa_cvolume_inc(&v, VOLUME_STEP);
   epulse_sink_volume_set(s->index, v);
}

static void
_volume_decrease_cb(E_Object *obj EINA_UNUSED, const char *params EINA_UNUSED)
{
   EINA_SAFETY_ON_NULL_RETURN(mixer_context->sink_default);

   Sink *s = mixer_context->sink_default;
   pa_cvolume v = s->volume;
   pa_cvolume_dec(&v, VOLUME_STEP);

   epulse_sink_volume_set(s->index, v);
}

static void
_volume_mute_cb(E_Object *obj EINA_UNUSED, const char *params EINA_UNUSED)
{
   EINA_SAFETY_ON_NULL_RETURN(mixer_context->sink_default);

   Sink *s = mixer_context->sink_default;
   int mute = !s->mute;
   if (!epulse_sink_mute_set(s->index, mute))
     {
        WRN("Could not mute the sink: %d", s->index);
        return;
     }
}

static void
_actions_register(void)
{
   mixer_context->actions.incr = e_action_add("volume_increase");
   if (mixer_context->actions.incr)
     {
        mixer_context->actions.incr->func.go = _volume_increase_cb;
        e_action_predef_name_set("Mixer", _("Increase Volume"),
                                 "volume_increase", NULL, NULL, 0);
     }

   mixer_context->actions.decr = e_action_add("volume_decrease");
   if (mixer_context->actions.decr)
     {
        mixer_context->actions.decr->func.go = _volume_decrease_cb;
        e_action_predef_name_set("Mixer", _("Decrease Volume"),
                                 "volume_decrease", NULL, NULL, 0);
     }

   mixer_context->actions.mute = e_action_add("volume_mute");
   if (mixer_context->actions.mute)
     {
        mixer_context->actions.mute->func.go = _volume_mute_cb;
        e_action_predef_name_set("Mixer", _("Mute volume"), "volume_mute",
                                 NULL, NULL, 0);
     }

   e_managers_keys_ungrab();
   e_managers_keys_grab();
}

static void
_actions_unregister(void)
{
   if (mixer_context->actions.incr)
     {
        e_action_predef_name_del("Mixer", _("Volume Increase"));
        e_action_del("volume_increase");
        mixer_context->actions.incr = NULL;
     }

   if (mixer_context->actions.decr)
     {
        e_action_predef_name_del("Mixer", _("Volume Mute"));
        e_action_del("volume_decrease");
        mixer_context->actions.decr = NULL;
     }

   if (mixer_context->actions.mute)
     {
        e_action_predef_name_del("Mixer", _("Volume Mute"));
        e_action_del("volume_mute");
        mixer_context->actions.mute = NULL;
     }

   e_managers_keys_ungrab();
   e_managers_keys_grab();
}

static void
_popup_del(Instance *inst)
{
   inst->slider = NULL;
   inst->check = NULL;

   E_FN_DEL(e_object_del, inst->popup);
}

static void
_popup_del_cb(void *obj)
{
   _popup_del(e_object_data_get(obj));
}

static Eina_Bool
_epulse_del_cb(void *data EINA_UNUSED, int type EINA_UNUSED,
               void *info EINA_UNUSED)
{
   mixer_context->epulse = NULL;
   if (mixer_context->epulse_event_handler)
      ecore_event_handler_del(mixer_context->epulse_event_handler);
   mixer_context->epulse_event_handler = NULL;

   return EINA_TRUE;
}

static void
_epulse_exec_cb(void *data, void *data2 EINA_UNUSED)
{
   Instance *inst = data;

   _popup_del(inst);
   _mixer_popup_input_window_destroy(inst);
   if (mixer_context->epulse)
      return;

   mixer_context->epulse = e_util_exe_safe_run("pavucontrol", NULL);
   if (mixer_context->epulse_event_handler)
      ecore_event_handler_del(mixer_context->epulse_event_handler);
   mixer_context->epulse_event_handler =
      ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _epulse_del_cb, NULL);
}

static void
_check_changed_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                  void *event EINA_UNUSED)
{
   Sink *s = mixer_context->sink_default;
   s->mute = !s->mute;
   if (!epulse_sink_mute_set(s->index, s->mute))
     {
        WRN("Could not mute the sink: %d", s->index);
        s->mute = !s->mute;
        return;
     }

   _mixer_gadget_update();
}

static void
_slider_changed_cb(void *data EINA_UNUSED, Evas_Object *obj,
                   void *event EINA_UNUSED)
{
   int val;
   pa_volume_t v;
   Sink *s = mixer_context->sink_default;

   val = (int)e_slider_value_get(obj);
   v = INT_TO_PA_VOLUME(val);

   pa_cvolume_set(&s->volume, s->volume.channels, v);
   epulse_sink_volume_set(s->index, s->volume);
}

static Evas_Object *
_popup_add_slider(Instance *inst)
{
   pa_volume_t vol = pa_cvolume_avg(&mixer_context->sink_default->volume);
   int value = PA_VOLUME_TO_INT(vol);

   Evas_Object *slider = e_slider_add(inst->popup->win->evas);

   evas_object_show(slider);
   e_slider_orientation_set(slider, 1);
   e_slider_value_range_set(slider, 0.0, 100.0);
   e_slider_value_format_display_set(slider, NULL);
   evas_object_smart_callback_add(slider, "changed", _slider_changed_cb,
                                  NULL);

   e_slider_value_set(slider, value);
   return slider;
}

static void
_sink_selected_cb(void *data)
{
   Sink *s = data;

   mixer_context->sink_default = s;
   _mixer_gadget_update();
}

static void
_popup_new(Instance *inst)
{
   Evas_Object *button, *list;
   Evas *evas;
   Evas_Coord mw, mh;
   Sink *s;
   Eina_List *l;
   int pos = 0;

   EINA_SAFETY_ON_NULL_RETURN(mixer_context->sink_default);

   inst->popup = e_gadcon_popup_new(inst->gcc);
   evas = inst->popup->win->evas;

   list = e_widget_list_add(evas, 0, 0);

   inst->list = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_size_min_set(inst->list, 120, 100);
   e_widget_list_object_append(list, inst->list, 1, 1, 0.5);

   EINA_LIST_FOREACH(mixer_context->sinks, l, s)
     {
        e_widget_ilist_append_full(inst->list, NULL, NULL, s->name,
                                   _sink_selected_cb,
                                   s, NULL);
        if (mixer_context->sink_default == s)
           e_widget_ilist_selected_set(inst->list, pos);

        pos++;
     }

   inst->slider = _popup_add_slider(inst);
   e_widget_list_object_append(list, inst->slider, 1, 1, 0.5);

   inst->mute = mixer_context->sink_default->mute;
   inst->check = e_widget_check_add(evas, _("Mute"),
                                    &inst->mute);
   e_widget_list_object_append(list, inst->check, 1, 0, 0.5);
   evas_object_smart_callback_add(inst->check, "changed", _check_changed_cb,
                                  NULL);

   button = e_widget_button_add(evas, NULL, "preferences-system",
                                _epulse_exec_cb, inst, NULL);
   e_widget_list_object_append(list, button, 1, 0, 0.5);

   e_widget_size_min_get(list, &mw, &mh);
   if (mh < 208) mh = 208;
   e_widget_size_min_set(list, 208, mh);

   e_gadcon_popup_content_set(inst->popup, list);

   e_gadcon_popup_show(inst->popup);
   e_object_data_set(E_OBJECT(inst->popup), inst);
   E_OBJECT_DEL_SET(inst->popup, _popup_del_cb);
   _mixer_popup_input_window_create(inst);
}

static void
_menu_cb(void *data, E_Menu *menu EINA_UNUSED, E_Menu_Item *mi EINA_UNUSED)
{
   _epulse_exec_cb(data, NULL);
}

static void
_menu_new(Instance *inst, Evas_Event_Mouse_Down *ev)
{
   E_Zone *zone;
   E_Menu *m;
   E_Menu_Item *mi;
   int x, y;


   zone = e_util_zone_current_get(e_manager_current_get());

   m = e_menu_new();

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Advanced"));
   e_util_menu_item_theme_icon_set(mi, "configure");
   e_menu_item_callback_set(mi, _menu_cb, inst);

   m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y, NULL, NULL);
   e_menu_activate_mouse(m, zone, x + ev->output.x, y + ev->output.y,
                         1, 1, E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
   evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
                            EVAS_BUTTON_NONE, ev->timestamp, NULL);
}

static void
_mouse_down_cb(void *data, Evas *evas EINA_UNUSED,
               Evas_Object *obj EINA_UNUSED, void *event)
{
   Instance *inst = data;
   Evas_Event_Mouse_Down *ev = event;

   if (ev->button == 1)
     {
        if (!inst->popup)
          {
             _popup_new(inst);
          }
     }
   else if (ev->button == 2)
     {
        _volume_mute_cb(NULL, NULL);
     }
   else if (ev->button == 3)
     {
        _menu_new(inst, ev);
     }
}

static void
_mouse_wheel_cb(void *data EINA_UNUSED, Evas *evas EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED, void *event)
{
   Evas_Event_Mouse_Wheel *ev = event;

   if (ev->z > 0)
     _volume_decrease_cb(NULL, NULL);
   else if (ev->z < 0)
     _volume_increase_cb(NULL, NULL);
}

/*
 * Gadcon functions
 */
static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   E_Gadcon_Client *gcc;
   Instance *inst;

   inst = E_NEW(Instance, 1);

   inst->gadget = edje_object_add(gc->evas);
   /*edje_object_file_set(inst->gadget, mixer_context->theme,
                        "e/modules/mixer/main");*/
   e_theme_edje_object_set(inst->gadget, "base/theme/modules/mixer",
                           "e/modules/mixer/main");

   gcc = e_gadcon_client_new(gc, name, id, style, inst->gadget);
   gcc->data = inst;
   inst->gcc = gcc;

   evas_object_event_callback_add(inst->gadget, EVAS_CALLBACK_MOUSE_DOWN,
                                  _mouse_down_cb, inst);
   evas_object_event_callback_add(inst->gadget, EVAS_CALLBACK_MOUSE_WHEEL,
                                  _mouse_wheel_cb, inst);
   mixer_context->instances = eina_list_append(mixer_context->instances, inst);

   if (mixer_context->sink_default)
     _mixer_gadget_update();

   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

   inst = gcc->data;
   evas_object_del(inst->gadget);
   mixer_context->instances = eina_list_remove(mixer_context->instances, inst);
   free(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient EINA_UNUSED)
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *client_class EINA_UNUSED)
{
   return "Mixer";
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class EINA_UNUSED, Evas *evas)
{
   Evas_Object *o;
   char buf[4096] = { 0 };

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/mixer.edj",
            e_module_dir_get(mixer_context->module));
   edje_object_file_set(o, buf, "icon");

   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class EINA_UNUSED)
{
   return _gadcon_class.name;
}

static Eina_Bool
_sink_default_cb(void *data EINA_UNUSED, int type EINA_UNUSED,
                 void *info)
{
   Epulse_Event *ev = info;
   Eina_List *l;
   Sink *s;

   EINA_LIST_FOREACH(mixer_context->sinks, l, s)
     {
        if (s->index == ev->index)
          {
             mixer_context->sink_default = s;
             goto end;
          }
     }

   s = malloc(sizeof(*s));
   s->index = ev->index;
   s->volume = ev->volume;
   s->name = strdup(ev->name);
   s->mute = ev->mute;

   mixer_context->sinks = eina_list_append(mixer_context->sinks, s);
   mixer_context->sink_default = s;

 end:
   _mixer_gadget_update();
   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_sink_changed_cb(void *data EINA_UNUSED, int type EINA_UNUSED,
                 void *info)
{
   Epulse_Event *ev = info;
   Eina_List *l;
   Sink *s;
   Eina_Bool volume_changed;

   EINA_LIST_FOREACH(mixer_context->sinks, l, s)
     {
        if (ev->index == s->index)
          {
             volume_changed = (s->mute != ev->mute ||
                               !pa_cvolume_equal(&s->volume, &ev->volume))
                ? EINA_TRUE : EINA_FALSE;
             s->mute = ev->mute;
             s->volume = ev->volume;
             if (ev->index == mixer_context->sink_default->index)
               {
                  _mixer_gadget_update();
                  if (volume_changed)
                     _notify(s->mute ? 0 : PA_VOLUME_TO_INT(
                         pa_cvolume_avg(&mixer_context->sink_default->volume)));
               }
          }
     }

    return ECORE_CALLBACK_DONE;
 }

 static Eina_Bool
 _disconnected_cb(void *data EINA_UNUSED, int type EINA_UNUSED,
                  void *info EINA_UNUSED)
 {
    Sink *s;

    EINA_LIST_FREE(mixer_context->sinks, s)
      {
         free(s->name);
         free(s);
      }

    mixer_context->sinks = NULL;
    mixer_context->sink_default = NULL;
    _mixer_gadget_update();

    return ECORE_CALLBACK_DONE;
 }

 static Eina_Bool
 _sink_added_cb(void *data EINA_UNUSED, int type EINA_UNUSED,
                void *info)
 {
    Epulse_Event *ev = info;
    Eina_List *l;
    Sink *s;

    EINA_LIST_FOREACH(mixer_context->sinks, l, s)
       if (s->index == ev->index)
          return ECORE_CALLBACK_DONE;

    s = malloc(sizeof(*s));
    s->index = ev->index;
    s->volume = ev->volume;
    s->name = strdup(ev->name);
    s->mute = ev->mute;

    mixer_context->sinks = eina_list_append(mixer_context->sinks, s);
    return ECORE_CALLBACK_DONE;
 }

 static Eina_Bool
 _sink_removed_cb(void *data EINA_UNUSED, int type EINA_UNUSED,
                  void *info)
 {
    Epulse_Event *ev = info;
    Eina_List *l, *ll;
    Sink *s;
    Eina_Bool need_change_sink;

    need_change_sink = (ev->index == mixer_context->sink_default->index)
       ? EINA_TRUE : EINA_FALSE;

    EINA_LIST_FOREACH_SAFE(mixer_context->sinks, l, ll, s)
      {
         if (ev->index == s->index)
           {
              free(s->name);
              free(s);
              mixer_context->sinks =
                 eina_list_remove_list(mixer_context->sinks, l);
           }
      }

    if (need_change_sink)
      {
         s = mixer_context->sinks->data;
         mixer_context->sink_default = s;
         _mixer_gadget_update();
      }

    return ECORE_CALLBACK_DONE;
 }

 EAPI void *
 e_modapi_init(E_Module *m)
 {
    char buf[4096];

    epulse_module = m;
#ifdef HAVE_ENOTIFY
   e_notification_init();
#endif
    EINA_SAFETY_ON_FALSE_RETURN_VAL(epulse_common_init("epulse_mod"),
                                    NULL);
    EINA_SAFETY_ON_FALSE_RETURN_VAL(epulse_init() > 0, NULL);
    if (!mixer_context)
      {
         mixer_context = E_NEW(Context, 1);

         mixer_context->sink_default_handler =
            ecore_event_handler_add(SINK_DEFAULT, _sink_default_cb, NULL);
         mixer_context->sink_changed_handler =
            ecore_event_handler_add(SINK_CHANGED, _sink_changed_cb, NULL);
         mixer_context->sink_added_handler =
            ecore_event_handler_add(SINK_ADDED, _sink_added_cb, NULL);
         mixer_context->sink_removed_handler =
            ecore_event_handler_add(SINK_REMOVED, _sink_removed_cb, NULL);
         mixer_context->disconnected_handler =
            ecore_event_handler_add(DISCONNECTED, _disconnected_cb, NULL);
         mixer_context->module = m;
         snprintf(buf, sizeof(buf), "%s/e-module-mixer.edj",
                  e_module_dir_get(mixer_context->module));
         mixer_context->theme = strdup(buf);
      }
    e_gadcon_provider_register(&_gadcon_class);
    _actions_register();

    return m;
 }

 EAPI int
 e_modapi_shutdown(E_Module *m EINA_UNUSED)
 {
    Sink *s;
    epulse_module = NULL;

    _actions_unregister();
    e_gadcon_provider_unregister((const E_Gadcon_Client_Class *)&_gadcon_class);

    if (mixer_context)
      {
         if (mixer_context->theme)
            free(mixer_context->theme);

        ecore_event_handler_del(mixer_context->sink_default_handler);
        ecore_event_handler_del(mixer_context->sink_changed_handler);
        ecore_event_handler_del(mixer_context->sink_added_handler);
        ecore_event_handler_del(mixer_context->sink_removed_handler);

        EINA_LIST_FREE(mixer_context->sinks, s)
          {
             free(s->name);
             free(s);
          }

        E_FREE(mixer_context);
     }
#ifdef HAVE_ENOTIFY
   e_notification_shutdown();
#endif
   epulse_common_shutdown();
   epulse_shutdown();
   return 1;
}

EAPI int
e_modapi_save(E_Module *m EINA_UNUSED)
{
   return 1;
}


static void _mixer_popup_del(Instance *inst);

static Eina_Bool
_mixer_popup_input_window_mouse_up_cb(void *data, int type __UNUSED__, void *event)
{
   Ecore_Event_Mouse_Button *ev = event;
   Instance *inst = data;

   if (ev->window != inst->ui.input.win)
     return ECORE_CALLBACK_PASS_ON;

   _mixer_popup_del(inst);

   return ECORE_CALLBACK_PASS_ON;
}

static void
_mixer_popup_input_window_destroy(Instance *inst)
{
   e_grabinput_release(0, inst->ui.input.win);
   ecore_x_window_free(inst->ui.input.win);
   inst->ui.input.win = 0;

   ecore_event_handler_del(inst->ui.input.mouse_up);
   inst->ui.input.mouse_up = NULL;

   ecore_event_handler_del(inst->ui.input.key_down);
   inst->ui.input.key_down = NULL;

   ecore_event_handler_del(inst->ui.input.wheel);
   inst->ui.input.wheel = NULL;
}

static void
_mixer_popup_input_window_create(Instance *inst)
{
   Ecore_X_Window_Configure_Mask mask;
   Ecore_X_Window w, popup_w;
   E_Manager *man;

   man = e_manager_current_get();

   w = ecore_x_window_input_new(man->root, 0, 0, man->w, man->h);
   mask = (ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE |
           ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING);
   popup_w = inst->popup->win->evas_win;
   ecore_x_window_configure(w, mask, 0, 0, 0, 0, 0, popup_w,
                            ECORE_X_WINDOW_STACK_BELOW);
   ecore_x_window_show(w);

   inst->ui.input.mouse_up =
     ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP,
                             _mixer_popup_input_window_mouse_up_cb, inst);
   inst->ui.input.wheel =
     ecore_event_handler_add(ECORE_EVENT_MOUSE_WHEEL,
                             _mixer_popup_input_window_mouse_up_cb, inst);

     inst->ui.input.win = w;
   e_grabinput_get(0, 0, inst->ui.input.win);
}


static void
_mixer_popup_del(Instance *inst)
{
   _mixer_popup_input_window_destroy(inst);
   e_object_del(E_OBJECT(inst->popup));
   inst->ui.label = NULL;
   inst->ui.left = NULL;
   inst->ui.right = NULL;
   inst->ui.mute = NULL;
   inst->ui.table = NULL;
   inst->ui.button = NULL;
   inst->popup = NULL;
}
