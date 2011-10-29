#ifndef PA_HACKS_H
#define PA_HACKS_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Ecore.h>
#include <Ecore_Con.h>
#include <inttypes.h>
#include "Pulse.h"
#include <math.h>

#ifndef STATEDIR
# define STATEDIR "/var"
#endif

#ifndef __UNUSED__
# define __UNUSED__ __attribute__((unused))
#endif

#define PA_PROTOCOL_VERSION 16
#define PA_NATIVE_COOKIE_LENGTH 256
#ifndef PA_MACHINE_ID
# define PA_MACHINE_ID "/var/lib/dbus/machine-id"
#endif

#ifndef _
# define _(X) (X)
#endif

#define DBG(...)            EINA_LOG_DOM_DBG(pa_log_dom, __VA_ARGS__)
#define INF(...)            EINA_LOG_DOM_INFO(pa_log_dom, __VA_ARGS__)
#define WRN(...)            EINA_LOG_DOM_WARN(pa_log_dom, __VA_ARGS__)
#define ERR(...)            EINA_LOG_DOM_ERR(pa_log_dom, __VA_ARGS__)
#define CRI(...)            EINA_LOG_DOM_CRIT(pa_log_dom, __VA_ARGS__)


typedef enum
{
    PA_COMMAND_ERROR,
    PA_COMMAND_TIMEOUT, /* pseudo command */
    PA_COMMAND_REPLY,
    PA_COMMAND_CREATE_PLAYBACK_STREAM,        /* Payload changed in v9, v12 (0.9.0, 0.9.8) */
    PA_COMMAND_DELETE_PLAYBACK_STREAM,
    PA_COMMAND_CREATE_RECORD_STREAM,          /* Payload changed in v9, v12 (0.9.0, 0.9.8) */
    PA_COMMAND_DELETE_RECORD_STREAM,
    PA_COMMAND_EXIT,
    PA_COMMAND_AUTH,
    PA_COMMAND_SET_CLIENT_NAME,
    PA_COMMAND_LOOKUP_SINK,
    PA_COMMAND_LOOKUP_SOURCE,
    PA_COMMAND_DRAIN_PLAYBACK_STREAM,
    PA_COMMAND_STAT,
    PA_COMMAND_GET_PLAYBACK_LATENCY,
    PA_COMMAND_CREATE_UPLOAD_STREAM,
    PA_COMMAND_DELETE_UPLOAD_STREAM,
    PA_COMMAND_FINISH_UPLOAD_STREAM,
    PA_COMMAND_PLAY_SAMPLE,
    PA_COMMAND_REMOVE_SAMPLE,
    PA_COMMAND_GET_SERVER_INFO,
    PA_COMMAND_GET_SINK_INFO,
    PA_COMMAND_GET_SINK_INFO_LIST,
    PA_COMMAND_GET_SOURCE_INFO,
    PA_COMMAND_GET_SOURCE_INFO_LIST,
    PA_COMMAND_GET_MODULE_INFO,
    PA_COMMAND_GET_MODULE_INFO_LIST,
    PA_COMMAND_GET_CLIENT_INFO,
    PA_COMMAND_GET_CLIENT_INFO_LIST,
    PA_COMMAND_GET_SINK_INPUT_INFO,          /* Payload changed in v11 (0.9.7) */
    PA_COMMAND_GET_SINK_INPUT_INFO_LIST,     /* Payload changed in v11 (0.9.7) */
    PA_COMMAND_GET_SOURCE_OUTPUT_INFO,
    PA_COMMAND_GET_SOURCE_OUTPUT_INFO_LIST,
    PA_COMMAND_GET_SAMPLE_INFO,
    PA_COMMAND_GET_SAMPLE_INFO_LIST,
    PA_COMMAND_SUBSCRIBE,
    PA_COMMAND_SET_SINK_VOLUME,
    PA_COMMAND_SET_SINK_INPUT_VOLUME,
    PA_COMMAND_SET_SOURCE_VOLUME,
    PA_COMMAND_SET_SINK_MUTE,
    PA_COMMAND_SET_SOURCE_MUTE,
    PA_COMMAND_CORK_PLAYBACK_STREAM,
    PA_COMMAND_FLUSH_PLAYBACK_STREAM,
    PA_COMMAND_TRIGGER_PLAYBACK_STREAM,
    PA_COMMAND_SET_DEFAULT_SINK,
    PA_COMMAND_SET_DEFAULT_SOURCE,
    PA_COMMAND_SET_PLAYBACK_STREAM_NAME,
    PA_COMMAND_SET_RECORD_STREAM_NAME,
    PA_COMMAND_KILL_CLIENT,
    PA_COMMAND_KILL_SINK_INPUT,
    PA_COMMAND_KILL_SOURCE_OUTPUT,
    PA_COMMAND_LOAD_MODULE,
    PA_COMMAND_UNLOAD_MODULE,
    PA_COMMAND_ADD_AUTOLOAD___OBSOLETE,
    PA_COMMAND_REMOVE_AUTOLOAD___OBSOLETE,
    PA_COMMAND_GET_AUTOLOAD_INFO___OBSOLETE,
    PA_COMMAND_GET_AUTOLOAD_INFO_LIST___OBSOLETE,
    PA_COMMAND_GET_RECORD_LATENCY,
    PA_COMMAND_CORK_RECORD_STREAM,
    PA_COMMAND_FLUSH_RECORD_STREAM,
    PA_COMMAND_PREBUF_PLAYBACK_STREAM,
    PA_COMMAND_REQUEST,
    PA_COMMAND_OVERFLOW,
    PA_COMMAND_UNDERFLOW,
    PA_COMMAND_PLAYBACK_STREAM_KILLED,
    PA_COMMAND_RECORD_STREAM_KILLED,
    PA_COMMAND_SUBSCRIBE_EVENT,
    PA_COMMAND_MOVE_SINK_INPUT,
    PA_COMMAND_MOVE_SOURCE_OUTPUT,
    PA_COMMAND_SET_SINK_INPUT_MUTE,
    PA_COMMAND_SUSPEND_SINK,
    PA_COMMAND_SUSPEND_SOURCE,
    PA_COMMAND_SET_PLAYBACK_STREAM_BUFFER_ATTR,
    PA_COMMAND_SET_RECORD_STREAM_BUFFER_ATTR,
    PA_COMMAND_UPDATE_PLAYBACK_STREAM_SAMPLE_RATE,
    PA_COMMAND_UPDATE_RECORD_STREAM_SAMPLE_RATE,
    PA_COMMAND_PLAYBACK_STREAM_SUSPENDED,
    PA_COMMAND_RECORD_STREAM_SUSPENDED,
    PA_COMMAND_PLAYBACK_STREAM_MOVED,
    PA_COMMAND_RECORD_STREAM_MOVED,
    PA_COMMAND_UPDATE_RECORD_STREAM_PROPLIST,
    PA_COMMAND_UPDATE_PLAYBACK_STREAM_PROPLIST,
    PA_COMMAND_UPDATE_CLIENT_PROPLIST,
    PA_COMMAND_REMOVE_RECORD_STREAM_PROPLIST,
    PA_COMMAND_REMOVE_PLAYBACK_STREAM_PROPLIST,
    PA_COMMAND_REMOVE_CLIENT_PROPLIST,
    PA_COMMAND_STARTED,
    PA_COMMAND_EXTENSION,
    PA_COMMAND_GET_CARD_INFO,
    PA_COMMAND_GET_CARD_INFO_LIST,
    PA_COMMAND_SET_CARD_PROFILE,
    PA_COMMAND_CLIENT_EVENT,
    PA_COMMAND_PLAYBACK_STREAM_EVENT,
    PA_COMMAND_RECORD_STREAM_EVENT,
    PA_COMMAND_PLAYBACK_BUFFER_ATTR_CHANGED,
    PA_COMMAND_RECORD_BUFFER_ATTR_CHANGED,
    PA_COMMAND_SET_SINK_PORT,
    PA_COMMAND_SET_SOURCE_PORT,
    PA_COMMAND_MAX
} PA_Commands;

typedef enum pa_subscription_event_type {
    PA_SUBSCRIPTION_EVENT_SINK = 0x0000U,
    PA_SUBSCRIPTION_EVENT_SOURCE = 0x0001U,
    PA_SUBSCRIPTION_EVENT_SINK_INPUT = 0x0002U,
    PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT = 0x0003U,
    PA_SUBSCRIPTION_EVENT_MODULE = 0x0004U,
    PA_SUBSCRIPTION_EVENT_CLIENT = 0x0005U,
    PA_SUBSCRIPTION_EVENT_SAMPLE_CACHE = 0x0006U,
    PA_SUBSCRIPTION_EVENT_SERVER = 0x0007U,
    PA_SUBSCRIPTION_EVENT_AUTOLOAD = 0x0008U,
    PA_SUBSCRIPTION_EVENT_CARD = 0x0009U,
    PA_SUBSCRIPTION_EVENT_FACILITY_MASK = 0x000FU,
    PA_SUBSCRIPTION_EVENT_NEW = 0x0000U,
    PA_SUBSCRIPTION_EVENT_CHANGE = 0x0010U,
    PA_SUBSCRIPTION_EVENT_REMOVE = 0x0020U,
    PA_SUBSCRIPTION_EVENT_TYPE_MASK = 0x0030U
} pa_subscription_event_type_t;

/** Subscription event mask, as used by pa_context_subscribe() */
typedef enum pa_subscription_mask {
    PA_SUBSCRIPTION_MASK_NULL = 0x0000U,
    PA_SUBSCRIPTION_MASK_SINK = 0x0001U,
    PA_SUBSCRIPTION_MASK_SOURCE = 0x0002U,
    PA_SUBSCRIPTION_MASK_SINK_INPUT = 0x0004U,
    PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT = 0x0008U,
    PA_SUBSCRIPTION_MASK_MODULE = 0x0010U,
    PA_SUBSCRIPTION_MASK_CLIENT = 0x0020U,
    PA_SUBSCRIPTION_MASK_SAMPLE_CACHE = 0x0040U,
    PA_SUBSCRIPTION_MASK_SERVER = 0x0080U,
    PA_SUBSCRIPTION_MASK_AUTOLOAD = 0x0100U,
    PA_SUBSCRIPTION_MASK_CARD = 0x0200U,
    PA_SUBSCRIPTION_MASK_ALL = 0x02ffU
} pa_subscription_mask_t;

/* The sequence descriptor header consists of 5 32bit integers: */
typedef enum {
    PA_PSTREAM_DESCRIPTOR_LENGTH,
    PA_PSTREAM_DESCRIPTOR_CHANNEL,
    PA_PSTREAM_DESCRIPTOR_OFFSET_HI,
    PA_PSTREAM_DESCRIPTOR_OFFSET_LO,
    PA_PSTREAM_DESCRIPTOR_FLAGS,
    PA_PSTREAM_DESCRIPTOR_MAX
} PA_Header;

/* Due to a stupid design flaw, proplists may only be at the END of a
 * packet or not before a STRING! Don't forget that! We can't really
 * fix this without breaking compat. */

typedef enum {
    PA_TAG_INVALID = 0,
    PA_TAG_STRING = 't',
    PA_TAG_STRING_NULL = 'N',
    PA_TAG_U32 = 'L',
    PA_TAG_U8 = 'B',
    PA_TAG_U64 = 'R',
    PA_TAG_S64 = 'r',
    PA_TAG_SAMPLE_SPEC = 'a',
    PA_TAG_ARBITRARY = 'x',
    PA_TAG_BOOLEAN_TRUE = '1',
    PA_TAG_BOOLEAN_FALSE = '0',
    PA_TAG_BOOLEAN = PA_TAG_BOOLEAN_TRUE,
    PA_TAG_TIMEVAL = 'T',
    PA_TAG_USEC = 'U'  /* 64bit unsigned */,
    PA_TAG_CHANNEL_MAP = 'm',
    PA_TAG_CVOLUME = 'v',
    PA_TAG_PROPLIST = 'P',
    PA_TAG_VOLUME = 'V'
} PA_Tag;

typedef enum
{
    PA_TAG_SIZE_INVALID = 0,
    PA_TAG_SIZE_STRING = 2,
    PA_TAG_SIZE_STRING_NULL = 1,
    PA_TAG_SIZE_U32 = 5,
    PA_TAG_SIZE_U8 = 'B',
    PA_TAG_SIZE_U64 = 'R',
    PA_TAG_SIZE_S64 = 'r',
    PA_TAG_SIZE_SAMPLE_SPEC = 7,
    PA_TAG_SIZE_ARBITRARY = PA_TAG_SIZE_U32,
    PA_TAG_SIZE_BOOLEAN = 1,
    PA_TAG_SIZE_TIMEVAL = 'T',
    PA_TAG_SIZE_USEC = 9  /* 64bit unsigned */,
    PA_TAG_SIZE_CHANNEL_MAP = 2,
    PA_TAG_SIZE_CVOLUME = 2,
    PA_TAG_SIZE_PROPLIST = 1 + PA_TAG_SIZE_STRING_NULL,
    PA_TAG_SIZE_VOLUME = 2
} PA_Tag_Size;   

/** Volume specification:
 *  PA_VOLUME_MUTED: silence;
 * < PA_VOLUME_NORM: decreased volume;
 *   PA_VOLUME_NORM: normal volume;
 * > PA_VOLUME_NORM: increased volume */
typedef uint32_t pa_volume_t;

/** Normal volume (100%, 0 dB) */
#define PA_VOLUME_NORM ((pa_volume_t) 0x10000U)

/** Muted (minimal valid) volume (0%, -inf dB) */
#define PA_VOLUME_MUTED ((pa_volume_t) 0U)

/** Maximum valid volume we can store. \since 0.9.15 */
#define PA_VOLUME_MAX ((pa_volume_t) UINT32_MAX-1)

/** Special 'invalid' volume. \since 0.9.16 */
#define PA_VOLUME_INVALID ((pa_volume_t) UINT32_MAX)
#define PA_CHANNELS_MAX 32U


/** Maximum allowed sample rate */
#define PA_RATE_MAX (48000U*4U)

/** Sample format */
typedef enum pa_sample_format {
    PA_SAMPLE_U8,
    /**< Unsigned 8 Bit PCM */

    PA_SAMPLE_ALAW,
    /**< 8 Bit a-Law */

    PA_SAMPLE_ULAW,
    /**< 8 Bit mu-Law */

    PA_SAMPLE_S16LE,
    /**< Signed 16 Bit PCM, little endian (PC) */

    PA_SAMPLE_S16BE,
    /**< Signed 16 Bit PCM, big endian */

    PA_SAMPLE_FLOAT32LE,
    /**< 32 Bit IEEE floating point, little endian (PC), range -1.0 to 1.0 */

    PA_SAMPLE_FLOAT32BE,
    /**< 32 Bit IEEE floating point, big endian, range -1.0 to 1.0 */

    PA_SAMPLE_S32LE,
    /**< Signed 32 Bit PCM, little endian (PC) */

    PA_SAMPLE_S32BE,
    /**< Signed 32 Bit PCM, big endian */

    PA_SAMPLE_S24LE,
    /**< Signed 24 Bit PCM packed, little endian (PC). \since 0.9.15 */

    PA_SAMPLE_S24BE,
    /**< Signed 24 Bit PCM packed, big endian. \since 0.9.15 */

    PA_SAMPLE_S24_32LE,
    /**< Signed 24 Bit PCM in LSB of 32 Bit words, little endian (PC). \since 0.9.15 */

    PA_SAMPLE_S24_32BE,
    /**< Signed 24 Bit PCM in LSB of 32 Bit words, big endian. \since 0.9.15 */

    PA_SAMPLE_MAX,
    /**< Upper limit of valid sample types */

    PA_SAMPLE_INVALID = -1
    /**< An invalid value */
} pa_sample_format_t;

/** A list of channel labels */
typedef enum pa_channel_position {
    PA_CHANNEL_POSITION_INVALID = -1,
    PA_CHANNEL_POSITION_MONO = 0,

    PA_CHANNEL_POSITION_FRONT_LEFT,               /* Apple, Dolby call this 'Left' */
    PA_CHANNEL_POSITION_FRONT_RIGHT,              /* Apple, Dolby call this 'Right' */
    PA_CHANNEL_POSITION_FRONT_CENTER,             /* Apple, Dolby call this 'Center' */

/** \cond fulldocs */
    PA_CHANNEL_POSITION_LEFT = PA_CHANNEL_POSITION_FRONT_LEFT,
    PA_CHANNEL_POSITION_RIGHT = PA_CHANNEL_POSITION_FRONT_RIGHT,
    PA_CHANNEL_POSITION_CENTER = PA_CHANNEL_POSITION_FRONT_CENTER,
/** \endcond */

    PA_CHANNEL_POSITION_REAR_CENTER,              /* Microsoft calls this 'Back Center', Apple calls this 'Center Surround', Dolby calls this 'Surround Rear Center' */
    PA_CHANNEL_POSITION_REAR_LEFT,                /* Microsoft calls this 'Back Left', Apple calls this 'Left Surround' (!), Dolby calls this 'Surround Rear Left'  */
    PA_CHANNEL_POSITION_REAR_RIGHT,               /* Microsoft calls this 'Back Right', Apple calls this 'Right Surround' (!), Dolby calls this 'Surround Rear Right'  */

    PA_CHANNEL_POSITION_LFE,                      /* Microsoft calls this 'Low Frequency', Apple calls this 'LFEScreen' */
/** \cond fulldocs */
    PA_CHANNEL_POSITION_SUBWOOFER = PA_CHANNEL_POSITION_LFE,
/** \endcond */

    PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER,     /* Apple, Dolby call this 'Left Center' */
    PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER,    /* Apple, Dolby call this 'Right Center */

    PA_CHANNEL_POSITION_SIDE_LEFT,                /* Apple calls this 'Left Surround Direct', Dolby calls this 'Surround Left' (!) */
    PA_CHANNEL_POSITION_SIDE_RIGHT,               /* Apple calls this 'Right Surround Direct', Dolby calls this 'Surround Right' (!) */

    PA_CHANNEL_POSITION_AUX0,
    PA_CHANNEL_POSITION_AUX1,
    PA_CHANNEL_POSITION_AUX2,
    PA_CHANNEL_POSITION_AUX3,
    PA_CHANNEL_POSITION_AUX4,
    PA_CHANNEL_POSITION_AUX5,
    PA_CHANNEL_POSITION_AUX6,
    PA_CHANNEL_POSITION_AUX7,
    PA_CHANNEL_POSITION_AUX8,
    PA_CHANNEL_POSITION_AUX9,
    PA_CHANNEL_POSITION_AUX10,
    PA_CHANNEL_POSITION_AUX11,
    PA_CHANNEL_POSITION_AUX12,
    PA_CHANNEL_POSITION_AUX13,
    PA_CHANNEL_POSITION_AUX14,
    PA_CHANNEL_POSITION_AUX15,
    PA_CHANNEL_POSITION_AUX16,
    PA_CHANNEL_POSITION_AUX17,
    PA_CHANNEL_POSITION_AUX18,
    PA_CHANNEL_POSITION_AUX19,
    PA_CHANNEL_POSITION_AUX20,
    PA_CHANNEL_POSITION_AUX21,
    PA_CHANNEL_POSITION_AUX22,
    PA_CHANNEL_POSITION_AUX23,
    PA_CHANNEL_POSITION_AUX24,
    PA_CHANNEL_POSITION_AUX25,
    PA_CHANNEL_POSITION_AUX26,
    PA_CHANNEL_POSITION_AUX27,
    PA_CHANNEL_POSITION_AUX28,
    PA_CHANNEL_POSITION_AUX29,
    PA_CHANNEL_POSITION_AUX30,
    PA_CHANNEL_POSITION_AUX31,

    PA_CHANNEL_POSITION_TOP_CENTER,               /* Apple calls this 'Top Center Surround' */

    PA_CHANNEL_POSITION_TOP_FRONT_LEFT,           /* Apple calls this 'Vertical Height Left' */
    PA_CHANNEL_POSITION_TOP_FRONT_RIGHT,          /* Apple calls this 'Vertical Height Right' */
    PA_CHANNEL_POSITION_TOP_FRONT_CENTER,         /* Apple calls this 'Vertical Height Center' */

    PA_CHANNEL_POSITION_TOP_REAR_LEFT,            /* Microsoft and Apple call this 'Top Back Left' */
    PA_CHANNEL_POSITION_TOP_REAR_RIGHT,           /* Microsoft and Apple call this 'Top Back Right' */
    PA_CHANNEL_POSITION_TOP_REAR_CENTER,          /* Microsoft and Apple call this 'Top Back Center' */

    PA_CHANNEL_POSITION_MAX
} pa_channel_position_t;

/** A mask of channel positions. \since 0.9.16 */
typedef uint64_t pa_channel_position_mask_t;

/** Makes a bit mask from a channel position. \since 0.9.16 */
#define PA_CHANNEL_POSITION_MASK(f) ((pa_channel_position_mask_t) (1ULL << (f)))

/** A list of channel mapping definitions for pa_channel_map_init_auto() */
typedef enum pa_channel_map_def {
    PA_CHANNEL_MAP_AIFF,
    /**< The mapping from RFC3551, which is based on AIFF-C */

/** \cond fulldocs */
    PA_CHANNEL_MAP_ALSA,
    /**< The default mapping used by ALSA. This mapping is probably
     * not too useful since ALSA's default channel mapping depends on
     * the device string used. */
/** \endcond */

    PA_CHANNEL_MAP_AUX,
    /**< Only aux channels */

    PA_CHANNEL_MAP_WAVEEX,
    /**< Microsoft's WAVEFORMATEXTENSIBLE mapping. This mapping works
     * as if all LSBs of dwChannelMask are set.  */

/** \cond fulldocs */
    PA_CHANNEL_MAP_OSS,
    /**< The default channel mapping used by OSS as defined in the OSS
     * 4.0 API specs. This mapping is probably not too useful since
     * the OSS API has changed in this respect and no longer knows a
     * default channel mapping based on the number of channels. */
/** \endcond */

    /**< Upper limit of valid channel mapping definitions */
    PA_CHANNEL_MAP_DEF_MAX,

    PA_CHANNEL_MAP_DEFAULT = PA_CHANNEL_MAP_AIFF
    /**< The default channel map */
} pa_channel_map_def_t;

typedef struct pa_sample_spec {
    pa_sample_format_t format;
    /**< The sample format */

    uint32_t rate;
    /**< The sample rate. (e.g. 44100) */

    uint8_t channels;
    /**< Audio channels. (1 for mono, 2 for stereo, ...) */
} pa_sample_spec;
/** A structure encapsulating a per-channel volume */
typedef struct pa_cvolume {
    uint8_t channels;                     /**< Number of channels */
    pa_volume_t values[PA_CHANNELS_MAX];  /**< Per-channel volume  */
} pa_cvolume;

typedef struct pa_channel_map {
    uint8_t channels;
    /**< Number of channels */

    pa_channel_position_t map[PA_CHANNELS_MAX];
    /**< Channel labels */
} pa_channel_map;

/** Type for usec specifications (unsigned). Always 64 bit. */
typedef uint64_t pa_usec_t;

/** Stores information about sinks. Please note that this structure
 * can be extended as part of evolutionary API updates at any time in
 * any new release. */
struct Pulse_Sink {
    const char *name;                  /**< Name of the sink */
    uint32_t index;                    /**< Index of the sink */
    const char *description;           /**< Description of this sink */
//    pa_sample_spec sample_spec;        /**< Sample spec of this sink */
    pa_channel_map channel_map;        /**< Channel map */
//    uint32_t owner_module;             /**< Index of the owning module of this sink, or PA_INVALID_INDEX */
    pa_cvolume volume;                 /**< Volume of the sink */
    Eina_List *ports;                  /**< output ports */
    const char *active_port;           /**< currently active port */
    Eina_Bool mute : 1;                          /**< Mute switch of the sink */
    Eina_Bool update : 1;
    Eina_Bool source : 1; /**< sink is actually a source */
};

typedef uint32_t pa_pstream_descriptor[PA_PSTREAM_DESCRIPTOR_MAX];

typedef struct
{
   pa_pstream_descriptor header;
   uint8_t *data;

   size_t dsize;
   size_t size;
   size_t pos;
   PA_Commands command;
   uint32_t tag_count;
   Eina_Bool auth : 1;
   Eina_Hash *props;
} Pulse_Tag;

typedef enum
{
   PA_STATE_INIT,
   PA_STATE_AUTH,
   PA_STATE_MOREAUTH,
   PA_STATE_CONNECTED,
} PA_State;

struct Pulse
{
   PA_State state;
   int fd;
   Ecore_Fd_Handler *fdh;
   Ecore_Con_Server *svr;
   Ecore_Event_Handler *con;
   const char *socket;
   Eina_List *oq;
   Eina_List *iq;
   Eina_Hash *tag_handlers;
   Eina_Hash *tag_cbs;
   uint32_t tag_count;
   Eina_Bool watching : 1;
};

extern int pa_log_dom;

uint8_t *tag_bool(Pulse_Tag *tag, Eina_Bool val);
uint8_t *untag_bool(Pulse_Tag *tag, Eina_Bool *val);
uint8_t *tag_uint32(Pulse_Tag *tag, uint32_t val);
uint8_t *untag_uint32(Pulse_Tag *tag, uint32_t *val);
uint8_t *tag_string(Pulse_Tag *tag, const char *val);
uint8_t *untag_string(Pulse_Tag *tag, const char **val);
uint8_t *tag_arbitrary(Pulse_Tag *tag, const void *val, uint32_t size);
uint8_t *untag_arbitrary(Pulse_Tag *tag, Eina_Binbuf **val);
uint8_t *untag_sample(Pulse_Tag *tag, pa_sample_spec *spec);
uint8_t *untag_channel_map(Pulse_Tag *tag, pa_channel_map *map);
uint8_t *untag_cvol(Pulse_Tag *tag, pa_cvolume *cvol);
uint8_t *untag_usec(Pulse_Tag *tag, uint64_t *val);
uint8_t *untag_proplist(Pulse_Tag *tag, Eina_Hash **props);
uint8_t *tag_simple_init(Pulse *conn, Pulse_Tag *tag, uint32_t val, PA_Tag type);
uint8_t *tag_proplist(Pulse_Tag *tag);
uint8_t *tag_volume(Pulse_Tag *tag, uint8_t channels, double vol);
uint8_t *tag_cvol(Pulse_Tag *tag, pa_cvolume *c);
void tag_finish(Pulse_Tag *tag);

Eina_Bool deserialize_tag(Pulse *conn, PA_Commands command, Pulse_Tag *tag);

void pulse_fake_free(void *d __UNUSED__, void *d2 __UNUSED__);
void pulse_tag_free(Pulse_Tag *tag);

void msg_recv_creds(Pulse *conn, Pulse_Tag *tag);
Eina_Bool msg_recv(Pulse *conn, Pulse_Tag *tag);
void msg_sendmsg_creds(Pulse *conn, Pulse_Tag *tag);
void msg_send_creds(Pulse *conn, Pulse_Tag *tag);
Eina_Bool msg_send(Pulse *conn, Pulse_Tag *tag);

extern Eina_Hash *pulse_sinks;
extern Eina_Hash *pulse_sources;

#endif
