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
typedef void (*Pulse_Cb)(Pulse *, Pulse_Tag_Id, void *);

int pulse_init(void);
void pulse_shutdown(void);

Pulse *pulse_new(void);
Eina_Bool pulse_connect(Pulse *conn);
void pulse_free(Pulse *conn);
void pulse_cb_set(Pulse *conn, uint32_t tagnum, Pulse_Cb cb);

uint32_t pulse_cards_get(Pulse *conn);
uint32_t pulse_sinks_get(Pulse *conn);
uint32_t pulse_sink_get(Pulse *conn, uint32_t idx);
uint32_t pulse_sink_mute_set(Pulse *conn, uint32_t idx, Eina_Bool mute);

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

uint32_t pulse_sink_channel_volume_set(Pulse *conn, Pulse_Sink *sink, uint32_t id, double vol);
double pulse_sink_channel_volume_get(Pulse_Sink *sink, unsigned int id);
float pulse_sink_channel_balance_get(Pulse_Sink *sink, unsigned int id);
float pulse_sink_channel_depth_get(Pulse_Sink *sink, unsigned int id);
unsigned int pulse_sink_channel_name_get_id(Pulse_Sink *sink, const char *name);
const char *pulse_sink_channel_id_get_name(Pulse_Sink *sink, unsigned int id);

#endif
