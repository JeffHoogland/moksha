#include "e_mod_main.h"

extern const char _e_bluez_Name[];

struct _E_Config_Dialog_Data
{
   E_Bluez_Instance *inst;
   const char       *name;
   Eina_Bool         mode;
   unsigned int      timeout;
   struct
   {
      Evas_Object *label;
      Evas_Object *slider;
      Evas_Object *help;
   } gui;
};

/* Local Function Prototypes */
static void        *_create_data(E_Config_Dialog *dialog);
static void         _free_data(E_Config_Dialog      *dialog,
                               E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog      *dialog,
                                  Evas                 *evas,
                                  E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog      *dialog,
                        E_Config_Dialog_Data *cfdata);

E_Config_Dialog *
e_bluez_config_dialog_new(E_Container      *con,
                          E_Bluez_Instance *inst)
{
   E_Config_Dialog *dialog;
   E_Config_Dialog_View *view;

   if (inst->conf_dialog)
     return inst->conf_dialog;

   view = E_NEW(E_Config_Dialog_View, 1);
   if (!view)
     return NULL;

   view->create_cfdata = _create_data;
   view->free_cfdata = _free_data;
   view->basic.create_widgets = _basic_create;
   view->basic.apply_cfdata = _basic_apply;

   dialog = e_config_dialog_new(con, _("Bluetooth Settings"),
                                _e_bluez_Name, "e_bluez_config_dialog_new",
                                e_bluez_theme_path(), 0, view, inst);

   return dialog;
}

static void *
_create_data(E_Config_Dialog *dialog)
{
   E_Config_Dialog_Data *cfdata;
   E_Bluez_Instance *inst;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata)
     return NULL;

   cfdata->inst = dialog->data;
   inst = cfdata->inst;
   if (!e_bluez_adapter_discoverable_get(inst->adapter, &cfdata->mode))
     cfdata->mode = 0;

   if (!e_bluez_adapter_discoverable_timeout_get
         (inst->adapter, &cfdata->timeout))
     cfdata->timeout = 0;
   cfdata->timeout /= 60;

   if (!e_bluez_adapter_name_get(inst->adapter, &cfdata->name))
     cfdata->name = NULL;
   cfdata->name = strdup(cfdata->name);

   return cfdata;
}

static void
_free_data(E_Config_Dialog      *dialog,
           E_Config_Dialog_Data *cfdata)
{
   E_Bluez_Instance *inst = dialog->data;

   inst->conf_dialog = NULL;
   E_FREE(cfdata);
}

static void
_cb_disable_timeout(void            *data,
                    Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   Eina_Bool disable = !cfdata->mode;

   e_widget_disabled_set(cfdata->gui.label, disable);
   e_widget_disabled_set(cfdata->gui.slider, disable);
   e_widget_disabled_set(cfdata->gui.help, disable);
}

static Evas_Object *
_basic_create(E_Config_Dialog *dialog __UNUSED__,
              Evas                   *evas,
              E_Config_Dialog_Data   *cfdata)
{
   Evas_Object *o, *ob;
   Evas_Object *check;
   char buf[48];
   const char *address;

   o = e_widget_list_add(evas, 0, 0);

   if (!e_bluez_adapter_address_get(cfdata->inst->adapter, &address))
     address = NULL;

   ob = e_widget_label_add(evas, _("Name"));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   ob = e_widget_entry_add(evas, (char **)&cfdata->name, NULL, NULL, NULL);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   check = e_widget_check_add
       (evas, _("Discoverable mode"), (int *)&cfdata->mode);
   e_widget_list_object_append(o, check, 1, 1, 0.5);

   ob = e_widget_label_add(evas, _("Discovarable Timeout"));
   cfdata->gui.label = ob;
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f minutes"), 0, 30, 1, 0,
                            NULL, (int *)&cfdata->timeout, 100);
   e_widget_slider_special_value_add(ob, 0.0, 0.0, _("Forever"));
   cfdata->gui.slider = ob;
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   e_widget_on_change_hook_set(check, _cb_disable_timeout, cfdata);
   _cb_disable_timeout(cfdata, NULL);

   snprintf(buf, sizeof(buf), _("MAC Address: %s"), address);
   ob = e_widget_label_add(evas, buf);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   return o;
}

static void
_method_success_check(void            *data,
                      DBusMessage *msg __UNUSED__,
                      DBusError       *error)
{
   const char *name = data;

   if ((!error) || (!dbus_error_is_set(error)))
     return;

   ERR("method %s() finished with error: %s %s\n",
       name, error->name, error->message);
   dbus_error_free(error);
}

static int
_basic_apply(E_Config_Dialog *dialog __UNUSED__,
             E_Config_Dialog_Data   *cfdata)
{
   E_Bluez_Instance *inst = cfdata->inst;

   if (!e_bluez_adapter_discoverable_set
         (inst->adapter, cfdata->mode,
         _method_success_check, "e_bluez_adapter_discoverable_get"))
     ERR("Can't set Discoverable on adapter");

   if (!e_bluez_adapter_discoverable_timeout_set
         (inst->adapter, cfdata->timeout * 60,
         _method_success_check, "e_bluez_adapter_discoverable_timeout_get"))
     ERR("Can't set DiscoverableTimeout on adapter");

   if (!e_bluez_adapter_name_set
         (inst->adapter, cfdata->name,
         _method_success_check, "e_bluez_adapter_name_get"))
     ERR("Can't set Name on adapter");

   return 1;
}

