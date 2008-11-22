#include "e_mod_main.h"

extern const char _Name[];

struct _E_Config_Dialog_Data
{
   int default_instance;
   struct mixer_config_ui
     {
        Evas_Object *list;
        struct mixer_config_ui_general
	  {
	     Evas_Object *frame;
             E_Radio_Group *radio;
	  } general;
     } ui;
};

static int
_find_default_instance_index(E_Mixer_Module_Context *ctxt)
{
   Eina_List *l;
   int i;

   for (i = 0, l = ctxt->instances; l != NULL; l = l->next, i++)
     if (l->data == ctxt->default_instance)
         return i;

   return 0;
}

static void *
_create_data(E_Config_Dialog *dialog)
{
   E_Config_Dialog_Data *cfdata;
   E_Mixer_Module_Context *ctxt;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata)
     return NULL;

   ctxt = dialog->data;
   cfdata->default_instance = _find_default_instance_index(ctxt);

   return cfdata;
}

static void
_free_data(E_Config_Dialog *dialog, E_Config_Dialog_Data *cfdata)
{
   E_Mixer_Module_Context *ctxt;

   ctxt = dialog->data;
   if (ctxt)
     ctxt->conf_dialog = NULL;

   E_FREE(cfdata);
}

static int
_basic_apply(E_Config_Dialog *dialog, E_Config_Dialog_Data *cfdata)
{
   E_Mixer_Module_Context *ctxt;

   ctxt = dialog->data;
   ctxt->default_instance = eina_list_nth(ctxt->instances,
					  cfdata->default_instance);
   if (ctxt->default_instance)
     {
	E_Mixer_Module_Config *conf;
	const char *id;

	conf = ctxt->conf;
	if (conf->default_gc_id)
	  eina_stringshare_del(conf->default_gc_id);

	id = ctxt->default_instance->gcc->cf->id;
	conf->default_gc_id = eina_stringshare_add(id);
     }

   return 1;
}

static void
_basic_create_general(E_Config_Dialog *dialog, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   struct mixer_config_ui_general *ui;
   E_Mixer_Module_Context *ctxt;
   Evas_Object *label;
   Eina_List *l;
   int i;

   ui = &cfdata->ui.general;
   ctxt = dialog->data;

   ui->frame = e_widget_framelist_add(evas, _("General Settings"), 0);

   label = e_widget_label_add(evas, _("Mixer to use for global actions:"));
   e_widget_framelist_object_append(ui->frame, label);

   ui->radio = e_widget_radio_group_new(&cfdata->default_instance);
   for (i = 0, l = ctxt->instances; l != NULL; l = l->next, i++)
     {
	E_Mixer_Instance *inst;
	E_Mixer_Gadget_Config *conf;
	Evas_Object *o;
	char name[128];
	char *card_name;

	inst = l->data;
	conf = inst->conf;

	card_name = e_mixer_system_get_card_name(conf->card);
	snprintf(name, sizeof(name), "%s: %s", card_name, conf->channel_name);
	free(card_name);

	o = e_widget_radio_add(evas, name, i, ui->radio);
	e_widget_framelist_object_append(ui->frame, o);
     }

   e_widget_list_object_append(cfdata->ui.list, ui->frame, 1, 1, 0.5);
}

static void
cb_mixer_app_del(E_Dialog *dialog, void *data)
{
   E_Mixer_Module_Context *ctxt = data;
   ctxt->mixer_dialog = NULL;
}

static void
cb_mixer_call(void *data, void *data2)
{
   E_Mixer_Module_Context *ctxt = data;
   E_Container *con;

   if (ctxt->mixer_dialog)
     {
	e_dialog_show(ctxt->mixer_dialog);
	return;
     }

   con = e_container_current_get(e_manager_current_get());
   ctxt->mixer_dialog = e_mixer_app_dialog_new(con, cb_mixer_app_del, ctxt);
}

static void
_basic_create_mixer_call(E_Config_Dialog *dialog, Evas *evas, E_Config_Dialog_Data *cfdata)
{
    Evas_Object *button;

    button = e_widget_button_add(evas, _("Launch mixer..."), NULL,
                                 cb_mixer_call, dialog->data, NULL);
    e_widget_list_object_append(cfdata->ui.list, button, 0, 0, 0.0);
}

static Evas_Object *
_basic_create(E_Config_Dialog *dialog, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   if (!cfdata)
     return NULL;

   cfdata->ui.list = e_widget_list_add(evas, 0, 0);
   _basic_create_general(dialog, evas, cfdata);
   _basic_create_mixer_call(dialog, evas, cfdata);
   return cfdata->ui.list;
}

E_Config_Dialog *
e_mixer_config_module_dialog_new(E_Container *con, E_Mixer_Module_Context *ctxt)
{
   E_Config_Dialog *dialog;
   E_Config_Dialog_View *view;

   if (e_config_dialog_find(_Name, "e_mixer_config_module_dialog_new"))
       return NULL;

   view = E_NEW(E_Config_Dialog_View, 1);
   if (!view)
       return NULL;

   view->create_cfdata = _create_data;
   view->free_cfdata = _free_data;
   view->basic.create_widgets = _basic_create;
   view->basic.apply_cfdata = _basic_apply;

   dialog = e_config_dialog_new(con, _("Mixer Module Settings"),
                                _Name, "e_mixer_config_module_dialog_new",
                                e_mixer_theme_path(), 0, view, ctxt);

   return dialog;
}
