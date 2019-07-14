#include "sinks_view.h"

#include "../lib/epulse.h"

#define SINKS_KEY "sinks.key"

struct Sink
{
   int index;
   pa_cvolume volume;
   const char *name;
   Eina_Bool mute;

   Elm_Object_Item *item;
   Eina_List *ports;
};

struct Sink_Port
{
   char *name;
   char *description;
   Eina_Bool active;
};

struct Sinks_View
{
   Evas_Object *self;
   Evas_Object *genlist;
   Elm_Genlist_Item_Class *itc;

   Eina_List *sinks;
   Ecore_Event_Handler *disconnected;
   Ecore_Event_Handler *sink_added;
   Ecore_Event_Handler *sink_changed;
   Ecore_Event_Handler *sink_removed;
};


static Eina_Bool
_disconnected_cb(void *data, int type EINA_UNUSED, void *info EINA_UNUSED)
{
   struct Sinks_View *sv = data;
   struct Sink *sink;

   EINA_LIST_FREE(sv->sinks, sink)
      elm_object_item_del(sink->item);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_sink_add_cb(void *data, int type EINA_UNUSED, void *info)
{
   struct Sinks_View *sv = data;
   struct Sink_Port *sp;
   Epulse_Event_Sink *ev = info;
   Port *port;
   Eina_List *l;
   struct Sink *sink = calloc(1, sizeof(struct Sink));
   EINA_SAFETY_ON_NULL_RETURN_VAL(sink, ECORE_CALLBACK_PASS_ON);

   sink->name = eina_stringshare_add(ev->base.name);
   sink->index = ev->base.index;
   sink->volume = ev->base.volume;
   sink->mute = ev->base.mute;

   EINA_LIST_FOREACH(ev->ports, l, port)
     {
        sp = calloc(1, sizeof(struct Sink_Port));
        sp->name = strdup(port->name);
        sp->description = strdup(port->description);
        if (port->active)
           sp->active = EINA_TRUE;
        sink->ports = eina_list_append(sink->ports, sp);
     }

   sv->sinks = eina_list_append(sv->sinks, sink);
   sink->item = elm_genlist_item_append(sv->genlist, sv->itc, sink, NULL,
                                        ELM_GENLIST_ITEM_NONE, NULL, sv);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_sink_removed_cb(void *data, int type EINA_UNUSED, void *info)
{
   struct Sinks_View *sv = data;
   Epulse_Event_Sink *ev = info;
   Eina_List *l, *ll;
   struct Sink *sink;

   EINA_LIST_FOREACH_SAFE(sv->sinks, l, ll, sink)
     {
        if (sink->index == ev->base.index)
          {
             sv->sinks = eina_list_remove_list(sv->sinks, l);
             elm_object_item_del(sink->item);
             break;
          }
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_sink_changed_cb(void *data, int type EINA_UNUSED, void *info)
{
   struct Sinks_View *sv = data;
   Epulse_Event_Sink *ev = info;
   Eina_List *l;
   struct Sink *sink;
   Evas_Object *item = NULL;

   EINA_LIST_FOREACH(sv->sinks, l, sink)
     {
        if (sink->index == ev->base.index)
          {
             pa_volume_t vol = pa_cvolume_avg(&ev->base.volume);

             item = elm_object_item_part_content_get(sink->item, "slider");
             sink->volume = ev->base.volume;
             if (item)
                elm_slider_value_set(item, PA_VOLUME_TO_INT(vol));

             item = elm_object_item_part_content_get(sink->item, "mute");
             sink->mute = ev->base.mute;
             if (item)
                elm_check_state_set(item, sink->mute);

             break;
          }
     }

   return ECORE_CALLBACK_DONE;
}

static void
_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED,
        void *event_info EINA_UNUSED)
{
   struct Sinks_View *sv = data;

   eina_list_free(sv->sinks);
   if (sv->sink_added)
     {
        ecore_event_handler_del(sv->sink_added);
        sv->sink_added = NULL;
     }
   if (sv->sink_changed)
     {
        ecore_event_handler_del(sv->sink_changed);
        sv->sink_changed = NULL;
     }
   if (sv->sink_removed)
     {
        ecore_event_handler_del(sv->sink_removed);
        sv->sink_removed = NULL;
     }
   if (sv->disconnected)
     {
        ecore_event_handler_del(sv->disconnected);
        sv->disconnected = NULL;
     }
}

static char *
_item_text_get(void *data, Evas_Object *obj EINA_UNUSED, const char *part)
{
   struct Sink *sink = data;

   if (!strcmp(part, "name"))
     {
        return strdup(sink->name);
     }

   return NULL;
}

static void
_item_del(void *data, Evas_Object *obj EINA_UNUSED)
{
   struct Sink *sink = data;
   struct Sink_Port *port;

   eina_stringshare_del(sink->name);
   EINA_LIST_FREE(sink->ports, port)
     {
        free(port->name);
        free(port->description);
        free(port);
     }
   free(sink);
}

static void
_volume_changed_cb(void *data, Evas_Object *o,
                   void *event_info EINA_UNUSED)
{
   struct Sink *sink = data;
   double val = elm_slider_value_get(o);
   pa_volume_t v = DB_TO_PA_VOLUME(val);

   pa_cvolume_set(&sink->volume, sink->volume.channels, v);

   epulse_sink_volume_set(sink->index, sink->volume);
}

static void
_mute_changed_cb(void *data, Evas_Object *o EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   struct Sink *sink = data;

   if (!epulse_sink_mute_set(sink->index, sink->mute))
     {
        ERR("Could not mute the sink: %d", sink->index);
        sink->mute = !sink->mute;
        return;
     }
}

static void
_port_selected_cb(void *data, Evas_Object *o,
                  void *event_info EINA_UNUSED)
{
   struct Sink *sink = data;
   Elm_Object_Item *item = event_info;
   struct Sink_Port *port = elm_object_item_data_get(item);

   if (!epulse_sink_port_set(sink->index, port->name))
      ERR("Could not change the port");
   else
      elm_object_text_set(o, port->description);
}

static Evas_Object *
_item_content_get(void *data, Evas_Object *obj, const char *part)
{
   Evas_Object *item = NULL;
   struct Sink *sink = data;

   if (!strcmp(part, "slider"))
     {
        pa_volume_t vol = pa_cvolume_avg(&sink->volume);
        item = elm_slider_add(obj);
        EINA_SAFETY_ON_NULL_RETURN_VAL(item, NULL);

        elm_slider_step_set(item, 1.0/BASE_VOLUME_STEP);
        elm_slider_unit_format_set(item, "%1.0f");
        elm_slider_indicator_format_set(item, "%1.0f");
        elm_slider_span_size_set(item, 120);
        elm_slider_min_max_set(item, 0.0, 100.0);
        elm_slider_value_set(item, PA_VOLUME_TO_INT(vol));
        evas_object_smart_callback_add(item, "delay,changed",
                                       _volume_changed_cb, sink);
     }
   else if (!strcmp(part, "mute"))
     {
        item = elm_check_add(obj);
        EINA_SAFETY_ON_NULL_RETURN_VAL(item, NULL);

        elm_object_style_set(item, "toggle");
        elm_object_translatable_part_text_set(item, "off", N_("Mute"));
        elm_object_translatable_part_text_set(item, "on", N_("Unmute"));

        elm_check_state_set(item, sink->mute);
        elm_check_state_pointer_set(item, &sink->mute);
        evas_object_smart_callback_add(item, "changed", _mute_changed_cb,
                                       sink);
     }
   else if (!strcmp(part, "hover"))
     {
        Eina_List *l;
        struct Sink_Port *sp;

        if (sink->ports)
           item = elm_hoversel_add(obj);

        EINA_LIST_FOREACH(sink->ports, l, sp)
          {
             elm_hoversel_item_add(item, sp->description, NULL,
                                   ELM_ICON_NONE, NULL, sp);
             if (sp->active)
                elm_object_text_set(item, sp->description);
          }
        evas_object_smart_callback_add(item, "selected",
                                       _port_selected_cb, sink);
     }

   return item;
}


Evas_Object *
sinks_view_add(Evas_Object *parent)
{
   Evas_Object *layout;
   struct Sinks_View *sv;

   sv = calloc(1, sizeof(struct Sinks_View));
   EINA_SAFETY_ON_NULL_RETURN_VAL(sv, NULL);

   layout = epulse_layout_add(parent, "sinks", "default");
   EINA_SAFETY_ON_NULL_GOTO(layout, err);

   evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL, _del_cb, sv);

   sv->genlist = elm_genlist_add(layout);
   EINA_SAFETY_ON_NULL_GOTO(sv->genlist, err_genlist);

   sv->disconnected = ecore_event_handler_add(DISCONNECTED,
                                              _disconnected_cb, sv);
   sv->sink_added = ecore_event_handler_add(SINK_ADDED, _sink_add_cb, sv);
   sv->sink_added = ecore_event_handler_add(SINK_CHANGED, _sink_changed_cb, sv);
   sv->sink_removed = ecore_event_handler_add(SINK_REMOVED,
                                              _sink_removed_cb, sv);

   sv->itc = elm_genlist_item_class_new();
   EINA_SAFETY_ON_NULL_GOTO(sv->itc, err_genlist);
   sv->itc->item_style = "sinks";
   sv->itc->func.text_get = _item_text_get;
   sv->itc->func.content_get = _item_content_get;
   sv->itc->func.del = _item_del;

   evas_object_data_set(layout, SINKS_KEY, sv);
   elm_layout_content_set(layout, "list", sv->genlist);

   evas_object_size_hint_weight_set(sv->genlist, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(sv->genlist, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);

   return layout;

 err_genlist:
   ecore_event_handler_del(sv->sink_added);
   ecore_event_handler_del(sv->sink_changed);
   ecore_event_handler_del(sv->sink_removed);
   free(layout);
 err:
   free(sv);

   return NULL;
}
