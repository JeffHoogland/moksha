#ifdef E_TYPEDEFS

/* enum for various event types */
typedef enum _E_Acpi_Type 
{
   E_ACPI_TYPE_UNKNOWN = 0,
      
   E_ACPI_TYPE_AC_ADAPTER, //
   E_ACPI_TYPE_BATTERY,
   E_ACPI_TYPE_BUTTON,
   E_ACPI_TYPE_FAN,
   E_ACPI_TYPE_LID,
   E_ACPI_TYPE_POWER,
   E_ACPI_TYPE_PROCESSOR,
   E_ACPI_TYPE_SLEEP,
   E_ACPI_TYPE_THERMAL,
   E_ACPI_TYPE_VIDEO,
   E_ACPI_TYPE_WIFI,
   E_ACPI_TYPE_HIBERNATE,
   E_ACPI_TYPE_ZOOM_OUT,
   E_ACPI_TYPE_ZOOM_IN,
   E_ACPI_TYPE_BRIGHTNESS_DOWN,
   E_ACPI_TYPE_BRIGHTNESS_UP,
   E_ACPI_TYPE_ASSIST,
   E_ACPI_TYPE_S1,
   E_ACPI_TYPE_VAIO
} E_Acpi_Type;

/* enum for acpi signals */
typedef enum _E_Acpi_Device_Signal 
{
   E_ACPI_DEVICE_SIGNAL_UNKNOWN,
   E_ACPI_DEVICE_SIGNAL_NOTIFY = 80,
   E_ACPI_DEVICE_SIGNAL_CHANGED = 82, // device added or removed
   E_ACPI_DEVICE_SIGNAL_AWAKE = 83,
   E_ACPI_DEVICE_SIGNAL_EJECT = 84
} E_Acpi_Device_Signal;

/* enum for lid status */
typedef enum _E_Acpi_Lid_Status 
{
   E_ACPI_LID_UNKNOWN,
   E_ACPI_LID_CLOSED,
   E_ACPI_LID_OPEN
} E_Acpi_Lid_Status;

/* struct used to pass to event handlers */
typedef struct _E_Event_Acpi E_Event_Acpi;

#else
# ifndef E_ACPI_H
#  define E_ACPI_H

struct _E_Event_Acpi 
{
   const char *device, *bus_id;
   int         type, signal, status;
};

EINTERN int e_acpi_init(void);
EINTERN int e_acpi_shutdown(void);

EAPI void e_acpi_events_freeze(void);
EAPI void e_acpi_events_thaw(void);

extern EAPI int E_EVENT_ACPI;

# endif
#endif
