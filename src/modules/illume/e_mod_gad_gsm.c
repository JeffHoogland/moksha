#include "e.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static E_DBus_Connection *conn = NULL;
static E_DBus_Connection *conn_system = NULL;
static E_DBus_Signal_Handler *changed_h = NULL;
static E_DBus_Signal_Handler *changed_fso_h = NULL;
static E_DBus_Signal_Handler *operatorch_h = NULL;
static E_DBus_Signal_Handler *operatorch_fso_h = NULL;
static E_DBus_Signal_Handler *namech_h = NULL;
static E_DBus_Signal_Handler *namech_system_h = NULL;

static Ecore_Timer *try_again_timer = NULL;

typedef enum _Phone_Sys
{
   PH_SYS_UNKNOWN,
   PH_SYS_QTOPIA,
   PH_SYS_FSO
} Phone_Sys;

static Phone_Sys detected_system = PH_SYS_UNKNOWN;

/***************************************************************************/
typedef struct _Instance Instance;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object *obj;
   int strength;
   char *oper;
};

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
     "illume-gsm",
     {
	_gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL
     },
   E_GADCON_CLIENT_STYLE_PLAIN
};
static E_Module *mod = NULL;
/**/
/***************************************************************************/

static int try_again(void *data);
static void *signal_unmarhsall(DBusMessage *msg, DBusError *err);
static void *operator_unmarhsall(DBusMessage *msg, DBusError *err);
static void signal_callback_qtopia(void *data, void *ret, DBusError *err);
static void signal_callback_fso(void *data, void *ret, DBusError *err);
static void operator_callback_qtopia(void *data, void *ret, DBusError *err);
static void operator_callback_fso(void *data, void *ret, DBusError *err);
static void signal_result_free(void *data);
static void operator_result_free(void *data);
static void get_signal(void *data);
static void get_operator(void *data);
static void signal_changed(void *data, DBusMessage *msg);
static void operator_changed(void *data, DBusMessage *msg);
static void fso_operator_changed(void *data, DBusMessage *msg);
static void name_changed(void *data, DBusMessage *msg);

static int
try_again(void *data)
{
//   printf("GSM-gadget: Try again called\n");
   get_signal(data);
   get_operator(data);
   try_again_timer = NULL;
   return 0;
}

/* called from the module core */
void
_e_mod_gad_gsm_init(E_Module *m)
{
   mod = m;
   e_gadcon_provider_register(&_gadcon_class);
}

void
_e_mod_gad_gsm_shutdown(void)
{
   e_gadcon_provider_unregister(&_gadcon_class);
   mod = NULL;
}

/* internal calls */
static Evas_Object *
_theme_obj_new(Evas *e, const char *custom_dir, const char *group)
{
   Evas_Object *o;
   
   o = edje_object_add(e);
   if (!e_theme_edje_object_set(o, "base/theme/modules/illume", group))
     {
	if (custom_dir)
	  {
	     char buf[PATH_MAX];
	     
	     snprintf(buf, sizeof(buf), "%s/illume.edj", custom_dir);
	     if (edje_object_file_set(o, buf, group))
	       {
		  printf("OK FALLBACK %s\n", buf);
	       }
	  }
     }
   return o;
}

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;
   
   inst = E_NEW(Instance, 1);
   o = _theme_obj_new(gc->evas, e_module_dir_get(mod),
		      "e/modules/illume/gadget/gsm");
   evas_object_show(o);
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;
   inst->gcc = gcc;
   inst->obj = o;
   e_gadcon_client_util_menu_attach(gcc);
   
   inst->strength = -1;
   inst->oper = NULL;
   
   ecore_init();
   ecore_string_init();
   e_dbus_init();
   
   conn = e_dbus_bus_get(DBUS_BUS_SESSION);
   conn_system = e_dbus_bus_get(DBUS_BUS_SYSTEM);

   if (conn)
     {
	namech_h = e_dbus_signal_handler_add(conn,
					     "org.freedesktop.DBus",
					     "/org/freedesktop/DBus",
					     "org.freedesktop.DBus",
					     "NameOwnerChanged",
					     name_changed, inst);
	changed_h = e_dbus_signal_handler_add(conn,
					      "org.openmoko.qtopia.Phonestatus",
					      "/Status",
					      "org.openmoko.qtopia.Phonestatus",
					      "signalStrengthChanged",
					      signal_changed, inst);
	operatorch_h = e_dbus_signal_handler_add(conn,
						 "org.openmoko.qtopia.Phonestatus",
						 "/Status",
						 "org.openmoko.qtopia.Phonestatus",
						 "networkOperatorChanged",
						 operator_changed, inst);
     }
   if (conn_system)
     {
	namech_system_h = e_dbus_signal_handler_add(conn_system,
						    "org.freedesktop.DBus",
						    "/org/freedesktop/DBus",
						    "org.freedesktop.DBus",
						    "NameOwnerChanged",
						    name_changed, inst);
	changed_fso_h = e_dbus_signal_handler_add(conn_system,
						  "org.freesmartphone.ogsmd",
						  "/org/freesmartphone/GSM/Device",
						  "org.freesmartphone.GSM.Network",
						  "SignalStrength",
						  signal_changed, inst);
	operatorch_fso_h = e_dbus_signal_handler_add(conn_system,
						     "org.freesmartphone.ogsmd",
						     "/org/freesmartphone/GSM/Device",
						     "org.freesmartphone.GSM.Network",
						     "Status",
						     fso_operator_changed, inst);
     }
   get_signal(inst);
   get_operator(inst);
   
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;
   
   if (conn) e_dbus_connection_close(conn);
   if (conn_system) e_dbus_connection_close(conn_system);
   e_dbus_shutdown();
   ecore_string_shutdown();
   ecore_shutdown();
   
   inst = gcc->data;
   evas_object_del(inst->obj);
   if (inst->oper) free(inst->oper);
   free(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc)
{
   Instance *inst;
   Evas_Coord mw, mh, mxw, mxh;
   
   inst = gcc->data;
   mw = 0, mh = 0;
   edje_object_size_min_get(inst->obj, &mw, &mh);
   edje_object_size_max_get(inst->obj, &mxw, &mxh);
   if ((mw < 1) || (mh < 1))
     edje_object_size_min_calc(inst->obj, &mw, &mh);
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   if ((mxw > 0) && (mxh > 0))
     e_gadcon_client_aspect_set(gcc, mxw, mxh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static char *
_gc_label(void)
{
   return "GSM (Illume)";
}

static Evas_Object *
_gc_icon(Evas *evas)
{
/* FIXME: need icon
   Evas_Object *o;
   char buf[4096];
   
   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-clock.edj",
	    e_module_dir_get(clock_module));
   edje_object_file_set(o, buf, "icon");
   return o;
 */
   return NULL;
}

static const char *
_gc_id_new(void)
{
   return _gadcon_class.name;
}

static void
update_operator(char *op, void *data)
{
   Instance *inst = data;
   char *poper;
   
   poper = inst->oper;
   if ((poper) && (op) && (!strcmp(op, poper))) return;
   if (op) inst->oper = strdup(op);
   else inst->oper = NULL;
   if (inst->oper != poper)
     {
	Edje_Message_String msg;
	
	if (inst->oper) msg.str = inst->oper;
	else msg.str = "";
	edje_object_message_send(inst->obj, EDJE_MESSAGE_STRING, 1, &msg);
     }
   if (poper) free(poper);
}

static void
update_signal(int sig, void *data)
{
   Instance *inst = data;
   int pstrength;
   
   pstrength = inst->strength;
   inst->strength = sig;

   if (inst->strength != pstrength)
     {
	Edje_Message_Float msg;
	double level;
	
	level = (double)inst->strength / 100.0;
	if (level < 0.0) level = 0.0;
	else if (level > 1.0) level = 1.0;
	msg.val = level;
	edje_object_message_send(inst->obj, EDJE_MESSAGE_FLOAT, 1, &msg);
	if ((pstrength == -1) && (inst->strength >= 0))
	  edje_object_signal_emit(inst->obj, "e,state,active", "e");
	else if ((pstrength >= 0) && (inst->strength == -1))
	  edje_object_signal_emit(inst->obj, "e,state,passive", "e");
     }
}


static void *
signal_unmarhsall(DBusMessage *msg, DBusError *err)
{
   dbus_int32_t val = -1;
   
   if (dbus_message_get_args(msg, NULL, DBUS_TYPE_INT32, &val, DBUS_TYPE_INVALID))
     {
	int *val_ret;
	
	val_ret = malloc(sizeof(int));
	if (val_ret)
	  {
	     *val_ret = val;
	     return val_ret;
	  }
     }
   return NULL;
}

static void *
operator_unmarhsall(DBusMessage *msg, DBusError *err)
{
   const char *str;

   if (dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID))
     {
	char *str_ret;
	
	str_ret = malloc(strlen(str)+1);
	if (str_ret)
	  {
	     strcpy(str_ret, str);
	     return str_ret;
	  }
     }
   return NULL;
}

static void *
_fso_operator_unmarhsall(DBusMessage *msg)
{
   /* We care only about the provider name right now. All the other status
    * informations get ingnored for the gadget for now */
   const char *provider, *name, *reg_stat;
   DBusMessageIter iter, a_iter, s_iter, v_iter;
   
   if (!dbus_message_has_signature(msg, "a{sv}")) return NULL;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_recurse(&iter, &a_iter);
   while (dbus_message_iter_get_arg_type(&a_iter) != DBUS_TYPE_INVALID)
     {
	dbus_message_iter_recurse(&a_iter, &s_iter);
	dbus_message_iter_get_basic(&s_iter, &name);
	
	if (strcmp(name, "registration") == 0)
	  {
	     dbus_message_iter_next(&s_iter);
	     dbus_message_iter_recurse(&s_iter, &v_iter);
	     dbus_message_iter_get_basic(&v_iter, &reg_stat);
	  }
	if (strcmp(name, "provider") == 0)
	  {
	     dbus_message_iter_next(&s_iter);
	     dbus_message_iter_recurse(&s_iter, &v_iter);
	     dbus_message_iter_get_basic(&v_iter, &provider);
	  }
	dbus_message_iter_next(&a_iter);
     }
   
   if (strcmp(reg_stat, "unregistered") == 0) provider = "No Service";
   else if (strcmp(reg_stat, "busy") == 0) provider = "Searching...";
   else if (strcmp(reg_stat, "denied") == 0) provider = "SOS only";
   
   return strdup(provider);
}

static void *
fso_operator_unmarhsall(DBusMessage *msg, DBusError *err)
{
   return _fso_operator_unmarhsall(msg);
}

static void
signal_callback_qtopia(void *data, void *ret, DBusError *err)
{
//   printf("GSM-gadget: Qtopia signal callback called\n");
   if (ret)
     {
	int *val_ret;
	
	if ((detected_system == PH_SYS_UNKNOWN) && (changed_h) && (conn))
	  {
	     e_dbus_signal_handler_del(conn, changed_h);
	     changed_h = e_dbus_signal_handler_add(conn,
						   "org.openmoko.qtopia.Phonestatus",
						   "/Status",
						   "org.openmoko.qtopia.Phonestatus",
						   "signalStrengthChanged",
						   signal_changed, data);
	     detected_system = PH_SYS_QTOPIA;
	  }
	val_ret = ret;
	update_signal(*val_ret, data);
     }
   else
     {
//	printf("GSM-gadget: Qtopia signal callback  else part called\n");
	detected_system = PH_SYS_UNKNOWN;
	if (try_again_timer) ecore_timer_del(try_again_timer);
	try_again_timer = ecore_timer_add(1.0, try_again, data);
     }
}

static void
signal_callback_fso(void *data, void *ret, DBusError *err)
{
//   printf("GSM-gadget: FSO signal callback called\n");
   if (ret)
     {
	int *val_ret;
	
	if ((detected_system == PH_SYS_UNKNOWN) && (changed_fso_h) && (conn_system))
	  {
	     e_dbus_signal_handler_del(conn_system, changed_fso_h);
	     changed_fso_h = e_dbus_signal_handler_add(conn_system,
						       "org.freesmartphone.ogsmd",
						       "/org/freesmartphone/GSM/Device",
						       "org.freesmartphone.GSM.Network",
						       "SignalStrength",
						       signal_changed, data);
	     detected_system = PH_SYS_FSO;
	  }
	val_ret = ret;
	update_signal(*val_ret, data);
     }
   else
     {
//	printf("GSM-gadget: FSO signal callback else part called\n");
	detected_system = PH_SYS_UNKNOWN;
	if (try_again_timer) ecore_timer_del(try_again_timer);
	try_again_timer = ecore_timer_add(1.0, try_again, data);
     }
}

static void
operator_callback_qtopia(void *data, void *ret, DBusError *err)
{
//   printf("GSM-gadget: Qtopia operator callback called\n");
   if (ret)
     {
	if ((detected_system == PH_SYS_UNKNOWN) && (operatorch_h) && (conn))
	  {
	     e_dbus_signal_handler_del(conn, operatorch_h);
	     operatorch_h = e_dbus_signal_handler_add(conn,
						      "org.openmoko.qtopia.Phonestatus",
						      "/Status",
						      "org.openmoko.qtopia.Phonestatus",
						      "networkOperatorChanged",
						      operator_changed, data);
	     detected_system = PH_SYS_QTOPIA;
	  }
	update_operator(ret, data);
     }
   else
     {
//	printf("GSM-gadget: Qtopia operator callback else part called\n");
	detected_system = PH_SYS_UNKNOWN;
	if (try_again_timer) ecore_timer_del(try_again_timer);
	try_again_timer = ecore_timer_add(1.0, try_again, data);
     }
}

static void
operator_callback_fso(void *data, void *ret, DBusError *err)
{
//   printf("GSM-gadget: FSO operator callback called\n");
   if (ret)
     {
	if ((detected_system == PH_SYS_UNKNOWN) && (operatorch_fso_h) && (conn_system))
	  {
	     e_dbus_signal_handler_del(conn_system, operatorch_fso_h);
	     operatorch_fso_h = e_dbus_signal_handler_add(conn_system,
							  "org.freesmartphone.ogsmd",
							  "/org/freesmartphone/GSM/Device",
							  "org.freesmartphone.GSM.Network",
							  "Status",
							  fso_operator_changed, data);
	     detected_system = PH_SYS_FSO;
	  }
	update_operator(ret, data);
     }
   else
     {
//	printf("GSM-gadget: FSO operator callback else part called\n");
	detected_system = PH_SYS_UNKNOWN;
	if (try_again_timer) ecore_timer_del(try_again_timer);
	try_again_timer = ecore_timer_add(1.0, try_again, data);
     }
}

static void
signal_result_free(void *data)
{
   free(data);
}

static void
operator_result_free(void *data)
{
   free(data);
}

static void
get_signal(void *data)
{
   DBusMessage *msg;
   
//   printf("GSM-gadget: Get signal called\n");
   if (((detected_system == PH_SYS_UNKNOWN) || (detected_system == PH_SYS_QTOPIA)) && (conn))
     {
	msg = dbus_message_new_method_call("org.openmoko.qtopia.Phonestatus",
					   "/Status",
					   "org.openmoko.qtopia.Phonestatus",
					   "signalStrength");
	if (msg)
	  {
	     e_dbus_method_call_send(conn, msg,
				     signal_unmarhsall,
				     signal_callback_qtopia,
				     signal_result_free, -1, data);
	     dbus_message_unref(msg);
	  }
     }
   if (((detected_system == PH_SYS_UNKNOWN) || (detected_system == PH_SYS_FSO)) && (conn_system))
     {
	msg = dbus_message_new_method_call("org.freesmartphone.ogsmd",
					   "/org/freesmartphone/GSM/Device",
					   "org.freesmartphone.GSM.Network",
					   "GetSignalStrength");
	if (msg)
	  {
	     e_dbus_method_call_send(conn_system, msg,
				     signal_unmarhsall,
				     signal_callback_fso,
				     signal_result_free, -1, data);
	     dbus_message_unref(msg);
	  }
     }
}

static void
get_operator(void *data)
{
   DBusMessage *msg;

//   printf("GSM-gadget: Get operator called\n");
   if (((detected_system == PH_SYS_UNKNOWN) || (detected_system == PH_SYS_QTOPIA)) && (conn))
     {
	msg = dbus_message_new_method_call("org.openmoko.qtopia.Phonestatus",
					   "/Status",
					   "org.openmoko.qtopia.Phonestatus",
					   "networkOperator");
	if (msg)
	  {
	     e_dbus_method_call_send(conn, msg,
				     operator_unmarhsall,
				     operator_callback_qtopia,
				     operator_result_free, -1, data);
	     dbus_message_unref(msg);
	  }
     }
   if (((detected_system == PH_SYS_UNKNOWN) || (detected_system == PH_SYS_FSO)) && (conn_system))
     {
	msg = dbus_message_new_method_call("org.freesmartphone.ogsmd",
					   "/org/freesmartphone/GSM/Device",
					   "org.freesmartphone.GSM.Network",
					   "GetStatus");
	if (msg)
	  {
	     e_dbus_method_call_send(conn_system, msg,
				     fso_operator_unmarhsall,
				     operator_callback_fso,
				     operator_result_free, -1, data);
	     dbus_message_unref(msg);
	  }
     }
}

static void
signal_changed(void *data, DBusMessage *msg)
{
   DBusError err;
   dbus_int32_t val = -1;
   
   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &val, DBUS_TYPE_INVALID))
     return;
   update_signal(val, data);
}

static void
operator_changed(void *data, DBusMessage *msg)
{
   DBusError err;
   char *str = NULL;
   
   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID))
     return;
   update_operator(str, data);
}

static void
fso_operator_changed(void *data, DBusMessage *msg)
{
   char *provider;
   provider = _fso_operator_unmarhsall(msg);
   update_operator(provider, data);
}

static void
name_changed(void *data, DBusMessage *msg)
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
   if ((!strcmp(s1, "org.openmoko.qtopia.Phonestatus")) && (conn))
     {
//	printf("GSM-gadget: Qtopia name owner changed\n");
	if (changed_h)
	  {
	     e_dbus_signal_handler_del(conn, changed_h);
	     changed_h = e_dbus_signal_handler_add(conn,
						   "org.openmoko.qtopia.Phonestatus",
						   "/Status",
						   "org.openmoko.qtopia.Phonestatus",
						   "signalStrengthChanged",
						   signal_changed, data);
	     get_signal(data);
	  }
	if (operatorch_h)
	  {
	     e_dbus_signal_handler_del(conn, operatorch_h);
	     operatorch_h = e_dbus_signal_handler_add(conn,
						      "org.openmoko.qtopia.Phonestatus",
						      "/Status",
						      "org.openmoko.qtopia.Phonestatus",
						      "networkOperatorChanged",
						      operator_changed, data);
	     get_operator(data);
	  }
     }
   else if ((!strcmp(s1, "org.freesmartphone.ogsmd")) && (conn_system))
     {
//	printf("GSM-gadget: FSO name owner changed\n");
	if (changed_fso_h) 
	  {
	     e_dbus_signal_handler_del(conn_system, changed_fso_h);
	     changed_fso_h = e_dbus_signal_handler_add(conn_system,
						       "org.freesmartphone.ogsmd",
						       "/org/freesmartphone/GSM/Device",
						       "org.freesmartphone.GSM.Network",
						       "SignalStrength",
						       signal_changed, data);
	     get_signal(data);
	  }
	if (operatorch_fso_h)
	  {
	     e_dbus_signal_handler_del(conn_system, operatorch_fso_h);
	     operatorch_fso_h = e_dbus_signal_handler_add(conn_system,
							  "org.freesmartphone.ogsmd",
							  "/org/freesmartphone/GSM/Device",
							  "org.freesmartphone.GSM.Network",
							  "Status",
							  fso_operator_changed, data);
	     get_operator(data);
	  }
     }
   return;
}
