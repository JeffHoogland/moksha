#ifndef PULSE_H
#define PULSE_H

#include <Eina.h>
#include <inttypes.h>

#define PULSE_SUCCESS (void*)1

extern int PULSE_EVENT_CONNECTED;
extern int PULSE_EVENT_DISCONNECTED;
extern int PULSE_EVENT_CHANGE;
typedef struct Pulse Pulse;
typedef uint32_t Pulse_Tag_Id;
typedef struct Pulse_Sink Pulse_Sink;
typedef struct Pulse_Sink Pulse_Source;
typedef void (*Pulse_Cb)(Pulse *, Pulse_Tag_Id, void *);

typedef struct Pulse_Sink_Port_Info {
    const char *name;                   /**< Name of this port */
    const char *description;            /**< Description of this port */
    uint32_t priority;                  /**< The higher this value is the more useful this port is as a default */
} Pulse_Sink_Port_Info;

int pulse_init(void);
void pulse_shutdown(void);

Pulse *pulse_new(void);
Eina_Bool pulse_connect(Pulse *conn);
void pulse_free(Pulse *conn);
void pulse_cb_set(Pulse *conn, uint32_t tagnum, Pulse_Cb cb);

uint32_t pulse_cards_get(Pulse *conn);
#define pulse_sinks_get(conn) pulse_types_get((conn), EINA_FALSE)
#define pulse_sources_get(conn) pulse_types_get((conn), EINA_TRUE)
uint32_t pulse_types_get(Pulse *conn, Eina_Bool source);
#define pulse_sink_get(conn, idx) pulse_type_get((conn), (idx), EINA_FALSE)
#define pulse_source_get(conn, idx) pulse_type_get((conn), (idx), EINA_FALSE)
uint32_t pulse_type_get(Pulse *conn, uint32_t idx, Eina_Bool source);
#define pulse_sink_mute_set(conn, idx, mute) pulse_type_mute_set((conn), (idx), (mute), EINA_FALSE)
#define pulse_source_mute_set(conn, idx, mute) pulse_type_mute_set((conn), (idx), (mute), EINA_TRUE)
uint32_t pulse_type_mute_set(Pulse *conn, uint32_t idx, Eina_Bool mute, Eina_Bool source);
#define pulse_sink_volume_set(conn, idx, channels, vol) pulse_type_volume_set((conn), (idx), (channels), (vol), EINA_FALSE)
#define pulse_source_volume_set(conn, idx, channels, vol) pulse_type_volume_set((conn), (idx), (channels), (vol), EINA_TRUE)
uint32_t pulse_type_volume_set(Pulse *conn, uint32_t sink_num, uint8_t channels, double vol, Eina_Bool source);

void pulse_sink_free(Pulse_Sink *sink);
const char *pulse_sink_name_get(Pulse_Sink *sink);
const char *pulse_sink_desc_get(Pulse_Sink *sink);
uint32_t pulse_sink_idx_get(Pulse_Sink *sink);
Eina_Bool pulse_sink_muted_get(Pulse_Sink *sink);
double pulse_sink_avg_get_pct(Pulse_Sink *sink);
float pulse_sink_balance_get(Pulse_Sink *sink);
uint8_t pulse_sink_channels_count(Pulse_Sink *sink);
Eina_List *pulse_sink_channel_names_get(Pulse_Sink *sink);
Eina_Bool pulse_sinks_watch(Pulse *conn);

const Eina_List *pulse_sink_ports_get(Pulse_Sink *sink);
const char *pulse_sink_port_active_get(Pulse_Sink *sink);
#define pulse_source_channel_volume_set pulse_sink_channel_volume_set
uint32_t pulse_sink_channel_volume_set(Pulse *conn, Pulse_Sink *sink, uint32_t id, double vol);
uint32_t pulse_sink_port_set(Pulse *conn, Pulse_Sink *sink, const char *port);
double pulse_sink_channel_volume_get(Pulse_Sink *sink, unsigned int id);
float pulse_sink_channel_balance_get(Pulse_Sink *sink, unsigned int id);
float pulse_sink_channel_depth_get(Pulse_Sink *sink, unsigned int id);
unsigned int pulse_sink_channel_name_get_id(Pulse_Sink *sink, const char *name);
const char *pulse_sink_channel_id_get_name(Pulse_Sink *sink, unsigned int id);

#endif
