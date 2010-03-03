#include "e.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void        _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int         _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int         _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data
{
   int show_cursor;
   int idle_cursor;
   int use_e_cursor;
   int cursor_size;

   Eina_List *disable_list;
   struct {
      Evas_Object *idle_cursor;
   } gui;
};

E_Config_Dialog *
e_int_config_cursor(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "appearance/mouse_cursor")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->basic.check_changed = _basic_check_changed;

   cfd = e_config_dialog_new(con,
			     _("Cursor Settings"),
			     "E", "appearance/mouse_cursor",
			     "preferences-desktop-pointer", 0, v, NULL);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;
   cfdata->show_cursor = e_config->show_cursor;
   cfdata->idle_cursor = e_config->idle_cursor;
   cfdata->use_e_cursor = e_config->use_e_cursor;
   cfdata->cursor_size = e_config->cursor_size;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   int changed = _basic_check_changed(cfd, cfdata);
   Eina_List *l;
   E_Manager *man;

   if (!changed) return 1;

   e_config->use_e_cursor = cfdata->use_e_cursor;
   e_config->show_cursor = cfdata->show_cursor;
   e_config->idle_cursor = cfdata->idle_cursor;
   e_config->cursor_size = cfdata->cursor_size;
   e_config_save_queue();

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
	if (man->pointer && !e_config->show_cursor)
	  {
	     e_pointer_hide(man->pointer);
	     continue;
	  }
	if (man->pointer) e_object_del(E_OBJECT(man->pointer));
	man->pointer = e_pointer_window_new(man->root, 1);
     }

   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   return ((e_config->show_cursor != cfdata->show_cursor) ||
	   (e_config->idle_cursor != cfdata->idle_cursor) ||
	   (e_config->use_e_cursor != cfdata->use_e_cursor) ||
	   (e_config->cursor_size != cfdata->cursor_size));
}

static void
_use_e_cursor_cb_change(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   Eina_Bool disabled = ((!cfdata->use_e_cursor) || (!cfdata->show_cursor));
   e_widget_disabled_set(cfdata->gui.idle_cursor, disabled);
}

static void
_show_cursor_cb_change(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   const Eina_List *l;
   Evas_Object *o;

   EINA_LIST_FOREACH(cfdata->disable_list, l, o)
      e_widget_disabled_set(o, !cfdata->show_cursor);

   _use_e_cursor_cb_change(cfdata, NULL);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ob, *of;
   E_Radio_Group *rg;

   o = e_widget_list_add(evas, 0, 0);

   ob = e_widget_check_add(evas, _("Show Cursor"), &(cfdata->show_cursor));
   e_widget_on_change_hook_set(ob, _show_cursor_cb_change, cfdata);
   e_widget_list_object_append(o, ob, 1, 0, 0.5);

   of = e_widget_framelist_add(evas, _("Settings"), 0);
   rg = e_widget_radio_group_new(&cfdata->use_e_cursor);
   cfdata->disable_list = eina_list_append(cfdata->disable_list, of);

   ob = e_widget_label_add(evas, _("Size"));
   e_widget_framelist_object_append(of, ob);
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ob);

   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f pixels"),
			    8, 128, 4, 0, NULL, &(cfdata->cursor_size), 100);
   e_widget_framelist_object_append(of, ob);
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ob);

   ob = e_widget_label_add(evas, _("Theme"));
   e_widget_framelist_object_append(of, ob);
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ob);

   ob = e_widget_radio_add(evas, _("X"), 0, rg);
   e_widget_on_change_hook_set(ob, _use_e_cursor_cb_change, cfdata);
   e_widget_framelist_object_append(of, ob);
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ob);

   ob = e_widget_radio_add(evas, _("Enlightenment"), 1, rg);
   e_widget_on_change_hook_set(ob, _use_e_cursor_cb_change, cfdata);
   e_widget_framelist_object_append(of, ob);
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ob);

   ob = e_widget_check_add(evas, _("Idle effects"),
			   &(cfdata->idle_cursor));
   e_widget_framelist_object_append(of, ob);
   cfdata->gui.idle_cursor = ob;

   e_widget_list_object_append(o, of, 1, 0, 0.5);

   _show_cursor_cb_change(cfdata, NULL);

   return o;
}
