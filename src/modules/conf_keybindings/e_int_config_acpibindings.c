#include "e.h"

/**
 * TODO:
 * Maybe use a toolbook widget instead of 2 lists ??
 * Maybe add a way to restore default acpi bindings ??
 */

/* local config structure */
struct _E_Config_Dialog_Data
{
   Eina_List       *bindings;
   Evas_Object     *o_bindings, *o_actions, *o_params;
   Evas_Object     *o_add, *o_del;
   const char      *bindex;

   E_Config_Dialog *cfd;
};

/* local function prototypes */
static void                  *_create_data(E_Config_Dialog *cfd);
static void                   _fill_data(E_Config_Dialog_Data *cfdata);
static void                   _free_data(E_Config_Dialog *cfd  __UNUSED__,
                                         E_Config_Dialog_Data *cfdata);
static int                    _basic_apply(E_Config_Dialog *cfd  __UNUSED__,
                                           E_Config_Dialog_Data *cfdata);
static Evas_Object           *_basic_create(E_Config_Dialog *cfd,
                                            Evas *evas,
                                            E_Config_Dialog_Data *cfdata);
static void                   _fill_bindings(E_Config_Dialog_Data *cfdata);
static void                   _fill_actions(E_Config_Dialog_Data *cfdata);
static E_Config_Binding_Acpi *_selected_binding_get(E_Config_Dialog_Data *cfdata);
static E_Action_Description  *_selected_action_get(E_Config_Dialog_Data *cfdata);
static const char            *_binding_label_get(E_Config_Binding_Acpi *bind);
static void                   _cb_bindings_changed(void *data);
static void                   _cb_actions_changed(void *data);
static void                   _cb_entry_changed(void *data,
                                                void *data2 __UNUSED__);
static void                   _cb_add_binding(void *data,
                                              void *data2 __UNUSED__);
static void                   _cb_del_binding(void *data,
                                              void *data2 __UNUSED__);
static Eina_Bool              _cb_grab_key_down(void *data,
                                                int type __UNUSED__,
                                                void *event);
static Eina_Bool              _cb_acpi_event(void *data,
                                             int type,
                                             void *event);

/* local variables */
static E_Dialog *grab_dlg = NULL;
static Ecore_X_Window grab_win = 0;
static Eina_List *grab_hdls = NULL;

E_Config_Dialog *
e_int_config_acpibindings(E_Container *con,
                          const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if ((e_config_dialog_find("E", "advanced/acpi_bindings")))
     return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;

   cfd = e_config_dialog_new(con, _("ACPI Bindings"), "E",
                             "advanced/acpi_bindings",
                             "preferences-system-power-management",
                             0, v, NULL);

   return cfd;
}

/* local functions */
static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   cfdata->cfd = cfd;
   return cfdata;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   Eina_List *l;
   E_Config_Binding_Acpi *binding;

   EINA_LIST_FOREACH(e_config->acpi_bindings, l, binding)
     {
        E_Config_Binding_Acpi *b2;

        b2 = E_NEW(E_Config_Binding_Acpi, 1);
        b2->context = binding->context;
        b2->type = binding->type;
        b2->status = binding->status;
        b2->action = eina_stringshare_ref(binding->action);
        b2->params = eina_stringshare_ref(binding->params);
        cfdata->bindings = eina_list_append(cfdata->bindings, b2);
     }
}

static void
_free_data(E_Config_Dialog *cfd  __UNUSED__,
           E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Acpi *binding;
   Ecore_Event_Handler *hdl;

   EINA_LIST_FREE(cfdata->bindings, binding)
     {
        if (binding->action) eina_stringshare_del(binding->action);
        if (binding->params) eina_stringshare_del(binding->params);
        E_FREE(binding);
     }

   /* free the handlers */
   EINA_LIST_FREE(grab_hdls, hdl)
     ecore_event_handler_del(hdl);

   if (grab_win)
     {
        e_grabinput_release(grab_win, grab_win);
        ecore_x_window_free(grab_win);
     }
   grab_win = 0;

   if (grab_dlg)
     {
        e_object_del(E_OBJECT(grab_dlg));
        e_acpi_events_thaw();
     }
   grab_dlg = NULL;

   E_FREE(cfdata);
}

static int
_basic_apply(E_Config_Dialog *cfd  __UNUSED__,
             E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Acpi *binding, *b2;
   Eina_List *l;

   EINA_LIST_FREE(e_config->acpi_bindings, binding)
     {
        e_bindings_acpi_del(binding->context, binding->type, binding->status,
                            binding->action, binding->params);
        if (binding->action) eina_stringshare_del(binding->action);
        if (binding->params) eina_stringshare_del(binding->params);
        E_FREE(binding);
     }

   EINA_LIST_FOREACH(cfdata->bindings, l, binding)
     {
        b2 = E_NEW(E_Config_Binding_Acpi, 1);
        b2->context = binding->context;
        b2->type = binding->type;
        b2->status = binding->status;
        b2->action = eina_stringshare_ref(binding->action);
        b2->params = eina_stringshare_ref(binding->params);
        e_config->acpi_bindings =
          eina_list_append(e_config->acpi_bindings, b2);

        e_bindings_acpi_add(b2->context, b2->type, b2->status,
                            b2->action, b2->params);
     }
   e_config_save_queue();

   return 1;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd,
              Evas *evas,
              E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ol, *of, *ow, *ot;

   ol = e_widget_list_add(evas, 0, 1);

   of = e_widget_frametable_add(evas, _("ACPI Bindings"), 0);
   ow = e_widget_ilist_add(evas, (24 * e_scale), (24 * e_scale),
                           &(cfdata->bindex));
   cfdata->o_bindings = ow;
   _fill_bindings(cfdata);
   e_widget_frametable_object_append(of, ow, 0, 0, 2, 1, 1, 1, 1, 1);

   ow = e_widget_button_add(evas, _("Add"), "list-add",
                            _cb_add_binding, cfdata, NULL);
   cfdata->o_add = ow;
   e_widget_frametable_object_append(of, ow, 0, 1, 1, 1, 1, 0, 1, 0);
   ow = e_widget_button_add(evas, _("Delete"), "list-remove",
                            _cb_del_binding, cfdata, NULL);
   cfdata->o_del = ow;
   e_widget_disabled_set(ow, EINA_TRUE);
   e_widget_frametable_object_append(of, ow, 1, 1, 1, 1, 1, 0, 1, 0);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);

   ot = e_widget_table_add(evas, 0);
   of = e_widget_framelist_add(evas, _("Action"), 0);
   ow = e_widget_ilist_add(evas, (24 * e_scale), (24 * e_scale), NULL);
   cfdata->o_actions = ow;
   _fill_actions(cfdata);
   e_widget_framelist_object_append(of, ow);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);

   ow = e_widget_framelist_add(evas, _("Action Params"), 0);
   cfdata->o_params =
     e_widget_entry_add(evas, NULL, _cb_entry_changed, cfdata, NULL);
   e_widget_disabled_set(cfdata->o_params, EINA_TRUE);
   e_widget_framelist_object_append(ow, cfdata->o_params);
   e_widget_table_object_append(ot, ow, 0, 1, 1, 1, 1, 0, 1, 0);
   e_widget_list_object_append(ol, ot, 1, 1, 0.5);

   e_dialog_resizable_set(cfd->dia, 1);
   return ol;
}

static void
_fill_bindings(E_Config_Dialog_Data *cfdata)
{
   Evas *evas;
   Eina_List *l;
   E_Config_Binding_Acpi *binding;
   int i = -1, mw;

   evas = evas_object_evas_get(cfdata->o_bindings);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_bindings);
   e_widget_ilist_clear(cfdata->o_bindings);

   EINA_LIST_FOREACH(cfdata->bindings, l, binding)
     {
        const char *lbl;
        char buff[32];

        i++;
        snprintf(buff, sizeof(buff), "%d", i);

        lbl = _binding_label_get(binding);

        e_widget_ilist_append(cfdata->o_bindings, NULL, lbl,
                              _cb_bindings_changed, cfdata, buff);
     }

   e_widget_ilist_go(cfdata->o_bindings);
   e_widget_size_min_get(cfdata->o_bindings, &mw, NULL);
   if (mw < (160 * e_scale)) mw = (160 * e_scale);
   e_widget_size_min_set(cfdata->o_bindings, mw, 200);
   e_widget_ilist_thaw(cfdata->o_bindings);
   edje_thaw();
   evas_event_thaw(evas);
}

static void
_fill_actions(E_Config_Dialog_Data *cfdata)
{
   Evas *evas;
   Eina_List *l, *ll;
   E_Action_Group *grp;
   E_Action_Description *dsc;
   int mw;

   evas = evas_object_evas_get(cfdata->o_actions);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_actions);
   e_widget_ilist_clear(cfdata->o_actions);

   EINA_LIST_FOREACH(e_action_groups_get(), l, grp)
     {
        if (!grp->acts) continue;
//        if ((strcmp(grp->act_grp, "Acpi")) &&
//            (strcmp(grp->act_grp, "System")) &&
//            (strcmp(grp->act_grp, "Launch"))) continue;
        e_widget_ilist_header_append(cfdata->o_actions, NULL, _(grp->act_grp));
        EINA_LIST_FOREACH(grp->acts, ll, dsc)
          e_widget_ilist_append(cfdata->o_actions, NULL, _(dsc->act_name),
                                _cb_actions_changed, cfdata, dsc->act_cmd);
     }

   e_widget_ilist_go(cfdata->o_actions);
   e_widget_size_min_get(cfdata->o_actions, &mw, NULL);
   if (mw < (160 * e_scale)) mw = (160 * e_scale);
   e_widget_size_min_set(cfdata->o_actions, mw, 200);
   e_widget_ilist_thaw(cfdata->o_actions);
   edje_thaw();
   evas_event_thaw(evas);
}

static E_Config_Binding_Acpi *
_selected_binding_get(E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Acpi *binding;

   if ((!cfdata) || (!cfdata->bindex)) return NULL;
   if (!(binding = eina_list_nth(cfdata->bindings, atoi(cfdata->bindex))))
     return NULL;
   return binding;
}

static E_Action_Description *
_selected_action_get(E_Config_Dialog_Data *cfdata)
{
   E_Action_Group *grp;
   E_Action_Description *dsc = NULL;
   Eina_List *l, *ll;
   const char *lbl;
   int sel;

   if (!cfdata) return NULL;
   sel = e_widget_ilist_selected_get(cfdata->o_actions);
   if (sel < 0) return NULL;
   if (!(lbl = e_widget_ilist_nth_label_get(cfdata->o_actions, sel)))
     return NULL;

   EINA_LIST_FOREACH(e_action_groups_get(), l, grp)
     {
        if (!grp->acts) continue;
//        if ((strcmp(grp->act_grp, "Acpi")) &&
//            (strcmp(grp->act_grp, "System")) &&
//            (strcmp(grp->act_grp, "Launch"))) continue;
        EINA_LIST_FOREACH(grp->acts, ll, dsc)
          {
             if ((dsc->act_name) && (!strcmp(_(dsc->act_name), lbl)))
               return dsc;
          }
     }

   return NULL;
}

static const char *
_binding_label_get(E_Config_Binding_Acpi *binding)
{
   if (binding->type == E_ACPI_TYPE_UNKNOWN) return NULL;
   if (binding->type == E_ACPI_TYPE_AC_ADAPTER)
     {
        if (binding->status == 0) return _("AC Adapter Unplugged");
        if (binding->status == 1) return _("AC Adapter Plugged");
        return _("Ac Adapter");
     }
   if (binding->type == E_ACPI_TYPE_BATTERY)
     return _("Battery");
   if (binding->type == E_ACPI_TYPE_BUTTON)
     return _("Button");
   if (binding->type == E_ACPI_TYPE_FAN)
     return _("Fan");
   if (binding->type == E_ACPI_TYPE_LID)
     {
        if (binding->status == E_ACPI_LID_UNKNOWN) return _("Lid Unknown");
        if (binding->status == E_ACPI_LID_CLOSED) return _("Lid Closed");
        if (binding->status == E_ACPI_LID_OPEN) return _("Lid Opened");
        return _("Lid");
     }
   if (binding->type == E_ACPI_TYPE_POWER)
     return _("Power Button");
   if (binding->type == E_ACPI_TYPE_PROCESSOR)
     return _("Processor");
   if (binding->type == E_ACPI_TYPE_SLEEP)
     return _("Sleep Button");
   if (binding->type == E_ACPI_TYPE_THERMAL)
     return _("Thermal");
   if (binding->type == E_ACPI_TYPE_VIDEO)
     return _("Video");
   if (binding->type == E_ACPI_TYPE_WIFI)
     return _("Wifi");
   if (binding->type == E_ACPI_TYPE_HIBERNATE)
     return _("Hibernate");
   if (binding->type == E_ACPI_TYPE_ZOOM_OUT)
     return _("Zoom Out");
   if (binding->type == E_ACPI_TYPE_ZOOM_IN)
     return _("Zoom In");
   if (binding->type == E_ACPI_TYPE_BRIGHTNESS_DOWN)
     return _("Brightness Down");
   if (binding->type == E_ACPI_TYPE_BRIGHTNESS_UP)
     return _("Brightness Up");
   if (binding->type == E_ACPI_TYPE_ASSIST)
     return _("Assist");
   if (binding->type == E_ACPI_TYPE_S1)
     return _("S1");
   if (binding->type == E_ACPI_TYPE_VAIO)
     return _("Vaio");

   return _("Unknown");
}

static void
_cb_bindings_changed(void *data)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Acpi *binding;
   const Eina_List *items;
   const E_Ilist_Item *item;
   int i = -1;

   if (!(cfdata = data)) return;
   e_widget_entry_clear(cfdata->o_params);
   if (!(binding = _selected_binding_get(cfdata)))
     {
        e_widget_disabled_set(cfdata->o_params, EINA_TRUE);
        e_widget_disabled_set(cfdata->o_del, EINA_TRUE);
        return;
     }

   e_widget_disabled_set(cfdata->o_del, EINA_FALSE);
   e_widget_ilist_unselect(cfdata->o_actions);

   EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->o_actions), items, item)
     {
        const char *val;

        i++;
        if (!(val = e_widget_ilist_item_value_get(item))) continue;
        if (strcmp(val, binding->action)) continue;
        e_widget_ilist_selected_set(cfdata->o_actions, i);
        break;
     }
}

static void
_cb_actions_changed(void *data)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Acpi *binding;
   E_Action_Description *dsc;

   if (!(cfdata = data)) return;
   e_widget_entry_clear(cfdata->o_params);
   if (!(binding = _selected_binding_get(cfdata)))
     {
        e_widget_disabled_set(cfdata->o_params, EINA_TRUE);
        e_widget_disabled_set(cfdata->o_del, EINA_TRUE);
        return;
     }
   if (!(dsc = _selected_action_get(cfdata)))
     {
        e_widget_disabled_set(cfdata->o_params, EINA_TRUE);
        return;
     }

   eina_stringshare_replace(&binding->action, dsc->act_cmd);
   e_widget_disabled_set(cfdata->o_params, !(dsc->editable));

   if ((!dsc->editable) && (dsc->act_params))
     e_widget_entry_text_set(cfdata->o_params, dsc->act_params);
   else if (binding->params)
     e_widget_entry_text_set(cfdata->o_params, binding->params);
   else
     {
        if ((!dsc->param_example) || (!dsc->param_example[0]))
          e_widget_entry_text_set(cfdata->o_params, _("<None>"));
        else
          e_widget_entry_text_set(cfdata->o_params, dsc->param_example);
     }
}

static void
_cb_entry_changed(void *data,
                  void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Acpi *binding;
   E_Action_Description *dsc;

   if (!(cfdata = data)) return;
   if (!(dsc = _selected_action_get(cfdata))) return;
   if (!dsc->editable) return;
   if (!(binding = _selected_binding_get(cfdata))) return;
   eina_stringshare_replace(&binding->params,
                            e_widget_entry_text_get(cfdata->o_params));
}

static void
_cb_add_binding(void *data,
                void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   E_Manager *man;

   if (grab_win != 0) return;
   if (!(cfdata = data)) return;
   man = e_manager_current_get();
   grab_dlg = e_dialog_new(e_container_current_get(man), "E",
                           "_acpibind_getbind_dialog");
   if (!grab_dlg) return;
   e_dialog_title_set(grab_dlg, _("ACPI Binding"));
   e_dialog_icon_set(grab_dlg, "preferences-system-power-management", 48);
   e_dialog_text_set(grab_dlg,
                     _("Please trigger the ACPI event you wish to bind to, "
                       "<br><br>or <hilight>Escape</hilight> to abort."));
   e_win_centered_set(grab_dlg->win, EINA_TRUE);
   e_win_borderless_set(grab_dlg->win, EINA_TRUE);

   grab_win = ecore_x_window_input_new(man->root, 0, 0, 1, 1);
   ecore_x_window_show(grab_win);
   e_grabinput_get(grab_win, 0, grab_win);

   grab_hdls =
     eina_list_append(grab_hdls,
                      ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                              _cb_grab_key_down, cfdata));
   grab_hdls =
     eina_list_append(grab_hdls,
                      ecore_event_handler_add(E_EVENT_ACPI,
                                              _cb_acpi_event, cfdata));

   /* freeze all incoming acpi events */
   e_acpi_events_freeze();

   e_dialog_show(grab_dlg);
   ecore_x_icccm_transient_for_set(grab_dlg->win->evas_win,
                                   cfdata->cfd->dia->win->evas_win);
}

static void
_cb_del_binding(void *data,
                void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Acpi *binding, *bind2;
   Eina_List *l;

   if (!(cfdata = data)) return;
   if (!(binding = _selected_binding_get(cfdata))) return;

   /* delete from e_config */
   EINA_LIST_FOREACH(e_config->acpi_bindings, l, bind2)
     {
        if ((binding->context == bind2->context) && (binding->type == bind2->type) &&
            (((binding->action) && (bind2->action) &&
              (!strcmp(binding->action, bind2->action))) ||
             ((!binding->action) && (!bind2->action))) &&
            (((binding->params) && (bind2->params) &&
              (!strcmp(binding->params, bind2->params))) ||
             ((!binding->params) && (!bind2->params))))
          {
             if (bind2->action) eina_stringshare_del(bind2->action);
             if (bind2->params) eina_stringshare_del(bind2->params);
             E_FREE(bind2);
             e_config->acpi_bindings =
               eina_list_remove_list(e_config->acpi_bindings, l);
             e_config_save_queue();
             break;
          }
     }

   /* delete from e_bindings */
   e_bindings_acpi_del(binding->context, binding->type, binding->status,
                       binding->action, binding->params);

   /* delete from dialog list */
   EINA_LIST_FOREACH(cfdata->bindings, l, bind2)
     {
        if ((binding->context == bind2->context) && (binding->type == bind2->type) &&
            (((binding->action) && (bind2->action) &&
              (!strcmp(binding->action, bind2->action))) ||
             ((!binding->action) && (!bind2->action))) &&
            (((binding->params) && (bind2->params) &&
              (!strcmp(binding->params, bind2->params))) ||
             ((!binding->params) && (!bind2->params))))
          {
             if (bind2->action) eina_stringshare_del(bind2->action);
             if (bind2->params) eina_stringshare_del(bind2->params);
             E_FREE(bind2);
             cfdata->bindings = eina_list_remove_list(cfdata->bindings, l);
             break;
          }
     }

   /* reset gui */
   e_widget_entry_clear(cfdata->o_params);
   e_widget_disabled_set(cfdata->o_params, EINA_TRUE);
   e_widget_ilist_unselect(cfdata->o_actions);
   e_widget_disabled_set(cfdata->o_del, EINA_TRUE);
   _fill_bindings(cfdata);
}

static Eina_Bool
_cb_grab_key_down(void *data,
                  int type __UNUSED__,
                  void *event)
{
   E_Config_Dialog_Data *cfdata;
   Ecore_Event_Key *ev;

   ev = event;
   if (ev->window != grab_win) return ECORE_CALLBACK_PASS_ON;
   if (!(cfdata = data)) return ECORE_CALLBACK_PASS_ON;
   if (!strcmp(ev->key, "Escape"))
     {
        Ecore_Event_Handler *hdl;

        /* free the handlers */
        EINA_LIST_FREE(grab_hdls, hdl)
          ecore_event_handler_del(hdl);

        /* kill the dialog window */
        e_grabinput_release(grab_win, grab_win);
        ecore_x_window_free(grab_win);
        grab_win = 0;
        e_object_del(E_OBJECT(grab_dlg));
        grab_dlg = NULL;

        /* unfreeze acpi events */
        e_acpi_events_thaw();
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_acpi_event(void *data,
               __UNUSED__ int type,
               void *event)
{
   E_Event_Acpi *ev;
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Acpi *binding;
   Ecore_Event_Handler *hdl;

   ev = event;
   if (!(cfdata = data)) return ECORE_CALLBACK_PASS_ON;

   /* free the handlers */
   EINA_LIST_FREE(grab_hdls, hdl)
     ecore_event_handler_del(hdl);

   /* kill the dialog window */
   e_grabinput_release(grab_win, grab_win);
   ecore_x_window_free(grab_win);
   grab_win = 0;
   e_object_del(E_OBJECT(grab_dlg));
   grab_dlg = NULL;

   /* unfreeze acpi events */
   e_acpi_events_thaw();

   /* NB: This may need more testing/parsing for event status */
   binding = E_NEW(E_Config_Binding_Acpi, 1);
   binding->context = E_BINDING_CONTEXT_NONE;
   binding->type = ev->type;
   binding->status = ev->status;
   binding->action = eina_stringshare_add("dim_screen");
   binding->params = NULL;

   cfdata->bindings = eina_list_append(cfdata->bindings, binding);
   _fill_bindings(cfdata);
   return ECORE_CALLBACK_DONE;
}

