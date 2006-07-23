#include "e.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void        _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data 
{
   E_Config_Dialog *cfd;
   Evas_Object *ilist;
   E_Dialog *confirm_dialog;
};

EAPI E_Config_Dialog *
e_int_config_shelf(E_Container *con) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create_widgets;
   
   cfd = e_config_dialog_new(con, _("Shelf Settings"), "enlightenment/shelf", 0, v, NULL);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   free(cfdata);
}

static void
_ilist_fill(E_Config_Dialog_Data *cfdata)
{
   Evas_List *l;
   E_Shelf *es;
   E_Config_Shelf *escfg;
   char buf[256];
   const char *label;
   Evas_Object *ob;
   int n;
   
   n = e_widget_ilist_selected_get(cfdata->ilist);
   e_widget_ilist_clear(cfdata->ilist);
   e_widget_ilist_go(cfdata->ilist);
   
   for (l = e_shelf_list(); l; l = l->next)
     {
	es = l->data;
        escfg = es->cfg;
	
	label = es->name;
	if (!label) label = "";
	snprintf(buf, sizeof(buf), "%s #%i", label, es->id);
	
	/* FIXME: proper icon */
	ob = edje_object_add(evas_object_evas_get(cfdata->ilist));

        switch(escfg->orient)
          {
          case E_GADCON_ORIENT_LEFT:
             e_util_edje_icon_set(ob, "enlightenment/shelf_position_left");
             break;
          case E_GADCON_ORIENT_RIGHT:
             e_util_edje_icon_set(ob, "enlightenment/shelf_position_right");
             break;
          case E_GADCON_ORIENT_TOP:
             e_util_edje_icon_set(ob, "enlightenment/shelf_position_top");
             break;
          case E_GADCON_ORIENT_BOTTOM:
             e_util_edje_icon_set(ob, "enlightenment/shelf_position_bottom");
             break;
          case E_GADCON_ORIENT_CORNER_TL:
             e_util_edje_icon_set(ob, "enlightenment/shelf_position_top_left");
             break;
          case E_GADCON_ORIENT_CORNER_TR:
             e_util_edje_icon_set(ob, "enlightenment/shelf_position_top_right");
             break;
          case E_GADCON_ORIENT_CORNER_BL:
             e_util_edje_icon_set(ob, "enlightenment/shelf_position_bottom_left");
             break;
          case E_GADCON_ORIENT_CORNER_BR:
             e_util_edje_icon_set(ob, "enlightenment/shelf_position_bottom_right");
             break;
          case E_GADCON_ORIENT_CORNER_LT:
             e_util_edje_icon_set(ob, "enlightenment/shelf_position_left_top");
             break;
          case E_GADCON_ORIENT_CORNER_RT:
             e_util_edje_icon_set(ob, "enlightenment/shelf_position_right_top");
             break;
          case E_GADCON_ORIENT_CORNER_LB:
             e_util_edje_icon_set(ob, "enlightenment/shelf_position_left_bottom");
             break;
          case E_GADCON_ORIENT_CORNER_RB:
             e_util_edje_icon_set(ob, "enlightenment/shelf_position_right_bottom");
             break;
          default:
             e_util_edje_icon_set(ob, "enlightenment/e");
             break;
          }
	
	e_widget_ilist_append(cfdata->ilist, ob, buf, NULL, NULL, NULL);
     }
   
   e_widget_min_size_set(cfdata->ilist, 155, 250);
   e_widget_ilist_go(cfdata->ilist);
   e_widget_ilist_selected_set(cfdata->ilist, n);
}

static void
_cb_add(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Shelf *cfg;

   cfdata = data;

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

   e_shelf_config_init();
   e_config_save_queue();
   
   _ilist_fill(cfdata);
   e_widget_ilist_selected_set(cfdata->ilist, e_widget_ilist_count(cfdata->ilist) - 1);
}

static void
_cb_confirm_dialog_yes(void *data)
{
   E_Shelf *es;
   E_Config_Shelf *cfg;
   E_Config_Dialog_Data *cfdata;
   char *dummy;
   int i;
   
   cfdata = data;
   sscanf(e_widget_ilist_selected_label_get(cfdata->ilist), "%s #%i", &dummy, &i);

   es = evas_list_nth(e_shelf_list(), i);
   if (es)
     {
	cfg = es->cfg;
	e_object_del(E_OBJECT(es));

	e_config->shelves = evas_list_remove(e_config->shelves, cfg);
	if (cfg->name) evas_stringshare_del(cfg->name);
	if (cfg->style) evas_stringshare_del(cfg->style);
	E_FREE(cfg);
	e_config_save_queue();

	_ilist_fill(cfdata);
     }
}
static void
_cb_del(void *data, void *data2)
{
   char buf[4096];
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   snprintf(buf, sizeof(buf), _("You requested to delete \"%s\".<br>"
			      "<br>"
			      "Are you sure you want to delete this shelf?"),
			      e_widget_ilist_selected_label_get(cfdata->ilist));

   e_confirm_dialog_show(_("Are you sure you want to delete this shelf?"), "enlightenment/exit",
			 buf, NULL, NULL, _cb_confirm_dialog_yes, NULL, cfdata, NULL);
}

static void
_cb_config(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   E_Shelf *es;
   char *dummy;
   int i;
   
   cfdata = data;
   sscanf(e_widget_ilist_selected_label_get(cfdata->ilist), "%s #%i", &dummy, &i);

   es = evas_list_nth(e_shelf_list(), i);
   if (!es) return;
   if (!es->config_dialog) e_int_shelf_config(es);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *oi, *ob, *ol;
   
   o = e_widget_list_add(evas, 0, 1);
   
   of = e_widget_framelist_add(evas, _("Configured Shelves"), 0);
   oi = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_ilist_selector_set(oi, 1);
   cfdata->ilist = oi;

   _ilist_fill(cfdata);
   e_widget_min_size_set(oi, 155, 250);
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
