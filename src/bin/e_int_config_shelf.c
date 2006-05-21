#include "e.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void        _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int         _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data 
{
   E_Config_Dialog *cfd;
   Evas_Object *ilist;
};

EAPI E_Config_Dialog *
e_int_config_shelf(E_Container *con) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = NULL;
   v->advanced.create_widgets = NULL;
   
   cfd = e_config_dialog_new(con, _("Shelf Settings"), NULL, 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   free(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   return 1;
}

static void
_cb_list(void *data)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
}

static void
_ilist_fill(E_Config_Dialog_Data *cfdata)
{
   Evas_List *l;
   E_Shelf *es;
   char buf[256];
   const char *label;
   Evas_Object *ob;
   int n;
   Evas_Coord wmw, wmh;
   
   n = e_widget_ilist_selected_get(cfdata->ilist);
   e_widget_ilist_clear(cfdata->ilist);
   for (l = e_shelf_list(); l; l = l->next)
     {
	es = l->data;
	
	label = es->name;
	if (!label) label = "";
	snprintf(buf, sizeof(buf), "%s #%i", label, es->id);
	
	/* FIXME: proper icon */
	ob = edje_object_add(evas_object_evas_get(cfdata->ilist));
	e_util_edje_icon_set(ob, "enlightenment/e");
	
	e_widget_ilist_append(cfdata->ilist, ob, buf, _cb_list, cfdata, NULL);
     }

   e_widget_min_size_get(cfdata->ilist, &wmw, &wmh);
   if (evas_list_count(l) > 0) 
     e_widget_min_size_set(cfdata->ilist, wmw, 250);
    else 
     e_widget_min_size_set(cfdata->ilist, 165, 250);	
   
   e_widget_ilist_go(cfdata->ilist);
   e_widget_ilist_selected_set(cfdata->ilist, n);
}

static void
_cb_add(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *shelves;
   
   cfdata = data;
   while ((shelves = e_shelf_list()))
     {
	E_Shelf *es;
	
	es = shelves->data;
	e_object_del(E_OBJECT(es));
     }
   ////
     {
	E_Config_Shelf *cfg;
	E_Shelf *es;
	
	cfg = E_NEW(E_Config_Shelf, 1);
	cfg->name = evas_stringshare_add("shelf");
	cfg->container = cfdata->cfd->con->num;
	cfg->zone = cfdata->cfd->dia->win->border->zone->num;
	cfg->popup = 1;
	cfg->layer = 200;
	cfg->orient = E_GADCON_ORIENT_CORNER_BR;
	cfg->fit_along = 1;
	cfg->fit_size = 0;
	cfg->style = evas_stringshare_add("default");
	cfg->size = 40;
	e_config->shelves = evas_list_append(e_config->shelves, cfg);
     }
   e_shelf_config_init();
   e_config_save_queue();
   
   _ilist_fill(cfdata);
}

static void
_cb_del(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   E_Shelf *es;
   E_Config_Shelf *cfg;
   
   cfdata = data;
   es = evas_list_nth(e_shelf_list(), e_widget_ilist_selected_get(cfdata->ilist));
   if (!es) return;
   cfg = es->cfg;
   e_object_del(E_OBJECT(es));
   
   e_config->shelves = evas_list_remove(e_config->shelves, cfg);
   if (cfg->name) evas_stringshare_del(cfg->name);
   if (cfg->style) evas_stringshare_del(cfg->style);
   E_FREE(cfg);
   
   e_config_save_queue();
   
   _ilist_fill(cfdata);
}

static void
_cb_config(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   E_Shelf *es;
   
   cfdata = data;
   es = evas_list_nth(e_shelf_list(), e_widget_ilist_selected_get(cfdata->ilist));
   if (!es) return;
   e_int_shelf_config(es);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *oi, *ob, *ol;
   
   o = e_widget_list_add(evas, 0, 1);
   
   of = e_widget_framelist_add(evas, _("Configured Shelves"), 0);
   oi = e_widget_ilist_add(evas, 80, 60, NULL);
   e_widget_ilist_selector_set(oi, 1);
   cfdata->ilist = oi;

   _ilist_fill(cfdata);
   
   e_widget_framelist_object_append(of, oi);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   ol = e_widget_table_add(evas, 0);
   
   ob = e_widget_button_add(evas, _("Add"), "widget/add", _cb_add, cfdata, NULL);
   e_widget_table_object_append(ol, ob, 0, 0, 1, 1,  1, 1, 1, 0);
   ob = e_widget_button_add(evas, _("Delete"), "widget/del", _cb_del, cfdata, NULL);
   e_widget_table_object_append(ol, ob, 0, 1, 1, 1,  1, 1, 1, 0);
   ob = e_widget_button_add(evas, _("Configure..."), "widget/config", _cb_config, cfdata, NULL);
   e_widget_table_object_append(ol, ob, 0, 2, 1, 1,  1, 1, 1, 0);
   
   e_widget_list_object_append(o, ol, 1, 1, 0.0);
   

   return o;
}
