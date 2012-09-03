#ifndef E_CONNMAN_AGENT_H
#define E_CONNMAN_AGENT_H

#include "E_Connman.h"

#define AGENT_PATH "/org/enlightenment/connman/agent"

E_Connman_Agent *econnman_agent_new(E_DBus_Connection *edbus_conn) EINA_ARG_NONNULL(1);
void econnman_agent_del(E_Connman_Agent *agent);

#endif /* E_CONNMAN_H */
