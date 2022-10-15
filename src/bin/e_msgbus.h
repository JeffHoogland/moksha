#ifdef E_TYPEDEFS

typedef struct _E_Msgbus_Data                     E_Msgbus_Data;
typedef struct _E_Msgbus_Data_Screensaver_Inhibit E_Msgbus_Data_Screensaver_Inhibit;

#else
#ifndef E_MSGBUS_H
#define E_MSGBUS_H

/* This is the dbus subsystem, but eldbus namespace is taken by eldbus */

struct _E_Msgbus_Data
{
   Eldbus_Connection *conn;
   Eldbus_Service_Interface *e_iface;
   Eldbus_Service_Interface *screensaver_iface;
   Eldbus_Service_Interface *screensaver_iface2;
   Eina_List                *screensaver_inhibits;
};

struct _E_Msgbus_Data_Screensaver_Inhibit
{
   const char *application;
   const char *reason;
   const char *sender;
   unsigned int cookie;
};

EINTERN int e_msgbus_init(void);
EINTERN int e_msgbus_shutdown(void);
EAPI Eldbus_Service_Interface *e_msgbus_interface_attach(const Eldbus_Service_Interface_Desc *desc);
EAPI void e_msgbus_screensaver_inhibit_remove(unsigned int cookie);

EAPI extern E_Msgbus_Data *e_msgbus_data;
#endif
#endif
