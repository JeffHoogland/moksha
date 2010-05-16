#ifdef E_TYPEDEFS

/* enum for various event types */
typedef enum _E_Acpi_Type 
{
   E_ACPI_TYPE_UNKNOWN = 0,
   E_ACPI_TYPE_BATTERY,
   E_ACPI_TYPE_BUTTON,
   E_ACPI_TYPE_FAN,
   E_ACPI_TYPE_LID,
   E_ACPI_TYPE_PROCESSOR,
   E_ACPI_TYPE_SLEEP,
   E_ACPI_TYPE_POWER,
   E_ACPI_TYPE_THERMAL,
   E_ACPI_TYPE_VIDEO,
   E_ACPI_TYPE_WIFI
} E_Acpi_Type;

/* struct used to pass to event handlers */
typedef struct _E_Event_Acpi E_Event_Acpi;

#else
# ifndef E_ACPI_H
#  define E_ACPI_H

struct _E_Event_Acpi 
{
   const char *device, *bus_id;
   int type, data;
};

EAPI int e_acpi_init(void);
EAPI int e_acpi_shutdown(void);

extern EAPI int E_EVENT_ACPI_LID;
extern EAPI int E_EVENT_ACPI_BATTERY;
extern EAPI int E_EVENT_ACPI_BUTTON;
extern EAPI int E_EVENT_ACPI_SLEEP;
extern EAPI int E_EVENT_ACPI_WIFI;

# endif
#endif
