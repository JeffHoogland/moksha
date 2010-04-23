#include "e_mod_main.h"

static void        *_create_data             (E_Config_Dialog *cfd);
static void         _free_data               (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data        (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets    (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

typedef struct _Plugin_Page Plugin_Page;

struct _Plugin_Page
{
  Evas_Object *list;
  Evas_Object *o_trigger;
  Evas_Object *o_trigger_only;
  E_Radio_Group *view_rg;
  Evas_Object *o_view_default;
  Evas_Object *o_view_list;
  Evas_Object *o_view_detail;
  Evas_Object *o_view_thumb;
  
  Eina_List *configs;
  
  char *trigger;
  int trigger_only;
  int view_mode;

  Plugin_Config *cur;
} ;

struct _E_Config_Dialog_Data
{
  int hide_input;
  int hide_list;

  int quick_nav;

  int width, height;
  double rel_x, rel_y;
  int scroll_animate;
  double scroll_speed;
  
  char *cmd_terminal;
  char *cmd_sudo;

  int view_mode;
  int view_zoom;
  int cycle_mode;

  int history_sort_mode;

  Plugin_Page page[3];
};


EAPI E_Config_Dialog *
evry_config_dialog(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "extensions/run_everything")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = NULL;
   v->advanced.create_widgets = NULL;
   cfd = e_config_dialog_new(con,
			     _("Everything Settings"),
			     "E", "extensions/run_everything",
			     "system-run", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
#define C(_name) cfdata->_name = evry_conf->_name
   C(height);
   C(width);
   C(hide_list);
   C(hide_input);
   C(quick_nav);
   C(rel_x);
   C(rel_y);
   C(view_mode);
   C(view_zoom);
   C(cycle_mode);
   C(history_sort_mode);
   C(scroll_animate);
   C(scroll_speed);
#undef C

   cfdata->page[0].configs = eina_list_clone(evry_conf->conf_subjects); 
   cfdata->page[1].configs = eina_list_clone(evry_conf->conf_actions); 
   cfdata->page[2].configs = eina_list_clone(evry_conf->conf_objects); 
   
   if (evry_conf->cmd_terminal)
     cfdata->cmd_terminal = strdup(evry_conf->cmd_terminal);   

   if (evry_conf->cmd_sudo)
     cfdata->cmd_sudo = strdup(evry_conf->cmd_sudo);   
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
   if (cfdata->page[0].configs) eina_list_free(cfdata->page[0].configs);
   if (cfdata->page[1].configs) eina_list_free(cfdata->page[1].configs);
   if (cfdata->page[2].configs) eina_list_free(cfdata->page[2].configs);

   E_FREE(cfdata->cmd_terminal);
   E_FREE(cfdata->cmd_sudo);
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   int i;
   
#define C(_name) evry_conf->_name = cfdata->_name
   C(height);
   C(width);
   C(hide_list);
   C(hide_input);
   C(quick_nav);
   C(rel_x);
   C(rel_y);
   C(view_mode);
   C(view_zoom);
   C(cycle_mode);
   C(history_sort_mode);
   C(scroll_animate);
   C(scroll_speed);
#undef C

   /* evry_conf->plugins = eina_list_sort(evry_conf->plugins, -1,
    * 				       _evry_cb_plugin_sort); */

   if (evry_conf->conf_subjects) eina_list_free(evry_conf->conf_subjects);
   if (evry_conf->conf_actions) eina_list_free(evry_conf->conf_actions);
   if (evry_conf->conf_objects) eina_list_free(evry_conf->conf_objects);
  
   evry_conf->conf_subjects = eina_list_clone(cfdata->page[0].configs);
   evry_conf->conf_actions = eina_list_clone(cfdata->page[1].configs);
   evry_conf->conf_objects = eina_list_clone(cfdata->page[2].configs);

   for (i = 0; i < 3; i++)
     {
	if (cfdata->page[i].cur)
	  {
	     if (cfdata->page[i].cur->trigger)
	       eina_stringshare_del(cfdata->page[i].cur->trigger);

	     if (cfdata->page[i].trigger[i])
	       cfdata->page[i].cur->trigger = eina_stringshare_add(cfdata->page[i].trigger);
	     else
	       cfdata->page[i].cur->trigger = NULL;
	
	     cfdata->page[i].cur->trigger_only = cfdata->page[i].trigger_only;

	     cfdata->page[i].cur->view_mode = cfdata->page[i].view_mode;
	  }
     }
   
   if (evry_conf->cmd_terminal)
     eina_stringshare_del(evry_conf->cmd_terminal);
   evry_conf->cmd_terminal = eina_stringshare_add(cfdata->cmd_terminal);
   if (evry_conf->cmd_sudo)
     eina_stringshare_del(evry_conf->cmd_sudo);
   evry_conf->cmd_sudo = eina_stringshare_add(cfdata->cmd_sudo);
   
   e_config_save_queue();
   return 1;
}

static void
_fill_list(Eina_List *plugins, Evas_Object *obj, int enabled __UNUSED__)
{
   Evas *evas;
   Evas_Coord w;
   Eina_List *l;
   Plugin_Config *pc;

   /* freeze evas, edje, and list widget */
   evas = evas_object_evas_get(obj);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(obj);
   e_widget_ilist_clear(obj);
   
   EINA_LIST_FOREACH(plugins, l, pc)
     e_widget_ilist_append(obj, NULL, pc->name, NULL, pc, NULL);
   
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
	Plugin_Config *pc;
	int prio = 0;

	l1 = eina_list_nth_list(plugins, sel);
	l2 = eina_list_nth_list(plugins, sel + dir);

	if (!l1 || !l2) return;
	pc = l1->data;
	l1->data = l2->data;
	l2->data = pc;

	_fill_list(plugins, list, 0);
	e_widget_ilist_selected_set(list, sel + dir);

	EINA_LIST_FOREACH(plugins, l1, pc)
	  pc->priority = prio++;
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

static void
_list_select_cb (void *data, Evas_Object *obj)
{
   int sel = e_widget_ilist_selected_get(obj);
   Plugin_Config *pc;
   Plugin_Page *page = data;
   
   if (sel >= 0)
     {
	pc = eina_list_nth(page->configs, sel);
	e_widget_entry_text_set(page->o_trigger, pc->trigger);
	e_widget_disabled_set(page->o_trigger, 0); 

	e_widget_check_checked_set(page->o_trigger_only, pc->trigger_only); 
	e_widget_disabled_set(page->o_trigger_only, 0); 

	e_widget_disabled_set(page->o_view_default, 0);
	e_widget_disabled_set(page->o_view_detail, 0);
	e_widget_disabled_set(page->o_view_list, 0);
	e_widget_disabled_set(page->o_view_thumb, 0);

	if (pc->view_mode == -1)
	  e_widget_radio_toggle_set(page->o_view_default, 1); 
	else if (pc->view_mode == 0)
	  e_widget_radio_toggle_set(page->o_view_list, 1);
	else if (pc->view_mode == 1)
	  e_widget_radio_toggle_set(page->o_view_detail, 1);
	else if (pc->view_mode == 2)
	  e_widget_radio_toggle_set(page->o_view_thumb, 1);
	
	page->cur = pc;
     }
   else
     {
	e_widget_entry_text_set(page->o_trigger, "");
	e_widget_disabled_set(page->o_trigger, 1); 
	e_widget_disabled_set(page->o_trigger_only, 1);
	e_widget_disabled_set(page->o_view_default, 1);
	e_widget_disabled_set(page->o_view_detail, 1);
	e_widget_disabled_set(page->o_view_list, 1);
	e_widget_disabled_set(page->o_view_thumb, 1);
	
	page->cur = NULL;
     }
}

static void
_plugin_config_cb(void *data, void *data2)
{
}

static Evas_Object *
_create_plugin_page(E_Config_Dialog_Data *cfdata, Evas *e, Plugin_Page *page)
{
   Evas_Object *ot, *o, *of, *ob;
   E_Radio_Group *rg;
   Evas_Object *list;
   
   of = e_widget_framelist_add(e, _("Active Plugins"), 0);
   page->list = e_widget_ilist_add(e, 24, 24, NULL);
   e_widget_on_change_hook_set(page->list, _list_select_cb, page);
   /* e_widget_on_change_hook_set(ol, _avail_list_cb_change, cfdata); */
   _fill_list(page->configs, page->list, 0);
   e_widget_framelist_object_append(of, page->list);

   o = e_widget_button_add(e, _("Move Up"), NULL,
			   _plugin_move_up_cb,
			   page->list, page->configs);
   e_widget_framelist_object_append(of, o);

   o = e_widget_button_add(e, _("Move Down"), NULL,
			   _plugin_move_down_cb,
			   page->list, page->configs);
   e_widget_framelist_object_append(of, o);

   ob = e_widget_table_add(e, 1);
   e_widget_table_object_append(ob, of, 0, 0, 1, 3, 1, 1, 1, 1);

   of = e_widget_framelist_add(e, _("Plugin Trigger"), 0);
   o = e_widget_entry_add(e, &(page->trigger), NULL, NULL, NULL);
   e_widget_disabled_set(o, 1);
   page->o_trigger = o;
   e_widget_framelist_object_append(of, o);
   o = e_widget_check_add(e, _("Show only when triggered"),
			  &(page->trigger_only));
   e_widget_disabled_set(o, 1);
   page->o_trigger_only = o;
   e_widget_framelist_object_append(of, o);
   e_widget_table_object_append(ob, of, 1, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(e, _("Plugin View"), 0);
   rg = e_widget_radio_group_new(&page->view_mode);
   o = e_widget_radio_add(e, _("Default"), -1, rg);
   e_widget_disabled_set(o, 1);
   page->o_view_default = o;
   e_widget_framelist_object_append(of, o);
   o = e_widget_radio_add(e, _("List"), 0, rg);
   e_widget_framelist_object_append(of, o);
   e_widget_disabled_set(o, 1);
   page->o_view_list = o;
   o = e_widget_radio_add(e, _("Detailed"), 1, rg);
   e_widget_disabled_set(o, 1);
   page->o_view_detail = o;
   e_widget_framelist_object_append(of, o);
   o = e_widget_radio_add(e, _("Icons"), 2, rg);
   e_widget_disabled_set(o, 1);
   page->o_view_thumb = o;
   e_widget_framelist_object_append(of, o);
   e_widget_table_object_append(ob, of, 1, 1, 1, 1, 1, 1, 1, 1);

   /* of = e_widget_framelist_add(e, _("Plugin Cofiguration"), 0);
    * o = e_widget_button_add(e, _("Configure"), NULL,
    * 			   _plugin_config_cb,
    * 			   cfdata->l_subject,
    * 			   cfdata->p_subject);
    * e_widget_framelist_object_append(of, o); */
   /* e_widget_table_object_append(ob, of, 1, 2, 1, 1, 1, 0, 1, 0); */
   
   return ob;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *e, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ot, *o, *of, *ob, *otb, *otb2;
   E_Radio_Group *rg;
   
   otb = e_widget_toolbook_add(e, 48 * e_scale, 48 * e_scale);

   o = e_widget_table_add(e, 0);

   /// GENERAL SETTNGS ///
   of = e_widget_framelist_add(e, _("General"), 0);

   ob = e_widget_check_add(e, _("Hide input when inactive"),
   			   &(cfdata->hide_input));
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_check_add(e, _("Hide list"),
   			   &(cfdata->hide_list));
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_label_add(e, _("Quick Navigation"));
   e_widget_framelist_object_append(of, ob);
    
   rg = e_widget_radio_group_new(&cfdata->quick_nav);
   ob = e_widget_radio_add(e, _("Off"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(e, _("Emacs style (ALT + n,p,f,b,m,i)"), 3, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(e, _("Vi style (ALT + h,j,k,l,n,p,m,i)"), 1, rg);
   e_widget_framelist_object_append(of, ob);

   e_widget_table_object_append(o, of, 0, 0, 1, 1, 0, 0, 0, 0); 

   of = e_widget_framelist_add(e, _("Default View"), 0);
   rg = e_widget_radio_group_new(&cfdata->view_mode); 
   ob = e_widget_radio_add(e, _("List"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(e, _("Detailed"), 1, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(e, _("Icons"), 2, rg);
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_check_add(e, _("Animate scrolling"),
   			   &(cfdata->scroll_animate));
   e_widget_framelist_object_append(of, ob);
   /* ob = e_widget_slider_add(e, 1, 0, _("%1.1f"),
    * 			    5, 20, 0.1, 0, &(cfdata->scroll_speed), NULL, 10);
    * e_widget_framelist_object_append(of, ob); */
   
   ob = e_widget_check_add(e, _("Up/Down select next item in icon view"),
   			   &(cfdata->cycle_mode));
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(o, of, 1, 0, 1, 1, 0, 1, 0, 0); 

   of = e_widget_framelist_add(e, _("Sorting"), 0);
   rg = e_widget_radio_group_new(&cfdata->history_sort_mode); 
   ob = e_widget_radio_add(e, _("By usage"), 0, rg);
   e_widget_radio_toggle_set(ob, (cfdata->history_sort_mode == 0));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(e, _("Most used"), 1, rg);
   e_widget_radio_toggle_set(ob, (cfdata->history_sort_mode == 1));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(e, _("Last used"), 2, rg);
   e_widget_radio_toggle_set(ob, (cfdata->history_sort_mode == 2));
   e_widget_framelist_object_append(of, ob);

   e_widget_table_object_append(o, of, 0, 1, 2, 1, 1, 0, 0, 0);
   
   of = e_widget_framelist_add(e, _("Commands"), 0);
   ob = e_widget_label_add(e, _("Terminal Command"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_entry_add(e, &(cfdata->cmd_terminal), NULL, NULL, NULL);
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_label_add(e, _("Sudo GUI"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_entry_add(e, &(cfdata->cmd_sudo), NULL, NULL, NULL);
   e_widget_framelist_object_append(of, ob);

   e_widget_table_object_append(o, of, 0, 2, 2, 1, 1, 0, 0, 0);
   
   e_widget_toolbook_page_append(otb, NULL, _("General Settings"),
				 o, 1, 0, 1, 0, 0.5, 0.0);

   /// PLUGINS ///
   otb2 = e_widget_toolbook_add(e, 48 * e_scale, 48 * e_scale);

   ob = _create_plugin_page(cfdata, e, &cfdata->page[0]);
   e_widget_toolbook_page_append(otb2, NULL, _("Subject Plugins"),
				 ob, 1, 0, 1, 0, 0.5, 0.0);

   ob = _create_plugin_page(cfdata, e, &cfdata->page[1]);
   e_widget_toolbook_page_append(otb2, NULL, _("Action Plugins"),
   				  ob, 1, 0, 1, 0, 0.5, 0.0);

   ob = _create_plugin_page(cfdata, e, &cfdata->page[2]);
   e_widget_toolbook_page_append(otb2, NULL, _("Object Plugins"),
   				 ob, 1, 0, 1, 0, 0.5, 0.0);
   
   e_widget_toolbook_page_append(otb, NULL, _("Plugins"),
				 otb2, 1, 0, 1, 0, 0.5, 0.0);

   e_widget_toolbook_page_show(otb2, 0);

   /// GEOMERY ///
   o = e_widget_list_add(e, 0, 0);
   of = e_widget_framelist_add(e, _("Popup Size"), 0);
   ob = e_widget_label_add(e, _("Popup Width"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(e, 1, 0, _("%1.0f"),
   			    evry_conf->min_w, 800, 5, 0, NULL,
   			    &(cfdata->width), 200);
   e_widget_framelist_object_append(of, ob);
   
   ob = e_widget_label_add(e, _("Popup Height"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(e, 1, 0, _("%1.0f"),
   			    evry_conf->min_h, 800, 5, 0, NULL,
   			    &(cfdata->height), 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(e, _("Popup Align"), 0);
   ob = e_widget_label_add(e, _("Vertical"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(e, 1, 0, _("%1.2f"),
   			    0.0, 1.0, 0.01, 0,
   			    &(cfdata->rel_y), NULL, 200);
   e_widget_framelist_object_append(of, ob);
   
   ob = e_widget_label_add(e, _("Horizontal"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(e, 1, 0, _("%1.2f"),
   			    0.0, 1.0, 0.01, 0,
   			    &(cfdata->rel_x), NULL, 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("Geometry"),
				 o, 1, 0, 1, 0, 0.5, 0.0);

   
   e_widget_toolbook_page_show(otb, 0);

   return otb;
}
