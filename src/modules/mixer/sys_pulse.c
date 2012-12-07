#include "e_mod_main.h"
#include "Pulse.h"


#define PULSE_BUS       "org.PulseAudio.Core1"
#define PULSE_PATH      "/org/pulseaudio/core1"
#define PULSE_INTERFACE "org.PulseAudio.Core1"

static Pulse *conn = NULL;
static Pulse_Server_Info *info = NULL;
static Pulse_Sink *default_sink = NULL;
static Ecore_Event_Handler *ph = NULL;
static Ecore_Event_Handler *pch = NULL;
static Ecore_Event_Handler *pdh = NULL;
static Eina_List *sinks = NULL;
static Eina_List *sources = NULL;
static Ecore_Poller *pulse_poller = NULL;
static Eina_Hash *queue_states = NULL;

static E_DBus_Connection *dbus = NULL;
static E_DBus_Signal_Handler *dbus_handler = NULL;
static Ecore_Timer *disc_timer = NULL;

static unsigned int disc_count = 0;
static unsigned int update_count = 0;
static Ecore_Timer *update_timer = NULL;

static Eina_Bool
_pulse_poller_cb(void *d __UNUSED__)
{
   char buf[4096];

   snprintf(buf, sizeof(buf), "%s/.pulse-cookie", getenv("HOME"));
   if (ecore_file_exists(buf))
     return !e_mixer_pulse_init();
   return EINA_TRUE;
}

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
           DBusError *error)
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

static void
_pulse_info_get(Pulse *d __UNUSED__, int type __UNUSED__, Pulse_Server_Info *ev)
{
   Eina_List *l;
   Pulse_Sink *sink;

   pulse_server_info_free(info);
   info = ev;
   EINA_LIST_FOREACH(sinks, l, sink)
     if (ev->default_sink == pulse_sink_name_get(sink))
       {
          if (default_sink == sink) return;
          default_sink = sink;
          if (!_mixer_using_default) e_mod_mixer_pulse_update();
          break;
       }
   e_mod_mixer_pulse_ready(EINA_TRUE);
}

static Eina_Bool
_pulse_update_timer(void *d EINA_UNUSED)
{
   e_mod_mixer_pulse_update();
   update_timer = NULL;
   return EINA_FALSE;
}

static Eina_Bool
_pulse_update(Pulse *d __UNUSED__, int type __UNUSED__, Pulse_Sink *ev __UNUSED__)
{
   Pulse_Tag_Id id;

   id = pulse_server_info_get(conn);
   if (id)
     pulse_cb_set(conn, id, (Pulse_Cb)_pulse_info_get);
   if (update_timer) ecore_timer_reset(update_timer);
   else update_timer = ecore_timer_add(0.2, _pulse_update_timer, NULL);
   return EINA_TRUE;
}

static void
_pulse_sinks_get(Pulse *p __UNUSED__, Pulse_Tag_Id id __UNUSED__, Eina_List *ev)
{
   Eina_List *l;
   Pulse_Sink *sink;

   E_FREE_LIST(sinks, pulse_sink_free);

   EINA_LIST_FOREACH(ev, l, sink)
     {
/*
        printf("Sink:\n");
        printf("\tname: %s\n", pulse_sink_name_get(sink));
        printf("\tidx: %"PRIu32"\n", pulse_sink_idx_get(sink));
        printf("\tdesc: %s\n", pulse_sink_desc_get(sink));
        printf("\tchannels: %u\n", pulse_sink_channels_count(sink));
        printf("\tmuted: %s\n", pulse_sink_muted_get(sink) ? "yes" : "no");
        printf("\tavg: %g\n", pulse_sink_avg_get_pct(sink));
        printf("\tbalance: %f\n", pulse_sink_balance_get(sink));
*/
        if (info && (!default_sink))
          {
             if (info->default_sink == pulse_sink_name_get(sink))
               {
                  default_sink = sink;
                  break;
               }
          }
     }

   sinks = ev;
   pulse_sinks_watch(conn);
   if (default_sink) e_mod_mixer_pulse_ready(EINA_TRUE);
}

static void
_pulse_sources_get(Pulse *p __UNUSED__, Pulse_Tag_Id id __UNUSED__, Eina_List *ev __UNUSED__)
{
/*
   Eina_List *l;
   Pulse_Sink *sink;
   sources = ev;

   EINA_LIST_FOREACH(ev, l, sink)
     {
        printf("Sources:\n");
        printf("\tname: %s\n", pulse_sink_name_get(sink));
        printf("\tidx: %"PRIu32"\n", pulse_sink_idx_get(sink));
        printf("\tdesc: %s\n", pulse_sink_desc_get(sink));
        printf("\tchannels: %u\n", pulse_sink_channels_count(sink));
        printf("\tmuted: %s\n", pulse_sink_muted_get(sink) ? "yes" : "no");
        printf("\tavg: %g\n", pulse_sink_avg_get_pct(sink));
        printf("\tbalance: %f\n", pulse_sink_balance_get(sink));
     }
 */
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
   if (!queue_states)
     queue_states = eina_hash_stringshared_new(free);
   pulse_cb_set(conn, id, (Pulse_Cb)_pulse_sinks_get);
   id = pulse_sources_get(conn);
   if (id)
     pulse_cb_set(conn, id, (Pulse_Cb)_pulse_sources_get);
   id = pulse_server_info_get(conn);
   if (id)
     pulse_cb_set(conn, id, (Pulse_Cb)_pulse_info_get);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_pulse_disc_timer(void *d __UNUSED__)
{
   disc_timer = NULL;
   if (disc_count < 5)
     {
        if (pulse_connect(conn)) return EINA_FALSE;
     }
   e_mod_mixer_pulse_ready(EINA_FALSE);
   e_mixer_pulse_shutdown();
   e_mixer_pulse_init();
   disc_count = 0;
   return EINA_FALSE;
}

static Eina_Bool
_pulse_disconnected(Pulse *d, int type __UNUSED__, Pulse *ev)
{
   Pulse_Sink *sink;

   if (d != ev) return ECORE_CALLBACK_PASS_ON;

   EINA_LIST_FREE(sinks, sink)
     pulse_sink_free(sink);
   EINA_LIST_FREE(sources, sink)
     pulse_sink_free(sink);
   pulse_server_info_free(info);
   if (queue_states) eina_hash_free(queue_states);
   queue_states = NULL;
   info = NULL;
   default_sink = NULL;
   if (update_timer) ecore_timer_del(update_timer);
   update_timer = NULL;

//   printf("PULSEAUDIO: disconnected at %g\n", ecore_time_unix_get());

   disc_count++;
   if (disc_timer) return ECORE_CALLBACK_RENEW;
   disc_timer = ecore_timer_add(1.5, _pulse_disc_timer, NULL);
   return ECORE_CALLBACK_RENEW;
}

static void
_pulse_state_queue(Pulse_Sink *sink, int left, int right, int mute)
{
   E_Mixer_Channel_State *state = NULL;

   if (queue_states)
     state = eina_hash_find(queue_states, pulse_sink_name_get(sink));
   else
     queue_states = eina_hash_stringshared_new(free);
   if (!state)
     {
        state = E_NEW(E_Mixer_Channel_State, 1);
        eina_hash_direct_add(queue_states, pulse_sink_name_get(sink), state);
        state->left = state->right = state->mute = -1;
     }
   if (left >= 0)
     state->left = left;
   if (right >= 0)
     state->right = right;
   if (mute >= 0)
     state->mute = mute;
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
   EINA_LIST_FOREACH(sources, l, sink)
     {
        const char *sink_name;

        sink_name = pulse_sink_name_get(sink);
        if ((sink_name == name) || (!strcmp(sink_name, name)))
          return sink;
     }
   return NULL;
}

static Eina_Bool
_pulse_queue_process(const Eina_Hash *h EINA_UNUSED, const char *key, E_Mixer_Channel_State *state, void *d EINA_UNUSED)
{
   Eina_List *l, *list[2] = {sinks, sources};
   void *s, *ch;
   int x;

   if ((state->mute == -1) && (state->left == -1) && (state->right == -1)) return EINA_TRUE;
   ch = (void*)1;
   for (x = 0; x < 2; x++)
     EINA_LIST_FOREACH(list[x], l, s)
       {
          if (key != pulse_sink_name_get(s)) continue;
          if ((state->left >= 0) || (state->right >= 0))
            e_mixer_pulse_set_volume(s, &ch, state->left, state->right);
          if (state->mute >= 0)
            e_mixer_pulse_set_mute(s, &ch, state->mute);
          state->left = state->right = state->mute = -1;
          return EINA_FALSE;
       }
   return EINA_TRUE;
}

static void
_pulse_result_cb(Pulse *p __UNUSED__, Pulse_Tag_Id id, void *ev)
{
   if (!ev) fprintf(stderr, "Command %u failed!\n", id);
   if (!update_count) return;
   if (--update_count) return;
   if (!queue_states) return;
   eina_hash_foreach(queue_states, (Eina_Hash_Foreach)_pulse_queue_process, NULL);
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
   if (dbus) goto error;
   if ((!conn) || (!pulse_connect(conn)))
     {
        DBusMessage *msg;
        double interval;

        e_dbus_init();
        dbus = e_dbus_bus_get(DBUS_BUS_SESSION);

        if (!dbus)
          {
             e_dbus_shutdown();
             return EINA_FALSE;
          }

        if (!pulse_poller)
          {
             interval = ecore_poller_poll_interval_get(ECORE_POLLER_CORE);
             /* polling every 5 seconds or so I guess ? */
             pulse_poller = ecore_poller_add(ECORE_POLLER_CORE, 5.0 / interval, _pulse_poller_cb, NULL);
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
   pulse_poller = NULL;
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
   EINA_LIST_FREE(sources, sink)
     pulse_sink_free(sink);
   pulse_server_info_free(info);
   info = NULL;
   default_sink = NULL;
   update_count = 0;
   if (update_timer) ecore_timer_del(update_timer);
   update_timer = NULL;

   pulse_free(conn);
   conn = NULL;
   if (ph) ecore_event_handler_del(ph);
   ph = NULL;
   if (pch) ecore_event_handler_del(pch);
   pch = NULL;
   if (pdh) ecore_event_handler_del(pdh);
   pdh = NULL;
   if (queue_states) eina_hash_free(queue_states);
   queue_states = NULL;
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
   return (E_Mixer_System *)_pulse_sink_find(name);
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
     ret = eina_list_append(ret, eina_stringshare_ref(pulse_sink_name_get(sink)));
   EINA_LIST_FOREACH(sources, l, sink)
     ret = eina_list_append(ret, eina_stringshare_ref(pulse_sink_name_get(sink)));
   return ret;
}

void
e_mixer_pulse_free_cards(Eina_List *cards)
{
   E_FREE_LIST(cards, eina_stringshare_del);
}

const char *
e_mixer_pulse_get_default_card(void)
{
   if (default_sink)
     return eina_stringshare_ref(pulse_sink_name_get(default_sink));
   return NULL;
}

const char *
e_mixer_pulse_get_card_name(const char *card)
{
   Pulse_Sink *sink;
   const char *s;

   sink = _pulse_sink_find(card);
   s = pulse_sink_desc_get(sink);
   if ((!s) || (!s[0])) s = pulse_sink_name_get(sink);
   return eina_stringshare_ref(s);
}

Eina_List *
e_mixer_pulse_get_channels(E_Mixer_System *self EINA_UNUSED)
{
   return eina_list_append(NULL, (void *)(1));
}

void
e_mixer_pulse_free_channels(Eina_List *channels)
{
   eina_list_free(channels);
}

Eina_List *
e_mixer_pulse_get_channels_names(E_Mixer_System *self EINA_UNUSED)
{
   return eina_list_append(NULL, eina_stringshare_add("Output"));
}

void
e_mixer_pulse_free_channels_names(Eina_List *channels_names)
{
   const char *str;
   EINA_LIST_FREE(channels_names, str)
     eina_stringshare_del(str);
}

const char *
e_mixer_pulse_get_default_channel_name(E_Mixer_System *self EINA_UNUSED)
{
   return eina_stringshare_add("Output");
}

E_Mixer_Channel *
e_mixer_pulse_get_channel_by_name(E_Mixer_System *self EINA_UNUSED, const char *name EINA_UNUSED)
{
   return (E_Mixer_Channel *)1;
}

void
e_mixer_pulse_channel_del(E_Mixer_Channel *channel __UNUSED__)
{
}

const char *
e_mixer_pulse_get_channel_name(E_Mixer_System *self EINA_UNUSED, E_Mixer_Channel *channel)
{
   if (!channel) return NULL;
   return eina_stringshare_add("Output");
}

int
e_mixer_pulse_get_volume(E_Mixer_System *self, E_Mixer_Channel *channel, int *left, int *right)
{
   double volume;
   int x, n;

   if (!channel) return 0;
   n = pulse_sink_channels_count((void *)self);
   for (x = 0; x < n; x++)
     {
        volume = pulse_sink_channel_volume_get((void *)self,
                                               ((uintptr_t)x));
        if (x == 0)
          {
             if (left) *left = (int)volume;
          }
        else if (x == 1)
          {
             if (right) *right = (int)volume;
          }
     }
   return 1;
}

int
e_mixer_pulse_set_volume(E_Mixer_System *self, E_Mixer_Channel *channel, int left, int right)
{
   uint32_t id = 0;
   int x, n;

   if (!channel) return 0;
   if (update_count > 1)
     {
        _pulse_state_queue((void*)self, left, right, -1);
        return 1;
     }
   n = pulse_sink_channels_count((void *)self);
   for (x = 0; x < n; x++, id = 0)
     {
        double vol;

        vol = lround(pulse_sink_channel_volume_get(self, x));
        if (x == 0)
          {
             if (vol != left)
               id = pulse_sink_channel_volume_set(conn, (void *)self, x, left);
          }
        else if (x == 1)
          {
             if (vol != right)
               id = pulse_sink_channel_volume_set(conn, (void *)self, x, right);
          }
        if (id)
          {
             pulse_cb_set(conn, id, (Pulse_Cb)_pulse_result_cb);
             update_count++;
          }
     }
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
   if (mute) *mute = pulse_sink_muted_get((void *)self);
   return 1;
}

int
e_mixer_pulse_set_mute(E_Mixer_System *self, E_Mixer_Channel *channel __UNUSED__, int mute)
{
   uint32_t id;
   Eina_Bool source = EINA_FALSE;

   if (update_count > 2)
     {
        _pulse_state_queue((void*)self, -1, -1, mute);
        return 1;
     }
   source = !!eina_list_data_find(sources, self);
   id = pulse_type_mute_set(conn, pulse_sink_idx_get((void *)self), mute, source);
   if (!id) return 0;
   update_count++;
   pulse_cb_set(conn, id, (Pulse_Cb)_pulse_result_cb);
   return 1;
}

int
e_mixer_pulse_get_state(E_Mixer_System *self, E_Mixer_Channel *channel, E_Mixer_Channel_State *state)
{
   if (!state) return 0;
   if (!channel) return 0;
   e_mixer_pulse_get_mute(self, channel, &(state->mute));
   e_mixer_pulse_get_volume(self, channel, &(state->left), &(state->right));
   return 1;
}

int
e_mixer_pulse_set_state(E_Mixer_System *self, E_Mixer_Channel *channel, const E_Mixer_Channel_State *state)
{
   e_mixer_pulse_set_volume(self, channel, state->left, state->right);
   e_mixer_pulse_set_mute(self, channel, state->mute);
   return 1;
}

int
e_mixer_pulse_has_capture(E_Mixer_System *self __UNUSED__, E_Mixer_Channel *channel __UNUSED__)
{
   return 0;
}

