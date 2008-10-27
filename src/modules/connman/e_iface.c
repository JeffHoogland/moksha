#include "e_iface.h"

/* IFACE INTERNALS */
static E_DBus_Connection *conn = NULL;
static Eina_List *callbacks = NULL;
static E_DBus_Signal_Handler *sigh_name_ownerchanged = NULL;
static E_DBus_Signal_Handler *sigh_interface_added = NULL;
static E_DBus_Signal_Handler *sigh_interface_removed = NULL;
static Eina_List *interfaces = NULL;

static Eina_List *
iface_callback(Eina_List *callbacks, Interface_Event event, Interface *iface, Interface_Network *ifnet)
{
   Eina_List *l;
   Eina_List *deletes = NULL;

   for (l = callbacks; l; l = l->next)
     {
	Interface_Callback *cb;

	cb = l->data;
	if (cb->delete_me)
	  {
	     deletes = eina_list_append(deletes, l->data);
	     continue;
	  }
	if (cb->event == event)
	  cb->func(cb->data, iface, ifnet);
     }
   while (deletes)
     {
	callbacks = eina_list_remove(callbacks, deletes->data);
	free(deletes->data);
	deletes = eina_list_remove_list(deletes, deletes);
     }
   return callbacks;
}

static Interface_IPv4 *
iface_ipv4_decode(DBusMessage *msg)
{
   DBusMessageIter array, iter, item, val;
   Interface_IPv4 *d;

   d = calloc(1, sizeof(Interface_IPv4));
   if (!d) return NULL;

   if (dbus_message_iter_init(msg, &array))
     {
	if (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_ARRAY)
	  {
	     dbus_message_iter_recurse(&array, &iter);
	     while (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_DICT_ENTRY)
	       {
                  char *key, *v;
		  int type;

		  dbus_message_iter_recurse(&iter, &item);
		  key = NULL;
		  dbus_message_iter_get_basic(&item, &key);
		  dbus_message_iter_next(&item);
		  dbus_message_iter_recurse(&item, &val);
		  type = dbus_message_iter_get_arg_type(&val);
		  v = NULL;
		  if ((!strcmp(key, "Method")) && (type == DBUS_TYPE_STRING))
		    {
		       dbus_message_iter_get_basic(&val, &v);
		       if (v) d->method = eina_stringshare_add(v);
		    }
		  else if ((!strcmp(key, "Address")) && (type == DBUS_TYPE_STRING))
		    {
		       dbus_message_iter_get_basic(&val, &v);
		       if (v) d->address = eina_stringshare_add(v);
		    }
		  else if ((!strcmp(key, "Gateway")) && (type == DBUS_TYPE_STRING))
		    {
		       dbus_message_iter_get_basic(&val, &v);
		       if (v) d->gateway = eina_stringshare_add(v);
		    }
		  else if ((!strcmp(key, "Netmask")) && (type == DBUS_TYPE_STRING))
		    {
		       dbus_message_iter_get_basic(&val, &v);
		       if (v) d->netmask = eina_stringshare_add(v);
		    }
		  dbus_message_iter_next(&iter);
	       }
	  }
     }
   return d;
}

static void *
iface_getipv4_unmarhsall(DBusMessage *msg, DBusError *err)
{
   return iface_ipv4_decode(msg);
}

static void
iface_getipv4_callback(void *data, void *ret, DBusError *err)
{
   Interface *iface = data;
   Interface_IPv4 *d = ret;

   if (!d)
     {
	iface_unref(iface);
	return;
     }
   if (iface->ipv4.method) eina_stringshare_del(iface->ipv4.method);
   if (iface->ipv4.address) eina_stringshare_del(iface->ipv4.address);
   if (iface->ipv4.gateway) eina_stringshare_del(iface->ipv4.gateway);
   if (iface->ipv4.netmask) eina_stringshare_del(iface->ipv4.netmask);
   memcpy(&(iface->ipv4), d, sizeof(Interface_IPv4));
   iface->callbacks = iface_callback(iface->callbacks,
				     IFACE_EVENT_IPV4_CHANGE,
				     iface, NULL);
   iface_unref(iface);
}

static void
iface_getipv4_result_free(void *data)
{
   Interface_IPv4 *d = data;
   free(d);
}

static Interface_Network_Selection *
iface_network_selection_decode(DBusMessage *msg)
{
   DBusMessageIter array, iter, item, val;
   Interface_Network_Selection *d;

   d = calloc(1, sizeof(Interface_Network_Selection));
   if (!d) return NULL;

   if (dbus_message_iter_init(msg, &array))
     {
	if (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_ARRAY)
	  {
	     dbus_message_iter_recurse(&array, &iter);
	     while (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_DICT_ENTRY)
	       {
                  char *key, *v;
		  int type;

		  dbus_message_iter_recurse(&iter, &item);
		  key = NULL;
		  dbus_message_iter_get_basic(&item, &key);
		  dbus_message_iter_next(&item);
		  dbus_message_iter_recurse(&item, &val);
		  type = dbus_message_iter_get_arg_type(&val);
		  v = NULL;
		  if ((!strcmp(key, "ESSID")) && (type == DBUS_TYPE_STRING))
		    {
		       dbus_message_iter_get_basic(&val, &v);
		       if (v) d->id = eina_stringshare_add(v);
		    }
		  else if ((!strcmp(key, "PSK")) && (type == DBUS_TYPE_STRING))
		    {
		       dbus_message_iter_get_basic(&val, &v);
		       if (v) d->pass = eina_stringshare_add(v);
		    }
		  dbus_message_iter_next(&iter);
	       }
	  }
     }
   return d;
}

static void *
iface_getnetwork_unmarhsall(DBusMessage *msg, DBusError *err)
{
   return iface_network_selection_decode(msg);
}

static void
iface_getnetwork_callback(void *data, void *ret, DBusError *err)
{
   Interface *iface = data;
   Interface_Network_Selection *d = ret;

   if (!d)
     {
	iface_unref(iface);
	return;
     }
   if (iface->network_selection.id) eina_stringshare_del(iface->network_selection.id);
   if (iface->network_selection.pass) eina_stringshare_del(iface->network_selection.pass);
   memcpy(&(iface->network_selection), d, sizeof(Interface_Network_Selection));
   iface->callbacks = iface_callback(iface->callbacks,
				     IFACE_EVENT_NETWORK_SELECTION_CHANGE,
				     iface, NULL);
   iface_unref(iface);
}

static void
iface_getnetwork_result_free(void *data)
{
   Interface_IPv4 *d = data;
   free(d);
}

static void *
iface_getproperties_unmarhsall(DBusMessage *msg, DBusError *err)
{
   DBusMessageIter array, iter, item, val;
   Interface_Properties *d;

   d = calloc(1, sizeof(Interface_Properties));
   if (!d) return NULL;

   if (dbus_message_iter_init(msg, &array))
     {
	if (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_ARRAY)
	  {
	     dbus_message_iter_recurse(&array, &iter);
	     while (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_DICT_ENTRY)
	       {
                  char *key, *v;
		  int type;

		  dbus_message_iter_recurse(&iter, &item);
		  key = NULL;
		  dbus_message_iter_get_basic(&item, &key);
		  dbus_message_iter_next(&item);
		  dbus_message_iter_recurse(&item, &val);
		  type = dbus_message_iter_get_arg_type(&val);
		  v = NULL;
		  if ((!strcmp(key, "Type")) && (type == DBUS_TYPE_STRING))
		    {
		       dbus_message_iter_get_basic(&val, &v);
		       if (v) d->type = eina_stringshare_add(v);
		    }
		  else if ((!strcmp(key, "State")) && (type == DBUS_TYPE_STRING))
		    {
		       dbus_message_iter_get_basic(&val, &v);
		       if (v) d->state = eina_stringshare_add(v);
		    }
		  else if ((!strcmp(key, "Policy")) && (type == DBUS_TYPE_STRING))
		    {
		       dbus_message_iter_get_basic(&val, &v);
		       if (v) d->policy = eina_stringshare_add(v);
		    }
		  else if ((!strcmp(key, "Driver")) && (type == DBUS_TYPE_STRING))
		    {
		       dbus_message_iter_get_basic(&val, &v);
		       if (v) d->driver = eina_stringshare_add(v);
		    }
		  else if ((!strcmp(key, "Vendor")) && (type == DBUS_TYPE_STRING))
		    {
		       dbus_message_iter_get_basic(&val, &v);
		       if (v) d->vendor = eina_stringshare_add(v);
		    }
		  else if ((!strcmp(key, "Product")) && (type == DBUS_TYPE_STRING))
		    {
		       dbus_message_iter_get_basic(&val, &v);
		       if (v) d->product = eina_stringshare_add(v);
		    }
		  else if ((!strcmp(key, "Signal")) && (type == DBUS_TYPE_UINT16))
		    {
		       dbus_uint16_t vi;
		       dbus_message_iter_get_basic(&val, &vi);
		       d->signal_strength = vi;
		    }
		  dbus_message_iter_next(&iter);
	       }
	  }
     }
   return d;
}

static void
iface_getproperties_callback(void *data, void *ret, DBusError *err)
{
   Interface *iface = data;
   Interface_Properties *d = ret;
   if (!d)
     {
	iface_unref(iface);
	iface_unref(iface);
	return;
     }
   if (iface->prop.product) eina_stringshare_del(iface->prop.product);
   if (iface->prop.vendor) eina_stringshare_del(iface->prop.vendor);
   if (iface->prop.driver) eina_stringshare_del(iface->prop.driver);
   if (iface->prop.state) eina_stringshare_del(iface->prop.state);
   if (iface->prop.policy) eina_stringshare_del(iface->prop.policy);
   if (iface->prop.type) eina_stringshare_del(iface->prop.type);
   memcpy(&(iface->prop), d, sizeof(Interface_Properties));
   if (!iface->newif)
     {
	callbacks = iface_callback(callbacks,
				   IFACE_EVENT_ADD,
				   iface, NULL);
	iface->newif = 1;
     }
   iface_unref(iface);
}

static void
iface_getproperties_result_free(void *data)
{
   Interface_Properties *d = data;
   free(d);
}

static void
iface_net_add(Interface *iface, const char *essid, const char *bssid, int signal_strength, const char *security)
{
   Interface_Network *net;
   Eina_List *l;

   for (l = iface->networks; l; l = l->next)
     {
	net = l->data;
	if (!strcmp(bssid, net->bssid))
	  {
	     int changes = 0;

	     net->last_seen_time = ecore_time_get();
	     if (((essid) && (net->essid) &&
		  (!strcmp(essid, net->essid))) ||
		 (!!essid != !!net->essid))
	       {
		  if (net->essid) eina_stringshare_del(net->essid);
		  if (essid) net->essid = eina_stringshare_add(essid);
		  else net->essid = NULL;
		  changes++;
	       }
	     if (signal_strength != net->signal_strength)
	       {
		  net->signal_strength = signal_strength;
		  changes++;
	       }
	     if (((security) && (net->security) &&
		  (!strcmp(security, net->security))) ||
		 (!!security != !!net->security))
	       {
		  if (net->security) eina_stringshare_del(net->security);
		  if (security) net->security = eina_stringshare_add(security);
		  else net->security = NULL;
		  changes++;
	       }
	     if (changes > 0)
		  iface->callbacks = iface_callback(iface->callbacks,
						    IFACE_EVENT_SCAN_NETWORK_CHANGE,
						    iface, net);
	     return;
	  }
     }
   net = calloc(1, sizeof(Interface_Network));
   if (net)
     {
	net->last_seen_time = ecore_time_get();
	if (essid) net->essid = eina_stringshare_add(essid);
	net->bssid = eina_stringshare_add(bssid);
	net->signal_strength = signal_strength;
	if (security) net->security = eina_stringshare_add(security);
	iface->networks = eina_list_append(iface->networks, net);
	iface->callbacks = iface_callback(iface->callbacks,
					  IFACE_EVENT_SCAN_NETWORK_ADD,
					  iface, net);
     }
}

static void
iface_sigh_network_found(void *data, DBusMessage *msg)
{
   Interface *iface = data;
   DBusMessageIter array, iter, item, val;

   if (dbus_message_iter_init(msg, &array))
     {
	if (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_ARRAY)
	  {
	     Interface_Network *net;

	     dbus_message_iter_recurse(&array, &iter);
	     net = calloc(1, sizeof(Interface_Network));
	     if (net)
	       {
		  while (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_DICT_ENTRY)
		    {
		       char *key, *v;
		       int type;

		       dbus_message_iter_recurse(&iter, &item);
		       key = NULL;
		       dbus_message_iter_get_basic(&item, &key);
		       dbus_message_iter_next(&item);
		       dbus_message_iter_recurse(&item, &val);
		       type = dbus_message_iter_get_arg_type(&val);
		       v = NULL;
		       if ((!strcmp(key, "ESSID")) && (type == DBUS_TYPE_STRING))
			 {
			    dbus_message_iter_get_basic(&val, &v);
			    if (v) net->essid = eina_stringshare_add(v);
			 }
		       else if ((!strcmp(key, "BSSID")) && (type == DBUS_TYPE_STRING))
			 {
			    dbus_message_iter_get_basic(&val, &v);
			    if (v) net->bssid = eina_stringshare_add(v);
			 }
		       else if ((!strcmp(key, "Signal")) && (type == DBUS_TYPE_UINT16))
			 {
			    dbus_uint16_t vi;
			    dbus_message_iter_get_basic(&val, &vi);
			    net->signal_strength = vi;
			 }
		       if ((!strcmp(key, "Security")) && (type == DBUS_TYPE_STRING))
			 {
			    dbus_message_iter_get_basic(&val, &v);
			    if (v) net->security = eina_stringshare_add(v);
			 }
		       dbus_message_iter_next(&iter);
		    }
		  if (net->bssid)
		    iface_net_add(iface, net->essid, net->bssid,
				  net->signal_strength, net->security);
		  if (net->essid) eina_stringshare_del(net->essid);
		  if (net->bssid) eina_stringshare_del(net->bssid);
		  if (net->security) eina_stringshare_del(net->security);
		  free(net);
	       }
	  }
     }
}

static void
iface_sigh_signal_changed(void *data, DBusMessage *msg)
{
   Interface *iface = data;

   /* FIXME: need to handle signal changed - and fix connman */
   /* FIXME: wpa_supplicant doesn't support this yet. see:
    * mlme.c: ieee80211_associated() */
   iface->callbacks = iface_callback(iface->callbacks,
				     IFACE_EVENT_SIGNAL_CHANGE,
				     iface, NULL);
}

static void
iface_sigh_state_changed(void *data, DBusMessage *msg)
{
   Interface *iface = data;
   DBusMessageIter iter;
   const char *s = NULL;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(s));
   if (!s) return;
   if (iface->prop.state) eina_stringshare_del(iface->prop.state);
   iface->prop.state = eina_stringshare_add(s);

   msg = dbus_message_new_method_call("org.freedesktop.connman",
				      iface->ifpath,
				      "org.freedesktop.connman.Interface",
				      "GetIPv4");
   if (msg)
     {
	e_dbus_method_call_send(conn, msg,
				iface_getipv4_unmarhsall,
				iface_getipv4_callback,
				iface_getipv4_result_free, -1, iface);
	dbus_message_unref(msg);
	iface_ref(iface);
     }
   iface->callbacks = iface_callback(iface->callbacks,
				     IFACE_EVENT_STATE_CHANGE,
				     iface, NULL);
}

static void
iface_sigh_policy_changed(void *data, DBusMessage *msg)
{
   Interface *iface = data;
   DBusMessageIter iter;
   const char *s = NULL;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(s));
   if (!s) return;
   if (iface->prop.policy) eina_stringshare_del(iface->prop.policy);
   iface->prop.policy = eina_stringshare_add(s);
   iface->callbacks = iface_callback(iface->callbacks,
				     IFACE_EVENT_POLICY_CHANGE,
				     iface, NULL);
}

static void
iface_sigh_network_changed(void *data, DBusMessage *msg)
{
   Interface *iface = data;
   Interface_Network_Selection *d;

   d = iface_network_selection_decode(msg);
   if (!d) return;

   if (iface->network_selection.id) eina_stringshare_del(iface->network_selection.id);
   if (iface->network_selection.pass) eina_stringshare_del(iface->network_selection.pass);
   memcpy(&(iface->network_selection), d, sizeof(Interface_Network_Selection));
   free(d);
   iface->callbacks = iface_callback(iface->callbacks,
				     IFACE_EVENT_NETWORK_SELECTION_CHANGE,
				     iface, NULL);
}

static void
iface_sigh_ipv4_changed(void *data, DBusMessage *msg)
{
   Interface *iface = data;
   Interface_IPv4 *d;

   d = iface_ipv4_decode(msg);
   if (!d) return;

   if (iface->ipv4.method) eina_stringshare_del(iface->ipv4.method);
   if (iface->ipv4.address) eina_stringshare_del(iface->ipv4.address);
   if (iface->ipv4.gateway) eina_stringshare_del(iface->ipv4.gateway);
   if (iface->ipv4.netmask) eina_stringshare_del(iface->ipv4.netmask);
   memcpy(&(iface->ipv4), d, sizeof(Interface_IPv4));
   free(d);
   iface->callbacks = iface_callback(iface->callbacks,
				     IFACE_EVENT_IPV4_CHANGE,
				     iface, NULL);
}

static int
iface_timer_network_timeout(void *data)
{
   Interface *iface = data;
   double now;
   Interface_Network *net;
   Eina_List *l, *l_del;

   now = ecore_time_get();
   iface_ref(iface);
   for (l = iface->networks; l;)
     {
	net = l->data;
	if ((now - net->last_seen_time) > 20.0)
	  {
	     l_del = l;
	     l = l->next;
	     iface->callbacks = iface_callback(iface->callbacks,
					       IFACE_EVENT_SCAN_NETWORK_DEL,
					       iface, net);
	     iface->networks = eina_list_remove_list(iface->networks, l_del);
	     if (net->essid) eina_stringshare_del(net->essid);
	     if (net->bssid) eina_stringshare_del(net->bssid);
	     if (net->security) eina_stringshare_del(net->security);
	     free(net);
	  }
	else
	  l = l->next;
     }
   iface_unref(iface);
   return ECORE_CALLBACK_RENEW;
}

static void *
iface_system_listinterfaces_unmarhsall(DBusMessage *msg, DBusError *err)
{
   DBusMessageIter array, iter;

   if (dbus_message_iter_init(msg, &array))
     {
	if (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_ARRAY)
	  {
	     dbus_message_iter_recurse(&array, &iter);
	     do
	       {
		  char *ifpath;

		  ifpath = NULL;
		  dbus_message_iter_get_basic(&iter, &ifpath);
		  if (ifpath)
		    {
		       if (!iface_find(ifpath))
			 {
			    printf("ADD 1 %s\n", ifpath);
			    iface_add(ifpath);
			 }
		    }
	       }
	     while (dbus_message_iter_next(&iter));
	}
     }
   return NULL;
}

static void
iface_system_listinterfaces_callback(void *data, void *ret, DBusError *err)
{
}

static void
iface_system_listinterfaces_result_free(void *data)
{
}

static void
iface_system_added(void *data, DBusMessage *msg)
{
   DBusMessageIter iter;
   const char *s = NULL;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(s));
   if (!s) return;
   if (iface_find(s)) return;
   printf("ADD 2 %s\n", s);
   iface_add(s);
}

static void
iface_system_removed(void *data, DBusMessage *msg)
{
   DBusMessageIter iter;
   const char *s = NULL;
   Interface *iface;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(s));
   if (!s) return;
   if (!(iface = iface_find(s))) return;
   callbacks = iface_callback(callbacks,
			      IFACE_EVENT_DEL,
			      iface, NULL);
   iface_unref(iface);
}

static void
iface_system_interfaces_list(void)
{
   DBusMessage *msg;

   msg = dbus_message_new_method_call("org.freedesktop.connman",
				      "/",
				      "org.freedesktop.connman.Manager",
				      "ListInterfaces");
   if (!msg) return;
   e_dbus_method_call_send(conn, msg,
			   iface_system_listinterfaces_unmarhsall,
			   iface_system_listinterfaces_callback,
			   iface_system_listinterfaces_result_free, -1, NULL);
   dbus_message_unref(msg);

   if (sigh_interface_added)
     e_dbus_signal_handler_del(conn, sigh_interface_added);
   sigh_interface_added = e_dbus_signal_handler_add
     (conn, "org.freedesktop.connman", "/", "org.freedesktop.connman.Manager",
      "InterfaceAdded", iface_system_added, NULL);
   if (sigh_interface_removed)
     e_dbus_signal_handler_del(conn, sigh_interface_removed);
   sigh_interface_removed = e_dbus_signal_handler_add
     (conn, "org.freedesktop.connman", "/", "org.freedesktop.connman.Manager",
      "InterfaceRemoved", iface_system_removed, NULL);
}

static void
iface_system_name_ownerchanged(void *data, DBusMessage *msg)
{
   DBusError err;
   const char *s1, *s2, *s3;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err,
			      DBUS_TYPE_STRING, &s1,
			      DBUS_TYPE_STRING, &s2,
			      DBUS_TYPE_STRING, &s3,
			      DBUS_TYPE_INVALID))
     return;
   if (strcmp(s1, "org.freedesktop.connman")) return;
   iface_system_interfaces_list();
}

/* IFACE PUBLIC API */
Interface *
iface_add(const char *ifpath)
{
   Interface *iface;
   DBusMessage *msg;

   iface = calloc(1, sizeof(Interface));
   iface->ref = 1;
   iface->ifpath = eina_stringshare_add(ifpath);

   msg = dbus_message_new_method_call("org.freedesktop.connman",
				      iface->ifpath,
				      "org.freedesktop.connman.Interface",
				      "GetProperties");
   if (!msg)
     {
	eina_stringshare_del(iface->ifpath);
	free(iface);
	return NULL;
     }
   e_dbus_method_call_send(conn, msg,
			   iface_getproperties_unmarhsall,
			   iface_getproperties_callback,
			   iface_getproperties_result_free, -1, iface);
   dbus_message_unref(msg);
   iface_ref(iface);

   msg = dbus_message_new_method_call("org.freedesktop.connman",
				      iface->ifpath,
				      "org.freedesktop.connman.Interface",
				      "GetIPv4");
   if (msg)
     {
	e_dbus_method_call_send(conn, msg,
				iface_getipv4_unmarhsall,
				iface_getipv4_callback,
				iface_getipv4_result_free, -1, iface);
	dbus_message_unref(msg);
	iface_ref(iface);
     }

   msg = dbus_message_new_method_call("org.freedesktop.connman",
				      iface->ifpath,
				      "org.freedesktop.connman.Interface",
				      "GetNetwork");
   if (msg)
     {
	e_dbus_method_call_send(conn, msg,
				iface_getnetwork_unmarhsall,
				iface_getnetwork_callback,
				iface_getnetwork_result_free, -1, iface);
	dbus_message_unref(msg);
	iface_ref(iface);
     }

   iface->sigh.network_found = e_dbus_signal_handler_add
     (conn, "org.freedesktop.connman", iface->ifpath,
      "org.freedesktop.connman.Interface", "NetworkFound",
      iface_sigh_network_found, iface);
   iface->sigh.signal_changed = e_dbus_signal_handler_add
     (conn, "org.freedesktop.connman", iface->ifpath,
      "org.freedesktop.connman.Interface", "SignalChanged",
      iface_sigh_signal_changed, iface);
   iface->sigh.state_changed = e_dbus_signal_handler_add
     (conn, "org.freedesktop.connman", iface->ifpath,
      "org.freedesktop.connman.Interface", "StateChanged",
      iface_sigh_state_changed, iface);
   iface->sigh.policy_changed = e_dbus_signal_handler_add
     (conn, "org.freedesktop.connman", iface->ifpath,
      "org.freedesktop.connman.Interface", "PolicyChanged",
      iface_sigh_policy_changed, iface);
   iface->sigh.network_changed = e_dbus_signal_handler_add
     (conn, "org.freedesktop.connman", iface->ifpath,
      "org.freedesktop.connman.Interface", "NetworkChanged",
      iface_sigh_network_changed, iface);
   iface->sigh.ipv4_changed = e_dbus_signal_handler_add
     (conn, "org.freedesktop.connman", iface->ifpath,
      "org.freedesktop.connman.Interface", "IPv4Changed",
      iface_sigh_ipv4_changed, iface);

   iface->network_timeout = ecore_timer_add(10.0, iface_timer_network_timeout,
					    iface);
   interfaces = eina_list_append(interfaces, iface);
   return iface;
}

void
iface_ref(Interface *iface)
{
   iface->ref++;
}

void
iface_unref(Interface *iface)
{
   iface->ref--;
   if (iface->ref != 0) return;

   iface->callbacks = iface_callback(iface->callbacks,
				     IFACE_EVENT_DEL,
				     iface, NULL);

   while (iface->callbacks)
     {
	free(iface->callbacks->data);
	iface->callbacks = eina_list_remove_list(iface->callbacks, iface->callbacks);
     }

   if (iface->network_timeout)
     ecore_timer_del(iface->network_timeout);
   if (iface->network_selection.id) eina_stringshare_del(iface->network_selection.id);
   if (iface->network_selection.pass) eina_stringshare_del(iface->network_selection.pass);
   while (iface->networks)
     {
	Interface_Network *net;

	net = iface->networks->data;
	if (net->essid) eina_stringshare_del(net->essid);
	if (net->bssid) eina_stringshare_del(net->bssid);
	if (net->security) eina_stringshare_del(net->security);
	free(net);
	iface->networks = eina_list_remove_list(iface->networks, iface->networks);
     }
   if (iface->prop.product) eina_stringshare_del(iface->prop.product);
   if (iface->prop.vendor) eina_stringshare_del(iface->prop.vendor);
   if (iface->prop.driver) eina_stringshare_del(iface->prop.driver);
   if (iface->prop.state) eina_stringshare_del(iface->prop.state);
   if (iface->prop.policy) eina_stringshare_del(iface->prop.policy);
   if (iface->prop.type) eina_stringshare_del(iface->prop.type);
   e_dbus_signal_handler_del(conn, iface->sigh.network_found);
   e_dbus_signal_handler_del(conn, iface->sigh.signal_changed);
   e_dbus_signal_handler_del(conn, iface->sigh.state_changed);
   e_dbus_signal_handler_del(conn, iface->sigh.policy_changed);
   e_dbus_signal_handler_del(conn, iface->sigh.network_changed);
   e_dbus_signal_handler_del(conn, iface->sigh.ipv4_changed);
   interfaces = eina_list_remove(interfaces, iface);
   free(iface);
}

Interface *
iface_find(const char *ifpath)
{
   Eina_List *l;

   if (!ifpath) return NULL;
   for (l = interfaces; l; l = l->next)
     {
	Interface *iface;

	iface = l->data;
	if (!strcmp(iface->ifpath, ifpath)) return iface;
     }
   return NULL;
}

void
iface_network_set(Interface *iface, const char *ssid, const char *pass)
{
   DBusMessage *msg;
   DBusMessageIter iter, array, item, val;
   const char *key;

   msg = dbus_message_new_method_call("org.freedesktop.connman",
				      iface->ifpath,
				      "org.freedesktop.connman.Interface",
				      "SetNetwork");
   if (!msg) return;
   dbus_message_iter_init_append(msg, &iter);
   dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY,
				    DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_VARIANT_AS_STRING
				    DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &array);

   dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY, NULL, &item);
   key = "ESSID";
   dbus_message_iter_append_basic(&item, DBUS_TYPE_STRING, &key);
   dbus_message_iter_open_container(&item, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &val);
   dbus_message_iter_append_basic(&val, DBUS_TYPE_STRING, &ssid);
   dbus_message_iter_close_container(&item, &val);
   dbus_message_iter_close_container(&array, &item);

   if (pass)
     {
	dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY, NULL, &item);
	key = "PSK";
	dbus_message_iter_append_basic(&item, DBUS_TYPE_STRING, &key);
	dbus_message_iter_open_container(&item, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &val);
	dbus_message_iter_append_basic(&val, DBUS_TYPE_STRING, &pass);
	dbus_message_iter_close_container(&item, &val);
	dbus_message_iter_close_container(&array, &item);
     }

   dbus_message_iter_close_container(&iter, &array);

   e_dbus_method_call_send(conn, msg, NULL, NULL, NULL, -1, NULL);
   dbus_message_unref(msg);
}

void
iface_policy_set(Interface *iface, const char *policy)
{
   DBusMessage *msg;
   DBusMessageIter iter;

   msg = dbus_message_new_method_call("org.freedesktop.connman",
				      iface->ifpath,
				      "org.freedesktop.connman.Interface",
				      "SetPolicy");
   if (!msg) return;
   dbus_message_iter_init_append(msg, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &policy);

   e_dbus_method_call_send(conn, msg, NULL, NULL, NULL, -1, NULL);
   dbus_message_unref(msg);
}

void
iface_scan(Interface *iface, const char *policy)
{
   DBusMessage *msg;

   msg = dbus_message_new_method_call("org.freedesktop.connman",
				      iface->ifpath,
				      "org.freedesktop.connman.Interface",
				      "Scan");
   if (!msg) return;
   e_dbus_method_call_send(conn, msg, NULL, NULL, NULL, -1, NULL);
   dbus_message_unref(msg);
}

void
iface_ipv4_set(Interface *iface, const char *method, const char *address, const char *gateway, const char *netmask)
{
   DBusMessage *msg;
   DBusMessageIter iter, array, item, val;
   const char *key;

   msg = dbus_message_new_method_call("org.freedesktop.connman",
				      iface->ifpath,
				      "org.freedesktop.connman.Interface",
				      "SetIPv4");
   if (!msg) return;
   dbus_message_iter_init_append(msg, &iter);
   dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY,
				    DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_VARIANT_AS_STRING
				    DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &array);

   if (method)
     {
	dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY, NULL, &item);
	key = "Method";
	dbus_message_iter_append_basic(&item, DBUS_TYPE_STRING, &key);
	dbus_message_iter_open_container(&item, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &val);
	dbus_message_iter_append_basic(&val, DBUS_TYPE_STRING, &method);
	dbus_message_iter_close_container(&item, &val);
	dbus_message_iter_close_container(&array, &item);
     }

   if (address)
     {
	dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY, NULL, &item);
	key = "Address";
	dbus_message_iter_append_basic(&item, DBUS_TYPE_STRING, &key);
	dbus_message_iter_open_container(&item, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &val);
	dbus_message_iter_append_basic(&val, DBUS_TYPE_STRING, &address);
	dbus_message_iter_close_container(&item, &val);
	dbus_message_iter_close_container(&array, &item);
     }

   if (gateway)
     {
	dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY, NULL, &item);
	key = "Gateway";
	dbus_message_iter_append_basic(&item, DBUS_TYPE_STRING, &key);
	dbus_message_iter_open_container(&item, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &val);
	dbus_message_iter_append_basic(&val, DBUS_TYPE_STRING, &gateway);
	dbus_message_iter_close_container(&item, &val);
	dbus_message_iter_close_container(&array, &item);
     }

   if (netmask)
     {
	dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY, NULL, &item);
	key = "Netmask";
	dbus_message_iter_append_basic(&item, DBUS_TYPE_STRING, &key);
	dbus_message_iter_open_container(&item, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &val);
	dbus_message_iter_append_basic(&val, DBUS_TYPE_STRING, &netmask);
	dbus_message_iter_close_container(&item, &val);
	dbus_message_iter_close_container(&array, &item);
     }

   dbus_message_iter_close_container(&iter, &array);

   e_dbus_method_call_send(conn, msg, NULL, NULL, NULL, -1, NULL);
   dbus_message_unref(msg);
}

void
iface_callback_add(Interface *iface, Interface_Event event, void (*func) (void *data, Interface *iface, Interface_Network *ifnet), void *data)
{
   Interface_Callback *cb;

   cb = calloc(1, sizeof(Interface_Callback));
   if (!cb) return;
   cb->event = event;
   cb->func = func;
   cb->data = data;
   iface->callbacks = eina_list_append(iface->callbacks, cb);
}

void
iface_callback_del(Interface *iface, Interface_Event event, void (*func) (void *data, Interface *iface, Interface_Network *ifnet), void *data)
{
   Eina_List *l;

   for (l = iface->callbacks; l; l = l->next)
     {
	Interface_Callback *cb;

	cb = l->data;
	if ((cb->event == event) && (cb->func == func) && (cb->data == data))
	  {
	     /* just flag for deletion - cleanup happens later */
	     cb->delete_me = 1;
	     return;
	  }
     }
}

void
iface_system_init(E_DBus_Connection *edbus_conn)
{
   conn = edbus_conn;
   /* init iface system - listen for connman name owner change - if we get it
    * connman was sarted or restarted this handles connman starting later
    * than the front-end gui */
   sigh_name_ownerchanged = e_dbus_signal_handler_add
     (conn, "org.freedesktop.DBus", "/org/freedesktop/DBus",
      "org.freedesktop.DBus", "NameOwnerChanged",
      iface_system_name_ownerchanged, NULL);
   /* do an initial list */
   iface_system_interfaces_list();

   /* FIXME: do we care about connman state? haven't found a use for it yet */
   /* FIXME: get state with GetState */
   /* FIXME: add signal handler StateChanged */
}

void
iface_system_shutdown(void)
{
   Eina_List *l, *tlist = NULL;

   for (l = interfaces; l; l = l->next)
     tlist = eina_list_append(tlist, l->data);
   while (tlist)
     {
	iface_unref(tlist->data);
	tlist = eina_list_remove_list(tlist, tlist);
     }
   if (sigh_name_ownerchanged)
     e_dbus_signal_handler_del(conn, sigh_name_ownerchanged);
   sigh_name_ownerchanged = NULL;
   if (sigh_interface_added)
     e_dbus_signal_handler_del(conn, sigh_interface_added);
   sigh_interface_added = NULL;
   if (sigh_interface_removed)
     e_dbus_signal_handler_del(conn, sigh_interface_removed);
   sigh_interface_removed = NULL;
   while (callbacks)
     {
	free(callbacks->data);
	callbacks = eina_list_remove_list(callbacks, callbacks);
     }
   conn = NULL;
}

void
iface_system_callback_add(Interface_Event event, void (*func) (void *data, Interface *iface, Interface_Network *ifnet), void *data)
{
   Interface_Callback *cb;

   cb = calloc(1, sizeof(Interface_Callback));
   if (!cb) return;
   cb->event = event;
   cb->func = func;
   cb->data = data;
   callbacks = eina_list_append(callbacks, cb);
}

void
iface_system_callback_del(Interface_Event event, void (*func) (void *data, Interface *iface, Interface_Network *ifnet), void *data)
{
   Eina_List *l;

   for (l = callbacks; l; l = l->next)
     {
	Interface_Callback *cb;

	cb = l->data;
	if ((cb->event == event) && (cb->func == func) && (cb->data == data))
	  {
	     /* just flag for deletion - cleanup happens later */
	     cb->delete_me = 1;
	     return;
	  }
     }
}









#if 0// TESTING CODE ONLY HERE - EXCERCISE THE API ABOVE
static void
cb_if_del(void *data, Interface *iface, Interface_Network *ifnet)
{
   printf("IF-- %s\n", iface->ifpath);
}

static void
cb_if_ipv4(void *data, Interface *iface, Interface_Network *ifnet)
{
   printf("IF   %s\n", iface->ifpath);
   printf("  IPV4: [%s][%s][%s][%s]\n",
	  iface->ipv4.method, iface->ipv4.address,
	  iface->ipv4.gateway, iface->ipv4.netmask);
}

static void
cb_if_net_sel(void *data, Interface *iface, Interface_Network *ifnet)
{
   printf("IF   %s\n", iface->ifpath);
   printf("  NET_SEL: [%s] [%s]\n",
	  iface->network_selection.id, iface->network_selection.pass);
}

static void
cb_if_scan_net_add(void *data, Interface *iface, Interface_Network *ifnet)
{
   printf("IF   %s\n", iface->ifpath);
   printf("  SCAN NET ADD: [%s] %i \"%s\" %s\n",
	  ifnet->bssid, ifnet->signal_strength, ifnet->essid, ifnet->security);
}

static void
cb_if_scan_net_del(void *data, Interface *iface, Interface_Network *ifnet)
{
   printf("IF   %s\n", iface->ifpath);
   printf("  SCAN NET DEL: [%s] %i \"%s\" %s\n",
	  ifnet->bssid, ifnet->signal_strength, ifnet->essid, ifnet->security);
}

static void
cb_if_scan_net_change(void *data, Interface *iface, Interface_Network *ifnet)
{
   printf("IF   %s\n", iface->ifpath);
   printf("  SCAN NET CHANGE: [%s] %i \"%s\" %s\n",
	  ifnet->bssid, ifnet->signal_strength, ifnet->essid, ifnet->security);
}

static void
cb_if_signal(void *data, Interface *iface, Interface_Network *ifnet)
{
   printf("IF   %s\n", iface->ifpath);
   printf("  SIGNAL: %i\n", iface->signal_strength);
}

static int connect_test = 0;
static int disconnect_test = 0;
static double connect_timeout_test = 5.0;
static char *essid_test = "";
static char *pass_test = "";

// this is testing - every 5 seconds connect then disconnect from the network
static int
delay_up(void *data)
{
   Interface *iface = data;

   // turn it back on
   iface_policy_set(iface, "auto");
   iface_network_set(iface, essid_test, pass_test);
   return 0;
}

static int
delay_down(void *data)
{
   Interface *iface = data;

   // turn interface off
   iface_policy_set(iface, "off");
   // in 5 seconds call delay_up
   if (connect_test)
     ecore_timer_add(connect_timeout_test, delay_up, iface);
   return 0;
}

static void
cb_if_state(void *data, Interface *iface, Interface_Network *ifnet)
{
   // .. iface->prop.state:
   // scanning
   // carrier
   // configure
   // ready
   // unknown
   // off
   // enabled
   // connect
   // connected
   printf("IF   %s\n", iface->ifpath);
   printf("  STATE: %s\n", iface->prop.state);
   // if we go into a ready state - in 5 seconds, call delay_down
   if (disconnect_test)
     {
	if (!strcmp(iface->prop.state, "ready"))
	  ecore_timer_add(connect_timeout_test, delay_down, iface);
     }
}

static void
cb_if_policy(void *data, Interface *iface, Interface_Network *ifnet)
{
   // .. iface->prop.policy:
   // unknown
   // off
   // ignore
   // auto
   // ask
   //
   printf("IF   %s\n", iface->ifpath);
   printf("  POLICY: %s\n", iface->prop.policy);
}

static void
cb_main_if_add(void *data, Interface *iface, Interface_Network *ifnet)
{
   printf("IF++ %s\n", iface->ifpath);

   // TESTING ...
   // add callbacks to events so we know when things happen
   iface_callback_add(iface, IFACE_EVENT_DEL, cb_if_del, NULL);
   iface_callback_add(iface, IFACE_EVENT_IPV4_CHANGE, cb_if_ipv4, NULL);
   iface_callback_add(iface, IFACE_EVENT_NETWORK_SELECTION_CHANGE, cb_if_net_sel, NULL);
   iface_callback_add(iface, IFACE_EVENT_SCAN_NETWORK_ADD, cb_if_scan_net_add, NULL);
   iface_callback_add(iface, IFACE_EVENT_SCAN_NETWORK_DEL, cb_if_scan_net_del, NULL);
   iface_callback_add(iface, IFACE_EVENT_SCAN_NETWORK_CHANGE, cb_if_scan_net_change, NULL);
   iface_callback_add(iface, IFACE_EVENT_SIGNAL_CHANGE, cb_if_signal, NULL);
   iface_callback_add(iface, IFACE_EVENT_STATE_CHANGE, cb_if_state, NULL);
   iface_callback_add(iface, IFACE_EVENT_POLICY_CHANGE, cb_if_policy, NULL);

   // .. iface->prop.type:
   // unknown
   // 80203
   // 80211
   // wimax
   // modem
   // bluetooth
   //
   // if it is 802.11 - try connect
   if (essid_test)
     {
	if ((iface->prop.type) && (!strcmp(iface->prop.type, "80211")))
	  {
	     iface_policy_set(iface, "auto");
	     iface_network_set(iface, essid_test, pass_test);
	  }
     }
}

//////// DRIVE THE TESt SYSTEM ABOVE ////////
int
main(int argc, char **argv)
{
   E_DBus_Connection *c;

     {
	int i;

	for (i = 0; i < argc; i++)
	  {
	     if (!strcmp(argv[i], "-ct"))
	       {
		  connect_test = 1;
	       }
	     else if (!strcmp(argv[i], "-dt"))
	       {
		  disconnect_test = 1;
	       }
	     else if (!strcmp(argv[i], "-e"))
	       {
		  essid_test = argv[++i];
	       }
	     else if (!strcmp(argv[i], "-p"))
	       {
		  pass_test = argv[++i];
	       }
	     else if (!strcmp(argv[i], "-t"))
	       {
		  connect_timeout_test = atof(argv[++i]);
	       }
	     else if (!strcmp(argv[i], "-h"))
	       {
		  printf
		    ("Options:\n"
		     "  -ct          Enable connection connect test\n"
		     "  -dt          Enable connection disconnect test\n"
		     "  -t N         Timeout for connect/disconnect test (seconds)\n"
		     "  -e XXX       Use Essid XXX\n"
		     "  -p XXX       Use passphrase XXX\n"
		     "  -h           This help\n"
		     );
		  exit(0);
	       }
	  }
     }

   ecore_init();
   eina_stringshare_init();
   ecore_app_args_set(argc, (const char **)argv);
   e_dbus_init();
   evas_init();

   c = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!c)
     {
	printf("ERROR: can't connect to system session\n");
	exit(-1);
     }

   // add calback that gets called whenever an interface is added to the system
   // or on initial lst of interfaces that are present
   iface_system_callback_add(IFACE_EVENT_ADD, cb_main_if_add, NULL);
   iface_system_init(c);

   ecore_main_loop_begin();

   iface_system_shutdown();

   e_dbus_connection_close(c);
   evas_shutdown();
   e_dbus_shutdown();
   eina_stringshare_shutdown();
   ecore_shutdown();

   return 0;
}
#endif
