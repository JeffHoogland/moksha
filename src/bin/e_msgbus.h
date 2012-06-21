#ifdef E_TYPEDEFS

typedef struct _E_Msgbus_Data E_Msgbus_Data;

#else
#ifndef E_MSGBUS_H
#define E_MSGBUS_H

/* This is the dbus subsystem, but e_dbus namespace is taken by e_dbus */

struct _E_Msgbus_Data
{
   E_DBus_Connection *conn;
   E_DBus_Object     *obj;
};

EINTERN int e_msgbus_init(void);
EINTERN int e_msgbus_shutdown(void);
EAPI void e_msgbus_interface_attach(E_DBus_Interface *iface);
EAPI void e_msgbus_interface_detach(E_DBus_Interface *iface);

#endif
#endif
