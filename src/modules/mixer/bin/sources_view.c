#include "sources_view.h"

#include "../lib/epulse.h"

#define SOURCES_KEY "sources.key"

struct Source
{
   int index;
   pa_cvolume volume;
   const char *name;
   Eina_Bool mute;

   Elm_Object_Item *item;
};

struct Sources_View
{
   Evas_Object *self;
   Evas_Object *genlist;
   Elm_Genlist_Item_Class *itc;

   Eina_List *sources;
   Ecore_Event_Handler *disconnected;
   Ecore_Event_Handler *source_added;
   Ecore_Event_Handler *source_changed;
   Ecore_Event_Handler *source_removed;
};

static Eina_Bool
_disconnected_cb(void *data, int type EINA_UNUSED, void *info EINA_UNUSED)
{
   struct Sources_View *sv = data;
   struct Source *source;

   EINA_LIST_FREE(sv->sources, source)
      elm_object_item_del(source->item);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_source_add_cb(void *data, int type EINA_UNUSED, void *info)
{
   struct Sources_View *sv = data;
   Epulse_Event *ev = info;
   struct Source *source = calloc(1, sizeof(struct Source));
   EINA_SAFETY_ON_NULL_RETURN_VAL(source, ECORE_CALLBACK_PASS_ON);

   source->name = eina_stringshare_add(ev->name);
   source->index = ev->index;
   source->volume = ev->volume;
   source->mute = ev->mute;

   sv->sources = eina_list_append(sv->sources, source);
   source->item = elm_genlist_item_append(sv->genlist, sv->itc, source, NULL,
                                          ELM_GENLIST_ITEM_NONE, NULL, sv);

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_source_removed_cb(void *data, int type EINA_UNUSED, void *info)
{
   struct Sources_View *sv = data;
   Epulse_Event *ev = info;
   Eina_List *l, *ll;
   struct Source *source;

   EINA_LIST_FOREACH_SAFE(sv->sources, l, ll, source)
     {
        if (source->index == ev->index)
          {
             sv->sources = eina_list_remove_list(sv->sources, l);
             elm_object_item_del(source->item);
             break;
          }
     }

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_source_changed_cb(void *data, int type EINA_UNUSED, void *info)
{
   struct Sources_View *sv = data;
   Epulse_Event *ev = info;
   Eina_List *l;
   struct Source *source;
   Evas_Object *item = NULL;

   EINA_LIST_FOREACH(sv->sources, l, source)
     {
        if (source->index == ev->index)
          {
             pa_volume_t vol = pa_cvolume_avg(&ev->volume);

             item = elm_object_item_part_content_get(source->item, "slider");
             source->volume = ev->volume;
             if (item)
                elm_slider_value_set(item, PA_VOLUME_TO_INT(vol));

             item = elm_object_item_part_content_get(source->item, "mute");
             source->mute = ev->mute;
             if (item)
               {
                  elm_check_state_set(item, source->mute);
               }
             break;
          }
     }

   return ECORE_CALLBACK_DONE;
}

static void
_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED,
        void *event_info EINA_UNUSED)
{
   struct Sources_View *sv = data;

   eina_list_free(sv->sources);
   if (sv->disconnected)
     {
        ecore_event_handler_del(sv->disconnected);
        sv->disconnected = NULL;
     }
   if (sv->source_added)
     {
        ecore_event_handler_del(sv->source_added);
        sv->source_added = NULL;
     }
   if (sv->source_changed)
     {
        ecore_event_handler_del(sv->source_changed);
        sv->source_changed = NULL;
     }
   if (sv->source_removed)
     {
        ecore_event_handler_del(sv->source_removed);
        sv->source_removed = NULL;
     }
}

static char *
_item_text_get(void *data, Evas_Object *obj EINA_UNUSED, const char *part)
{
   struct Source *source = data;

   if (!strcmp(part, "name"))
     {
        return strdup(source->name);
     }

   return NULL;
}

static void
_item_del(void *data, Evas_Object *obj EINA_UNUSED)
{
   struct Source *source = data;

   eina_stringshare_del(source->name);
   free(source);
}

static void
_volume_changed_cb(void *data, Evas_Object *o,
                   void *event_info EINA_UNUSED)
{
   struct Source *source = data;
   double val = elm_slider_value_get(o);
   pa_volume_t v = DB_TO_PA_VOLUME(val);

   pa_cvolume_set(&source->volume, source->volume.channels, v);

   epulse_source_volume_set(source->index, source->volume);
}

static void
_mute_changed_cb(void *data, Evas_Object *o EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   struct Source *source = data;

   if (!epulse_source_mute_set(source->index, source->mute))
     {
        ERR("Could not mute the source: %d", source->index);
        source->mute = !source->mute;
        return;
     }
}

static Evas_Object *
_item_content_get(void *data, Evas_Object *obj, const char *part)
{
   Evas_Object *item = NULL;
   struct Source *source = data;

   if (!strcmp(part, "slider"))
     {
        pa_volume_t vol = pa_cvolume_avg(&source->volume);
        item = elm_slider_add(obj);
        EINA_SAFETY_ON_NULL_RETURN_VAL(item, NULL);

        elm_slider_step_set(item, 1.0/BASE_VOLUME_STEP);
        elm_slider_unit_format_set(item, "%1.0f");
        elm_slider_indicator_format_set(item, "%1.0f");
        elm_slider_span_size_set(item, 120);
        elm_slider_min_max_set(item, 0.0, 100.0);
        elm_slider_value_set(item, PA_VOLUME_TO_INT(vol));
        evas_object_smart_callback_add(item, "delay,changed",
                                       _volume_changed_cb, source);
     }
   else if (!strcmp(part, "mute"))
     {
        item = elm_check_add(obj);
        EINA_SAFETY_ON_NULL_RETURN_VAL(item, NULL);

        elm_object_style_set(item, "toggle");
        elm_object_part_text_set(item, "off", "Mute");
        elm_object_part_text_set(item, "on", "Unmute");

        elm_check_state_set(item, source->mute);
        elm_check_state_pointer_set(item, &source->mute);
        evas_object_smart_callback_add(item, "changed", _mute_changed_cb,
                                       source);
     }

   return item;
}

Evas_Object *
sources_view_add(Evas_Object *parent)
{
   Evas_Object *layout;
   struct Sources_View *sv;

   sv = calloc(1, sizeof(struct Sources_View));
   EINA_SAFETY_ON_NULL_RETURN_VAL(sv, NULL);

   layout = epulse_layout_add(parent, "sources", "default");
   EINA_SAFETY_ON_NULL_GOTO(layout, err);

   evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL, _del_cb, sv);

   sv->genlist = elm_genlist_add(layout);
   EINA_SAFETY_ON_NULL_GOTO(sv->genlist, err_genlist);

   sv->disconnected = ecore_event_handler_add(DISCONNECTED,
                                              _disconnected_cb, sv);
   sv->source_added = ecore_event_handler_add(SOURCE_ADDED, _source_add_cb, sv);
   sv->source_added = ecore_event_handler_add(SOURCE_CHANGED,
                                              _source_changed_cb, sv);
   sv->source_removed = ecore_event_handler_add(SOURCE_REMOVED,
                                              _source_removed_cb, sv);

   sv->itc = elm_genlist_item_class_new();
   EINA_SAFETY_ON_NULL_GOTO(sv->itc, err_genlist);
   sv->itc->item_style = "sources";
   sv->itc->func.text_get = _item_text_get;
   sv->itc->func.content_get = _item_content_get;
   sv->itc->func.del = _item_del;

   evas_object_data_set(layout, SOURCES_KEY, sv);
   elm_layout_content_set(layout, "list", sv->genlist);

   evas_object_size_hint_weight_set(sv->genlist, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(sv->genlist, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);

   return layout;

 err_genlist:
   ecore_event_handler_del(sv->disconnected);
   ecore_event_handler_del(sv->source_added);
   ecore_event_handler_del(sv->source_changed);
   ecore_event_handler_del(sv->source_removed);
   free(layout);
 err:
   free(sv);

   return NULL;
}
