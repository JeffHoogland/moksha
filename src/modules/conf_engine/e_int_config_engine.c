#include "e.h"

EAPI E_Config_Dialog *e_int_config_engine(E_Container *con);
static void *_create_data(E_Config_Dialog *cfd);
static void  _fill_data(E_Config_Dialog_Data *cfdata);
static void  _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int   _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);


struct _E_Config_Dialog_Data 
{
   E_Config_Dialog *cfd;

   int evas_engine_default;
};

EAPI E_Config_Dialog *
e_int_config_engine(E_Container *con) 
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

   cfd = e_config_dialog_new(con,
			     _("Engine Settings"),
			    "E", "_config_engine_dialog",
			     "enlightenment/engine", 0, v, NULL);
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
   e_config->evas_engine_default = cfdata->evas_engine_default;
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *ob, *of;
   E_Radio_Group *rg;
   Evas_List *l;
   int engine;

   o = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_framelist_add(evas, _("Default Engine"), 0);
   rg = e_widget_radio_group_new(&(cfdata->evas_engine_default));
   for (l = e_config_engine_list(); l; l = l->next)
     {
        if (!strcmp("SOFTWARE", l->data)) engine = E_EVAS_ENGINE_SOFTWARE_X11;
        else if (!strcmp("GL", l->data)) engine = E_EVAS_ENGINE_GL_X11;
        else if (!strcmp("XRENDER", l->data)) engine = E_EVAS_ENGINE_XRENDER_X11;
		else continue;
        ob = e_widget_radio_add(evas, _(l->data), engine, rg);
        e_widget_framelist_object_append(of, ob);
     }
   e_widget_list_object_append(o, of, 1, 1, 0.5);   

   e_dialog_resizable_set(cfd->dia, 0);
   return o;
}
