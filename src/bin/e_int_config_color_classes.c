#include "e.h"

static void        *_create_data          (E_Config_Dialog *cfd);
static void         _free_data            (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data     (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _adv_apply_data       (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_adv_create_widgets   (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data 
{
   char *cur_class;
};

EAPI E_Config_Dialog *
e_int_config_color_classes(E_Container *con) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = _adv_apply_data;
   v->advanced.create_widgets = _adv_create_widgets;
   
   cfd = e_config_dialog_new(con, _("Colors"), "E", "_config_color_classes",
			     "enlightenment/themes", 0, v, NULL);
   return cfd;
}

static void 
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   cfdata->cur_class = NULL;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
   Evas_List *l;
   
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Color Classes"), 0);
   ob = e_widget_ilist_add(evas, 16, 16, &(cfdata->cur_class));
   for (l = edje_color_class_list(); l; l = l->next) 
     {
	e_widget_ilist_append(ob, NULL, l->data, NULL, NULL, NULL);
     }
   e_widget_ilist_go(ob);
   e_widget_min_size_set(ob, 100, 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   return o;
}

static int
_adv_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   return 1;
}

static Evas_Object *
_adv_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
   Evas_List *l;
   
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Color Classes"), 0);
   ob = e_widget_ilist_add(evas, 16, 16, &(cfdata->cur_class));
   for (l = edje_color_class_list(); l; l = l->next) 
     {
	e_widget_ilist_append(ob, NULL, l->data, NULL, NULL, NULL);
     }
   e_widget_ilist_go(ob);
   e_widget_min_size_set(ob, 100, 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   e_dialog_resizable_set(cfd->dia, 1);
   return o;
}
