#include "e_mod_main.h"

extern const char _e_connman_Name[];

extern const char *e_str_enabled;
extern const char *e_str_available;
extern const char *e_str_connected;
extern const char *e_str_offline;

struct connman_config_technologies
{
   EINA_INLIST;
   Evas_Object          *obj;
   E_Connman_Technology *technology;
   int                   enabled;
};

struct _E_Config_Dialog_Data
{
   E_Connman_Module_Context *ctxt;
   const char               *selected_network;
   struct connman_config_network_ui
   {
      Evas_Object *hlayout;
      Evas_Object *netframe;
      Evas_Object *netlist;
/*
        Evas_Object *o_up;
        Evas_Object *o_down;
        Evas_Object *o_add;
        Evas_Object *o_del;
 */
      Evas_Object *setframe;
      struct connman_config_network_settings_ui
      {
         Evas_Object *scr_general;
         Evas_Object *list_general;
         Evas_Object *lb_autoconn;
         Evas_Object *lb_autoconn_val;
         Evas_Object *lb_favorite;
         Evas_Object *lb_favorite_val;
         Evas_Object *lb_type;
         Evas_Object *lb_type_val;
         Evas_Object *lb_ipv4_method;
         Evas_Object *lb_ipv4_method_val;
         Evas_Object *lb_ipv4_address;
         Evas_Object *lb_ipv4_address_val;
         Evas_Object *lb_ipv4_netmask;
         Evas_Object *lb_ipv4_netmask_val;

#if 0 // need to do proxy stuff to enalbe the toolbook
         Evas_Object *settings_otb;
         Evas_Object *list_proxy;
#endif
      } settings_otb;
   } networks;
   struct connman_config_switch_ui
   {
      Evas_Object *vlayout;
      Evas_Object *type_frame;
      Evas_Object *off_frame;
      Eina_Inlist *technologies;
      Evas_Object *o_off;
      int          offline_mode;
   } switches;
};

/* Local Function Prototypes */
static void        *_create_data(E_Config_Dialog *dialog);
static void         _free_data(E_Config_Dialog      *dialog,
                               E_Config_Dialog_Data *cfdata);
static void         _fill_data(E_Config_Dialog_Data     *cfdata,
                               E_Connman_Module_Context *ctxt);
static Evas_Object *_basic_create(E_Config_Dialog      *dialog,
                                  Evas                 *evas,
                                  E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog      *dialog,
                        E_Config_Dialog_Data *cfdata);

struct connman_service_move_data
{
   const char               *service_path;
   const char               *service_ref_path;
   E_Connman_Module_Context *ctxt;
};

enum _Conmman_Move_Direction {
   SERVICE_MOVE_DOWN,
   SERVICE_MOVE_UP
};

static void
_connman_service_move_cb(void            *data,
                         DBusMessage *msg __UNUSED__,
                         DBusError       *error)
{
   struct connman_service_move_data *d = data;
   if (error && dbus_error_is_set(error))
     {
        ERR("%s method failed with message \'%s\'",
            error->name, error->message);
        dbus_error_free(error);
     }
   else
     DBG("Changed service order");

   eina_stringshare_del(d->service_ref_path);
   eina_stringshare_del(d->service_path);
   E_FREE(d);
}

static void
_connman_service_move(E_Connman_Service           *service,
                      const E_Connman_Service     *service_ref,
                      enum _Conmman_Move_Direction direction)
{
   struct connman_service_move_data *d;
   int ret;

   d = E_NEW(struct connman_service_move_data, 1);
   if (!d)
     return;

   d->service_ref_path = eina_stringshare_ref(service_ref->path);
   d->service_path = eina_stringshare_ref(service->path);
   d->ctxt = service->ctxt;

   DBG("Try to move %s %s %s\n", d->service_path,
       (direction == SERVICE_MOVE_UP) ? "before" : "after",
       d->service_ref_path);

   if (direction == SERVICE_MOVE_UP)
     ret = e_connman_service_move_before
         (service->element, d->service_ref_path, _connman_service_move_cb, d);
   else
     ret = e_connman_service_move_after
         (service->element, d->service_ref_path, _connman_service_move_cb, d);

   if (!ret)
     {
        eina_stringshare_del(d->service_ref_path);
        eina_stringshare_del(d->service_path);
        E_FREE(d);
        _connman_operation_error_show(_("Re-order preferred services"));
     }
}

struct _connman_technology_onoff_data
{
   const char               *type;
   E_Connman_Module_Context *ctxt;
   Eina_Bool                 on;
};

static void
_connman_technology_onoff_cb(void            *data,
                             DBusMessage *msg __UNUSED__,
                             DBusError       *error)
{
   struct _connman_technology_onoff_data *d = data;
   if (error && dbus_error_is_set(error))
     {
        ERR("%s method failed with message \'%s\'.",
            error->name, error->message);
        dbus_error_free(error);
     }
   else
     DBG("Technology %s has been %s.", d->type, d->on ? "enabled" : "disabled");

   eina_stringshare_del(d->type);
   E_FREE(d);
}

static void
_connman_technology_onoff(E_Connman_Module_Context *ctxt,
                          const char               *type,
                          Eina_Bool                 on)
{
   int ret;
   struct _connman_technology_onoff_data *d;

   d = E_NEW(struct _connman_technology_onoff_data, 1);
   if (!d)
     {
        _connman_operation_error_show("No memory available");
        return;
     }

   d->type = eina_stringshare_add(type);
   d->ctxt = ctxt;
   d->on = on;

   if(on)
     ret = e_connman_manager_technology_enable
         (type, _connman_technology_onoff_cb, d);
   else
     ret = e_connman_manager_technology_disable
         (type, _connman_technology_onoff_cb, d);

   if(!ret)
     {
        eina_stringshare_del(type);
        E_FREE(d);
     }

   return;
}

E_Config_Dialog *
e_connman_config_dialog_new(E_Container              *con,
                            E_Connman_Module_Context *ctxt)
{
   E_Config_Dialog *dialog;
   E_Config_Dialog_View *view;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(!!ctxt->conf_dialog, ctxt->conf_dialog);

   view = E_NEW(E_Config_Dialog_View, 1);
   if (!view)
     return NULL;

   view->create_cfdata = _create_data;
   view->free_cfdata = _free_data;
   view->basic.create_widgets = _basic_create;
   view->basic.apply_cfdata = _basic_apply;

   dialog = e_config_dialog_new
       (con, _("Connection Manager"),
       _e_connman_Name, "e_connman_config_dialog_new",
       e_connman_theme_path(), 0, view, ctxt);
   e_dialog_resizable_set(dialog->dia, 1);

   return dialog;
}

static void *
_create_data(E_Config_Dialog *dialog)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata)
     return NULL;
   _fill_data(cfdata, dialog->data);
   return cfdata;
}

static void
_free_data(E_Config_Dialog      *dialog,
           E_Config_Dialog_Data *cfdata)
{
   E_Connman_Module_Context *ctxt = dialog->data;
   struct connman_config_switch_ui *ui = &cfdata->switches;

   while(ui->technologies)
     {
        struct connman_config_technologies *t;

        t = (struct connman_config_technologies *)ui->technologies;
        ui->technologies = eina_inlist_remove
            (ui->technologies, EINA_INLIST_GET(t));
        E_FREE(t);
     }

   ctxt->conf_dialog = NULL;
   E_FREE(cfdata);
}

static inline void
_fill_data(E_Config_Dialog_Data     *cfdata,
           E_Connman_Module_Context *ctxt)
{
   cfdata->ctxt = ctxt;
   cfdata->switches.technologies = NULL;
}

void
_cb_scr_general_show(void            *data,
                     Evas *evas       __UNUSED__,
                     Evas_Object     *obj,
                     void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   struct connman_config_network_ui *ui;
   ui = &cfdata->networks;

   if (e_widget_ilist_selected_get(ui->netlist) < 0)
     evas_object_hide(obj);
}

static void
_network_settings_general_page_create(Evas                 *evas,
                                      E_Config_Dialog_Data *cfdata)
{
   struct connman_config_network_settings_ui *ui;
   Evas_Coord mw, mh;
   ui = &cfdata->networks.settings_otb;

   ui->list_general = e_widget_list_add(evas, 0, 0);
#define _APPEND_ITEM(item, label)                              \
  do                                                           \
    {                                                          \
       ui->lb_##item = e_widget_label_add(evas, _(label));     \
       ui->lb_##item ## _val = e_widget_entry_add              \
           (evas, NULL, NULL, NULL, NULL);                     \
       e_widget_entry_readonly_set(ui->lb_##item ## _val, 1);  \
       e_widget_list_object_append                             \
         (ui->list_general, ui->lb_##item, 1, 0, 0.0);         \
       e_widget_list_object_append                             \
         (ui->list_general, ui->lb_##item ## _val, 1, 0, 0.0); \
    } while(0)

   _APPEND_ITEM(autoconn, _("Auto-connect:"));
   _APPEND_ITEM(favorite, _("Favorite:"));
   _APPEND_ITEM(type, _("Type:"));
   _APPEND_ITEM(ipv4_method, _("IP method:"));
   _APPEND_ITEM(ipv4_address, _("IP address:"));
   _APPEND_ITEM(ipv4_netmask, _("Netmask:"));
#undef _APPEND_ITEM

   evas_object_show(ui->list_general);
   e_widget_size_min_get(ui->list_general, &mw, &mh);
   if (mw < 100 * e_scale)
     mw = 100 * e_scale;
   if (mh < 100 * e_scale)
     mh = 100 * e_scale;
   evas_object_resize(ui->list_general, mw, mh);

   ui->scr_general = e_widget_scrollframe_simple_add(evas, ui->list_general);
   e_widget_size_min_set(ui->scr_general, 100 * e_scale, 100 * e_scale);

   evas_object_event_callback_add(ui->scr_general, EVAS_CALLBACK_SHOW, _cb_scr_general_show, cfdata);
}

#if 0 // need to do proxy, until then hide the toolbook complexity
static void
_network_settings_proxy_page_create(Evas                 *evas,
                                    E_Config_Dialog_Data *cfdata)
{
   struct connman_config_network_settings_ui *ui;
   Evas_Object *label_todo;
   ui = &cfdata->networks.settings_otb;

   ui->list_proxy = e_widget_list_add(evas, 0, 0);

   label_todo = e_widget_label_add(evas, "TODO");
   e_widget_list_object_append(ui->list_proxy, label_todo, 1, 1, 0.0);
}

#endif

static void
_network_settings_create(Evas                 *evas,
                         E_Config_Dialog_Data *cfdata)
{
   struct connman_config_network_ui *ui;

   ui = &cfdata->networks;
   ui->setframe = e_widget_framelist_add(evas, _("Settings"), 0);

#if 0 // need to do proxy, until then hide the toolbook complexity
   ui->settings_otb.settings_otb = e_widget_toolbook_add(evas, 24 * e_scale, 24 * e_scale);

   _network_settings_general_page_create(evas, cfdata);
   e_widget_toolbook_page_append
     (ui->settings_otb.settings_otb, NULL, _("General"),
     ui->settings_otb.scr_general, 1, 1, 1, 1, 0.5, 0.0);

   _network_settings_proxy_page_create(evas, cfdata);
   e_widget_toolbook_page_append
     (ui->settings_otb.settings_otb, NULL, _("Proxy"),
     ui->settings_otb.list_proxy, 1, 0, 1, 0, 0.5, 0.0);

   e_widget_size_min_set(ui->settings_otb.settings_otb, 100, 100);
   e_widget_toolbook_page_show(ui->settings_otb.settings_otb, 0);
   e_widget_framelist_object_append(ui->setframe, ui->settings_otb.settings_otb);
#else
   _network_settings_general_page_create(evas, cfdata);
   e_widget_framelist_object_append(ui->setframe, ui->settings_otb.scr_general);
#endif
}

static inline void
_networks_fill_details(E_Config_Dialog_Data *cfdata,
                       Evas_Object *list     __UNUSED__,
                       int sel               __UNUSED__)
{
   E_Connman_Service *service;
   E_Connman_Module_Context *ctxt = cfdata->ctxt;
   struct connman_config_network_settings_ui *ui = &cfdata->networks.settings_otb;

   service = _connman_ctxt_find_service_stringshare
       (ctxt, cfdata->selected_network);
   if (!service)
     {
        ERR("service not found: %s.", cfdata->selected_network);
        return;
     }
   e_widget_entry_text_set(ui->lb_autoconn_val,
                           service->auto_connect ? _("True") : _("False"));
   e_widget_entry_text_set(ui->lb_favorite_val,
                           service->favorite ? _("True") : _("False"));
   e_widget_entry_text_set(ui->lb_type_val, service->type);
   e_widget_entry_text_set(ui->lb_ipv4_method_val, service->ipv4_method);
   e_widget_entry_text_set(ui->lb_ipv4_address_val, service->ipv4_address);
   e_widget_entry_text_set(ui->lb_ipv4_netmask_val, service->ipv4_netmask);

   evas_object_show(ui->list_general);
   evas_object_show(ui->scr_general);
}

/*
   static inline void
   _networks_disable_buttons(E_Config_Dialog_Data *cfdata, Evas_Object *list, int sel)
   {
   Evas_Object *o_up = cfdata->networks.o_up;
   Evas_Object *o_down = cfdata->networks.o_down;

   if (sel >= 0)
     {
        int index = e_widget_ilist_selected_get(list);
        int count = e_widget_ilist_count(list);
        e_widget_disabled_set(o_up, !index);

        e_widget_disabled_set(o_down, (count > index + 1)?0:1);
     }
   else
     {
        e_widget_disabled_set(o_up, 1);
        e_widget_disabled_set(o_down, 1);
     }
   }
 */
void
_cb_service_selected(void *data)
{
   E_Config_Dialog_Data *cfdata = data;
   Evas_Object *list = cfdata->networks.netlist;
   int sel = e_widget_ilist_selected_get(list);
//   _networks_disable_buttons(cfdata, list, sel);
   _networks_fill_details(cfdata, list, sel);
}

static unsigned int
_networks_list_fill(Evas                 *evas,
                    E_Config_Dialog_Data *cfdata)
{
   Evas_Object *list = cfdata->networks.netlist;
   E_Connman_Module_Context *ctxt = cfdata->ctxt;
   E_Connman_Service *service;

   EINA_INLIST_FOREACH(ctxt->services, service)
     {
        Evas_Object *icon = _connman_service_new_list_item(evas, service);

        e_widget_ilist_append
          (list, icon, service->name, _cb_service_selected,
          cfdata, service->path);
     }

   return eina_inlist_count(ctxt->services);
}

static void
_networks_button_up_cb(void       *data,
                       void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   Evas_Object *netlist = cfdata->networks.netlist;
   E_Connman_Module_Context *ctxt = cfdata->ctxt;
   E_Connman_Service *service, *service_ref;
   int sel;

   sel = e_widget_ilist_selected_get(netlist);
   if (sel <= 0)
     return;

   service = _connman_ctxt_find_service_stringshare
       (ctxt, cfdata->selected_network);
   e_widget_ilist_selected_set(netlist, sel - 1);

   service_ref = _connman_ctxt_find_service_stringshare
       (ctxt, cfdata->selected_network);

   e_widget_ilist_selected_set(netlist, sel);

   _connman_service_move(service, service_ref, SERVICE_MOVE_UP);
}

static void
_networks_button_down_cb(void       *data,
                         void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   Evas_Object *netlist = cfdata->networks.netlist;
   E_Connman_Module_Context *ctxt = cfdata->ctxt;
   E_Connman_Service *service, *service_ref;
   int sel;
   int count;

   sel = e_widget_ilist_selected_get(netlist);
   count = e_widget_ilist_count(netlist);
   if (sel < 0 || (count == sel + 1))
     return;

   service = _connman_ctxt_find_service_stringshare
       (ctxt, cfdata->selected_network);
   e_widget_ilist_selected_set(netlist, sel + 1);
   service_ref = _connman_ctxt_find_service_stringshare
       (ctxt, cfdata->selected_network);
   e_widget_ilist_selected_set(netlist, sel);

   _connman_service_move(service, service_ref, SERVICE_MOVE_DOWN);
}

static void
_networks_list_create(Evas                 *evas,
                      E_Config_Dialog_Data *cfdata)
{
   struct connman_config_network_ui *ui;
   Evas_Object *ot;
   Evas_Coord mw, mh;

   ui = &cfdata->networks;
   ui->netframe = e_widget_framelist_add(evas, _("All networks"), 0);
   ui->netlist = e_widget_ilist_add(evas, 24, 24, &cfdata->selected_network);
   e_widget_ilist_multi_select_set(ui->netlist, 0);
   e_widget_on_change_hook_set(ui->netlist, NULL, cfdata);
   e_widget_size_min_set(ui->netlist, 100, 100);
   e_widget_ilist_selected_set(ui->netlist, 0);
   e_widget_framelist_object_append(ui->netframe, ui->netlist);

#if 0
   /* Buttons */
   ot = e_widget_table_add(evas, 0);
   ui->o_up = e_widget_button_add
       (evas, _("Up"), "go-up", _networks_button_up_cb, cfdata, NULL);
   e_widget_disabled_set(ui->o_up, 1);
   e_widget_table_object_append(ot, ui->o_up, 0, 0, 1, 1, 1, 0, 1, 0);
   ui->o_down = e_widget_button_add
       (evas, _("Down"), "go-down", _networks_button_down_cb, cfdata, NULL);
   e_widget_disabled_set(ui->o_down, 1);
   e_widget_table_object_append(ot, ui->o_down, 1, 0, 1, 1, 1, 0, 1, 0);
   ui->o_add = e_widget_button_add
       (evas, _("Add"), "list-add", NULL, cfdata, NULL);
   e_widget_disabled_set(ui->o_add, 1);
   e_widget_table_object_append(ot, ui->o_add, 0, 1, 2, 1, 1, 0, 1, 0);

   e_widget_size_min_get(ot, &mw, &mh);
   e_widget_framelist_object_append_full(ui->netframe, ot,
                                         1, 1, /* fill */
                                         1, 0, /* expand */
                                         0.5, 0.5, /* align */
                                         mw, mh, /* min */
                                         99999, 99999 /* max */
                                         );
#endif
}

static void
_networks_page_create(Evas                 *evas,
                      E_Config_Dialog_Data *cfdata)
{
   struct connman_config_network_ui *ui;

   /*
    * TODO:
    *
    *     Add a elm_widget_pager similar to elm_pager and push pop the
    *     list and the associated element settings when the list
    *     element is selected (needs a view/edit/info/whatever button
    *     as the list elements may be reordered, thus we need th
    *     selection).
    *
    *     This should reduce the width of this dialog.
    */

   ui = &cfdata->networks;
   ui->hlayout = e_widget_list_add(evas, 0, 1);
   _networks_list_create(evas, cfdata);
   e_widget_list_object_append(ui->hlayout, ui->netframe, 1, 1, 0.0);
   _network_settings_create(evas, cfdata);
   e_widget_list_object_append(ui->hlayout, ui->setframe, 1, 1, 0.0);
   evas_object_hide(ui->settings_otb.scr_general);
}

static inline void
_switches_page_create_technologies(Evas                            *evas,
                                   E_Connman_Module_Context        *ctxt,
                                   struct connman_config_switch_ui *ui)
{
   struct E_Connman_Technology *t;
   EINA_INLIST_FOREACH(ctxt->technologies, t)
     {
        struct connman_config_technologies *t_list;

        t_list = E_NEW(struct connman_config_technologies, 1);
        t_list->technology = t;
        t_list->enabled = ((t->state == e_str_enabled) ||
                           (t->state == e_str_connected));
        t_list->obj = e_widget_check_add(evas, _(t->name), &t_list->enabled);

        ui->technologies = eina_inlist_append
            (ui->technologies, EINA_INLIST_GET(t_list));
        e_widget_framelist_object_append(ui->type_frame, t_list->obj);
     }
}

static void
_switches_page_create(Evas                 *evas,
                      E_Config_Dialog_Data *cfdata)
{
   struct connman_config_switch_ui *ui;
   E_Connman_Module_Context *ctxt = cfdata->ctxt;

   ui = &cfdata->switches;
   ui->vlayout = e_widget_list_add(evas, 0, 0);
   ui->type_frame = e_widget_framelist_add(evas, _("Network types"), 0);
   ui->offline_mode = ctxt->offline_mode;

   _switches_page_create_technologies(evas, ctxt, ui);

   e_widget_list_object_append(ui->vlayout, ui->type_frame, 1, 1, 0.0);
   ui->off_frame = e_widget_framelist_add(evas, _("Disable networking"), 0);
   ui->o_off = e_widget_check_add(evas, _("Offline mode"), &ui->offline_mode);
   e_widget_framelist_object_append(ui->off_frame, ui->o_off);
   e_widget_list_object_append(ui->vlayout, ui->off_frame, 1, 1, 0.0);
}

static Evas_Object *
_basic_create(E_Config_Dialog *dialog __UNUSED__,
              Evas                   *evas,
              E_Config_Dialog_Data   *cfdata)
{
   Evas_Object *otb;

   otb = e_widget_toolbook_add(evas, 48 * e_scale, 48 * e_scale);

   _networks_page_create(evas, cfdata);
   e_widget_toolbook_page_append
     (otb, NULL, _("Networks Settings"),
     cfdata->networks.hlayout, 1, 1, 1, 1, 0.5, 0.0);
   _switches_page_create(evas, cfdata);
   e_widget_toolbook_page_append
     (otb, NULL, _("Network Switches"),
     cfdata->switches.vlayout, 1, 1, 0, 0, 0.5, 0.5);

   _networks_list_fill(evas, cfdata);
   e_widget_toolbook_page_show(otb, 0);
   e_widget_size_min_resize(otb);

   return otb;
}

static int
_basic_apply(E_Config_Dialog *dialog __UNUSED__,
             E_Config_Dialog_Data   *cfdata)
{
   E_Connman_Module_Context *ctxt = cfdata->ctxt;
   struct connman_config_switch_ui *sw = &cfdata->switches;
   struct connman_config_technologies *t;

   EINA_INLIST_FOREACH(sw->technologies, t)
     {
        int was_enabled = ((t->technology->state == e_str_enabled) ||
                           (t->technology->state == e_str_connected));
        if (t->enabled != was_enabled)
          _connman_technology_onoff(ctxt, t->technology->type, t->enabled);
     }
   if (ctxt->offline_mode != sw->offline_mode)
     _connman_toggle_offline_mode(ctxt);

   return 1;
}

