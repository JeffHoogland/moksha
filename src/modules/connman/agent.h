#ifndef E_CONNMAN_AGENT_H
#define E_CONNMAN_AGENT_H

#include "E_Connman.h"

unsigned int econnman_agent_init(E_DBus_Connection *edbus_conn) EINA_ARG_NONNULL(1);
unsigned int econnman_agent_shutdown(void);

#endif /* E_CONNMAN_H */
