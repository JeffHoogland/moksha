#include "epulse.h"

typedef struct _Epulse_Context Epulse_Context;
struct _Epulse_Context {
   pa_mainloop_api api;
   pa_context *context;
   pa_context_state_t state;
   void *data;
};

static unsigned int _init_count = 0;
static Epulse_Context *ctx = NULL;
extern pa_mainloop_api functable;

int SINK_ADDED = 0;
int SINK_CHANGED = 0;
int SINK_DEFAULT = 0;
int SINK_REMOVED = 0;
int SINK_INPUT_ADDED = 0;
int SINK_INPUT_CHANGED = 0;
int SINK_INPUT_REMOVED = 0;
int SOURCE_ADDED = 0;
int SOURCE_CHANGED = 0;
int SOURCE_REMOVED = 0;
int SOURCE_INPUT_ADDED = 0;
int SOURCE_INPUT_REMOVED = 0;
int DISCONNECTED = 0;

static void
_event_free_cb(void *user_data EINA_UNUSED, void *func_data)
{
   Epulse_Event *ev = func_data;

   if (ev->name)
      free(ev->name);

   free(ev);
}

static void
_event_sink_input_free_cb(void *user_data EINA_UNUSED, void *func_data)
{
   Epulse_Event_Sink_Input *ev = func_data;

   if (ev->base.name)
      free(ev->base.name);

   if (ev->icon)
      free(ev->icon);

   free(ev);
}

static void
_event_sink_free_cb(void *user_data EINA_UNUSED, void *func_data)
{
   Epulse_Event_Sink *ev = func_data;
   Port *port;

   if (ev->base.name)
      free(ev->base.name);

   EINA_LIST_FREE(ev->ports, port)
     {
        free(port->name);
        free(port->description);
        free(port);
     }

   free(ev);
}

static void
_sink_cb(pa_context *c EINA_UNUSED, const pa_sink_info *info, int eol,
         void *userdata EINA_UNUSED)
{
   Epulse_Event_Sink *ev;
   Port *port;
   uint32_t i;

   if (eol < 0)
     {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
           return;

        ERR("Sink callback failure");
        return;
     }

   if (eol > 0)
      return;

   DBG("sink index: %d\nsink name: %s", info->index,
       info->name);

   ev = calloc(1, sizeof(Epulse_Event_Sink));
   ev->base.index = info->index;
   ev->base.name = strdup(info->description);
   ev->base.volume = info->volume;
   ev->base.mute = !!info->mute;

   for (i = 0; i < info->n_ports; i++)
     {
        port = calloc(1, sizeof(Port));
        EINA_SAFETY_ON_NULL_GOTO(port, error);

        port->available = !!info->ports[i]->available;
        port->priority = info->ports[i]->priority;
        port->name = strdup(info->ports[i]->name);
        port->description =  strdup(info->ports[i]->description);
        ev->ports = eina_list_append(ev->ports, port);
        if (info->ports[i]->name == info->active_port->name)
           port->active = EINA_TRUE;
     }

   ecore_event_add(SINK_ADDED, ev, _event_sink_free_cb, NULL);
   return;

 error:
   _event_sink_free_cb(NULL, ev);
}

static void
_sink_changed_cb(pa_context *c EINA_UNUSED, const pa_sink_info *info, int eol,
                 void *userdata EINA_UNUSED)
{
   Epulse_Event_Sink *ev;
   Port *port;
   uint32_t i;

   if (eol < 0)
     {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
           return;

        ERR("Sink callback failure");
        return;
     }

   if (eol > 0)
      return;

   DBG("sink index: %d\nsink name: %s", info->index,
       info->name);

   ev = calloc(1, sizeof(Epulse_Event_Sink));
   ev->base.index = info->index;
   ev->base.name = strdup(info->name);
   ev->base.volume = info->volume;
   ev->base.mute = !!info->mute;

   for (i = 0; i < info->n_ports; i++)
     {
        port = calloc(1, sizeof(Port));
        EINA_SAFETY_ON_NULL_GOTO(port, error);

        port->priority = info->ports[i]->priority;
        port->available = !!info->ports[i]->available;
        port->name = strdup(info->ports[i]->description ?:
                            info->ports[i]->name);
        ev->ports = eina_list_append(ev->ports, port);
        if (info->ports[i]->name == info->active_port->name)
           port->active = EINA_TRUE;
     }

   ecore_event_add(SINK_CHANGED, ev, _event_sink_free_cb, NULL);
   return;

 error:
   _event_sink_free_cb(NULL, ev);
}

static void
_sink_remove_cb(int index, void *data EINA_UNUSED)
{
   Epulse_Event_Sink *ev;
   DBG("Removing sink: %d", index);

   ev = calloc(1, sizeof(Epulse_Event_Sink));
   ev->base.index = index;

   ecore_event_add(SINK_REMOVED, ev, _event_sink_free_cb, NULL);
}

static const char *
_icon_from_properties(pa_proplist *l)
{
   const char *t;

   if ((t = pa_proplist_gets(l, PA_PROP_MEDIA_ICON_NAME)))
      return t;

   if ((t = pa_proplist_gets(l, PA_PROP_WINDOW_ICON_NAME)))
      return t;

   if ((t = pa_proplist_gets(l, PA_PROP_APPLICATION_ICON_NAME)))
      return t;

   if ((t = pa_proplist_gets(l, PA_PROP_MEDIA_ROLE)))
     {

        if (strcmp(t, "video") == 0 ||
            strcmp(t, "phone") == 0)
           return t;

        if (strcmp(t, "music") == 0)
           return "audio";

        if (strcmp(t, "game") == 0)
           return "applications-games";

        if (strcmp(t, "event") == 0)
           return "dialog-information";
     }

   return "audio-card";
}

static void
_sink_input_cb(pa_context *c EINA_UNUSED, const pa_sink_input_info *info,
               int eol, void *userdata EINA_UNUSED)
{
   Epulse_Event_Sink_Input *ev;

   if (eol < 0)
     {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
           return;

        ERR("Sink input callback failure");
        return;
     }

   if (eol > 0)
      return;

   DBG("sink input index: %d\nsink input name: %s", info->index,
       info->name);

   ev = calloc(1, sizeof(Epulse_Event_Sink_Input));
   ev->base.index = info->index;
   ev->base.name = strdup(info->name);
   ev->base.volume = info->volume;
   ev->base.mute = !!info->mute;
   ev->sink = info->sink;
   ev->icon = strdup(_icon_from_properties(info->proplist));

   ecore_event_add(SINK_INPUT_ADDED, ev, _event_sink_input_free_cb, NULL);
}

static void
_sink_input_changed_cb(pa_context *c EINA_UNUSED,
                       const pa_sink_input_info *info, int eol,
                       void *userdata EINA_UNUSED)
{
   Epulse_Event_Sink_Input *ev;

   if (eol < 0)
     {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
           return;

        ERR("Sink input changed callback failure");
        return;
     }

   if (eol > 0)
      return;

   DBG("sink input changed index: %d\n", info->index);

   ev = calloc(1, sizeof(Epulse_Event_Sink_Input));
   ev->base.index = info->index;
   ev->base.volume = info->volume;
   ev->base.mute = !!info->mute;

   ecore_event_add(SINK_INPUT_CHANGED, ev, _event_sink_input_free_cb,
                   NULL);
}

static void
_sink_input_remove_cb(int index, void *data EINA_UNUSED)
{
   Epulse_Event_Sink_Input *ev;

   DBG("Removing sink input: %d", index);

   ev = calloc(1, sizeof(Epulse_Event_Sink_Input));
   ev->base.index = index;

   ecore_event_add(SINK_INPUT_REMOVED, ev, _event_sink_input_free_cb, NULL);
}

static void
_source_cb(pa_context *c EINA_UNUSED, const pa_source_info *info,
           int eol, void *userdata EINA_UNUSED)
{
   Epulse_Event *ev;

   if (eol < 0)
     {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
           return;

        ERR("Source callback failure");
        return;
     }

   if (eol > 0)
      return;

   ev = calloc(1, sizeof(Epulse_Event));
   ev->index = info->index;
   ev->name = strdup(info->name);
   ev->volume = info->volume;
   ev->mute = !!info->mute;

   ecore_event_add(SOURCE_ADDED, ev, _event_free_cb, NULL);
}

static void
_source_changed_cb(pa_context *c EINA_UNUSED,
                       const pa_source_info *info, int eol,
                       void *userdata EINA_UNUSED)
{
   Epulse_Event *ev;

   if (eol < 0)
     {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
           return;

        ERR("Source changed callback failure");
        return;
     }

   if (eol > 0)
      return;

   DBG("source changed index: %d\n", info->index);

   ev = calloc(1, sizeof(Epulse_Event));
   ev->index = info->index;
   ev->volume = info->volume;
   ev->mute = !!info->mute;

   ecore_event_add(SOURCE_CHANGED, ev, _event_free_cb,
                   NULL);
}

static void
_source_remove_cb(int index, void *data EINA_UNUSED)
{
   Epulse_Event *ev;

   DBG("Removing source: %d", index);

   ev = calloc(1, sizeof(Epulse_Event));
   ev->index = index;

   ecore_event_add(SOURCE_REMOVED, ev, _event_free_cb, NULL);
}

static void
_sink_default_cb(pa_context *c EINA_UNUSED, const pa_sink_info *info, int eol,
                 void *userdata EINA_UNUSED)
{
   Epulse_Event_Sink *ev;

   if (eol < 0)
     {
        if (pa_context_errno(c) == PA_ERR_NOENTITY)
           return;

        ERR("Sink callback failure");
        return;
     }

   if (eol > 0)
      return;

   DBG("sink index: %d\nsink name: %s", info->index,
       info->name);

   ev = calloc(1, sizeof(Epulse_Event_Sink));
   ev->base.index = info->index;
   ev->base.name = strdup(info->description);
   ev->base.volume = info->volume;
   ev->base.mute = !!info->mute;

   ecore_event_add(SINK_DEFAULT, ev, _event_sink_free_cb, NULL);
}

static void
_server_info_cb(pa_context *c, const pa_server_info *info,
           void *userdata)
{
   pa_operation *o;

   if (!(o = pa_context_get_sink_info_by_name(c, info->default_sink_name,
                                              _sink_default_cb, userdata)))
     {
        ERR("pa_context_get_sink_info_by_name() failed");
        return;
     }
   pa_operation_unref(o);
}

static void
_subscribe_cb(pa_context *c, pa_subscription_event_type_t t,
              uint32_t index, void *data)
{
   pa_operation *o;

   switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
    case PA_SUBSCRIPTION_EVENT_SINK:
       if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) ==
           PA_SUBSCRIPTION_EVENT_REMOVE)
          _sink_remove_cb(index, data);
       else if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) ==
                PA_SUBSCRIPTION_EVENT_NEW)
         {
            if (!(o = pa_context_get_sink_info_by_index(c, index,
                                                        _sink_cb, data)))
              {
                 ERR("pa_context_get_sink_info_by_index() failed");
                 return;
              }
            pa_operation_unref(o);
         }
       else
         {
            if (!(o = pa_context_get_sink_info_by_index(c, index,
                                                        _sink_changed_cb,
                                                        data)))
              {
                 ERR("pa_context_get_sink_info_by_index() failed");
                 return;
              }
            pa_operation_unref(o);
         }
       break;

    case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
       if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) ==
           PA_SUBSCRIPTION_EVENT_REMOVE)
          _sink_input_remove_cb(index, data);
       else if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) ==
                PA_SUBSCRIPTION_EVENT_NEW)
         {
            if (!(o = pa_context_get_sink_input_info(c, index,
                                                     _sink_input_cb, data)))
              {
                 ERR("pa_context_get_sink_input_info() failed");
                 return;
              }
            pa_operation_unref(o);
         }
       else
         {
            if (!(o = pa_context_get_sink_input_info(c, index,
                                                     _sink_input_changed_cb,
                                                     data)))
              {
                 ERR("pa_context_get_sink_input_info() failed");
                 return;
              }
            pa_operation_unref(o);
         }
       break;

    case PA_SUBSCRIPTION_EVENT_SOURCE:
       if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) ==
           PA_SUBSCRIPTION_EVENT_REMOVE)
          _source_remove_cb(index, data);
       else if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) ==
                PA_SUBSCRIPTION_EVENT_NEW)
         {
            if (!(o = pa_context_get_source_info_by_index(c, index,
                                                          _source_cb, data)))
              {
                 ERR("pa_context_get_source_info() failed");
                 return;
              }
            pa_operation_unref(o);
         }
       else
         {
            if (!(o = pa_context_get_source_info_by_index(c, index,
                                                          _source_changed_cb,
                                                          data)))
              {
                 ERR("pa_context_get_source_info() failed");
                 return;
              }
            pa_operation_unref(o);
         }
       break;
    
    // Ignore these events to suppress unneeded warnings
    case  PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT:
       break;
    case PA_SUBSCRIPTION_EVENT_CLIENT:
       break;

    default:
       WRN("Event not handled %d %d", t, t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK);
       break;
   }
}

static Eina_Bool _epulse_connect(void *data);

static void
_epulse_pa_state_cb(pa_context *context, void *data)
{
   pa_operation *o;

   switch (pa_context_get_state(context))
     {
      case PA_CONTEXT_UNCONNECTED:
      case PA_CONTEXT_CONNECTING:
      case PA_CONTEXT_AUTHORIZING:
      case PA_CONTEXT_SETTING_NAME:
         break;

      case PA_CONTEXT_READY:
         {
            pa_context_set_subscribe_callback(context, _subscribe_cb, ctx);
            if (!(o = pa_context_subscribe(context, (pa_subscription_mask_t)
                                           (PA_SUBSCRIPTION_MASK_SINK|
                                            PA_SUBSCRIPTION_MASK_SOURCE|
                                            PA_SUBSCRIPTION_MASK_SINK_INPUT|
                                            PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT|
                                            PA_SUBSCRIPTION_MASK_CLIENT|
                                            PA_SUBSCRIPTION_MASK_SERVER|
                                            PA_SUBSCRIPTION_MASK_CARD),
                                           NULL, NULL)))
              {
                 ERR("pa_context_subscribe() failed");
                 return;
              }
            pa_operation_unref(o);

            if (!(o = pa_context_get_sink_info_list(context, _sink_cb, ctx)))
              {
                 ERR("pa_context_get_sink_info_list() failed");
                 return;
              }
            pa_operation_unref(o);

            if (!(o = pa_context_get_sink_input_info_list(context,
                                                          _sink_input_cb,
                                                          ctx)))
              {
                 ERR("pa_context_get_sink_input_info_list() failed");
                 return;
              }
            pa_operation_unref(o);

            if (!(o = pa_context_get_source_info_list(context, _source_cb,
                                                      ctx)))
              {
                 ERR("pa_context_get_source_info_list() failed");
                 return;
              }
            pa_operation_unref(o);

            if (!(o = pa_context_get_server_info(context, _server_info_cb,
                                                 ctx)))
              {
                 ERR("pa_context_get_server_info() failed");
                 return;
              }
            pa_operation_unref(o);
            break;
         }

      case PA_CONTEXT_FAILED:
         WRN("PA_CONTEXT_FAILED");
         ecore_event_add(DISCONNECTED, NULL, NULL, NULL);
         _epulse_connect(data);
         return;

      case PA_CONTEXT_TERMINATED:
         ERR("PA_CONTEXT_TERMINATE:");
         // fall through
      default:
         exit(0);
         return;
     }
}

static Eina_Bool
_epulse_connect(void *data)
{
   pa_proplist *proplist;
   Epulse_Context *c = data;

   proplist = pa_proplist_new();
   pa_proplist_sets(proplist, PA_PROP_APPLICATION_NAME, "Efl Volume Control");
   pa_proplist_sets(proplist, PA_PROP_APPLICATION_ID,
                    "org.enlightenment.volumecontrol");
   pa_proplist_sets(proplist, PA_PROP_APPLICATION_ICON_NAME, "audio-card");
   c->context = pa_context_new_with_proplist(&(c->api), NULL, proplist);
   if (!c->context)
     {
        WRN("Could not create the pulseaudio context");
        goto err;
     }

   pa_context_set_state_callback(c->context, _epulse_pa_state_cb, c);
   if (pa_context_connect(c->context, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0)
     {
        WRN("Could not connect to pulse");
        goto err;
     }

   pa_proplist_free(proplist);
   return ECORE_CALLBACK_DONE;

 err:
   pa_proplist_free(proplist);
   return ECORE_CALLBACK_RENEW;
}

int
epulse_init(void)
{
   if (_init_count > 0)
      goto end;

   ctx = calloc(1, sizeof(Epulse_Context));
   if (!ctx)
     {
        ERR("Could not create Epulse Context");
        return 0;
     }

   DISCONNECTED = ecore_event_type_new();
   SINK_ADDED = ecore_event_type_new();
   SINK_CHANGED = ecore_event_type_new();
   SINK_DEFAULT = ecore_event_type_new();
   SINK_REMOVED = ecore_event_type_new();
   SINK_INPUT_ADDED = ecore_event_type_new();
   SINK_INPUT_CHANGED = ecore_event_type_new();
   SINK_INPUT_REMOVED = ecore_event_type_new();
   SOURCE_ADDED = ecore_event_type_new();
   SOURCE_CHANGED = ecore_event_type_new();
   SOURCE_REMOVED = ecore_event_type_new();
   SOURCE_INPUT_ADDED = ecore_event_type_new();
   SOURCE_INPUT_REMOVED = ecore_event_type_new();

   ctx->api = functable;
   ctx->api.userdata = ctx;

   /* The reason of compares with EINA_TRUE is because ECORE_CALLBACK_RENEW 
      is EINA_TRUE. The function _epulse_connect returns ECORE_CALLBACK_RENEW
      when could not connect to pulse.
   */
   if (_epulse_connect(ctx) == EINA_TRUE)
      goto err;

 end:
   _init_count++;
   return _init_count;

 err:
   free(ctx);
   return 0;
}

void
epulse_shutdown(void)
{
   if (_init_count == 0)
      return;

   _init_count--;
   if (_init_count > 0)
      return;

   pa_context_unref(ctx->context);
   free(ctx);
   ctx = NULL;
}

Eina_Bool
epulse_source_volume_set(int index, pa_cvolume volume)
{
   pa_operation* o;
   EINA_SAFETY_ON_FALSE_RETURN_VAL((ctx && ctx->context), EINA_FALSE);

   if (!(o = pa_context_set_source_volume_by_index(ctx->context,
                                                   index, &volume, NULL, NULL)))
     {
        ERR("pa_context_set_source_volume_by_index() failed");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

Eina_Bool
epulse_source_mute_set(int index, Eina_Bool mute)
{
   pa_operation* o;
   EINA_SAFETY_ON_FALSE_RETURN_VAL((ctx && ctx->context), EINA_FALSE);

   if (!(o = pa_context_set_source_mute_by_index(ctx->context,
                                                 index, mute, NULL, NULL)))
     {
        ERR("pa_context_set_source_mute() failed");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

Eina_Bool
epulse_sink_volume_set(int index, pa_cvolume volume)
{
   pa_operation* o;
   EINA_SAFETY_ON_FALSE_RETURN_VAL((ctx && ctx->context), EINA_FALSE);

   if (!(o = pa_context_set_sink_volume_by_index(ctx->context,
                                                 index, &volume, NULL, NULL)))
     {
        ERR("pa_context_set_sink_volume_by_index() failed");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

Eina_Bool
epulse_sink_mute_set(int index, Eina_Bool mute)
{
   pa_operation* o;
   EINA_SAFETY_ON_FALSE_RETURN_VAL((ctx && ctx->context), EINA_FALSE);

   if (!(o = pa_context_set_sink_mute_by_index(ctx->context,
                                               index, mute, NULL, NULL)))
     {
        ERR("pa_context_set_sink_mute() failed");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

Eina_Bool
epulse_sink_input_volume_set(int index, pa_cvolume volume)
{
   pa_operation* o;
   EINA_SAFETY_ON_FALSE_RETURN_VAL((ctx && ctx->context), EINA_FALSE);

   if (!(o = pa_context_set_sink_input_volume(ctx->context,
                                              index, &volume, NULL, NULL)))
     {
        ERR("pa_context_set_sink_input_volume_by_index() failed");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

Eina_Bool
epulse_sink_input_mute_set(int index, Eina_Bool mute)
{
   pa_operation* o;
   EINA_SAFETY_ON_FALSE_RETURN_VAL((ctx && ctx->context), EINA_FALSE);

   if (!(o = pa_context_set_sink_input_mute(ctx->context,
                                            index, mute, NULL, NULL)))
     {
        ERR("pa_context_set_sink_input_mute() failed");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

Eina_Bool
epulse_sink_input_move(int index, int sink_index)
{
   pa_operation* o;
   EINA_SAFETY_ON_FALSE_RETURN_VAL((ctx && ctx->context), EINA_FALSE);

   if (!(o = pa_context_move_sink_input_by_index(ctx->context,
                                                 index, sink_index, NULL,
                                                 NULL)))
     {
        ERR("pa_context_move_sink_input_by_index() failed");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

Eina_Bool
epulse_sink_port_set(int index, const char *port)
{
   pa_operation* o;
   EINA_SAFETY_ON_FALSE_RETURN_VAL((ctx && ctx->context), EINA_FALSE);

   if (!(o = pa_context_set_sink_port_by_index(ctx->context,
                                               index, port, NULL,
                                               NULL)))
     {
        ERR("pa_context_set_source_port_by_index() failed");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}
