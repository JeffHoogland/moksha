/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#ifndef HAVE_EDBUS
#define E_DBus_Interface void
#endif

typedef struct _E_Msgbus_Data E_Msgbus_Data;

#else
#ifndef E_MSGBUS_H
#define E_MSGBUS_H

/* This is the dbus subsystem, but e_dbus namespace is taken by e_dbus */

struct _E_Msgbus_Data 
{
#ifdef HAVE_EDBUS
   E_DBus_Connection *conn;
   E_DBus_Object     *obj;
#endif
};

EAPI int e_msgbus_init(void);
EAPI int e_msgbus_shutdown(void);

#endif
#endif
