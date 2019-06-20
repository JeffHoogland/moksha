#include "common.h"

#include <pulse/pulseaudio.h>

#define PA_VOLUME_TO_INT(_vol) \
   (((_vol+1)*100+PA_VOLUME_NORM/2)/PA_VOLUME_NORM)
#define INT_TO_PA_VOLUME(_vol) \
   (!_vol) ? 0 : ((PA_VOLUME_NORM*(_vol+1)-PA_VOLUME_NORM/2)/100)
#define DB_TO_PA_VOLUME(_vol) \
   ((!eina_dbl_exact(_vol, 0))) ? 0 : ((PA_VOLUME_NORM*(_vol+1)-PA_VOLUME_NORM/2)/100)

typedef struct _Port Port;
struct _Port {
   Eina_Bool active;
   Eina_Bool available;
   int priority;
   char *name;
   char *description;
};

typedef struct _Epulse_Event Epulse_Event;
struct _Epulse_Event
{
   int index;
   char *name;
   pa_cvolume volume;
   Eina_Bool mute;
};

typedef struct _Epulse_Event_Sink Epulse_Event_Sink;
struct _Epulse_Event_Sink {
   Epulse_Event base;
   Eina_List *ports;
};

typedef struct _Epulse_Event_Sink_Input Epulse_Event_Sink_Input;
struct _Epulse_Event_Sink_Input {
   Epulse_Event base;
   int sink;
   char *icon;
};

EAPI extern int DISCONNECTED;
EAPI extern int SINK_ADDED;
EAPI extern int SINK_CHANGED;
EAPI extern int SINK_DEFAULT;
EAPI extern int SINK_REMOVED;
EAPI extern int SINK_INPUT_ADDED;
EAPI extern int SINK_INPUT_CHANGED;
EAPI extern int SINK_INPUT_REMOVED;
EAPI extern int SOURCE_ADDED;
EAPI extern int SOURCE_CHANGED;
EAPI extern int SOURCE_REMOVED;
EAPI extern int SOURCE_INPUT_ADDED;
EAPI extern int SOURCE_INPUT_REMOVED;

EAPI int epulse_init(void);
EAPI Eina_Bool epulse_source_volume_set(int index, pa_cvolume volume);
EAPI Eina_Bool epulse_source_mute_set(int index, Eina_Bool mute);
EAPI Eina_Bool epulse_sink_volume_set(int index, pa_cvolume volume);
EAPI Eina_Bool epulse_sink_mute_set(int index, Eina_Bool mute);
EAPI Eina_Bool epulse_sink_port_set(int index, const char *port);
EAPI Eina_Bool epulse_sink_input_volume_set(int index, pa_cvolume volume);
EAPI Eina_Bool epulse_sink_input_mute_set(int index, Eina_Bool mute);
EAPI Eina_Bool epulse_sink_input_move(int index, int sink_index);
EAPI void epulse_shutdown(void);
