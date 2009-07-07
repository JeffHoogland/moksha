
#include "e_mod_main.h"

/* typedef struct _E_Config_Dialog_Data E_Config_Dialog_Data; */

static void        *_create_data             (E_Config_Dialog *cfd);
static void         _free_data               (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data        (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets    (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);


struct _E_Config_Dialog_Data 
{
  int width, height;

  int scroll_animate;

  Evas_Object *l_avail;
  
};


EAPI E_Config_Dialog *
evry_config_dialog(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_everything_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = NULL;
   v->advanced.create_widgets = NULL;
   cfd = e_config_dialog_new(con,
			     _("Everything Settings"),
			     "E", "_config_everything_dialog",
			     "system-run", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   cfdata->scroll_animate = evry_conf->scroll_animate;
   cfdata->height = evry_conf->height;
   cfdata->width = evry_conf->width;
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
   evry_conf->width = cfdata->width;
   evry_conf->height = cfdata->height;
   evry_conf->scroll_animate = cfdata->scroll_animate;

   e_config_save_queue();
   return 1;
}

static void
_fill_list(Evas_Object *obj, int enabled)
{
   Evas *evas;
   Evas_Coord w;
   Eina_List *l;
   Evry_Plugin *p;
   
   /* freeze evas, edje, and list widget */
   evas = evas_object_evas_get(obj);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(obj);
   e_widget_ilist_clear(obj);

   EINA_LIST_FOREACH(evry_conf->plugins, l, p)
     e_widget_ilist_append(obj, NULL, p->name, NULL, p, NULL);

   e_widget_ilist_go(obj);
   e_widget_min_size_get(obj, &w, NULL);
   e_widget_min_size_set(obj, w > 180 ? w : 180, 200);
   e_widget_ilist_thaw(obj);
   edje_thaw();
   evas_event_thaw(evas);
}

static void
_plugin_move(Evas_Object *list, int dir)
{
   int sel;
   Eina_List *l1, *l2;

   sel = e_widget_ilist_selected_get(list);

   if (sel >= 0)
     {
	Evry_Plugin *p;
	Evas *evas;
	int prio = 0;
	
	l1 = eina_list_nth_list(evry_conf->plugins, sel);
	l2 = eina_list_nth_list(evry_conf->plugins, sel + dir);

	if (!l1 || !l2) return;
	p = l1->data;
	l1->data = l2->data;
	l2->data = p;

	_fill_list(list, 0); 
	e_widget_ilist_selected_set(list, sel + dir);

	EINA_LIST_FOREACH(evry_conf->plugins, l1, p)
	  p->config->priority = prio++;
     }
}

static void
_plugin_move_up_cb(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;

   _plugin_move(cfdata->l_avail, -1);
}

static void
_plugin_move_down_cb(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   _plugin_move(cfdata->l_avail, 1);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob, *otb;

   otb = e_widget_toolbook_add(evas, 48 * e_scale, 48 * e_scale);
   
   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ob = e_widget_label_add(evas, _("Popup Width"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 300, 800, 5, 0, NULL, &(cfdata->width), 200);
   e_widget_framelist_object_append(of, ob);   

   ob = e_widget_label_add(evas, _("Popup Height"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 200, 800, 5, 0, NULL, &(cfdata->height), 200);
   e_widget_framelist_object_append(of, ob);   
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Scroll Settings"), 0);
   ob = e_widget_check_add(evas, _("Scroll Animate"), &(cfdata->scroll_animate));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("General Settings"), o, 0, 0, 0, 0, 0.5, 0.0);
   e_widget_toolbook_page_show(otb, 0);

   of = e_widget_framelist_add(evas, _("Available Plugins"), 0);
   o = e_widget_ilist_add(evas, 24, 24, NULL);
   cfdata->l_avail = o;
   /* e_widget_ilist_multi_select_set(ol, 1); */
   /* e_widget_on_change_hook_set(ol, _avail_list_cb_change, cfdata); */
   _fill_list(o, 0);
   e_widget_framelist_object_append(of, o);
   o = e_widget_button_add(evas, _("Move Up"), NULL, _plugin_move_up_cb, cfdata, NULL);
   e_widget_framelist_object_append(of, o);
   o = e_widget_button_add(evas, _("Move Down"), NULL, _plugin_move_down_cb, cfdata, NULL);
   e_widget_framelist_object_append(of, o);

   e_widget_toolbook_page_append(otb, NULL, _("Plugins"), of, 0, 0, 0, 0, 0.5, 0.0);
   
   return otb;
}
