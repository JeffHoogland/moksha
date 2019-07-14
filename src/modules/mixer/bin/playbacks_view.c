#include "playbacks_view.h"

#include "../lib/epulse.h"

#define PLAYBACKS_KEY "playbacks.key"

struct Sink
{
   int index;
   const char *name;
};

struct Playbacks_View
{
   Evas_Object *self;
   Evas_Object *genlist;
   Elm_Genlist_Item_Class *itc;

   Eina_List *inputs;
   Eina_List *sinks;

   Ecore_Event_Handler *disconnected;
   Ecore_Event_Handler *sink_input_added;
   Ecore_Event_Handler *sink_input_changed;
   Ecore_Event_Handler *sink_input_removed;

   Ecore_Event_Handler *sink_added;
   Ecore_Event_Handler *sink_removed;
};

struct Sink_Input
{
   struct Playbacks_View *pv;

   int index;
   int sink_index;
   pa_cvolume volume;
   const char *name;
   const char *icon;
   Eina_Bool mute;

   Elm_Object_Item *item;
};

 static Eina_Bool
_disconnected_cb(void *data, int type EINA_UNUSED, void *info EINA_UNUSED)
{
   struct Playbacks_View *pv = data;
   struct Sink_Input *si;
   struct Sink *sink;

   EINA_LIST_FREE(pv->inputs, si)
      elm_object_item_del(si->item);

   EINA_LIST_FREE(pv->sinks, sink)
     {
        eina_stringshare_del(sink->name);
        free(sink);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_sink_input_add_cb(void *data, int type EINA_UNUSED,
                   void *info)
{
   struct Playbacks_View *pv = data;
   Epulse_Event_Sink_Input *ev = info;
   struct Sink_Input *input = calloc(1, sizeof(struct Sink_Input));
   EINA_SAFETY_ON_NULL_RETURN_VAL(input, ECORE_CALLBACK_PASS_ON);

   input->name = eina_stringshare_add(ev->base.name);
   input->icon = eina_stringshare_add(ev->icon);
   input->index = ev->base.index;
   input->sink_index = ev->sink;
   input->volume = ev->base.volume;
   input->mute = ev->base.mute;
   input->pv = pv;

   pv->inputs = eina_list_append(pv->inputs, input);
   input->item = elm_genlist_item_append(pv->genlist, pv->itc, input, NULL,
                                         ELM_GENLIST_ITEM_NONE, NULL, pv);

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_sink_input_removed_cb(void *data, int type EINA_UNUSED,
                       void *info)
{
   struct Playbacks_View *pv = data;
   Epulse_Event_Sink_Input *ev = info;
   Eina_List *l, *ll;
   struct Sink_Input *input;

   EINA_LIST_FOREACH_SAFE(pv->inputs, l, ll, input)
     {
        if (input->index == ev->base.index)
          {
             pv->inputs = eina_list_remove_list(pv->inputs, l);
             elm_object_item_del(input->item);
             break;
          }
     }

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_sink_input_changed_cb(void *data, int type EINA_UNUSED,
                       void *info)
{
   struct Playbacks_View *pv = data;
   Epulse_Event_Sink_Input *ev = info;
   Eina_List *l;
   struct Sink_Input *input;
   Evas_Object *item = NULL;

   EINA_LIST_FOREACH(pv->inputs, l, input)
     {
        if (input->index == ev->base.index)
          {
             pa_volume_t vol = pa_cvolume_avg(&ev->base.volume);

             item = elm_object_item_part_content_get(input->item, "item");
             input->volume = ev->base.volume;
             if (item)
                elm_slider_value_set(item, PA_VOLUME_TO_INT(vol));

             item = elm_object_item_part_content_get(input->item, "mute");
             input->mute = ev->base.mute;
             if (item)
               {
                  elm_check_state_set(item, input->mute);
               }
             break;
          }
     }

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_sink_add_cb(void *data, int type EINA_UNUSED,
             void *info)
{
   struct Playbacks_View *pv = data;
   Epulse_Event_Sink *ev = info;
   struct Sink *sink = calloc(1, sizeof(struct Sink));
   EINA_SAFETY_ON_NULL_RETURN_VAL(sink, ECORE_CALLBACK_PASS_ON);

   sink->name = eina_stringshare_add(ev->base.name);
   sink->index = ev->base.index;

   pv->sinks = eina_list_append(pv->sinks, sink);
   elm_genlist_realized_items_update(pv->genlist);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_sink_removed_cb(void *data, int type EINA_UNUSED,
                 void *info)
{
   struct Playbacks_View *pv = data;
   Epulse_Event_Sink *ev = info;
   Eina_List *l, *ll;
   struct Sink *sink;

   EINA_LIST_FOREACH_SAFE(pv->sinks, l, ll, sink)
     {
        if (sink->index == ev->base.index)
          {
             pv->sinks = eina_list_remove_list(pv->sinks, l);
             break;
          }
     }

   elm_genlist_realized_items_update(pv->genlist);
   return ECORE_CALLBACK_PASS_ON;
}

static void
_del_cb(void *data,
        Evas *e EINA_UNUSED,
        Evas_Object *o EINA_UNUSED,
        void *event_info EINA_UNUSED)
{
   struct Playbacks_View *pv = data;
   struct Sink *sink;
   eina_list_free(pv->inputs);

   EINA_LIST_FREE(pv->sinks, sink)
     {
        eina_stringshare_del(sink->name);
        free(sink);
     }

#define ECORE_EVENT_HANDLER_DEL(_handle)        \
   if (pv->_handle)                             \
     {                                          \
        ecore_event_handler_del(pv->_handle);   \
        pv->_handle = NULL;                     \
     }

   ECORE_EVENT_HANDLER_DEL(disconnected)
   ECORE_EVENT_HANDLER_DEL(sink_input_added)
   ECORE_EVENT_HANDLER_DEL(sink_input_changed)
   ECORE_EVENT_HANDLER_DEL(sink_input_removed)
   ECORE_EVENT_HANDLER_DEL(sink_added)
   ECORE_EVENT_HANDLER_DEL(sink_removed)
#undef ECORE_EVENT_HANDLER_DEL
}

static char *
_item_text_get(void *data, Evas_Object *obj EINA_UNUSED, const char *part)
{
   struct Sink_Input *input = data;

   if (!strcmp(part, "name"))
     {
        return strdup(input->name);
     }

   return NULL;
}

static void
_item_del(void *data, Evas_Object *obj EINA_UNUSED)
{
   struct Sink_Input *input = data;

   eina_stringshare_del(input->name);
   eina_stringshare_del(input->icon);
   free(input);
}

static void
_volume_changed_cb(void *data, Evas_Object *o,
                   void *event_info EINA_UNUSED)
{
   struct Sink_Input *input = data;
   double val = elm_slider_value_get(o);
   pa_volume_t v = DB_TO_PA_VOLUME(val);

   pa_cvolume_set(&input->volume, input->volume.channels, v);

   epulse_sink_input_volume_set(input->index, input->volume);
}

static void
_mute_changed_cb(void *data, Evas_Object *o EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   struct Sink_Input *input = data;

   if (!epulse_sink_input_mute_set(input->index, input->mute))
     {
        ERR("Could not mute the input: %d", input->index);
        input->mute = !input->mute;
        return;
     }
}

static void
_sink_selected(void *data, Evas_Object *obj, void *event_info)
{
   struct Sink_Input *input = data;
   Elm_Object_Item *item = event_info;
   struct Sink *sink = elm_object_item_data_get(item);

   epulse_sink_input_move(input->index, sink->index);
   elm_object_text_set(obj, sink->name);
}

static Evas_Object *
_item_content_get(void *data, Evas_Object *obj, const char *part)
{
   Evas_Object *item = NULL;
   struct Sink_Input *input = data;

   if (!strcmp(part, "slider"))
     {
        pa_volume_t vol = pa_cvolume_avg(&input->volume);
        item = elm_slider_add(obj);
        EINA_SAFETY_ON_NULL_RETURN_VAL(item, NULL);

        elm_slider_step_set(item, 1.0/BASE_VOLUME_STEP);
        elm_slider_unit_format_set(item, "%1.0f");
        elm_slider_indicator_format_set(item, "%1.0f");
        elm_slider_span_size_set(item, 120);
        elm_slider_min_max_set(item, 0.0, 100.0);
        elm_slider_value_set(item, PA_VOLUME_TO_INT(vol));

        evas_object_smart_callback_add(item, "delay,changed",
                                       _volume_changed_cb, input);
     }
   else if (!strcmp(part, "mute"))
     {
        item = elm_check_add(obj);
        EINA_SAFETY_ON_NULL_RETURN_VAL(item, NULL);

        elm_object_style_set(item, "toggle");
        elm_object_translatable_part_text_set(item, "off", N_("Mute"));
        elm_object_translatable_part_text_set(item, "on", N_("Unmute"));

        elm_check_state_set(item, input->mute);
        elm_check_state_pointer_set(item, &input->mute);
        evas_object_smart_callback_add(item, "changed", _mute_changed_cb,
                                       input);
     }
   else if (!strcmp(part, "icon"))
     {
        EINA_SAFETY_ON_NULL_RETURN_VAL(input->icon, NULL);

        item = elm_icon_add(obj);
        EINA_SAFETY_ON_NULL_RETURN_VAL(item, NULL);

        elm_icon_standard_set(item, input->icon);
     }
   else if (!strcmp(part, "hover"))
     {
        Eina_List *l;
        struct Sink *sink;
        item = elm_hoversel_add(obj);

        EINA_LIST_FOREACH(input->pv->sinks, l, sink)
          {
             if (sink->index == input->sink_index)
                elm_object_text_set(item, sink->name);
             elm_hoversel_item_add(item, sink->name, NULL,
                                   ELM_ICON_NONE, NULL, sink);
          }
        evas_object_smart_callback_add(item, "selected",
                                  _sink_selected, input);
     }

   return item;
}

Evas_Object *
playbacks_view_add(Evas_Object *parent)
{
   Evas_Object *layout;
   struct Playbacks_View *pv;

   pv = calloc(1, sizeof(struct Playbacks_View));
   EINA_SAFETY_ON_NULL_RETURN_VAL(pv, NULL);

   layout = epulse_layout_add(parent, "playbacks", "default");
   EINA_SAFETY_ON_NULL_GOTO(layout, err);

   evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL, _del_cb, pv);

   pv->genlist = elm_genlist_add(layout);
   EINA_SAFETY_ON_NULL_GOTO(pv->genlist, err_genlist);

   pv->disconnected = ecore_event_handler_add(DISCONNECTED,
                                              _disconnected_cb, pv);
   pv->sink_input_added = ecore_event_handler_add(SINK_INPUT_ADDED,
                                                  _sink_input_add_cb, pv);
   pv->sink_input_added = ecore_event_handler_add(SINK_INPUT_CHANGED,
                                                  _sink_input_changed_cb, pv);
   pv->sink_input_removed = ecore_event_handler_add(SINK_INPUT_REMOVED,
                                                    _sink_input_removed_cb, pv);
   pv->sink_added = ecore_event_handler_add(SINK_ADDED,
                                            _sink_add_cb, pv);
   pv->sink_removed = ecore_event_handler_add(SINK_REMOVED,
                                              _sink_removed_cb, pv);

   pv->itc = elm_genlist_item_class_new();
   EINA_SAFETY_ON_NULL_GOTO(pv->itc, err_genlist);
   pv->itc->item_style = "playbacks";
   pv->itc->func.text_get = _item_text_get;
   pv->itc->func.content_get = _item_content_get;
   pv->itc->func.del = _item_del;

   evas_object_data_set(layout, PLAYBACKS_KEY, pv);
   elm_layout_content_set(layout, "list", pv->genlist);

   evas_object_size_hint_weight_set(pv->genlist, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(pv->genlist, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);

   return layout;

 err_genlist:
   ecore_event_handler_del(pv->sink_input_added);
   ecore_event_handler_del(pv->sink_input_changed);
   ecore_event_handler_del(pv->sink_input_removed);
   ecore_event_handler_del(pv->sink_added);
   ecore_event_handler_del(pv->sink_removed);
   free(layout);
 err:
   free(pv);
   return NULL;
}
