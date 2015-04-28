#include "e.h"

static void *_create_data(E_Config_Dialog *cfd);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _cb_composite_change(void *data, Evas_Object *obj);
static void _cb_confirm_yes(void *data);
static void _cb_confirm_no(void *data);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;

   int use_composite;
   Evas_Object *o_composite;
};

E_Config_Dialog *
e_int_config_engine(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "advanced/engine")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->basic.check_changed = _basic_check_changed;

   cfd = e_config_dialog_new(con, _("Engine Settings"), "E", "advanced/engine",
			     "preferences-engine", 0, v, NULL);
   return cfd;
}

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
   cfdata->use_composite = e_config->use_composite;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   e_config->use_composite = cfdata->use_composite;
   e_config_save_queue();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   return !(cfdata->use_composite == e_config->use_composite);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ob, *of;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ob = e_widget_check_add(evas, _("Use ARGB instead of shaped windows"),
			   &(cfdata->use_composite));
   cfdata->o_composite = ob;
   e_widget_on_change_hook_set(ob, _cb_composite_change, cfdata);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 0, 0.5);

   return o;
}

static void
_cb_composite_change(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = NULL;

   if (!(cfdata = data)) return;
   if (cfdata->use_composite)
     {
	if (!ecore_x_screen_is_composited(0))
	  {
	     /* pop dialog */
	     e_confirm_dialog_show(_("Use ARGB instead of shaped windows"),
				   "preferences-engine",
				   _("You have chosen to enable ARGB composite "
				     "support,<br>but your current screen does "
				     "not support composite.<br><br>"
				     "Are you sure you wish to enable ARGB support?"),
				   _("Enable"), NULL, _cb_confirm_yes, _cb_confirm_no,
				   cfdata, cfdata, NULL, NULL);
	  }
     }
}

static void
_cb_confirm_yes(void *data)
{
   E_Config_Dialog_Data *cfdata = NULL;

   if (!(cfdata = data)) return;
   cfdata->use_composite = 1;
}

static void
_cb_confirm_no(void *data)
{
   E_Config_Dialog_Data *cfdata = NULL;

   if (!(cfdata = data)) return;
   cfdata->use_composite = 0;
   e_widget_check_checked_set(cfdata->o_composite, 0);
}
