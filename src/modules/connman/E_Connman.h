#ifndef E_CONNMAN_H
#define E_CONNMAN_H

#include <stdint.h>
#include <stdio.h>

#include <Eina.h>
#include <Ecore.h>
#include <E_DBus.h>

#include "log.h"

/* Ecore Events */
extern int E_CONNMAN_EVENT_MANAGER_IN;
extern int E_CONNMAN_EVENT_MANAGER_OUT;

/* Daemon monitoring */

unsigned int e_connman_system_init(E_DBus_Connection *edbus_conn) EINA_ARG_NONNULL(1);
unsigned int e_connman_system_shutdown(void);

#endif /* E_CONNMAN_H */
