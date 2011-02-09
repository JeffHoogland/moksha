#include "e.h"

/* TODO:
 * 
 * Sanatize data received from acpi for message status into something
 * meaningful (ie: 00000002 == LID_CLOSED, etc, etc).
 * 
 * Find someone with a WIFI that actually emits ACPI events and add/debug the 
 * E_EVENT_ACPI for wifi.
 * 
 */

/* local structures */
/* for simple acpi device mapping */
typedef struct _E_ACPI_Device_Simple      E_ACPI_Device_Simple; 
typedef struct _E_ACPI_Device_Multiplexed E_ACPI_Device_Multiplexed;
 
struct _E_ACPI_Device_Simple 
{
   const char *name;
   // ->
   int         type;
};

struct _E_ACPI_Device_Multiplexed
{
   const char *name;
   const char *bus;
   int         status;
   // ->
   int         type;
};

/* local function prototypes */
static Eina_Bool _e_acpi_cb_server_del(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_acpi_cb_server_data(void *data __UNUSED__, int type __UNUSED__, void *event);
static void _e_acpi_cb_event_free(void *data __UNUSED__, void *event);
static int _e_acpi_lid_status_get(const char *device, const char *bus);
static Eina_Bool _e_acpi_cb_event(void *data __UNUSED__, int type __UNUSED__, void *event);

/* local variables */
static int _e_acpi_events_frozen = 0;
static Ecore_Con_Server *_e_acpid = NULL;
static Eina_List *_e_acpid_hdls = NULL;
static Eina_Strbuf *acpibuf = NULL;

static E_ACPI_Device_Simple _devices_simple[] =
{
   /* NB: DO NOT TRANSLATE THESE. */
   {"ac_adapter",   E_ACPI_TYPE_AC_ADAPTER},
   {"battery",      E_ACPI_TYPE_BATTERY},
   {"button/lid",   E_ACPI_TYPE_LID},
   {"button/power", E_ACPI_TYPE_POWER},
   {"button/sleep", E_ACPI_TYPE_SLEEP},
   {"fan",          E_ACPI_TYPE_FAN},
   {"processor",    E_ACPI_TYPE_PROCESSOR},
   {"thermal_zone", E_ACPI_TYPE_THERMAL},
   {"video",        E_ACPI_TYPE_VIDEO},
   
   {NULL,           E_ACPI_TYPE_UNKNOWN}
};

static E_ACPI_Device_Multiplexed _devices_multiplexed[] =
{
   /* NB: DO NOT TRANSLATE THESE. */
/* Sony VAIO - VPCF115FM / PCG-81114L - nvidia gfx */
   {"sony/hotkey", NULL,   0x10, E_ACPI_TYPE_BRIGHTNESS_DOWN},
   {"sony/hotkey", NULL,   0x11, E_ACPI_TYPE_BRIGHTNESS_UP},
   {"sony/hotkey", NULL,   0x12, E_ACPI_TYPE_VIDEO},
   {"sony/hotkey", NULL,   0x14, E_ACPI_TYPE_ZOOM_OUT},
   {"sony/hotkey", NULL,   0x15, E_ACPI_TYPE_ZOOM_IN},
   {"sony/hotkey", NULL,   0x17, E_ACPI_TYPE_HIBERNATE},
   {"sony/hotkey", NULL,   0xa6, E_ACPI_TYPE_ASSIST},
   {"sony/hotkey", NULL,   0x20, E_ACPI_TYPE_S1},
   {"sony/hotkey", NULL,   0xa5, E_ACPI_TYPE_VAIO},

/* Sony VAIO - X505 - intel gfx */
   {"sony/hotkey", NULL,   0x0e, E_ACPI_TYPE_MUTE},
   {"sony/hotkey", NULL,   0x0f, E_ACPI_TYPE_VOLUME},
   {"sony/hotkey", NULL,   0x10, E_ACPI_TYPE_BRIGHTNESS},
   {"sony/hotkey", NULL,   0x12, E_ACPI_TYPE_VIDEO},
   
/* HP Compaq Presario - CQ61 - intel gfx */
/** interesting these get auto-mapped to keys in x11. here for documentation
 ** but not enabled as we can use regular keybinds for these
   {"video",       "DD03", 0x87, E_ACPI_TYPE_BRIGHTNESS_DOWN},
   {"video",       "DD03", 0x86, E_ACPI_TYPE_BRIGHTNESS_UP},
   {"video",       "OVGA", 0x80, E_ACPI_TYPE_VIDEO},
 */
/* END */   
   {NULL,          NULL, 0x00, E_ACPI_TYPE_UNKNOWN}
};

/* public variables */
EAPI int E_EVENT_ACPI = 0;

/* public functions */
EINTERN int
e_acpi_init(void) 
{
   E_EVENT_ACPI = ecore_event_type_new();

   /* check for running acpid */
   if (!ecore_file_exists("/var/run/acpid.socket")) return 1;

   /* try to connect to acpid socket */
   _e_acpid = ecore_con_server_connect(ECORE_CON_LOCAL_SYSTEM, 
				       "/var/run/acpid.socket", -1, NULL);
   if (!_e_acpid) return 1;

   /* setup handlers */
   _e_acpid_hdls = 
      eina_list_append(_e_acpid_hdls, 
                       ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DEL, 
                                               _e_acpi_cb_server_del, NULL));
   _e_acpid_hdls = 
      eina_list_append(_e_acpid_hdls, 
                       ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DATA, 
                                               _e_acpi_cb_server_data, NULL));
   
   /* Add handlers for standard acpi events */
   _e_acpid_hdls = 
      eina_list_append(_e_acpid_hdls, 
                       ecore_event_handler_add(E_EVENT_ACPI,
                                               _e_acpi_cb_event, NULL));
   return 1;
}

EINTERN int
e_acpi_shutdown(void) 
{
   Ecore_Event_Handler *hdl;

   /* cleanup event handlers */
   EINA_LIST_FREE(_e_acpid_hdls, hdl) ecore_event_handler_del(hdl);

   /* kill the server if existing */
   if (_e_acpid)
     {
        ecore_con_server_del(_e_acpid);
        _e_acpid = NULL;
     }
   return 1;
}

EAPI void 
e_acpi_events_freeze(void) 
{
   _e_acpi_events_frozen++;
}

EAPI void 
e_acpi_events_thaw(void) 
{
   _e_acpi_events_frozen--;
   if (_e_acpi_events_frozen < 0) _e_acpi_events_frozen = 0;
}

/* local functions */
static Eina_Bool
_e_acpi_cb_server_del(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Con_Event_Server_Del *ev;
   Ecore_Event_Handler *hdl;

   ev = event;
   if (ev->server != _e_acpid) return ECORE_CALLBACK_PASS_ON;

   /* cleanup event handlers */
   EINA_LIST_FREE(_e_acpid_hdls, hdl) ecore_event_handler_del(hdl);

   /* kill the server if existing */
   if (_e_acpid)
     {
        ecore_con_server_del(_e_acpid);
        _e_acpid = NULL;
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_acpi_cb_server_data(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Con_Event_Server_Data *ev;
   E_Event_Acpi *acpi_event;
   int sig, status, i, done = 0;
   char device[1024], bus[1024], *sdata;
   const char *str, *p;
   
   ev = event;

   /* write out actual acpi received data to stdout for debugging
   res = fwrite(ev->data, ev->size, 1, stdout);
    */
   /* data from a server isnt a string - its not 0 byte terminated. it's just
    * a blob of data. copy to string and 0 byte terminate it so it can be
    * string-swizzled/parsed etc. */
   if (!acpibuf) acpibuf = eina_strbuf_new();
   eina_strbuf_append_n(acpibuf, ev->data, ev->size);
   str = eina_strbuf_string_get(acpibuf);
   p = strchr(str, '\n');
   if (!p) return ECORE_CALLBACK_PASS_ON;
   while (p)
     {
        sdata = alloca(p - str + 1);
        strncpy(sdata, str, (int)(p - str));
        sdata[p - str] = 0;
        /* parse out this acpi string into separate pieces */
        if (sscanf(sdata, "%1023s %1023s %x %x", 
                   device, bus, &sig, &status) == 4)
          {
             /* create new event structure to raise */
             acpi_event = E_NEW(E_Event_Acpi, 1);
             acpi_event->bus_id = eina_stringshare_add(bus);
             acpi_event->signal = sig;
             acpi_event->status = status;
             
             /* FIXME: add in a key faking layer */
             if (!done)
               {
                  for (i = 0; _devices_multiplexed[i].name; i++)
                    {
                       // if device name matches
                       if ((!strcmp(device, _devices_multiplexed[i].name)) &&
                           // AND busname not set OR device name matches
                           (!_devices_multiplexed[i].bus ||
                               (_devices_multiplexed[i].bus &&
                                   (!strcmp(bus, _devices_multiplexed[i].bus)))) &&
                           // AND status matches
                           (_devices_multiplexed[i].status == status))
                         {
                            acpi_event->type = _devices_multiplexed[i].type;
                            done = 1;
                            break;
                         }
                    }
               }
             if (!done)
               {
                  // if device name matches
                  for (i = 0; _devices_simple[i].name; i++)
                    {
                       if (!strcmp(device, _devices_simple[i].name))
                         {
                            acpi_event->type = _devices_simple[i].type;
                            done = 1;
                            break;
                         }
                    }
               }
             if (!done)
               {
                  free(acpi_event);
                  acpi_event = NULL;
               }
             else
               {
                  switch (acpi_event->type)
                    {
                     case E_ACPI_TYPE_LID:
                       acpi_event->status = 
                          _e_acpi_lid_status_get(device, bus);
                       break;
                     default:
                       break;
                    }
                  /* actually raise the event */
                  ecore_event_add(E_EVENT_ACPI, acpi_event, 
                                  _e_acpi_cb_event_free, NULL);
               }
          }
        str = p + 1;
        p = strchr(str, '\n');
     }
   if (str[0] == 0)
     {
        eina_strbuf_free(acpibuf);
        acpibuf = NULL;
     }
   else
     {
        Eina_Strbuf *newbuf;
        
        newbuf = eina_strbuf_new();
        eina_strbuf_append(newbuf, str);
        eina_strbuf_free(acpibuf);
        acpibuf = newbuf;
     }
   return ECORE_CALLBACK_PASS_ON;
}

static void 
_e_acpi_cb_event_free(void *data __UNUSED__, void *event) 
{
   E_Event_Acpi *ev;

   if (!(ev = event)) return;
   if (ev->device) eina_stringshare_del(ev->device);
   if (ev->bus_id) eina_stringshare_del(ev->bus_id);
   E_FREE(ev);
}

static int 
_e_acpi_lid_status_get(const char *device, const char *bus) 
{
   FILE *f;
   int i = 0;
   char buff[PATH_MAX], *ret;

   /* the acpi driver code in the kernel has a nice acpi function to return
    * the lid status easily, but that function is not exposed for user_space
    * so we need to check the proc fs to get the actual status */

   /* make sure we have a device and bus */
   if ((!device) || (!bus)) return E_ACPI_LID_UNKNOWN;

   /* open the state file from /proc */
   snprintf(buff, sizeof(buff), "/proc/acpi/%s/%s/state", device, bus);
   if (!(f = fopen(buff, "r"))) return E_ACPI_LID_UNKNOWN;

   /* read the line from state file */
   buff[0] = '\0';
   ret = fgets(buff, 1024, f);
   fclose(f);

   /* parse out state file */
   i = 0;
   while (buff[i] != ':') i++;
   while (!isalnum(buff[i])) i++;

   /* compare value from state file and return something sane */
   if (!strcmp(buff, "open")) return E_ACPI_LID_OPEN;
   else if (!strcmp(buff, "closed")) return E_ACPI_LID_CLOSED;
   else return E_ACPI_LID_UNKNOWN;
}

static Eina_Bool
_e_acpi_cb_event(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Acpi *ev;

   ev = event;
   if (_e_acpi_events_frozen > 0) return ECORE_CALLBACK_PASS_ON;
   e_bindings_acpi_event_handle(E_BINDING_CONTEXT_NONE, NULL, ev);
   return ECORE_CALLBACK_PASS_ON;
}
