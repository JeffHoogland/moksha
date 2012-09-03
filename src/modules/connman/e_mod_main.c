#include "e.h"
#include "e_mod_main.h"

#include "E_Connman.h"

static E_Module *connman_mod;
static char tmpbuf[4096]; /* general purpose buffer, just use immediately */

const char _e_connman_name[] = "connman";
const char _e_connman_Name[] = "Connection Manager";
int _e_connman_log_dom = -1;

const char *
e_connman_theme_path(void)
{
#define TF "/e-module-connman.edj"
   size_t dirlen;

   dirlen = strlen(connman_mod->dir);
   if (dirlen >= sizeof(tmpbuf) - sizeof(TF))
     return NULL;

   memcpy(tmpbuf, connman_mod->dir, dirlen);
   memcpy(tmpbuf + dirlen, TF, sizeof(TF));

   return tmpbuf;
#undef TF
}

static void _econnman_mod_manager_update_inst(E_Connman_Module_Context *ctxt,
                                              E_Connman_Instance *inst,
                                              enum Connman_State state,
                                              enum Connman_Service_Type type)
{
   Evas_Object *o = inst->ui.gadget;

   switch (state)
     {
      case CONNMAN_STATE_ONLINE:
         edje_object_signal_emit(o, "e,changed,connected,yes", "e");
         edje_object_signal_emit(o, "e,changed,state,online", "e");
         break;
      case CONNMAN_STATE_READY:
         edje_object_signal_emit(o, "e,changed,connected,yes", "e");
         edje_object_signal_emit(o, "e,changed,state,ready", "e");
         break;
      case CONNMAN_STATE_IDLE:
         edje_object_signal_emit(o, "e,changed,connected,no", "e");
         edje_object_signal_emit(o, "e,changed,state,idle", "e");
         break;
      case CONNMAN_STATE_OFFLINE:
         edje_object_signal_emit(o, "e,changed,connected,no", "e");
         edje_object_signal_emit(o, "e,changed,state,disconnect", "e");
         break;
      case CONNMAN_STATE_NONE:
         edje_object_signal_emit(o, "e,changed,connected,no", "e");
         edje_object_signal_emit(o, "e,changed,state,failure", "e");
         break;
     }

   switch (type)
     {
      case CONNMAN_SERVICE_TYPE_ETHERNET:
         edje_object_signal_emit(o, "e,changed,technology,ethernet", "e");
         break;
      case CONNMAN_SERVICE_TYPE_WIFI:
         edje_object_signal_emit(o, "e,changed,technology,wifi", "e");
         break;
      case CONNMAN_SERVICE_TYPE_NONE:
         if (state == CONNMAN_STATE_ONLINE || state == CONNMAN_STATE_READY)
           edje_object_signal_emit(o, "e,changed,technology,none,others", "e");
         else
           edje_object_signal_emit(o, "e,changed,technology,none,others", "e");

         break;
     }

   DBG("state=%d type=%d", state, type);
}

void econnman_mod_manager_update(struct Connman_Manager *cm)
{
   enum Connman_Service_Type type;
   E_Connman_Module_Context *ctxt = connman_mod->data;
   E_Connman_Instance *inst;
   Eina_List *l;

   DBG("cm->services=%p", cm->services);

   if (cm->services)
     {
        struct Connman_Service *cs = EINA_INLIST_CONTAINER_GET(cm->services,
                                                      struct Connman_Service);
        type = cs->type;
     }
   else
     type = CONNMAN_SERVICE_TYPE_NONE;

   EINA_LIST_FOREACH(ctxt->instances, l, inst)
     _econnman_mod_manager_update_inst(ctxt, inst, cm->state, type);
}

static void _econnman_gadget_setup(E_Connman_Instance *inst)
{
   E_Connman_Module_Context *ctxt = inst->ctxt;
   Evas_Object *o = inst->ui.gadget;

   DBG("has_manager=%d", ctxt->has_manager);

   if (!ctxt->has_manager)
     {
        edje_object_signal_emit(o, "e,unavailable", "e");
        edje_object_part_text_set(o, "e.text.name", _("No ConnMan"));
        edje_object_part_text_set(o, "e.text.error",
                                  _("No ConnMan server found."));
        edje_object_signal_emit(o, "e,changed,connected,no", "e");
        edje_object_part_text_set(o, "e.text.offline_mode", "");
        edje_object_signal_emit(o, "e,changed,offline_mode,no", "e");
     }
   else
     edje_object_signal_emit(o, "e,available", "e");

   return;
}

void econnman_mod_manager_inout(struct Connman_Manager *cm, bool in)
{
   E_Connman_Module_Context *ctxt = connman_mod->data;
   const Eina_List *l;
   E_Connman_Instance *inst;

   DBG("Manager %s", in ? "in" : "out");
   ctxt->has_manager = in;

   EINA_LIST_FOREACH(ctxt->instances, l, inst)
     _econnman_gadget_setup(inst);

   econnman_mod_manager_update(cm);
}

static void
_econnman_cb_mouse_down(void *data, Evas *evas __UNUSED__,
                       Evas_Object *obj __UNUSED__, void *event)
{
}

static void
_econnman_cb_mouse_in(void *data, Evas *evas __UNUSED__,
                     Evas_Object *obj __UNUSED__, void *event __UNUSED__)
{
}

static void
_econnman_cb_mouse_out(void *data, Evas *evas __UNUSED__,
                      Evas_Object *obj __UNUSED__, void *event __UNUSED__)
{
}

/* Gadcon Api Functions */

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   E_Connman_Instance *inst;
   E_Connman_Module_Context *ctxt;

   if (!connman_mod)
     return NULL;

   ctxt = connman_mod->data;

   inst = E_NEW(E_Connman_Instance, 1);
   inst->ctxt = ctxt;
   inst->ui.gadget = edje_object_add(gc->evas);
   e_theme_edje_object_set(inst->ui.gadget, "base/theme/modules/connman",
                           "e/modules/connman/main");

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->ui.gadget);
   inst->gcc->data = inst;

   evas_object_event_callback_add
     (inst->ui.gadget, EVAS_CALLBACK_MOUSE_DOWN, _econnman_cb_mouse_down, inst);
   evas_object_event_callback_add
     (inst->ui.gadget, EVAS_CALLBACK_MOUSE_IN, _econnman_cb_mouse_in, inst);
   evas_object_event_callback_add
     (inst->ui.gadget, EVAS_CALLBACK_MOUSE_OUT, _econnman_cb_mouse_out, inst);

   _econnman_gadget_setup(inst);

   ctxt->instances = eina_list_append(ctxt->instances, inst);

   return inst->gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   E_Connman_Module_Context *ctxt;
   E_Connman_Instance *inst;

   if (!connman_mod)
     return;

   ctxt = connman_mod->data;
   if (!ctxt)
     return;

   inst = gcc->data;
   if (!inst)
     return;

   evas_object_del(inst->ui.gadget);

   ctxt->instances = eina_list_remove(ctxt->instances, inst);

   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _(_e_connman_Name);
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;

   o = edje_object_add(evas);
   edje_object_file_set(o, e_connman_theme_path(), "icon");
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   E_Connman_Module_Context *ctxt;
   Eina_List *instances;

   if (!connman_mod)
     return NULL;

   ctxt = connman_mod->data;
   if (!ctxt)
     return NULL;

   instances = ctxt->instances;
   snprintf(tmpbuf, sizeof(tmpbuf), "connman.%d", eina_list_count(instances));
   return tmpbuf;
}

static const E_Gadcon_Client_Class _gc_class =
{
   GADCON_CLIENT_CLASS_VERSION, _e_connman_name,
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL,
      e_gadcon_site_is_not_toolbar
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, _e_connman_Name };

static E_Config_Dialog * _econnman_config(E_Container *con,
                                          const char *params __UNUSED__)
{
   E_Connman_Module_Context *ctxt;

   if (!connman_mod)
     return NULL;

   ctxt = connman_mod->data;
   if (!ctxt)
     return NULL;

   if (!ctxt->conf_dialog)
     ctxt->conf_dialog = e_connman_config_dialog_new(con, ctxt);

   return ctxt->conf_dialog;
}

static const char _reg_cat[] = "extensions";
static const char _reg_item[] = "extensions/connman";

static void
_econnman_configure_registry_register(void)
{
   e_configure_registry_category_add(_reg_cat, 90, _("Extensions"), NULL,
                                     "preferences-extensions");
   e_configure_registry_item_add(_reg_item, 110, _(_e_connman_Name), NULL,
                                 e_connman_theme_path(),
                                 _econnman_config);
}

static void
_econnman_configure_registry_unregister(void)
{
   e_configure_registry_item_del(_reg_item);
   e_configure_registry_category_del(_reg_cat);
}

EAPI void *
e_modapi_init(E_Module *m)
{
   E_Connman_Module_Context *ctxt;
   E_DBus_Connection *c;

   c = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!c)
     goto error_dbus_bus_get;
   if (!e_connman_system_init(c))
     goto error_connman_system_init;

   ctxt = E_NEW(E_Connman_Module_Context, 1);
   if (!ctxt)
     goto error_connman_context;

   ctxt->conf_dialog = NULL;
   connman_mod = m;

   if (_e_connman_log_dom < 0)
     {
        _e_connman_log_dom = eina_log_domain_register("econnman", EINA_COLOR_ORANGE);
        if (_e_connman_log_dom < 0)
          {
             EINA_LOG_CRIT("could not register logging domain econnman");
             goto error_log_domain;
          }
     }

   _econnman_configure_registry_register();
   e_gadcon_provider_register(&_gc_class);

   return ctxt;

error_log_domain:
   _e_connman_log_dom = -1;
   connman_mod = NULL;
   E_FREE(ctxt);
error_connman_context:
   e_connman_system_shutdown();
error_connman_system_init:
error_dbus_bus_get:
   return NULL;
}

static void
_econnman_instances_free(E_Connman_Module_Context *ctxt)
{
   while (ctxt->instances)
     {
        E_Connman_Instance *inst = ctxt->instances->data;
        ctxt->instances = eina_list_remove_list(ctxt->instances,
                                                ctxt->instances);
        e_object_del(E_OBJECT(inst->gcc));
     }
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   E_Connman_Module_Context *ctxt;

   ctxt = m->data;
   if (!ctxt)
     return 0;

   _econnman_instances_free(ctxt);
   _econnman_configure_registry_unregister();
   e_gadcon_provider_unregister(&_gc_class);

   E_FREE(ctxt);
   connman_mod = NULL;

   e_connman_system_shutdown();
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   E_Connman_Module_Context *ctxt;

   ctxt = m->data;
   if (!ctxt)
     return 0;
   return 1;
}
