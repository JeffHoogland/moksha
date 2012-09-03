#ifndef E_CONNMAN_H
#define E_CONNMAN_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <Eina.h>
#include <Ecore.h>
#include <E_DBus.h>

#include "log.h"

enum Connman_State
{
   CONNMAN_STATE_NONE = -1, /* All unknown states */
   CONNMAN_STATE_OFFLINE,
   CONNMAN_STATE_IDLE,
   CONNMAN_STATE_READY,
   CONNMAN_STATE_ONLINE,
};

enum Connman_Service_Type
{
   CONNMAN_SERVICE_TYPE_NONE = -1, /* All non-supported types */
   CONNMAN_SERVICE_TYPE_ETHERNET,
   CONNMAN_SERVICE_TYPE_WIFI,
};

struct Connman_Object
{
   const char *path;
   Eina_List *handlers; /* E_DBus_Signal_Handler */
};

struct Connman_Manager
{
   struct Connman_Object obj;

   Eina_Inlist *services; /* The prioritized list of services */

   /* Properties */
   enum Connman_State state;
   bool offline_mode;

   /* Private */
   struct
     {
        DBusPendingCall *get_services;
        DBusPendingCall *get_properties;
     } pending;
};

struct Connman_Service
{
   struct Connman_Object obj;
   EINA_INLIST;

   /* Properties */
   char *name;
   enum Connman_State state;
   enum Connman_Service_Type type;
};

/* Ecore Events */
extern int E_CONNMAN_EVENT_MANAGER_IN;
extern int E_CONNMAN_EVENT_MANAGER_OUT;

/* Daemon monitoring */

unsigned int e_connman_system_init(E_DBus_Connection *edbus_conn) EINA_ARG_NONNULL(1);
unsigned int e_connman_system_shutdown(void);

/* UI calls from econnman */

void econnman_mod_manager_update(struct Connman_Manager *cm);

#endif /* E_CONNMAN_H */
