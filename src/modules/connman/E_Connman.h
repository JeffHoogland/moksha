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
   CONNMAN_STATE_ASSOCIATION,
   CONNMAN_STATE_CONFIGURATION,
   CONNMAN_STATE_READY,
   CONNMAN_STATE_ONLINE,
   CONNMAN_STATE_DISCONNECT,
   CONNMAN_STATE_FAILURE,
};

enum Connman_Service_Type
{
   CONNMAN_SERVICE_TYPE_NONE = -1, /* All non-supported types */
   CONNMAN_SERVICE_TYPE_ETHERNET,
   CONNMAN_SERVICE_TYPE_WIFI,
   CONNMAN_SERVICE_TYPE_BLUETOOTH,
   CONNMAN_SERVICE_TYPE_CELLULAR,
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
   Eina_Array *security;
   enum Connman_State state;
   enum Connman_Service_Type type;
   uint8_t strength;

   /* Private */
   struct
     {
        DBusPendingCall *connect;
        DBusPendingCall *disconnect;
        void *data;
     } pending;
};

/* Ecore Events */
extern int E_CONNMAN_EVENT_MANAGER_IN;
extern int E_CONNMAN_EVENT_MANAGER_OUT;

/* Daemon monitoring */

unsigned int e_connman_system_init(E_DBus_Connection *edbus_conn) EINA_ARG_NONNULL(1);
unsigned int e_connman_system_shutdown(void);

/* Requests from UI */

/**
 * Find service using a non-stringshared path
 */
struct Connman_Service *econnman_manager_find_service(struct Connman_Manager *cm, const char *path) EINA_ARG_NONNULL(1, 2);

typedef void (*Econnman_Simple_Cb)(void *data, const char *error);

bool econnman_service_connect(struct Connman_Service *cs, Econnman_Simple_Cb cb, void *data);
bool econnman_service_disconnect(struct Connman_Service *cs, Econnman_Simple_Cb cb, void *data);

/* UI calls from econnman */

/*
 * TODO: transform these in proper callbacks or ops that UI calls to register
 * itself
 */

void econnman_mod_manager_update(struct Connman_Manager *cm);
void econnman_mod_manager_inout(struct Connman_Manager *cm);
void econnman_mod_services_changed(struct Connman_Manager *cm);

/* Util */

const char *econnman_state_to_str(enum Connman_State state);
const char *econnman_service_type_to_str(enum Connman_Service_Type type);

#endif /* E_CONNMAN_H */
