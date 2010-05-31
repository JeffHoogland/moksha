#include "e.h"

/**
 * TODO: 
 * Maybe add buttons for Add/Remove of acpi bindings ??
 * Maybe use a toolbook widget instead of 2 lists ??
 * Maybe add a way to restore default acpi bindings ??
 */

/* local config structure */
struct _E_Config_Dialog_Data 
{
   Eina_List *bindings;
   Evas_Object *o_bindings, *o_actions, *o_params;
   const char *bindex;
};

/* local function prototypes */
static void *_create_data(E_Config_Dialog *cfd);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _fill_bindings(E_Config_Dialog_Data *cfdata);
static void _fill_actions(E_Config_Dialog_Data *cfdata);
static E_Config_Binding_Acpi *_selected_binding_get(E_Config_Dialog_Data *cfdata);
static E_Action_Description *_selected_action_get(E_Config_Dialog_Data *cfdata);
static const char *_binding_get_label(E_Config_Binding_Acpi *bind);
static void _cb_bindings_changed(void *data);
static void _cb_actions_changed(void *data);
static void _cb_entry_changed(void *data, void *data2 __UNUSED__);

E_Config_Dialog *
e_int_config_acpibindings(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "advanced/acpi_bindings")) 
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
   return cfdata;
}

static void 
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   Eina_List *l;
   E_Config_Binding_Acpi *bind;

   EINA_LIST_FOREACH(e_config->acpi_bindings, l, bind) 
     {
	E_Config_Binding_Acpi *b2;

	b2 = E_NEW(E_Config_Binding_Acpi, 1);
	b2->context = bind->context;
	b2->type = bind->type;
	b2->status = bind->status;
	b2->action = eina_stringshare_ref(bind->action);
	b2->params = eina_stringshare_ref(bind->params);
	cfdata->bindings = eina_list_append(cfdata->bindings, b2);
     }
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_Config_Binding_Acpi *bind;

   EINA_LIST_FREE(cfdata->bindings, bind) 
     {
	if (bind->action) eina_stringshare_del(bind->action);
	if (bind->params) eina_stringshare_del(bind->params);
	E_FREE(bind);
     }

   E_FREE(cfdata);
}

static int 
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_Config_Binding_Acpi *bind, *b2;
   Eina_List *l;

   EINA_LIST_FREE(e_config->acpi_bindings, bind) 
     {
	e_bindings_acpi_del(bind->context, bind->type, bind->status, 
			    bind->action, bind->params);
	if (bind->action) eina_stringshare_del(bind->action);
	if (bind->params) eina_stringshare_del(bind->params);
	E_FREE(bind);
     }

   EINA_LIST_FOREACH(cfdata->bindings, l, bind) 
     {
	b2 = E_NEW(E_Config_Binding_Acpi, 1);
	b2->context = bind->context;
	b2->type = bind->type;
	b2->status = bind->status;
	b2->action = eina_stringshare_ref(bind->action);
	b2->params = eina_stringshare_ref(bind->params);
	e_config->acpi_bindings = 
	  eina_list_append(e_config->acpi_bindings, b2);

	e_bindings_acpi_add(b2->context, b2->type, b2->status, 
			    b2->action, b2->params);
     }
   e_config_save_queue();

   return 1;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *ol, *of, *ow, *ot;

   ol = e_widget_list_add(evas, 0, 1);

   of = e_widget_framelist_add(evas, _("ACPI Bindings"), 0);
   ow = e_widget_ilist_add(evas, (24 * e_scale), (24 * e_scale), 
			   &(cfdata->bindex));
   cfdata->o_bindings = ow;
   _fill_bindings(cfdata);
   e_widget_framelist_object_append(of, ow);
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
   E_Config_Binding_Acpi *bind;
   int i = -1, mw;

   evas = evas_object_evas_get(cfdata->o_bindings);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_bindings);
   e_widget_ilist_clear(cfdata->o_bindings);

   EINA_LIST_FOREACH(cfdata->bindings, l, bind) 
     {
	const char *lbl;
	char buff[32];

	i++;
	if (!(lbl = _binding_get_label(bind))) continue;
	snprintf(buff, sizeof(buff), "%d", i);
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
	if ((strcmp(grp->act_grp, "Acpi")) && 
	    (strcmp(grp->act_grp, "System")) && 
	    (strcmp(grp->act_grp, "Launch"))) continue;
	e_widget_ilist_header_append(cfdata->o_actions, NULL, grp->act_grp);
	EINA_LIST_FOREACH(grp->acts, ll, dsc) 
	  e_widget_ilist_append(cfdata->o_actions, NULL, dsc->act_name, 
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
   E_Config_Binding_Acpi *bind;

   if (!cfdata) return NULL;
   if (!(bind = eina_list_nth(cfdata->bindings, atoi(cfdata->bindex)))) 
     return NULL;
   return bind;
}

static E_Action_Description *
_selected_action_get(E_Config_Dialog_Data *cfdata) 
{
   E_Action_Group *grp;
   E_Action_Description *dsc;
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
	if ((strcmp(grp->act_grp, "Acpi")) && 
	    (strcmp(grp->act_grp, "System")) && 
	    (strcmp(grp->act_grp, "Launch"))) continue;
	EINA_LIST_FOREACH(grp->acts, ll, dsc) 
	  {
	     if ((dsc->act_name) && (!strcmp(dsc->act_name, lbl))) 
	       return dsc;
	  }
     }

   return dsc;
}

static const char *
_binding_get_label(E_Config_Binding_Acpi *bind) 
{
   char *ret;

   if (bind->type == E_ACPI_TYPE_UNKNOWN) return NULL;
   else if (bind->type == E_ACPI_TYPE_AC_ADAPTER) 
     {
	ret = _("Ac Adapter");
	if (bind->status == 0) ret = _("AC Adapter Unplugged");
	else if (bind->status == 1) ret = _("AC Adapter Plugged");
     }
   else if (bind->type == E_ACPI_TYPE_BATTERY) ret = _("Battery");
   else if (bind->type == E_ACPI_TYPE_BUTTON) ret = _("Button");
   else if (bind->type == E_ACPI_TYPE_FAN) ret = _("Fan");
   else if (bind->type == E_ACPI_TYPE_LID) 
     {
	ret = _("Lid");
	if (bind->status == 0) ret = _("Lid Closed");
	else if (bind->status == 1) ret = _("Lid Opened");
     }
   else if (bind->type == E_ACPI_TYPE_POWER) ret = _("Power Button");
   else if (bind->type == E_ACPI_TYPE_PROCESSOR) ret = _("Processor");
   else if (bind->type == E_ACPI_TYPE_SLEEP) ret = _("Sleep Button");
   else if (bind->type == E_ACPI_TYPE_THERMAL) ret = _("Thermal");
   else if (bind->type == E_ACPI_TYPE_VIDEO) ret = _("Video");
   else if (bind->type == E_ACPI_TYPE_WIFI) ret = _("Wifi");

   return ret;
}

static void 
_cb_bindings_changed(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Acpi *bind;
   Eina_List *items;
   const E_Ilist_Item *item;
   int i = -1;

   if (!(cfdata = data)) return;
   if (!(bind = _selected_binding_get(cfdata))) 
     {
	e_widget_entry_clear(cfdata->o_params);
	e_widget_disabled_set(cfdata->o_params, EINA_TRUE);
	return;
     }

   e_widget_ilist_unselect(cfdata->o_actions);
   EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->o_actions), items, item) 
     {
	const char *val;

	i++;
	if (!(val = e_widget_ilist_item_value_get(item))) continue;
	if (strcmp(val, bind->action)) continue;
	e_widget_ilist_selected_set(cfdata->o_actions, i);
	break;
     }
}

static void 
_cb_actions_changed(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Acpi *bind;
   E_Action_Description *dsc;

   if (!(cfdata = data)) return;
   if (!(bind = _selected_binding_get(cfdata))) 
     {
	e_widget_entry_clear(cfdata->o_params);
	e_widget_disabled_set(cfdata->o_params, EINA_TRUE);
	return;
     }
   if (!(dsc = _selected_action_get(cfdata))) 
     {
	e_widget_entry_clear(cfdata->o_params);
	e_widget_disabled_set(cfdata->o_params, EINA_TRUE);
	return;
     }

   eina_stringshare_replace(&bind->action, dsc->act_cmd);
   e_widget_disabled_set(cfdata->o_params, !(dsc->editable));

   if ((!dsc->editable) && (dsc->act_params))
     e_widget_entry_text_set(cfdata->o_params, dsc->act_params); 
   else if (bind->params)
     e_widget_entry_text_set(cfdata->o_params, bind->params); 
   else 
     {
	if ((!dsc->param_example) || (!dsc->param_example[0]))
	  e_widget_entry_text_set(cfdata->o_params, _("<None>"));
	else
	  e_widget_entry_text_set(cfdata->o_params, dsc->param_example);
     }
}

static void 
_cb_entry_changed(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Acpi *bind;
   E_Action_Description *dsc;

   if (!(cfdata = data)) return;
   if (!(dsc = _selected_action_get(cfdata))) return;
   if (!dsc->editable) return;
   if (!(bind = _selected_binding_get(cfdata))) return;
   eina_stringshare_replace(&bind->params, 
			    e_widget_entry_text_get(cfdata->o_params));
}
