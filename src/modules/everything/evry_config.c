#include "e_mod_main.h"

/* typedef struct _E_Config_Dialog_Data E_Config_Dialog_Data; */

static void        *_create_data             (E_Config_Dialog *cfd);
static void         _free_data               (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data        (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets    (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
/* static int _subject = type_subject;
 * static int _action  = type_action;
 * static int _object  = type_object; */


struct _E_Config_Dialog_Data
{
  int hide_input;
  int hide_list;

  int quick_nav;

  int width, height;
  int scroll_animate;

  char *cmd_terminal;

  Evas_Object *l_subject;
  Evas_Object *l_action;
  Evas_Object *l_object;

  Eina_List *p_subject;
  Eina_List *p_action;
  Eina_List *p_object;
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
   Eina_List *l;
   Evry_Plugin *p;

   /* cfdata->scroll_animate = evry_conf->scroll_animate; */
   cfdata->height = evry_conf->height;
   cfdata->width = evry_conf->width;
   cfdata->hide_list = evry_conf->hide_list;
   cfdata->hide_input = evry_conf->hide_input;
   cfdata->quick_nav = evry_conf->quick_nav;
   
   EINA_LIST_FOREACH(evry_conf->plugins, l, p)
     if (p->type == type_subject)
       cfdata->p_subject = eina_list_append(cfdata->p_subject, p);
     else if (p->type == type_action)
       cfdata->p_action = eina_list_append(cfdata->p_action, p);
     else if (p->type == type_object)
       cfdata->p_object = eina_list_append(cfdata->p_object, p);

   if (evry_conf->cmd_terminal)
     cfdata->cmd_terminal = strdup(evry_conf->cmd_terminal);   
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->p_subject) eina_list_free(cfdata->p_subject);
   if (cfdata->p_action)  eina_list_free(cfdata->p_action);
   if (cfdata->p_object)  eina_list_free(cfdata->p_object);
   E_FREE(cfdata->cmd_terminal);
   E_FREE(cfdata);
}

static int
_evry_cb_plugin_sort(const void *data1, const void *data2)
{
   const Evry_Plugin *p1 = data1;
   const Evry_Plugin *p2 = data2;
   return p1->config->priority - p2->config->priority;
}

static int
_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   /* evry_conf->scroll_animate = cfdata->scroll_animate; */
   evry_conf->width = cfdata->width;
   evry_conf->height = cfdata->height;
   evry_conf->hide_input = cfdata->hide_input;
   evry_conf->hide_list = cfdata->hide_list;
   evry_conf->quick_nav = cfdata->quick_nav;
   evry_conf->plugins = eina_list_sort(evry_conf->plugins,
				       eina_list_count(evry_conf->plugins),
				       _evry_cb_plugin_sort);

   if (evry_conf->cmd_terminal)
     eina_stringshare_del(evry_conf->cmd_terminal);
   evry_conf->cmd_terminal = eina_stringshare_add(cfdata->cmd_terminal);
   
   e_config_save_queue();
   return 1;
}

static void
_fill_list(Eina_List *plugins, Evas_Object *obj, int enabled __UNUSED__)
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

   EINA_LIST_FOREACH(plugins, l, p)
     e_widget_ilist_append(obj, NULL, p->name, NULL, p, NULL);

   e_widget_ilist_go(obj);
   e_widget_size_min_get(obj, &w, NULL);
   e_widget_size_min_set(obj, w > 180 ? w : 180, 200);
   e_widget_ilist_thaw(obj);
   edje_thaw();
   evas_event_thaw(evas);
}

static void
_plugin_move(Eina_List *plugins, Evas_Object *list, int dir)
{
   int sel;
   Eina_List *l1, *l2;

   sel = e_widget_ilist_selected_get(list);

   if (sel >= 0)
     {
	Evry_Plugin *p;
	int prio = 0;

	l1 = eina_list_nth_list(plugins, sel);
	l2 = eina_list_nth_list(plugins, sel + dir);

	if (!l1 || !l2) return;
	p = l1->data;
	l1->data = l2->data;
	l2->data = p;

	_fill_list(plugins, list, 0);
	e_widget_ilist_selected_set(list, sel + dir);

	EINA_LIST_FOREACH(plugins, l1, p)
	  p->config->priority = prio++;
     }
}

static void
_plugin_move_up_cb(void *data, void *data2)
{
   _plugin_move(data2, data, -1);
}

static void
_plugin_move_down_cb(void *data, void *data2)
{
   _plugin_move(data2, data, 1);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob, *otb;

   otb = e_widget_toolbook_add(evas, 48 * e_scale, 48 * e_scale);

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("General Settings"), 0);

   ob = e_widget_check_add(evas, _("Hide input when inactive"),
   			   &(cfdata->hide_input));
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_check_add(evas, _("Hide list"),
   			   &(cfdata->hide_list));
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_check_add(evas, _("Quick Navigation (ALT + h,j,k,l,n,p,m,i)"),
   			   &(cfdata->quick_nav));
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_label_add(evas, _("Terminal Command"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_entry_add(evas, &(cfdata->cmd_terminal), NULL, NULL, NULL);
   e_widget_framelist_object_append(of, ob);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);


   of = e_widget_framelist_add(evas, _("Size"), 0);
   ob = e_widget_label_add(evas, _("Popup Width"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"),
   			    evry_conf->min_w, 800, 5, 0, NULL,
   			    &(cfdata->width), 200);
   e_widget_framelist_object_append(of, ob);
   
   ob = e_widget_label_add(evas, _("Popup Height"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"),
   			    evry_conf->min_h, 800, 5, 0, NULL,
   			    &(cfdata->height), 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   /* of = e_widget_framelist_add(evas, _("Scroll Settings"), 0);
    * ob = e_widget_check_add(evas, _("Scroll Animate"),
    * 			   &(cfdata->scroll_animate));
    * e_widget_framelist_object_append(of, ob);
    * e_widget_list_object_append(o, of, 1, 1, 0.5); */


   e_widget_toolbook_page_append(otb, NULL, _("General Settings"),
				 o, 1, 0, 1, 0, 0.5, 0.0);

   ob = e_widget_list_add(evas, 1, 1);
   of = e_widget_framelist_add(evas, _("Active Plugins"), 0);
   o = e_widget_ilist_add(evas, 24, 24, NULL);
   cfdata->l_subject = o;
   /* e_widget_on_change_hook_set(ol, _avail_list_cb_change, cfdata); */
   _fill_list(cfdata->p_subject, o, 0);
   e_widget_framelist_object_append(of, o);
   o = e_widget_button_add(evas, _("Move Up"), NULL,
			   _plugin_move_up_cb,
			   cfdata->l_subject,
			   cfdata->p_subject);
   e_widget_framelist_object_append(of, o);
   o = e_widget_button_add(evas, _("Move Down"), NULL,
			   _plugin_move_down_cb,
			   cfdata->l_subject,
			   cfdata->p_subject);
   e_widget_framelist_object_append(of, o);
   e_widget_list_object_append(ob, of, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Subject Plugins"),
				 of, 1, 0, 1, 0, 0.5, 0.0);


   ob = e_widget_list_add(evas, 1, 1);
   of = e_widget_framelist_add(evas, _("Active Plugins"), 0);
   o = e_widget_ilist_add(evas, 24, 24, NULL);
   cfdata->l_action = o;
   _fill_list(cfdata->p_action, o, 0);
   e_widget_framelist_object_append(of, o);
   o = e_widget_button_add(evas, _("Move Up"), NULL,
			   _plugin_move_up_cb,
			   cfdata->l_action,
			   cfdata->p_action);
   e_widget_framelist_object_append(of, o);
   o = e_widget_button_add(evas, _("Move Down"), NULL,
			   _plugin_move_down_cb,
			   cfdata->l_action,
			   cfdata->p_action);
   e_widget_framelist_object_append(of, o);
   e_widget_list_object_append(ob, of, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Action Plugins"),
				 of, 1, 0, 1, 0, 0.5, 0.0);

   ob = e_widget_list_add(evas, 1, 1);
   of = e_widget_framelist_add(evas, _("Active Plugins"), 0);
   o = e_widget_ilist_add(evas, 24, 24, NULL);
   cfdata->l_object = o;
   _fill_list(cfdata->p_object, o, 0);
   e_widget_framelist_object_append(of, o);
   o = e_widget_button_add(evas, _("Move Up"), NULL,
			   _plugin_move_up_cb,
			   cfdata->l_object,
			   cfdata->p_object);
   e_widget_framelist_object_append(of, o);
   o = e_widget_button_add(evas, _("Move Down"), NULL,
			   _plugin_move_down_cb,
			   cfdata->l_object,
			   cfdata->p_object);
   e_widget_framelist_object_append(of, o);
   e_widget_list_object_append(ob, of, 1, 1, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("Object Plugins"),
				 ob, 1, 0, 1, 0, 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);

   return otb;
}
