/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
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

EAPI int e_msgbus_init(void);
EAPI int e_msgbus_shutdown(void);
EAPI void e_msgbus_interface_attach(E_DBus_Interface *iface);
EAPI void e_msgbus_interface_detach(E_DBus_Interface *iface);

#endif
#endif
