#include "e.h"

/* local function prototypes */
static int _e_acpi_cb_server_del(void *data __UNUSED__, int type __UNUSED__, void *event);
static int _e_acpi_cb_server_data(void *data __UNUSED__, int type __UNUSED__, void *event);
static void _e_acpi_cb_event_free(void *data __UNUSED__, void *event);

/* local variables */
static Ecore_Con_Server *_e_acpid = NULL;
static Eina_List *_e_acpid_hdls = NULL;

/* public variables */
EAPI int E_EVENT_ACPI_LID = 0;
EAPI int E_EVENT_ACPI_BATTERY = 0;
EAPI int E_EVENT_ACPI_BUTTON = 0;
EAPI int E_EVENT_ACPI_SLEEP = 0;
EAPI int E_EVENT_ACPI_WIFI = 0;

/* public functions */
EAPI int 
e_acpi_init(void) 
{
   E_EVENT_ACPI_LID = ecore_event_type_new();
   E_EVENT_ACPI_BATTERY = ecore_event_type_new();
   E_EVENT_ACPI_BUTTON = ecore_event_type_new();
   E_EVENT_ACPI_SLEEP = ecore_event_type_new();
   E_EVENT_ACPI_WIFI = ecore_event_type_new();

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
   return 1;
}

EAPI int 
e_acpi_shutdown(void) 
{
   Ecore_Event_Handler *hdl;

   /* cleanup event handlers */
   EINA_LIST_FREE(_e_acpid_hdls, hdl)
     ecore_event_handler_del(hdl);

   /* kill the server if existing */
   if (_e_acpid) ecore_con_server_del(_e_acpid);
   _e_acpid = NULL;
   return 1;
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
   int res;

   ev = event;

   res = fwrite(ev->data, ev->size, 1, stdout);
   printf("\n");

   /* TODO: Need to parse this data and raise events according to
    * the type of acpi object. See ACPI notes below */

   /* raise the event

   E_Event_Acpi *acpi_event;

   acpi_event = E_NEW(E_Event_Acpi, 1);
   acpi_event->device = "battery";
   acpi_event->bus_id = "BAT0";
   acpi_event->event_type = "BATTERY_STATUS_CHANGED"; // make these standard E_ACPI enums
   acpi_event->event_data = 1; //  change to something more meaningful
   ecore_event_add(E_EVENT_ACPI_LID, acpi_event, _e_acpi_cb_event_free, NULL);
    */

   return 1;
}

static void 
_e_acpi_cb_event_free(void *data __UNUSED__, void *event) 
{
   E_FREE(event);
}


/* ACPI NOTES
 *
 * http://www.columbia.edu/~ariel/acpi/acpi_howto.txt (Section 6.4)
 *
 * Typical data looks like:
 *  completed event "processor CPU0 00000080 00000004"
 *  received event "ac_adapter AC 00000080 00000001"
 *  received event "battery BAT0 00000080 00000001"
 *  received event "button/power PBTN 00000080 00000001"
 */
