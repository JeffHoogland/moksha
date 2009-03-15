#ifndef E_IFACE_H
#define E_IFACE_H

#include <Ecore.h>
#include <E_DBus.h>
#include <Evas.h>
#include <eina_stringshare.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* IFACE HEADER */
typedef enum _Interface_Event
{
   IFACE_EVENT_ADD,
     IFACE_EVENT_DEL,
     IFACE_EVENT_IPV4_CHANGE,
     IFACE_EVENT_NETWORK_SELECTION_CHANGE,
     IFACE_EVENT_SCAN_NETWORK_ADD,
     IFACE_EVENT_SCAN_NETWORK_DEL,
     IFACE_EVENT_SCAN_NETWORK_CHANGE,
     IFACE_EVENT_SIGNAL_CHANGE,
     IFACE_EVENT_STATE_CHANGE,
     IFACE_EVENT_POLICY_CHANGE,
} Interface_Event;

typedef struct _Interface_Network Interface_Network;
typedef struct _Interface_Properties Interface_Properties;
typedef struct _Interface_IPv4 Interface_IPv4;
typedef struct _Interface_Network_Selection Interface_Network_Selection;
typedef struct _Interface_Callback Interface_Callback;
typedef struct _Interface Interface;

struct _Interface_Network
{
   const char *essid;
   const char *bssid;
   const char *security;
   int signal_strength;
   double last_seen_time;
};

struct _Interface_Properties
{
   const char *product;
   const char *vendor;
   int signal_strength;
   const char *driver;
   const char *state;
   const char *policy;
   const char *type;
};

struct _Interface_IPv4
{
   const char *method;
   const char *address;
   const char *gateway;
   const char *netmask;
};

struct _Interface_Network_Selection
{
   const char *id;
   const char *pass;
};

struct _Interface_Callback
{
   Interface_Event event;
   void (*func) (void *data, Interface *iface, Interface_Network *ifnet);
   void *data;
   unsigned char delete_me : 1;
};

struct _Interface
{
   /* public - read-only. look but don't touch */
   const char *ifpath;
   Interface_Properties prop;
   Interface_IPv4 ipv4;
   Interface_Network_Selection network_selection;
   Eina_List *networks;
   int signal_strength;

   /* private - don't touch */
   Eina_List *callbacks;
   struct {
      E_DBus_Signal_Handler *network_found;
      E_DBus_Signal_Handler *signal_changed;
      E_DBus_Signal_Handler *state_changed;
      E_DBus_Signal_Handler *policy_changed;
      E_DBus_Signal_Handler *network_changed;
      E_DBus_Signal_Handler *ipv4_changed;
   } sigh;
   Ecore_Timer *network_timeout;
   int ref;
   unsigned char newif : 1;
};

/* IFACE PUBLIC API */
Interface *iface_add         (const char *ifpath);
void       iface_ref         (Interface *iface);
void       iface_unref       (Interface *iface);
Interface *iface_find        (const char *ifpath);
void       iface_network_set (Interface *iface, const char *ssid, const char *pass);
void       iface_policy_set  (Interface *iface, const char *policy);
void       iface_scan        (Interface *iface, const char *policy);
void       iface_ipv4_set    (Interface *iface, const char *method, const char *address, const char *gateway, const char *netmask);
void       iface_callback_add(Interface *iface, Interface_Event event, void (*func) (void *data, Interface *iface, Interface_Network *ifnet), void *data);
void       iface_callback_del(Interface *iface, Interface_Event event, void (*func) (void *data, Interface *iface, Interface_Network *ifnet), void *data);

void       iface_system_init         (E_DBus_Connection *edbus_conn);
void iface_system_shutdown(void);
void       iface_system_callback_add (Interface_Event event, void (*func) (void *data, Interface *iface, Interface_Network *ifnet), void *data);
void       iface_system_callback_del (Interface_Event event, void (*func) (void *data, Interface *iface, Interface_Network *ifnet), void *data);
#endif
