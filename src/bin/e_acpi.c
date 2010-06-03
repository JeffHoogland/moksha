#include "e.h"

/* TODO:
 * 
 * Sanatize data received from acpi for message status into something
 * meaningful (ie: 00000002 == LID_CLOSED, etc, etc).
 * 
 * Find someone with a WIFI that actually emits ACPI events and add/debug the 
 * E_EVENT_ACPI for wifi.
 * 
 * Add e_actions for bindings.
 */

/* local structures */
typedef struct _ACPIDevice ACPIDevice; //  for mapping device names to type
struct _ACPIDevice 
{
   const char *name;
   int type;
};

/* local function prototypes */
static int _e_acpi_cb_server_del(void *data __UNUSED__, int type __UNUSED__, void *event);
static int _e_acpi_cb_server_data(void *data __UNUSED__, int type __UNUSED__, void *event);
static void _e_acpi_cb_event_free(void *data __UNUSED__, void *event);
static int _e_acpi_lid_status_get(const char *device, const char *bus);
static int _e_acpi_cb_event(void *data __UNUSED__, int type __UNUSED__, void *event);

/* local variables */
static int _e_acpi_events_frozen = 0;
static Ecore_Con_Server *_e_acpid = NULL;
static Eina_List *_e_acpid_hdls = NULL;
static Eina_Hash *_e_acpid_devices = NULL;
static ACPIDevice _devices[] = 
{
   /* DO NOT TRANSLATE THESE. */
   /* standardized ACPI device name, corresponding E_ACPI_TYPE */
   {"ac_adapter", E_ACPI_TYPE_AC_ADAPTER},
   {"battery", E_ACPI_TYPE_BATTERY},
   {"button/lid", E_ACPI_TYPE_LID},
   {"button/power", E_ACPI_TYPE_POWER},
   {"button/sleep", E_ACPI_TYPE_SLEEP},
   {"fan", E_ACPI_TYPE_FAN},
   {"processor", E_ACPI_TYPE_PROCESSOR},
   {"thermal_zone", E_ACPI_TYPE_THERMAL},
   {"video", E_ACPI_TYPE_VIDEO},
   {NULL, E_ACPI_TYPE_UNKNOWN}
};

/* public variables */
EAPI int E_EVENT_ACPI_UNKNOWN = 0;
EAPI int E_EVENT_ACPI_AC_ADAPTER = 0;
EAPI int E_EVENT_ACPI_BATTERY = 0;
EAPI int E_EVENT_ACPI_FAN = 0;
EAPI int E_EVENT_ACPI_LID = 0;
EAPI int E_EVENT_ACPI_POWER = 0;
EAPI int E_EVENT_ACPI_PROCESSOR = 0;
EAPI int E_EVENT_ACPI_SLEEP = 0;
EAPI int E_EVENT_ACPI_THERMAL = 0;
EAPI int E_EVENT_ACPI_VIDEO = 0;
EAPI int E_EVENT_ACPI_WIFI = 0;

/* public functions */
EAPI int 
e_acpi_init(void) 
{
   const ACPIDevice *dev;

   E_EVENT_ACPI_UNKNOWN = ecore_event_type_new();
   E_EVENT_ACPI_AC_ADAPTER = ecore_event_type_new();
   E_EVENT_ACPI_BATTERY = ecore_event_type_new();
   E_EVENT_ACPI_FAN = ecore_event_type_new();
   E_EVENT_ACPI_LID = ecore_event_type_new();
   E_EVENT_ACPI_POWER = ecore_event_type_new();
   E_EVENT_ACPI_PROCESSOR = ecore_event_type_new();
   E_EVENT_ACPI_SLEEP = ecore_event_type_new();
   E_EVENT_ACPI_THERMAL = ecore_event_type_new();
   E_EVENT_ACPI_VIDEO = ecore_event_type_new();
   E_EVENT_ACPI_WIFI = ecore_event_type_new();

   /* check for running acpid */
   if (!ecore_file_exists("/var/run/acpid.socket")) return 1;

   /* try to connect to acpid socket */
   _e_acpid = ecore_con_server_connect(ECORE_CON_LOCAL_SYSTEM, 
				       "/var/run/acpid.socket", -1, NULL);
   if (!_e_acpid) return 1;

   /* create new device hash and fill it */
   _e_acpid_devices = eina_hash_string_superfast_new(NULL);
   for (dev = _devices; dev->type > E_ACPI_TYPE_UNKNOWN; dev++) 
     eina_hash_direct_add(_e_acpid_devices, dev->name, dev);

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
		      ecore_event_handler_add(E_EVENT_ACPI_AC_ADAPTER, 
					      _e_acpi_cb_event, NULL));
   _e_acpid_hdls = 
     eina_list_append(_e_acpid_hdls, 
		      ecore_event_handler_add(E_EVENT_ACPI_LID, 
					      _e_acpi_cb_event, NULL));
   _e_acpid_hdls = 
     eina_list_append(_e_acpid_hdls, 
		      ecore_event_handler_add(E_EVENT_ACPI_POWER, 
					      _e_acpi_cb_event, NULL));
   _e_acpid_hdls = 
     eina_list_append(_e_acpid_hdls, 
		      ecore_event_handler_add(E_EVENT_ACPI_SLEEP, 
					      _e_acpi_cb_event, NULL));
   return 1;
}

EAPI int 
e_acpi_shutdown(void) 
{
   Ecore_Event_Handler *hdl;

   /* destroy the device hash */
   if (_e_acpid_devices) eina_hash_free(_e_acpid_devices);
   _e_acpid_devices = NULL;

   /* cleanup event handlers */
   EINA_LIST_FREE(_e_acpid_hdls, hdl)
     ecore_event_handler_del(hdl);

   /* kill the server if existing */
   if (_e_acpid) ecore_con_server_del(_e_acpid);
   _e_acpid = NULL;
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
static int 
_e_acpi_cb_server_del(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   Ecore_Con_Event_Server_Del *ev;
   Ecore_Event_Handler *hdl;

   ev = event;
   if (ev->server != _e_acpid) return 1;

   /* cleanup event handlers */
   EINA_LIST_FREE(_e_acpid_hdls, hdl)
     ecore_event_handler_del(hdl);

   /* kill the server if existing */
   if (_e_acpid) ecore_con_server_del(_e_acpid);
   _e_acpid = NULL;
   return 1;
}

static int 
_e_acpi_cb_server_data(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   Ecore_Con_Event_Server_Data *ev;
   ACPIDevice *dev;
   E_Event_Acpi *acpi_event;
   int res, sig, status, event_type;
   char device[1024], bus[1024];

   ev = event;

   /* write out actual acpi received data to stdout for debugging
   res = fwrite(ev->data, ev->size, 1, stdout);
    */

   /* parse out this acpi string into separate pieces */
   if (sscanf(ev->data, "%s %s %d %d", device, bus, &sig, &status) != 4)
     return 1;

   /* create new event structure to raise */
   acpi_event = E_NEW(E_Event_Acpi, 1);
   acpi_event->bus_id = eina_stringshare_add(bus);
   acpi_event->signal = sig;
   acpi_event->status = status;

   /* determine which device this event is for */
   if ((dev = eina_hash_find(_e_acpid_devices, device))) 
     {
	acpi_event->device = eina_stringshare_add(dev->name);
	acpi_event->type = dev->type;
     }
   else 
     {
	acpi_event->device = eina_stringshare_add(device);
	acpi_event->type = E_ACPI_TYPE_UNKNOWN;
     }

   /* based on device type, determine the event to raise */
   switch (acpi_event->type) 
     {
      case E_ACPI_TYPE_AC_ADAPTER:
	event_type = E_EVENT_ACPI_AC_ADAPTER;
	break;
      case E_ACPI_TYPE_BATTERY:
	event_type = E_EVENT_ACPI_BATTERY;
	break;
      case E_ACPI_TYPE_FAN:
	event_type = E_EVENT_ACPI_FAN;
	break;
      case E_ACPI_TYPE_LID:
	event_type = E_EVENT_ACPI_LID;
	acpi_event->status = 
	  _e_acpi_lid_status_get(acpi_event->device, acpi_event->bus_id);
	break;
      case E_ACPI_TYPE_POWER:
	event_type = E_EVENT_ACPI_POWER;
	break;
      case E_ACPI_TYPE_PROCESSOR:
	event_type = E_EVENT_ACPI_PROCESSOR;
	break;
      case E_ACPI_TYPE_SLEEP:
	event_type = E_EVENT_ACPI_SLEEP;
	break;
      case E_ACPI_TYPE_THERMAL:
	event_type = E_EVENT_ACPI_THERMAL;
	break;
      case E_ACPI_TYPE_VIDEO:
	event_type = E_EVENT_ACPI_VIDEO;
	break;
      default:
	event_type = E_EVENT_ACPI_UNKNOWN;
	break;
     }

   /* actually raise the event */
   ecore_event_add(event_type, acpi_event, _e_acpi_cb_event_free, NULL);
   return 1;
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
   if (!strcmp(buff, "open"))
     return E_ACPI_LID_OPEN;
   else if (!strcmp(buff, "closed"))
     return E_ACPI_LID_CLOSED;
   else
     return E_ACPI_LID_UNKNOWN;
}

static int 
_e_acpi_cb_event(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   E_Event_Acpi *ev;

   ev = event;
   if (_e_acpi_events_frozen > 0) return 1;
   e_bindings_acpi_event_handle(E_BINDING_CONTEXT_NONE, NULL, ev);
   return 1;
}
