#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "e.h"

/* sub-module initializers */
void msgbus_lang_init(Eina_Array *ifaces);
void msgbus_desktop_init(Eina_Array *ifaces);
void msgbus_audit_init(Eina_Array *ifaces);

/**
 * @addtogroup Optional_Control
 * @{
 *
 * @defgroup Module_Msgbus MsgBus (DBus Remote Control)
 *
 * Allows Enlightenment to be controlled by DBus IPC (Inter Process
 * Communication) protocol.
 *
 * @see http://www.freedesktop.org/wiki/Software/dbus
 * @}
 */

#endif
