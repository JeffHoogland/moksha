#include "e.h"

static void *_create_data(E_Config_Dialog *cfd);
static void  _fill_data(E_Config_Dialog_Data *cfdata);
static void  _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int   _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int   _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _cb_composite_change(void *data, Evas_Object *obj);
static void _cb_confirm_yes(void *data);
static void _cb_confirm_no(void *data);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;

   int use_composite;
   int evas_engine_default;
   Evas_Object *o_composite;
};

EAPI E_Config_Dialog *
e_int_config_engine(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_engine_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->basic.check_changed = _basic_check_changed;

   cfd = e_config_dialog_new(con,
			     _("Engine Settings"),
			    "E", "_config_engine_dialog",
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
   cfdata->evas_engine_default = e_config->evas_engine_default;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   e_config->use_composite = cfdata->use_composite;
   e_config->evas_engine_default = cfdata->evas_engine_default;
   e_config_save_queue();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   return !((cfdata->use_composite == e_config->use_composite) &&
	    (cfdata->evas_engine_default == e_config->evas_engine_default));
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ob, *of;
   E_Radio_Group *rg;
   Eina_List *l;
   int engine;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ob = e_widget_check_add(evas, _("Enable Composite"),
			   &(cfdata->use_composite));
   cfdata->o_composite = ob;
   e_widget_on_change_hook_set(ob, _cb_composite_change, cfdata);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Default Engine"), 0);
   rg = e_widget_radio_group_new(&(cfdata->evas_engine_default));
   for (l = e_config_engine_list(); l; l = l->next)
     {
        if (!strcmp("SOFTWARE", l->data)) engine = E_EVAS_ENGINE_SOFTWARE_X11;
        else if (!strcmp("GL", l->data)) engine = E_EVAS_ENGINE_GL_X11;
        else if (!strcmp("XRENDER", l->data)) engine = E_EVAS_ENGINE_XRENDER_X11;
        else if (!strcmp("SOFTWARE_16", l->data)) engine = E_EVAS_ENGINE_SOFTWARE_X11_16;
	else continue;
        ob = e_widget_radio_add(evas, _(l->data), engine, rg);
        e_widget_framelist_object_append(of, ob);
     }
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   e_dialog_resizable_set(cfd->dia, 0);
   return o;
}

static void
_cb_composite_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata = NULL;

   if (!(cfdata = data)) return;
   if (cfdata->use_composite)
     {
	if (!ecore_x_screen_is_composited(0))
	  {
	     /* pop dialog */
	     e_confirm_dialog_show(_("Enable Composite Support ?"),
				   "preferences-engine",
				   _("You have chosen to enable composite "
				     "support,<br>but your current screen does "
				     "not support composite.<br><br>"
				     "Are you sure you wish to enable composite support ?"),
				   NULL, NULL, _cb_confirm_yes, _cb_confirm_no,
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
