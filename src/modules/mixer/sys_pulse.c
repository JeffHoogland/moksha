#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "e_mod_system.h"
#include "e_mod_main.h"
#include <Ecore.h>
#include "Pulse.h"

#define PULSE_BUS "org.PulseAudio.Core1"
#define PULSE_PATH "/org/pulseaudio/core1"
#define PULSE_INTERFACE "org.PulseAudio.Core1"

static Pulse *conn = NULL;
static Ecore_Event_Handler *ph = NULL;
static Ecore_Event_Handler *pch = NULL;
static Ecore_Event_Handler *pdh = NULL;
static Eina_List *sinks = NULL;

static E_DBus_Connection *dbus = NULL;
static E_DBus_Signal_Handler *dbus_handler = NULL;
static double last_disc = 0;

static void
_dbus_poll(void *data   __UNUSED__,
           DBusMessage *msg)
{
   DBusError err;
   const char *name, *from, *to;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err,
                              DBUS_TYPE_STRING, &name,
                              DBUS_TYPE_STRING, &from,
                              DBUS_TYPE_STRING, &to,
                              DBUS_TYPE_INVALID))
     dbus_error_free(&err);

   //printf("name: %s\nfrom: %s\nto: %s\n", name, from, to);
   if ((name) && !strcmp(name, PULSE_BUS))
     e_mixer_pulse_init();
   if (dbus_handler)
     {
        e_dbus_signal_handler_del(dbus, dbus_handler);
        dbus_handler = NULL;
     }
   if (dbus)
     {
        e_dbus_connection_close(dbus);
        dbus = NULL;
        e_dbus_shutdown();
     }
}

static void
_dbus_test(void *data       __UNUSED__,
           DBusMessage *msg __UNUSED__,
           DBusError       *error)
{
   if ((error) && (dbus_error_is_set(error)))
     {
        dbus_error_free(error);
        if (dbus_handler)
          {
             e_dbus_signal_handler_del(dbus, dbus_handler);
             dbus_handler = NULL;
          }
        if (dbus)
          {
             e_dbus_connection_close(dbus);
             dbus = NULL;
             e_dbus_shutdown();
          }
        e_mod_mixer_pulse_ready(EINA_FALSE);
        return;
     }
}

static Eina_Bool
_pulse_update(Pulse *d __UNUSED__, int type __UNUSED__, Pulse_Sink *ev __UNUSED__)
{
   e_mod_mixer_pulse_update();
   return EINA_TRUE;
}

static void
_pulse_sinks_get(Pulse *p __UNUSED__, Pulse_Tag_Id id __UNUSED__, Eina_List *ev)
{
   Eina_List *l;
   Pulse_Sink *sink;
   sinks = ev;
   EINA_LIST_FOREACH(ev, l, sink)
     {
        printf("Sink:\n");
        printf("\tname: %s\n", pulse_sink_name_get(sink));
        printf("\tidx: %"PRIu32"\n", pulse_sink_idx_get(sink));
        printf("\tdesc: %s\n", pulse_sink_desc_get(sink));
        printf("\tchannels: %u\n", pulse_sink_channels_count(sink));
        printf("\tmuted: %s\n", pulse_sink_muted_get(sink) ? "yes" : "no");
        printf("\tavg: %g\n", pulse_sink_avg_get_pct(sink));
        printf("\tbalance: %f\n", pulse_sink_balance_get(sink));
     }
   pulse_sinks_watch(conn);
   e_mod_mixer_pulse_ready(EINA_TRUE);
}

static Eina_Bool
_pulse_connected(Pulse *d, int type __UNUSED__, Pulse *ev)
{
   Pulse_Tag_Id id;
   if (d != ev) return ECORE_CALLBACK_PASS_ON;
   id = pulse_sinks_get(conn);
   if (!id)
     {
        e_mixer_pulse_shutdown();
        e_mixer_default_setup();
        return ECORE_CALLBACK_RENEW;
     }
   pulse_cb_set(conn, id, (Pulse_Cb)_pulse_sinks_get);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_pulse_disconnected(Pulse *d, int type __UNUSED__, Pulse *ev)
{
   Pulse_Sink *sink;

   if (d != ev) return ECORE_CALLBACK_PASS_ON;

   EINA_LIST_FREE(sinks, sink)
     pulse_sink_free(sink);

   if (last_disc && (ecore_time_unix_get() - last_disc < 1))
     {
        e_mixer_pulse_shutdown();
        last_disc = 0;
        e_mod_mixer_pulse_ready(EINA_FALSE);
     }
   else
     {
        pulse_connect(conn);
        last_disc = ecore_time_unix_get();
     }
   return ECORE_CALLBACK_RENEW;
}

static Pulse_Sink *
_pulse_sink_find(const char *name)
{
   Eina_List *l;
   Pulse_Sink *sink;
   EINA_LIST_FOREACH(sinks, l, sink)
     {
        const char *sink_name;

        sink_name = pulse_sink_name_get(sink);
        if ((sink_name == name) || (!strcmp(sink_name, name)))
          return sink;
     }
   return NULL;
}

static void
_pulse_result_cb(Pulse *p __UNUSED__, Pulse_Tag_Id id, void *ev)
{
   if (!ev) fprintf(stderr, "Command %u failed!\n", id);
}

Eina_Bool
e_mixer_pulse_ready(void)
{
   return !!sinks;
}

Eina_Bool
e_mixer_pulse_init(void)
{
   pulse_init();
   conn = pulse_new();
   EINA_SAFETY_ON_NULL_GOTO(conn, error);
   if (dbus) goto error;
   if (!pulse_connect(conn))
     {
        DBusMessage *msg;

        e_dbus_init();
        dbus = e_dbus_bus_get(DBUS_BUS_SESSION);

        if (!dbus)
          {
             e_dbus_shutdown();
             return EINA_FALSE;
          }
        if (!dbus_handler)
          dbus_handler = e_dbus_signal_handler_add(dbus,
                                                   E_DBUS_FDO_BUS, E_DBUS_FDO_PATH,
                                                   E_DBUS_FDO_INTERFACE,
                                                   "NameOwnerChanged", (E_DBus_Signal_Cb)_dbus_poll, NULL);

        msg = dbus_message_new_method_call(PULSE_BUS, PULSE_PATH, PULSE_INTERFACE, "suuuuuup");
        e_dbus_method_call_send(dbus, msg, NULL, (E_DBus_Callback_Func)_dbus_test, NULL, -1, NULL); /* test for not running pulse */
        dbus_message_unref(msg);
        pulse_free(conn);
        conn = NULL;
        pulse_shutdown();
        return EINA_TRUE;
     }
   ph = ecore_event_handler_add(PULSE_EVENT_CONNECTED, (Ecore_Event_Handler_Cb)_pulse_connected, conn);
   pch = ecore_event_handler_add(PULSE_EVENT_CHANGE, (Ecore_Event_Handler_Cb)_pulse_update, conn);
   pdh = ecore_event_handler_add(PULSE_EVENT_DISCONNECTED, (Ecore_Event_Handler_Cb)_pulse_disconnected, conn);
   return EINA_TRUE;
error:
   pulse_free(conn);
   conn = NULL;
   pulse_shutdown();
   return EINA_FALSE;
}

void
e_mixer_pulse_shutdown(void)
{
   Pulse_Sink *sink;
   EINA_LIST_FREE(sinks, sink)
     pulse_sink_free(sink);

   pulse_free(conn);
   conn = NULL;
   ecore_event_handler_del(ph);
   ph = NULL;
   ecore_event_handler_del(pch);
   pch = NULL;
   ecore_event_handler_del(pdh);
   pdh = NULL;
   if (dbus_handler)
     {
        e_dbus_signal_handler_del(dbus, dbus_handler);
        dbus_handler = NULL;
     }
   if (dbus)
     {
        e_dbus_connection_close(dbus);
        dbus = NULL;
        e_dbus_shutdown();
     }
   pulse_shutdown();
}

E_Mixer_System *
e_mixer_pulse_new(const char *name)
{
   return (E_Mixer_System*)_pulse_sink_find(name);
}

void
e_mixer_pulse_del(E_Mixer_System *self __UNUSED__)
{
}

Eina_List *
e_mixer_pulse_get_cards(void)
{
   Eina_List *l, *ret = NULL;
   Pulse_Sink *sink;

   EINA_LIST_FOREACH(sinks, l, sink)
     ret = eina_list_append(ret, pulse_sink_name_get(sink));
   return ret;
}

void
e_mixer_pulse_free_cards(Eina_List *cards)
{
   eina_list_free(cards);
}

const char *
e_mixer_pulse_get_default_card(void)
{
   if (eina_list_data_get(sinks))
     return eina_stringshare_ref(pulse_sink_name_get(eina_list_last(sinks)->data));
   return NULL;
}

const char *
e_mixer_pulse_get_card_name(const char *card)
{
   Pulse_Sink *sink;
   sink = _pulse_sink_find(card);
   return eina_stringshare_add(pulse_sink_name_get(sink));
}

Eina_List *
e_mixer_pulse_get_channels(E_Mixer_System *self)
{
   uintptr_t id;
   Eina_List *ret = NULL;

   for (id=0; id < pulse_sink_channels_count((void*)self); id++)
     ret = eina_list_append(ret, (void*)id);
   return ret;
}

void
e_mixer_pulse_free_channels(Eina_List *channels)
{
   eina_list_free(channels);
}

Eina_List *
e_mixer_pulse_get_channels_names(E_Mixer_System *self)
{
   return pulse_sink_channel_names_get((void*)self);;
}

void
e_mixer_pulse_free_channels_names(Eina_List *channels_names)
{
   const char *str;
   EINA_LIST_FREE(channels_names, str)
     eina_stringshare_del(str);
}

const char *
e_mixer_pulse_get_default_channel_name(E_Mixer_System *self __UNUSED__)
{
   return eina_stringshare_add("Front");
}

E_Mixer_Channel *
e_mixer_pulse_get_channel_by_name(E_Mixer_System *self, const char *name)
{
   unsigned int x;
   x = pulse_sink_channel_name_get_id((void*)self, name);
   if (x == UINT_MAX) return (intptr_t*)-2;
   return (uintptr_t*)((uintptr_t)x);
}

void
e_mixer_pulse_channel_del(E_Mixer_Channel *channel __UNUSED__)
{
}

const char *
e_mixer_pulse_get_channel_name(E_Mixer_System *self, E_Mixer_Channel *channel)
{
   if (channel == (E_Mixer_Channel *)-2) return NULL;
   return pulse_sink_channel_id_get_name((void*)self, (uintptr_t)channel);
}

int
e_mixer_pulse_get_volume(E_Mixer_System *self, E_Mixer_Channel *channel, int *left, int *right)
{
   double volume;

   volume = pulse_sink_channel_volume_get((void*)self, (uintptr_t)channel);
   if (left) *left = (int)volume;
   if (right) *right = (int)volume;

   return 1;
}

int
e_mixer_pulse_set_volume(E_Mixer_System *self, E_Mixer_Channel *channel, int left, int right)
{
   uint32_t id;

   id = pulse_sink_channel_volume_set(conn, (void*)self, (uintptr_t)channel, (left + right) / 2);
   if (!id) return 0;
   pulse_cb_set(conn, id, (Pulse_Cb)_pulse_result_cb);
   return 1;
}

int
e_mixer_pulse_can_mute(E_Mixer_System *self __UNUSED__, E_Mixer_Channel *channel __UNUSED__)
{
   return 1;
}

int
e_mixer_pulse_get_mute(E_Mixer_System *self, E_Mixer_Channel *channel __UNUSED__, int *mute)
{
   if (mute) *mute = pulse_sink_muted_get((void*)self);

   return 1;
}

int
e_mixer_pulse_set_mute(E_Mixer_System *self, E_Mixer_Channel *channel __UNUSED__, int mute)
{
   uint32_t idx, id;

   idx = pulse_sink_idx_get((void*)self);
   id = pulse_sink_mute_set(conn, pulse_sink_idx_get((void*)self), mute);
   if (!id) return 0;
   pulse_cb_set(conn, id, (Pulse_Cb)_pulse_result_cb);
   return 1;
}

int
e_mixer_pulse_get_state(E_Mixer_System *self, E_Mixer_Channel *channel, E_Mixer_Channel_State *state)
{
   double vol;
   
   if (!state) return 0;

   vol = pulse_sink_channel_volume_get((void*)self, (uintptr_t)channel);
   state->mute = pulse_sink_muted_get((void*)self);
   state->left = state->right = (int)vol;
   
   return 1;
}

int
e_mixer_pulse_set_state(E_Mixer_System *self, E_Mixer_Channel *channel, const E_Mixer_Channel_State *state)
{
   uint32_t id;

   id = pulse_sink_channel_volume_set(conn, (void*)self, (uintptr_t)channel, (state->left + state->right) / 2);
   if (!id) return 0;
   pulse_cb_set(conn, id, (Pulse_Cb)_pulse_result_cb);
   return 1;
}

int
e_mixer_pulse_has_capture(E_Mixer_System *self __UNUSED__, E_Mixer_Channel *channel __UNUSED__)
{
   return 0;
}
