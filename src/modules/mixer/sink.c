#include "pa.h"


/** Makes a bit mask from a channel position. \since 0.9.16 */
#define PA_CHANNEL_POSITION_MASK(f) ((pa_channel_position_mask_t) (1ULL << (f)))


#define PA_CHANNEL_POSITION_MASK_LEFT                                   \
    (PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_FRONT_LEFT)           \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_REAR_LEFT)          \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER) \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_SIDE_LEFT)          \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_FRONT_LEFT)     \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_REAR_LEFT))     \

#define PA_CHANNEL_POSITION_MASK_RIGHT                                  \
    (PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_FRONT_RIGHT)          \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_REAR_RIGHT)         \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER) \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_SIDE_RIGHT)         \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_FRONT_RIGHT)    \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_REAR_RIGHT))

#define PA_CHANNEL_POSITION_MASK_CENTER                                 \
    (PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_FRONT_CENTER)         \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_REAR_CENTER)        \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_CENTER)         \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_FRONT_CENTER)   \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_REAR_CENTER))

#define PA_CHANNEL_POSITION_MASK_FRONT                                  \
    (PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_FRONT_LEFT)           \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_FRONT_RIGHT)        \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_FRONT_CENTER)       \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER) \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER) \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_FRONT_LEFT)     \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_FRONT_RIGHT)    \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_FRONT_CENTER))

#define PA_CHANNEL_POSITION_MASK_REAR                                   \
    (PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_REAR_LEFT)            \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_REAR_RIGHT)         \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_REAR_CENTER)        \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_REAR_LEFT)      \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_REAR_RIGHT)     \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_REAR_CENTER))

#define PA_CHANNEL_POSITION_MASK_SIDE_OR_TOP_CENTER                     \
    (PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_SIDE_LEFT)            \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_SIDE_RIGHT)         \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_CENTER))

#define PA_CHANNEL_POSITION_MASK_TOP                                    \
    (PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_CENTER)           \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_FRONT_LEFT)     \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_FRONT_RIGHT)    \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_FRONT_CENTER)   \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_REAR_LEFT)      \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_REAR_RIGHT)     \
     | PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_TOP_REAR_CENTER))

#define PA_CHANNEL_POSITION_MASK_ALL            \
    ((pa_channel_position_mask_t) (PA_CHANNEL_POSITION_MASK(PA_CHANNEL_POSITION_MAX)-1))

const char *const channel_name_table[PA_CHANNEL_POSITION_MAX] = {
    [PA_CHANNEL_POSITION_MONO] = _("Mono"),

    [PA_CHANNEL_POSITION_FRONT_CENTER] = _("Front Center"),
    [PA_CHANNEL_POSITION_FRONT_LEFT] = _("Front Left"),
    [PA_CHANNEL_POSITION_FRONT_RIGHT] = _("Front Right"),

    [PA_CHANNEL_POSITION_REAR_CENTER] = _("Rear Center"),
    [PA_CHANNEL_POSITION_REAR_LEFT] = _("Rear Left"),
    [PA_CHANNEL_POSITION_REAR_RIGHT] = _("Rear Right"),

    [PA_CHANNEL_POSITION_LFE] = _("Subwoofer"),

    [PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER] = _("Front Left-of-center"),
    [PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER] = _("Front Right-of-center"),

    [PA_CHANNEL_POSITION_SIDE_LEFT] = _("Side Left"),
    [PA_CHANNEL_POSITION_SIDE_RIGHT] = _("Side Right"),

    [PA_CHANNEL_POSITION_AUX0] = _("Auxiliary 0"),
    [PA_CHANNEL_POSITION_AUX1] = _("Auxiliary 1"),
    [PA_CHANNEL_POSITION_AUX2] = _("Auxiliary 2"),
    [PA_CHANNEL_POSITION_AUX3] = _("Auxiliary 3"),
    [PA_CHANNEL_POSITION_AUX4] = _("Auxiliary 4"),
    [PA_CHANNEL_POSITION_AUX5] = _("Auxiliary 5"),
    [PA_CHANNEL_POSITION_AUX6] = _("Auxiliary 6"),
    [PA_CHANNEL_POSITION_AUX7] = _("Auxiliary 7"),
    [PA_CHANNEL_POSITION_AUX8] = _("Auxiliary 8"),
    [PA_CHANNEL_POSITION_AUX9] = _("Auxiliary 9"),
    [PA_CHANNEL_POSITION_AUX10] = _("Auxiliary 10"),
    [PA_CHANNEL_POSITION_AUX11] = _("Auxiliary 11"),
    [PA_CHANNEL_POSITION_AUX12] = _("Auxiliary 12"),
    [PA_CHANNEL_POSITION_AUX13] = _("Auxiliary 13"),
    [PA_CHANNEL_POSITION_AUX14] = _("Auxiliary 14"),
    [PA_CHANNEL_POSITION_AUX15] = _("Auxiliary 15"),
    [PA_CHANNEL_POSITION_AUX16] = _("Auxiliary 16"),
    [PA_CHANNEL_POSITION_AUX17] = _("Auxiliary 17"),
    [PA_CHANNEL_POSITION_AUX18] = _("Auxiliary 18"),
    [PA_CHANNEL_POSITION_AUX19] = _("Auxiliary 19"),
    [PA_CHANNEL_POSITION_AUX20] = _("Auxiliary 20"),
    [PA_CHANNEL_POSITION_AUX21] = _("Auxiliary 21"),
    [PA_CHANNEL_POSITION_AUX22] = _("Auxiliary 22"),
    [PA_CHANNEL_POSITION_AUX23] = _("Auxiliary 23"),
    [PA_CHANNEL_POSITION_AUX24] = _("Auxiliary 24"),
    [PA_CHANNEL_POSITION_AUX25] = _("Auxiliary 25"),
    [PA_CHANNEL_POSITION_AUX26] = _("Auxiliary 26"),
    [PA_CHANNEL_POSITION_AUX27] = _("Auxiliary 27"),
    [PA_CHANNEL_POSITION_AUX28] = _("Auxiliary 28"),
    [PA_CHANNEL_POSITION_AUX29] = _("Auxiliary 29"),
    [PA_CHANNEL_POSITION_AUX30] = _("Auxiliary 30"),
    [PA_CHANNEL_POSITION_AUX31] = _("Auxiliary 31"),

    [PA_CHANNEL_POSITION_TOP_CENTER] = _("Top Center"),

    [PA_CHANNEL_POSITION_TOP_FRONT_CENTER] = _("Top Front Center"),
    [PA_CHANNEL_POSITION_TOP_FRONT_LEFT] = _("Top Front Left"),
    [PA_CHANNEL_POSITION_TOP_FRONT_RIGHT] = _("Top Front Right"),

    [PA_CHANNEL_POSITION_TOP_REAR_CENTER] = _("Top Rear Center"),
    [PA_CHANNEL_POSITION_TOP_REAR_LEFT] = _("Top Rear Left"),
    [PA_CHANNEL_POSITION_TOP_REAR_RIGHT] = _("Top Rear Right")
};

static Eina_Bool on_left(pa_channel_position_t p) {
    return !!(PA_CHANNEL_POSITION_MASK(p) & PA_CHANNEL_POSITION_MASK_LEFT);
}

static Eina_Bool on_right(pa_channel_position_t p) {
    return !!(PA_CHANNEL_POSITION_MASK(p) & PA_CHANNEL_POSITION_MASK_RIGHT);
}
#if 0
static Eina_Bool on_center(pa_channel_position_t p) {
    return !!(PA_CHANNEL_POSITION_MASK(p) & PA_CHANNEL_POSITION_MASK_CENTER);
}

static Eina_Bool on_lfe(pa_channel_position_t p) {
    return p == PA_CHANNEL_POSITION_LFE;
}
#endif
static Eina_Bool on_front(pa_channel_position_t p) {
    return !!(PA_CHANNEL_POSITION_MASK(p) & PA_CHANNEL_POSITION_MASK_FRONT);
}

static Eina_Bool on_rear(pa_channel_position_t p) {
    return !!(PA_CHANNEL_POSITION_MASK(p) & PA_CHANNEL_POSITION_MASK_REAR);
}

static void
pulse_sink_port_info_free(Pulse_Sink_Port_Info *pi)
{
   if (!pi) return;
   eina_stringshare_del(pi->name);
   eina_stringshare_del(pi->description);
   free(pi);
}

void
pulse_sink_free(Pulse_Sink *sink)
{
   Pulse_Sink_Port_Info *pi;
   if (!sink) return;
   if (sink->source)
     {
        if (eina_hash_del_by_key(pulse_sources, (uintptr_t*)&sink->index))
          return;
     }
   else
     {
        if (eina_hash_del_by_key(pulse_sinks, (uintptr_t*)&sink->index))
          return;
     }
   eina_stringshare_del(sink->name);
   eina_stringshare_del(sink->description);
   EINA_LIST_FREE(sink->ports, pi)
     pulse_sink_port_info_free(pi);
   eina_stringshare_del(sink->active_port);
   free(sink);
}

const Eina_List *
pulse_sink_ports_get(Pulse_Sink *sink)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sink, NULL);
   return sink->ports;
}

const char *
pulse_sink_port_active_get(Pulse_Sink *sink)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sink, NULL);
   return sink->active_port;
}

const char *
pulse_sink_name_get(Pulse_Sink *sink)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sink, NULL);

   return sink->name;
}

const char *
pulse_sink_desc_get(Pulse_Sink *sink)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sink, NULL);

   return sink->description;
}

uint32_t
pulse_sink_idx_get(Pulse_Sink *sink)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sink, 0);

   return sink->index;
}

Eina_Bool
pulse_sink_muted_get(Pulse_Sink *sink)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sink, EINA_FALSE);

   return sink->mute;
}

uint8_t
pulse_sink_channels_count(Pulse_Sink *sink)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sink, 0);

   return sink->volume.channels;
}

double
pulse_sink_avg_get_pct(Pulse_Sink *sink)
{
   double avg;
   int x;

   EINA_SAFETY_ON_NULL_RETURN_VAL(sink, -1.0);

   for (avg = 0, x = 0; x < sink->volume.channels; x++)
     avg += sink->volume.values[x];
   avg /= sink->volume.channels;

   if (avg <= PA_VOLUME_MUTED) return 0.0;

   if (avg == PA_VOLUME_NORM) return 100.0;

   return (avg * 100 + PA_VOLUME_NORM / 2) / PA_VOLUME_NORM;
}

Eina_List *
pulse_sink_channel_names_get(Pulse_Sink *sink)
{
   Eina_List *ret = NULL;
   unsigned int x;
   
   EINA_SAFETY_ON_NULL_RETURN_VAL(sink, NULL);
   for (x = 0; x < sink->volume.channels; x++)
      ret = eina_list_append(ret, pulse_sink_channel_id_get_name(sink, x));
   return ret;
}

unsigned int
pulse_sink_channel_name_get_id(Pulse_Sink *sink, const char *name)
{
   unsigned int x;
   
   EINA_SAFETY_ON_NULL_RETURN_VAL(sink, UINT_MAX);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, UINT_MAX);
   for (x = 0; x < sink->channel_map.channels; x++)
     {
        if (!strcmp(name, channel_name_table[sink->channel_map.map[x]]))
           return x;
     }
   return UINT_MAX;
}

const char *
pulse_sink_channel_id_get_name(Pulse_Sink *sink, unsigned int id)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sink, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(id >= sink->channel_map.channels, NULL);
   return eina_stringshare_add(channel_name_table[sink->channel_map.map[id]]);
}

double
pulse_sink_channel_volume_get(Pulse_Sink *sink, unsigned int id)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sink, -1);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(id >= sink->channel_map.channels, -1);
   return (sink->volume.values[id] * 100 + PA_VOLUME_NORM / 2) / PA_VOLUME_NORM;
}

float
pulse_sink_channel_balance_get(Pulse_Sink *sink, unsigned int id)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sink, -1.0);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(id >= sink->channel_map.channels, -1.0);

   if (on_left(sink->channel_map.map[id])) return 0.0;
   if (on_right(sink->channel_map.map[id])) return 1.0;
   return 0.5;
}

float
pulse_sink_channel_depth_get(Pulse_Sink *sink, unsigned int id)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sink, -1.0);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(id >= sink->channel_map.channels, -1.0);

   if (on_rear(sink->channel_map.map[id])) return 0.0;
   if (on_front(sink->channel_map.map[id])) return 1.0;
   return 0.5;
}

float
pulse_sink_balance_get(Pulse_Sink *sink)
{
    int c;
    float l, r;
    pa_volume_t left = 0, right = 0;
    unsigned n_left = 0, n_right = 0;

    for (c = 0; c < sink->channel_map.channels; c++) {
        if (on_left(sink->channel_map.map[c])) {
            left += sink->volume.values[c];
            n_left++;
        } else if (on_right(sink->channel_map.map[c])) {
            right += sink->volume.values[c];
            n_right++;
        }
    }

    if (n_left <= 0)
        l = PA_VOLUME_NORM;
    else
        l = left / n_left;

    if (n_right <= 0)
        r = PA_VOLUME_NORM;
    else
        r = right / n_right;
    l /= (float)PA_VOLUME_NORM;
    r /= (float)PA_VOLUME_NORM;
    return r - l;
}
