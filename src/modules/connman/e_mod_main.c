/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"
#include "e_iface.h"

// FUCK.
//
// connman now completely changed api in git upstream (just found out) after
// weeks of no changes - huge change. nothing seems compatible anymore.
//
// ...
//
// this basically puts this all on hold. hoo-ray!
//
// back to ignoring network stuff.

// FIXME: need config to
// 1. list instance id -> iface config
// 2. list networks (essid, password, dhcp?, ip, gw, netmask)


/***************************************************************************/
/**/
/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc);
static char *_gc_label(void);
static Evas_Object *_gc_icon(Evas *evas);
static const char *_gc_id_new(void);
/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
     "connman",
     {
        _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL
     },
   E_GADCON_CLIENT_STYLE_PLAIN
};
/**/
/***************************************************************************/

static void gadget_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event);

/***************************************************************************/
/**/
/* actual module specifics */

typedef struct _Instance Instance;
typedef struct _Conf Conf;
typedef struct _Conf_Interface Conf_Interface;
typedef struct _Conf_Network Conf_Network;


struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_net; // FIXME: clock to go...

   E_Gadcon_Popup  *popup;
   Evas_Object     *popup_ilist_obj;
   E_Dialog        *if_dia;
   Evas_Object     *if_radio_device;
   Evas_Object     *if_ilist_obj;
   E_Dialog        *net_dia;
   E_Dialog        *manual_dia;
   E_Dialog        *netlist_dia;

   struct {
      int             ifmode;
      int             ifmode_tmp;
      char           *ifpath;
      char           *ifpath_tmp;
      char           *bssid;
      char           *sec;
      Conf_Network   *cfnet, *cfnet_new;
      Conf_Interface *cfif;
   } config;
};

struct _Conf_Interface
{
   const char   *name;
   const char   *id;
   const char   *ifpath;
   int           ifmode;
   Conf_Network *netconf; // if ethernet - then this will hold config
};

struct _Conf_Network
{
   char *name;
   char *essid;
   char *password;
   char *ip;
   char *gateway;
   char *netmask;
   int dhcp;
   int remember_password;
   int use_always;
   int addme;
};

struct _Conf
{
   Eina_List *interfaces;
   Eina_List *networks;
};

static E_Module *connman_module = NULL;
static E_DBus_Connection *connman_dbus = NULL;
static Eina_List *instances = NULL;
static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_interface_edd = NULL;
static E_Config_DD *conf_network_edd = NULL;
static Conf *conf = NULL;

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;
   Eina_List *l;

   inst = E_NEW(Instance, 1);

   o = edje_object_add(gc->evas);
   e_theme_edje_object_set(o, "base/theme/modules/connman",
			   "e/modules/connman/main");
   evas_object_show(o);

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
				  gadget_cb_mouse_down, inst);

   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;

   inst->gcc = gcc;
   inst->o_net = o;

   e_gadcon_client_util_menu_attach(gcc);

   instances = eina_list_append(instances, inst);

   if (!conf)
     {
	conf = E_NEW(Conf, 1);
	e_config_save_queue();
     }
   for (l = conf->interfaces; l; l = l->next)
     {
	Conf_Interface *cfif;

	cfif = l->data;
	if ((cfif->name) && (!strcmp(name, cfif->name)) &&
	    (cfif->id) && (!strcmp(name, cfif->id)))
	  {
	     inst->config.cfif = cfif;
	     break;
	  }
     }
   if (!inst->config.cfif)
     {
        Conf_Interface *cfif;

	cfif = E_NEW(Conf_Interface, 1);
	cfif->name = eina_stringshare_add(name);
	cfif->id = eina_stringshare_add(id);
	conf->interfaces = eina_list_append(conf->interfaces, cfif);
	inst->config.cfif = cfif;
	e_config_save_queue();
	// FIXME:  check interfaces - if one matches, do the if init
     }
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

   inst = gcc->data;
   instances = eina_list_remove(instances, inst);

   evas_object_del(inst->o_net);
   free(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc)
{
   Instance *inst;
   Evas_Coord mw, mh;

   inst = gcc->data;
   mw = 0, mh = 0;
   edje_object_size_min_get(inst->o_net, &mw, &mh);
   if ((mw < 1) || (mh < 1))
     edje_object_size_min_calc(inst->o_net, &mw, &mh);
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   e_gadcon_client_aspect_set(gcc, mw, mh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static char *
_gc_label(void)
{
   return _("Connection Manager");
}

static Evas_Object *
_gc_icon(Evas *evas)
{
   Evas_Object *o;
   char buf[4096];

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-connman.edj",
	    e_module_dir_get(connman_module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(void)
{
   return _gadcon_class.name;
}

/**/
/***************************************************************************/

/***************************************************************************/
/**/
static void if_dialog_hide(Instance *inst);
static void popup_hide(Instance *inst);
static void net_dialog_show(Instance *inst, Conf_Network *cfnet);
static void net_dialog_hide(Instance *inst);
static void popup_ifnet_nets_refresh(Instance *inst);

static Eina_List *ifaces = NULL;

static int
inst_if_matches(Instance *inst, Interface *iface)
{
   if ((inst->config.ifmode == 0) && (iface->prop.type) &&
       (!strcmp(iface->prop.type, "80211")))
     return 1;
   if ((inst->config.ifmode == 1) && (iface->prop.type) &&
       (!strcmp(iface->prop.type, "80203")))
     return 1;
   if ((inst->config.ifpath) && (!strcmp(iface->ifpath, inst->config.ifpath)))
     return 1;
   return 0;
}

static Interface *
if_get(Instance *inst)
{
   Eina_List *l;
   Interface *iface = NULL;

   if (inst->config.ifpath)
     iface = iface_find(inst->config.ifpath);
   else
     {
	for (l = ifaces; l; l = l->next)
	  {
	     iface = l->data;
	     if (inst_if_matches(inst, iface)) return iface;
	  }
     }
   return iface;
}

static int
net_join(Instance *inst, Interface *iface, Conf_Network *cfnet)
{
   if (!inst->config.sec)
     {
	iface_policy_set(iface, "auto");
	iface_network_set(iface, cfnet->essid, "");
	if (cfnet->dhcp)
	  {
	     iface_ipv4_set(iface, "dhcp", NULL, NULL, NULL);
	  }
	else
	  iface_ipv4_set(iface, "static",
			 cfnet->ip, cfnet->gateway,
			 cfnet->netmask);
	if ((!cfnet->remember_password) && (cfnet->password))
	  {
	     eina_stringshare_del(cfnet->password);
	     cfnet->password = NULL;
	  }
     }
   else
     {
	if (!cfnet->password)
	  {
	     net_dialog_show(inst, cfnet);
	  }
	else
	  {
	     iface_policy_set(iface, "auto");
	     iface_network_set(iface, cfnet->essid, cfnet->password);
	     if (cfnet->dhcp)
	       {
		  iface_ipv4_set(iface, "dhcp", NULL, NULL, NULL);
	       }
	     else
	       iface_ipv4_set(iface, "static",
			      cfnet->ip, cfnet->gateway,
			      cfnet->netmask);
	     if ((!cfnet->remember_password) && (cfnet->password))
	       {
		  eina_stringshare_del(cfnet->password);
		  cfnet->password = NULL;
	       }
	  }
     }
   return 0;
}

#define STR_SHARE(x) \
do { char *___s; ___s = (x); \
   if (___s) { (x) = eina_stringshare_add(___s); free(___s); } \
} while (0);

#define STR_UNSHARE(x) \
do { char *___s; ___s = (x); \
   if (___s) { (x) = strdup(___s); eina_stringshare_del(___s); } \
} while (0);

static void
net_dialog_cb_ok(void *data, E_Dialog *dialog)
{
   Instance *inst;
   Conf_Network *cfnet;
   Interface *iface;

   inst = data;
   cfnet = inst->config.cfnet_new;
   inst->config.cfnet = cfnet;
   inst->config.cfnet_new = NULL;
   if (cfnet->addme)
     conf->networks = eina_list_prepend(conf->networks, cfnet);
   STR_SHARE(cfnet->name);
   STR_SHARE(cfnet->essid);
   STR_SHARE(cfnet->password);
   STR_SHARE(cfnet->ip);
   STR_SHARE(cfnet->gateway);
   STR_SHARE(cfnet->netmask);
   net_dialog_hide(inst);
   iface = if_get(inst);
   if (iface) net_join(inst, iface, cfnet);
   // FIXME: if ethernet - save cfnet to instances->confnet
   e_config_save_queue();
}

static void
net_dialog_cb_cancel(void *data, E_Dialog *dialog)
{
   Instance *inst;
   Conf_Network *cfnet;

   inst = data;
   cfnet = inst->config.cfnet_new;
   inst->config.cfnet_new = NULL;
   if (cfnet->addme)
     {
	E_FREE(cfnet->name);
	E_FREE(cfnet->essid);
	E_FREE(cfnet->password);
	E_FREE(cfnet->ip);
	E_FREE(cfnet->gateway);
	E_FREE(cfnet->netmask);
	free(cfnet);
     }
   else if (inst->config.cfnet)
     {
	cfnet = inst->config.cfnet;
	STR_SHARE(cfnet->name);
	STR_SHARE(cfnet->essid);
	STR_SHARE(cfnet->password);
	STR_SHARE(cfnet->ip);
	STR_SHARE(cfnet->gateway);
	STR_SHARE(cfnet->netmask);
     }
   net_dialog_hide(inst);
}

static void
net_dialog_cb_del(E_Win *win)
{
   E_Dialog *dialog;
   Instance *inst;
   Conf_Network *cfnet;

   dialog = win->data;
   inst = dialog->data;
   cfnet = inst->config.cfnet_new;
   inst->config.cfnet_new = NULL;
   if (cfnet->addme)
     {
	E_FREE(cfnet->name);
	E_FREE(cfnet->essid);
	E_FREE(cfnet->password);
	E_FREE(cfnet->ip);
	E_FREE(cfnet->gateway);
	E_FREE(cfnet->netmask);
	free(cfnet);
     }
   else if (inst->config.cfnet)
     {
	cfnet = inst->config.cfnet;
	STR_SHARE(cfnet->name);
	STR_SHARE(cfnet->essid);
	STR_SHARE(cfnet->password);
	STR_SHARE(cfnet->ip);
	STR_SHARE(cfnet->gateway);
	STR_SHARE(cfnet->netmask);
     }
   net_dialog_hide(inst);
}

// FIXME: if iface deleted - del dialog
// FIXME: if ifnet del (this ifnet) del dialog
static void
net_dialog_show(Instance *inst, Conf_Network *cfnet)
{
   E_Dialog *dialog;
   Evas *evas;
   Evas_Object *table, *o, *button;
   Evas_Coord mw, mh;
   int row = 0;

   dialog = e_dialog_new(inst->gcc->gadcon->zone->container, "e", "e_connman_net_dialog");
   e_dialog_title_set(dialog, "Connection Details");
   dialog->data = inst;
   evas = e_win_evas_get(dialog->win);

   table = e_widget_table_add(evas, 0);

   o = e_widget_label_add(evas, "Personal Name");
   e_widget_table_object_append(table, o, 0, row, 1, 1, 1, 1, 0, 0);
   o = e_widget_entry_add(evas, &(cfnet->name), NULL, NULL, NULL);
   e_widget_min_size_get(o, &mw, &mh);
   mw = 160;
   e_widget_min_size_set(o, mw, mh);
   e_widget_table_object_append(table, o, 1, row, 1, 1, 1, 1, 0, 0);
   row++;

   // FIXME: onyl for wifi - not LAN
   o = e_widget_label_add(evas, "ESSID");
   e_widget_table_object_append(table, o, 0, row, 1, 1, 1, 1, 0, 0);
   o = e_widget_label_add(evas, cfnet->essid);
   e_widget_table_object_append(table, o, 1, row, 1, 1, 1, 1, 0, 0);
   row++;

   o = e_widget_check_add(evas, "Use when available", &(cfnet->use_always));
   e_widget_table_object_append(table, o, 0, row, 2, 1, 1, 1, 0, 0);
   row++;

   if (inst->config.sec)
     {
	o = e_widget_label_add(evas, "Password");
	e_widget_table_object_append(table, o, 0, row, 1, 1, 1, 1, 0, 0);
	o = e_widget_entry_add(evas, &(cfnet->password), NULL, NULL, NULL);
	e_widget_min_size_get(o, &mw, &mh);
	mw = 160;
	e_widget_min_size_set(o, mw, mh);
	e_widget_table_object_append(table, o, 1, row, 1, 1, 1, 1, 0, 0);
	row++;

	o = e_widget_check_add(evas, "Remember password", &(cfnet->remember_password));
	e_widget_table_object_append(table, o, 0, row, 2, 1, 1, 1, 0, 0);
	row++;
     }

   // FIXME: manual dialog needs to work
   button = e_widget_button_add(evas, "Address Details", NULL, NULL,
				inst, NULL);
   e_widget_table_object_append(table, button, 0, row, 2, 1, 0, 0, 0, 0);
   row++;

   e_widget_min_size_get(table, &mw, &mh);
   e_dialog_content_set(dialog, table, mw, mh);

   e_win_delete_callback_set(dialog->win, net_dialog_cb_del);

   e_dialog_button_add(dialog, "OK", NULL, net_dialog_cb_ok, inst);
   e_dialog_button_add(dialog, "Cancel", NULL, net_dialog_cb_cancel, inst);
   e_dialog_button_focus_num(dialog, 1);
   e_win_centered_set(dialog->win, 1);
   e_dialog_show(dialog);

   inst->net_dia = dialog;
}

static void
net_dialog_hide(Instance *inst)
{
   if (inst->net_dia)
     {
	e_object_del(E_OBJECT(inst->net_dia));
	inst->net_dia = NULL;
     }
}

// FIXME: need a "manual network settings" dialog:
// * checkbox "use dhcp"
// * entry "ip"
// * entry "gateway"
// * entry "netmask"
static void
manual_dialog_show(Instance *inst)
{
}

static void
manual_dialog_hide(Instance *inst)
{
}

// FIXME: need saved networks list dialog
// * ilist of saved network names
// * delete button
static void
netlist_dialog_show(Instance *inst)
{
   Conf_Network *cfnet;

   cfnet = inst->config.cfnet_new;
   inst->config.cfnet_new = NULL;
   if ((cfnet) && (cfnet->addme))
     {
	E_FREE(cfnet->name);
	E_FREE(cfnet->essid);
	E_FREE(cfnet->password);
	E_FREE(cfnet->ip);
	E_FREE(cfnet->gateway);
	E_FREE(cfnet->netmask);
	free(cfnet);
     }
   else if (inst->config.cfnet)
     {
	cfnet = inst->config.cfnet;
	STR_SHARE(cfnet->name);
	STR_SHARE(cfnet->essid);
	STR_SHARE(cfnet->password);
	STR_SHARE(cfnet->ip);
	STR_SHARE(cfnet->gateway);
	STR_SHARE(cfnet->netmask);
     }
   net_dialog_hide(inst);
   manual_dialog_hide(inst);
   if_dialog_hide(inst);
   E_FREE(inst->config.ifpath);
}

static void
netlist_dialog_hide(Instance *inst)
{
}

static void
button_cb_netlist(void *data, void *data2)
{
   Instance *inst;

   inst = data;
   if (!inst->netlist_dia) netlist_dialog_show(inst);
   else netlist_dialog_hide(inst);
}

static void
if_dialog_cb_ok(void *data, E_Dialog *dialog)
{
   Instance *inst;

   inst = data;
   if_dialog_hide(inst);
   E_FREE(inst->config.ifpath);
   inst->config.ifmode = inst->config.ifmode_tmp;
   inst->config.ifpath = inst->config.ifpath_tmp;
   inst->config.ifpath_tmp = NULL;

   if (inst->config.cfif)
     {
	if (inst->config.cfif->ifpath)
	  {
	     eina_stringshare_del(inst->config.cfif->ifpath);
	     inst->config.cfif->ifpath = NULL;
	  }
	if (inst->config.ifpath)
	  inst->config.cfif->ifpath = eina_stringshare_add(inst->config.ifpath);
	inst->config.cfif->ifmode = inst->config.ifmode;
     }
   popup_ifnet_nets_refresh(inst);
   e_config_save_queue();
}

static void
if_dialog_cb_cancel(void *data, E_Dialog *dialog)
{
   Instance *inst;

   inst = data;
   if_dialog_hide(inst);
   E_FREE(inst->config.ifpath_tmp);
}

static void
if_dialog_cb_del(E_Win *win)
{
   E_Dialog *dialog;
   Instance *inst;

   dialog = win->data;
   inst = dialog->data;
   if_dialog_hide(inst);
   E_FREE(inst->config.ifpath_tmp);
}

static void
if_radio_cb_generic(void *data, Evas_Object *obj, void *event_info)
{
   Instance *inst;

   inst = data;
   if (inst->config.ifmode != 2)
     {
	e_widget_ilist_unselect(inst->if_ilist_obj);
	E_FREE(inst->config.ifpath);
     }
}

static void
if_ilist_cb_if_sel(void *data)
{
   Instance *inst;

   inst = data;
   e_widget_radio_toggle_set(inst->if_radio_device, 1);
}

static void
if_ilist_update(Instance *inst)
{
   Evas_Object *ilist;
   Eina_List *l;
   int i;

   ilist = inst->if_ilist_obj;
   if (!ilist) return;
   e_widget_ilist_freeze(ilist);
   e_widget_ilist_clear(ilist);

   for (i = 0, l = ifaces; l; l = l->next, i++)
     {
	Interface *iface;
	char buf[256];
	const char *vendor, *product, *type;
	Evas_Object *icon;

	iface = l->data;
	vendor = iface->prop.vendor;
	product = iface->prop.product;
	type = iface->prop.type;
	icon = NULL;
	if (type)
	  {
	     if (!strcmp(type, "80211"))
	       {
		  type = "WiFi";
		  // FIXME: find an icon;
	       }
	     else if (!strcmp(type, "80203"))
	       {
		  type = "LAN";
		  // FIXME: find an icon;
	       }
	     else if (!strcmp(type, "wimax"))
	       {
		  type = "WiMax";
		  // FIXME: find an icon;
	       }
	     else if (!strcmp(type, "modem"))
	       {
		  type = "Modem";
		  // FIXME: find an icon;
	       }
	     else if (!strcmp(type, "bluetooth"))
	       {
		  type = "Bluetooth";
		  // FIXME: find an icon;
	       }
	  }
	if (!vendor) vendor = "Unknown";
	if (!product) product = "Unknown";
	if (!type) type = "Unknown";
	snprintf(buf, sizeof(buf), "%s (%s)", type, product);
	e_widget_ilist_append(ilist, icon, buf, if_ilist_cb_if_sel, inst,
			      iface->ifpath);
	if ((inst->config.ifpath) &&
	    (!strcmp(inst->config.ifpath, iface->ifpath)))
	  e_widget_ilist_selected_set(ilist, i);
     }

   e_widget_ilist_go(ilist);
   e_widget_ilist_thaw(ilist);
}

static void
if_dialog_show(Instance *inst)
{
   E_Dialog *dialog;
   Evas *evas;
   Evas_Object *list, *flist, *o, *ilist, *button;
   Evas_Coord mw, mh;
   E_Radio_Group *rg;

   dialog = e_dialog_new(inst->gcc->gadcon->zone->container, "e", "e_connman_iface_dialog");
   e_dialog_title_set(dialog, _("Network Connection Settings"));
   dialog->data = inst;
   evas = e_win_evas_get(dialog->win);

   list = e_widget_list_add(evas, 0, 0);

   flist = e_widget_framelist_add(evas, _("Network Device"), 0);

   inst->config.ifmode_tmp = inst->config.ifmode;
   rg = e_widget_radio_group_new(&(inst->config.ifmode_tmp));

   o = e_widget_radio_add(evas, _("Wifi"), 0, rg);
   evas_object_smart_callback_add(o, "changed", if_radio_cb_generic, inst);
   e_widget_framelist_object_append(flist, o);
   o = e_widget_radio_add(evas, _("LAN"), 1, rg);
   evas_object_smart_callback_add(o, "changed", if_radio_cb_generic, inst);
   e_widget_framelist_object_append(flist, o);
   o = e_widget_radio_add(evas, _("Specific Device"), 2, rg);
   inst->if_radio_device = o;
   e_widget_framelist_object_append(flist, o);

   if (inst->config.ifpath)
     inst->config.ifpath_tmp = strdup(inst->config.ifpath);
   else
     inst->config.ifpath_tmp = NULL;
   ilist = e_widget_ilist_add(evas, 48, 48, &(inst->config.ifpath_tmp));
   inst->if_ilist_obj = ilist;

   e_widget_ilist_freeze(ilist);

   if_ilist_update(inst);

   e_widget_ilist_go(ilist);
   e_widget_ilist_thaw(ilist);

   e_widget_min_size_set(ilist, 240, 180);
   e_widget_framelist_object_append(flist, ilist);

   e_widget_list_object_append(list, flist, 1, 0, 0.5);

   // FIXME: netlist needs to work
   button = e_widget_button_add(evas, _("Networks"), NULL, button_cb_netlist,
				inst, NULL);
   e_widget_list_object_append(list, button, 1, 0, 0.5);

   e_widget_min_size_get(list, &mw, &mh);
   e_dialog_content_set(dialog, list, mw, mh);

   e_win_delete_callback_set(dialog->win, if_dialog_cb_del);

   e_dialog_button_add(dialog, _("OK"), NULL, if_dialog_cb_ok, inst);
   e_dialog_button_add(dialog, _("Cancel"), NULL, if_dialog_cb_cancel, inst);
   e_dialog_button_focus_num(dialog, 1);
   e_win_centered_set(dialog->win, 1);
   e_dialog_show(dialog);

   inst->if_dia = dialog;
}

static void
if_dialog_hide(Instance *inst)
{
   if (inst->if_dia)
     {
	e_object_del(E_OBJECT(inst->if_dia));
	inst->if_dia = NULL;
	inst->if_radio_device = NULL;
	inst->if_ilist_obj = NULL;
     }
}





static void
popup_cb_setup(void *data, void *data2)
{
   Instance *inst;

   inst = data;
   popup_hide(inst);
   if (!inst->if_dia) if_dialog_show(inst);
   else if_dialog_hide(inst);
}

static void
popup_cb_resize(Evas_Object *obj, int *w, int *h)
{
   int mw, mh;

   e_widget_min_size_get(obj, &mw, &mh);

   if (mh < 180) mh = 180;
   if (mw < 160) mw = 160;

   if (*w) *w = (mw + 8);
   if (*h) *h = (mh + 8);
}

static void
popup_ifnet_icon_adjust(Evas_Object *icon, Interface_Network *ifnet)
{
   Edje_Message_Int_Set *msg;
   Eina_List *l;
   int saved = 0;

   msg = alloca(sizeof(Edje_Message_Int_Set) + (0 * sizeof(int)));
   msg->count = 1;
   msg->val[0] = ifnet->signal_strength;
   edje_object_message_send(icon, EDJE_MESSAGE_INT_SET, 1, msg);

   if (ifnet->security)
     {
	if (!strcmp(ifnet->security, "WEP"))
	  edje_object_signal_emit(icon, "e,state,security,wep", "e");
	else if (!strcmp(ifnet->security, "WPA"))
	  edje_object_signal_emit(icon, "e,state,security,wpa", "e");
	else if (!strcmp(ifnet->security, "RSN"))
	  edje_object_signal_emit(icon, "e,state,security,rsn", "e");
     }
   else
     edje_object_signal_emit(icon, "e,state,security,open", "e");
   if (conf)
     {
	for (l = conf->networks; l; l = l->next)
	  {
	     Conf_Network *cfnet;

	     cfnet = l->data;
	     if ((cfnet->essid) && (ifnet->essid) &&
		 (!strcmp(cfnet->essid, ifnet->essid)))
	       {
		  saved = 1;
		  break;
	       }
	  }
     }
   if (saved)
     edje_object_signal_emit(icon, "e,state,saved,on", "e");
   else
     edje_object_signal_emit(icon, "e,state,saved,off", "e");
}

static void
popup_cb_ifnet_sel(void *data)
{
   Instance *inst;
   Eina_List *l;
   Interface *iface;

   inst = data;
   if (!inst->config.bssid) return;
   iface = if_get(inst);
   if (!iface) return;
   for (l = iface->networks; l; l = l->next)
     {
	Interface_Network *ifnet;

	ifnet = l->data;
	E_FREE(inst->config.sec);
	if (ifnet->security) inst->config.sec = strdup(ifnet->security);
	if (!strcmp(inst->config.bssid, ifnet->bssid))
	  {
	     Conf_Network *cfnet;

	     printf("SEL %s\n", ifnet->essid);
	     if (!conf)
	       conf = E_NEW(Conf, 1);
	     for (l = conf->networks; l; l = l->next)
	       {
		  cfnet = l->data;
		  if (!strcmp(cfnet->essid, ifnet->essid))
		    {
		       STR_UNSHARE(cfnet->name);
		       STR_UNSHARE(cfnet->essid);
		       STR_UNSHARE(cfnet->password);
		       STR_UNSHARE(cfnet->ip);
		       STR_UNSHARE(cfnet->gateway);
		       STR_UNSHARE(cfnet->netmask);
		       inst->config.cfnet_new = cfnet;
		       net_dialog_show(inst, cfnet);
		       popup_hide(inst);
		       return;
		    }
	       }
	     cfnet = E_NEW(Conf_Network, 1);
	     conf->networks = eina_list_prepend(conf->networks, cfnet);
	     if (ifnet->essid)
	       {
		  cfnet->name = strdup(ifnet->essid);
		  cfnet->essid = strdup(ifnet->essid);
	       }
	     else
	       cfnet->name = strdup("NONE");
	     cfnet->remember_password = 1;
	     cfnet->dhcp = 1;
	     cfnet->addme = 1;
	     inst->config.cfnet_new = cfnet;
	     net_dialog_show(inst, cfnet);
	     popup_hide(inst);
	     return;
	  }
     }
   popup_hide(inst);
}

static void
popup_ifnet_net_add(Instance *inst, Interface_Network *ifnet)
{
   const char *label;
   Evas_Object *icon;

   label = ifnet->essid;
   if (!label) label = "NONE";
   icon = edje_object_add(evas_object_evas_get(inst->popup_ilist_obj));
   e_theme_edje_object_set(icon, "base/theme/modules/connman",
			   "e/modules/connman/network");
   popup_ifnet_icon_adjust(icon, ifnet);
   e_widget_ilist_append(inst->popup_ilist_obj, icon, label,
			 popup_cb_ifnet_sel, inst, ifnet->bssid);
   evas_object_show(icon);
}

/*
static int
popup_ifnet_cb_sort(void *data1, void *data2)
{
   Interface_Network *ifnet1, *ifnet2;

   ifnet1 = data1;
   ifnet2 = data2;
   return ifnet2->signal_strength - ifnet1->signal_strength;
}
*/
static void
popup_ifnet_nets_refresh(Instance *inst)
{
   Eina_List *l, *networks = NULL;
   Interface *iface;
   Evas_Object *ilist;

   if (!inst->popup_ilist_obj) return;
   ilist = inst->popup_ilist_obj;

   iface = if_get(inst);

   e_widget_ilist_freeze(ilist);
   e_widget_ilist_clear(ilist);

   if (iface)
     {
	for (l = iface->networks; l; l = l->next)
	  networks = eina_list_append(networks, l->data);
     }
/*
   if (networks)
     networks = eina_list_sort(networks,
			       eina_list_count(networks),
			       popup_ifnet_cb_sort);
 */
   for (l = networks; l; l = l->next)
     {
	Interface_Network *ifnet;

	ifnet = l->data;
	popup_ifnet_net_add(inst, ifnet);
     }
   if (networks) eina_list_free(networks);

   e_widget_ilist_go(ilist);
   e_widget_ilist_thaw(ilist);
}

static int
ifnet_num_get(Interface *iface, Interface_Network *ifnet)
{
   Eina_List *l;
   int i;

   for (i = 0, l = iface->networks; l; l = l->next, i++)
     {
	if (ifnet == l->data) return i;
     }
   return -1;
}

static void
popup_ifnet_add(Instance *inst, Interface *iface, Interface_Network *ifnet)
{
   if (!inst->popup_ilist_obj) return;
   popup_ifnet_net_add(inst, ifnet);
   e_widget_ilist_go(inst->popup_ilist_obj);
//   popup_ifnet_nets_refresh(inst);
}

static void
popup_ifnet_del(Instance *inst, Interface *iface, Interface_Network *ifnet)
{
   int i;

   if (!inst->popup_ilist_obj) return;
   i = ifnet_num_get(iface, ifnet);
   if (i < 0) return;
   e_widget_ilist_remove_num(inst->popup_ilist_obj, i);
   e_widget_ilist_go(inst->popup_ilist_obj);
//   popup_ifnet_nets_refresh(inst);
}

static void
popup_ifnet_change(Instance *inst, Interface *iface, Interface_Network *ifnet)
{
   const char *label;
   Evas_Object *icon;
   int i;

   if (!inst->popup_ilist_obj) return;
   i = ifnet_num_get(iface, ifnet);
   if (i < 0) return;
   icon = e_widget_ilist_nth_icon_get(inst->popup_ilist_obj, i);
   if (icon) popup_ifnet_icon_adjust(icon, ifnet);
   label = ifnet->essid;
   if (!label) label = "NONE";
   e_widget_ilist_nth_label_set(inst->popup_ilist_obj, i, label);
//   popup_ifnet_nets_refresh(inst);
}

static void
popup_show(Instance *inst)
{
   Evas_Object *base, *ilist, *button, *o;
   Evas *evas;
   Evas_Coord mw, mh;

   inst->popup = e_gadcon_popup_new(inst->gcc, popup_cb_resize);
   evas = inst->popup->win->evas;

   edje_freeze();

   base = e_widget_table_add(evas, 0);

   o = edje_object_add(evas);
   e_theme_edje_object_set(o, "base/theme/modules/connman",
			   "e/modules/connman/network");
   edje_object_size_min_get(o, &mw, &mh);
   if ((mw < 1) || (mh < 1)) edje_object_size_min_calc(o, &mw, &mh);
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   evas_object_del(o);
   ilist = e_widget_ilist_add(evas, mw, mh, &(inst->config.bssid));
   inst->popup_ilist_obj = ilist;

   e_widget_ilist_freeze(ilist);

   popup_ifnet_nets_refresh(inst);

   e_widget_ilist_go(ilist);
   e_widget_ilist_thaw(ilist);

   e_widget_min_size_set(ilist, 240, 320);
   e_widget_table_object_append(base, ilist,
			       0, 0, 1, 1, 1, 1, 1, 1);

   button = e_widget_button_add(evas, _("Settings"), NULL, popup_cb_setup,
				inst, NULL);
   e_widget_table_object_append(base, button,
			       0, 1, 1, 1, 0, 0, 0, 0);

   edje_thaw();

   e_gadcon_popup_content_set(inst->popup, base);
   e_gadcon_popup_show(inst->popup);
}

static void
popup_hide(Instance *inst)
{
   if (inst->popup)
     {
	e_object_del(E_OBJECT(inst->popup));
	inst->popup = NULL;
	inst->popup_ilist_obj = NULL;
     }
}

static void
gadget_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Instance *inst;
   Evas_Event_Mouse_Down *ev;

   ev = event;
   inst = data;
   if (ev->button == 1)
     {
	if (!inst->popup) popup_show(inst);
	else popup_hide(inst);
     }
}

static void
inst_enable(Instance *inst)
{
   // FIXME: emit signal to enable
}

static void
inst_disable(Instance *inst)
{
   // FIXME: emit signal to disable
}

static void
inst_type(Instance *inst, const char *type)
{
   // FIXME: emit signal to change type
}

static void
inst_connected(Instance *inst)
{
   // FIXME: emit signal
}

static void
inst_disconnected(Instance *inst)
{
   // FIXME: emit signal
}

static void
inst_connecting(Instance *inst)
{
   // FIXME: emit signal
}

static void
inst_associating(Instance *inst)
{
   // FIXME: emit signal
}

static void
inst_associated(Instance *inst)
{
   // FIXME: emit signal
}

static void
inst_signal(Instance *inst, int sig)
{
   // FIXME: emit signal
}

static void
inst_unknown(Instance *inst)
{
   // FIXME: emit signal
}

static void
inst_off(Instance *inst)
{
   // FIXME: emit signal
}

static void
inst_on(Instance *inst)
{
   // FIXME: emit signal
}

static void
cb_if_del(void *data, Interface *iface, Interface_Network *ifnet)
{
   Eina_List *l;

   printf("IF-- %s\n", iface->ifpath);
   ifaces = eina_list_remove(ifaces, iface);
   for (l = instances; l; l = l->next)
     {
	Instance *inst;

	inst = l->data;
	if (inst_if_matches(inst, iface))
	  inst_disable(inst);
	if_ilist_update(inst);
     }
}

static void
cb_if_ipv4(void *data, Interface *iface, Interface_Network *ifnet)
{
   Eina_List *l;

   printf("IF   %s\n", iface->ifpath);
   printf("  IPV4: [%s][%s][%s][%s]\n",
	  iface->ipv4.method, iface->ipv4.address,
	  iface->ipv4.gateway, iface->ipv4.netmask);
   for (l = instances; l; l = l->next)
     {
	Instance *inst;

	inst = l->data;
	if (inst_if_matches(inst, iface))
	  inst_connected(inst);
     }
}

static void
cb_if_net_sel(void *data, Interface *iface, Interface_Network *ifnet)
{
   Eina_List *l;

   printf("IF   %s\n", iface->ifpath);
   printf("  NET_SEL: [%s] [%s]\n",
	  iface->network_selection.id, iface->network_selection.pass);
   // FIXME: change status to say we managed to select a network
   for (l = instances; l; l = l->next)
     {
	Instance *inst;

	inst = l->data;
	if (inst_if_matches(inst, iface))
	  inst_associating(inst);
     }
}

static void
cb_if_scan_net_add(void *data, Interface *iface, Interface_Network *ifnet)
{
   Eina_List *l, *l2;

//   printf("IF   %s\n", iface->ifpath);
//   printf("  SCAN NET ADD: [%s] %i \"%s\" %s\n",
//	  ifnet->bssid, ifnet->signal_strength, ifnet->essid, ifnet->security);
   for (l = instances; l; l = l->next)
     {
	Instance *inst;

	inst = l->data;
	if (inst_if_matches(inst, iface))
	  popup_ifnet_add(l->data, iface, ifnet);
	if (!inst->config.cfnet)
	  {
	     for (l2 = conf->networks; l2; l2 = l2->next)
	       {
		  Conf_Network *cfnet;

		  cfnet = l2->data;
		  if ((ifnet->essid) && (cfnet->essid) &&
		      (!strcmp(ifnet->essid, cfnet->essid)))
		    {
		       inst->config.cfnet = cfnet;
		       net_join(inst, iface, cfnet);
		       break;
		    }
	       }
	  }
     }
}

static void
cb_if_scan_net_del(void *data, Interface *iface, Interface_Network *ifnet)
{
   Eina_List *l;

//   printf("IF   %s\n", iface->ifpath);
//   printf("  SCAN NET DEL: [%s] %i \"%s\" %s\n",
//	  ifnet->bssid, ifnet->signal_strength, ifnet->essid, ifnet->security);
   for (l = instances; l; l = l->next)
     {
	Instance *inst;

	inst = l->data;
	if (inst_if_matches(inst, iface))
	  {
	     popup_ifnet_del(l->data, iface, ifnet);
	  }
     }
}

static void
cb_if_scan_net_change(void *data, Interface *iface, Interface_Network *ifnet)
{
   Eina_List *l;

//   printf("IF   %s\n", iface->ifpath);
//   printf("  SCAN NET CHANGE: [%s] %i \"%s\" %s\n",
//	  ifnet->bssid, ifnet->signal_strength, ifnet->essid, ifnet->security);
   for (l = instances; l; l = l->next)
     {
	Instance *inst;

	inst = l->data;
	if (inst_if_matches(inst, iface))
	  popup_ifnet_change(l->data, iface, ifnet);
     }
}

static void
cb_if_signal(void *data, Interface *iface, Interface_Network *ifnet)
{
   Eina_List *l;

   printf("IF   %s\n", iface->ifpath);
   printf("  SIGNAL: %i\n", iface->signal_strength);
   for (l = instances; l; l = l->next)
     {
	Instance *inst;

	inst = l->data;
	if (inst_if_matches(inst, iface))
	  inst_signal(inst, iface->signal_strength);
     }
}

static void
cb_if_state(void *data, Interface *iface, Interface_Network *ifnet)
{
   Eina_List *l;

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
   // FIXME: show state instnaces for this iface
   for (l = instances; l; l = l->next)
     {
	Instance *inst;

	inst = l->data;
	if (inst_if_matches(inst, iface))
	  {
	     if (!strcmp(iface->prop.state, "unknown"))
	       inst_disconnected(inst);
	     else if (!strcmp(iface->prop.state, "off"))
	       inst_disconnected(inst);
	     else if (!strcmp(iface->prop.state, "carrier"))
	       inst_connecting(inst);
	     else if (!strcmp(iface->prop.state, "configure"))
	       inst_associated(inst);
	     else if (!strcmp(iface->prop.state, "ready"))
	       inst_connected(inst);
	     else if (!strcmp(iface->prop.state, "enabled"))
	       inst_associating(inst);
	     else if (!strcmp(iface->prop.state, "connect"))
	       inst_connecting(inst);
	     else if (!strcmp(iface->prop.state, "connected"))
	       inst_connected(inst);
	  }
     }
}

static void
cb_if_policy(void *data, Interface *iface, Interface_Network *ifnet)
{
   Eina_List *l;

   // .. iface->prop.policy:
   // unknown
   // off
   // ignore
   // auto
   // ask
   //
   printf("IF   %s\n", iface->ifpath);
   printf("  POLICY: %s\n", iface->prop.policy);
   for (l = instances; l; l = l->next)
     {
	Instance *inst;

	inst = l->data;
	if (inst_if_matches(inst, iface))
	  {
	     if (!strcmp(iface->prop.policy, "unknown"))
	       inst_unknown(inst);
	     else if (!strcmp(iface->prop.policy, "off"))
	       inst_off(inst);
	     else if (!strcmp(iface->prop.policy, "ignore"))
	       inst_off(inst);
	     else if (!strcmp(iface->prop.policy, "auto"))
	       inst_on(inst);
	     else if (!strcmp(iface->prop.policy, "ask"))
	       inst_on(inst);
	  }
     }
}

static void
cb_main_if_add(void *data, Interface *iface, Interface_Network *ifnet)
{
   Eina_List *l;

   printf("IF++ %s\n", iface->ifpath);
   ifaces = eina_list_append(ifaces, iface);
   iface_callback_add(iface, IFACE_EVENT_DEL, cb_if_del, NULL);
   iface_callback_add(iface, IFACE_EVENT_IPV4_CHANGE, cb_if_ipv4, NULL);
   iface_callback_add(iface, IFACE_EVENT_NETWORK_SELECTION_CHANGE, cb_if_net_sel, NULL);
   iface_callback_add(iface, IFACE_EVENT_SCAN_NETWORK_ADD, cb_if_scan_net_add, NULL);
   iface_callback_add(iface, IFACE_EVENT_SCAN_NETWORK_DEL, cb_if_scan_net_del, NULL);
   iface_callback_add(iface, IFACE_EVENT_SCAN_NETWORK_CHANGE, cb_if_scan_net_change, NULL);
   iface_callback_add(iface, IFACE_EVENT_SIGNAL_CHANGE, cb_if_signal, NULL);
   iface_callback_add(iface, IFACE_EVENT_STATE_CHANGE, cb_if_state, NULL);
   iface_callback_add(iface, IFACE_EVENT_POLICY_CHANGE, cb_if_policy, NULL);

   for (l = instances; l; l = l->next)
     {
	Instance *inst;

	inst = l->data;
	if (inst_if_matches(inst, iface))
	  {
	     inst_enable(inst);
	     if (!strcmp(iface->prop.policy, "unknown"))
	       inst_unknown(inst);
	     else if (!strcmp(iface->prop.policy, "off"))
	       inst_off(inst);
	     else if (!strcmp(iface->prop.policy, "ignore"))
	       inst_off(inst);
	     else if (!strcmp(iface->prop.policy, "auto"))
	       inst_on(inst);
	     else if (!strcmp(iface->prop.policy, "ask"))
	       inst_on(inst);

	     if (inst->config.cfif->netconf)
	       {
		  // FIXME: must be ethernet - bring up netconf
	       }
	  }
	if_ilist_update(l->data);
     }
}

/**/
/***************************************************************************/

/***************************************************************************/
/**/
/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Connection Manager"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   connman_module = m;

   conf_network_edd = E_CONFIG_DD_NEW("Conf_Network", Conf_Network);
   E_CONFIG_VAL(conf_network_edd, Conf_Network, name, STR);
   E_CONFIG_VAL(conf_network_edd, Conf_Network, essid, STR);
   E_CONFIG_VAL(conf_network_edd, Conf_Network, password, STR);
   E_CONFIG_VAL(conf_network_edd, Conf_Network, ip, STR);
   E_CONFIG_VAL(conf_network_edd, Conf_Network, gateway, STR);
   E_CONFIG_VAL(conf_network_edd, Conf_Network, netmask, STR);
   E_CONFIG_VAL(conf_network_edd, Conf_Network, dhcp, INT);
   E_CONFIG_VAL(conf_network_edd, Conf_Network, remember_password, INT);
   E_CONFIG_VAL(conf_network_edd, Conf_Network, use_always, INT);

   conf_interface_edd = E_CONFIG_DD_NEW("Conf_Interface", Conf_Interface);
   E_CONFIG_VAL(conf_interface_edd, Conf_Interface, name, STR);
   E_CONFIG_VAL(conf_interface_edd, Conf_Interface, id, STR);
   E_CONFIG_VAL(conf_interface_edd, Conf_Interface, ifpath, STR);
   E_CONFIG_VAL(conf_interface_edd, Conf_Interface, ifmode, INT);
   E_CONFIG_SUB(conf_interface_edd, Conf_Interface, netconf, conf_network_edd);

   conf_edd = E_CONFIG_DD_NEW("Conf", Conf);
   E_CONFIG_LIST(conf_edd, Conf, interfaces, conf_interface_edd);
   E_CONFIG_LIST(conf_edd, Conf, networks, conf_network_edd);

   conf = e_config_domain_load("module.connman", conf_edd);

   connman_dbus = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (connman_dbus)
     {
	iface_system_callback_add(IFACE_EVENT_ADD, cb_main_if_add, NULL);
	iface_system_init(connman_dbus);
     }

   e_gadcon_provider_register(&_gadcon_class);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   // FIXME: free conf

   E_CONFIG_DD_FREE(conf_network_edd);
   E_CONFIG_DD_FREE(conf_interface_edd);
   E_CONFIG_DD_FREE(conf_edd);

   conf_network_edd = NULL;
   conf_interface_edd = NULL;
   conf_edd = NULL;

   e_gadcon_provider_unregister(&_gadcon_class);
   if (connman_dbus)
     {
	if (ifaces)
	  {
	     eina_list_free(ifaces);
	     ifaces = NULL;
	  }
	iface_system_shutdown();
//	e_dbus_connection_close(connman_dbus);
	connman_dbus = NULL;
     }

   connman_module = NULL;

   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   if (conf) e_config_domain_save("module.connman", conf_edd, conf);
   return 1;
}
