#include "e_mod_main.h"

/* actual module specifics */
static Eina_Array* ifaces = NULL;

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "IPC Extension"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   ifaces = eina_array_new(5);

   msgbus_lang_init(ifaces);
   msgbus_desktop_init(ifaces);
   msgbus_audit_init(ifaces);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   E_DBus_Interface* iface;
   Eina_Array_Iterator iter;
   size_t i;
   EINA_ARRAY_ITER_NEXT(ifaces, i, iface, iter)
     {
	e_msgbus_interface_detach(iface);
	e_dbus_interface_unref(iface);
     }
   eina_array_free(ifaces);
   ifaces = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
