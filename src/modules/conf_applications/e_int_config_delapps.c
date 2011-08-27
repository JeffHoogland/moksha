#include "e.h"

struct _E_Config_Dialog_Data
{
   Eina_List *desks;
   Ecore_Event_Handler *desk_change_handler;
   
   struct {
      Evas_Object *list, *del;
   } obj;
};

/* local function prototypes */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata);
static Eina_Bool _desks_update(void *data, int ev_type __UNUSED__, void *ev __UNUSED__);
static int _cb_desks_sort(const void *data1, const void *data2);
static void _fill_apps_list(E_Config_Dialog_Data *cfdata, Evas_Object *il);
static void _btn_cb_del(void *data, void *data2);
static void _widget_list_selection_changed(void *data, Evas_Object *obj __UNUSED__);

E_Config_Dialog *
e_int_config_delapps(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "applications/del_applications"))
      return NULL;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;

   cfd = e_config_dialog_new(con, _("Delete Launchers"),
                             "E", "applications/del_applications",
                             "preferences-applications-del", 0, v, NULL);
   return cfd;
}

/* local function prototypes */
static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->desk_change_handler = ecore_event_handler_add
      (EFREET_EVENT_DESKTOP_CACHE_UPDATE, _desks_update, cfdata);
   
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Efreet_Desktop *desk;
   
   EINA_LIST_FREE(cfdata->desks, desk) efreet_desktop_free(desk);
   if (cfdata->desk_change_handler)
      ecore_event_handler_del(cfdata->desk_change_handler);
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *of, *li, *ob;
   Evas_Coord mw, mh;
   
   of = e_widget_table_add(evas, 0);
   
   li = e_widget_ilist_add(evas, 24, 24, NULL);
   cfdata->obj.list = li;
   e_widget_ilist_multi_select_set(li, EINA_TRUE);
   e_widget_size_min_get(li, &mw, &mh);
   if (mw < (200 * e_scale)) mw = 200 * e_scale;
   if (mh < (160 * e_scale)) mh = 160 * e_scale;
   e_widget_size_min_set(li, mw, mh);
   e_widget_on_change_hook_set(li, _widget_list_selection_changed, cfdata);
   e_widget_table_object_append(of, li, 0, 1, 1, 1, 1, 1, 1, 1);
   
   _fill_apps_list(cfdata, cfdata->obj.list);
   e_widget_ilist_go(li);
   
   ob = e_widget_button_add(evas, _("Delete"), NULL, _btn_cb_del, cfdata, NULL);
   cfdata->obj.del = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_table_object_append(of, ob, 0, 2, 1, 1, 1, 1, 1, 0);
   
   e_dialog_resizable_set(cfd->dia, 1);
   e_win_centered_set(cfd->dia->win, 1);
   return of;
}

static Eina_Bool
_desks_update(void *data, int ev_type __UNUSED__, void *ev __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   Efreet_Desktop *desk;
   
   EINA_LIST_FREE(cfdata->desks, desk) efreet_desktop_free(desk);
   _fill_apps_list(cfdata, cfdata->obj.list);
   return ECORE_CALLBACK_PASS_ON;
}         

static int
_cb_desks_sort(const void *data1, const void *data2)
{
   const Efreet_Desktop *d1, *d2;
   
   if (!(d1 = data1)) return 1;
   if (!d1->name) return 1;
   if (!(d2 = data2)) return -1;
   if (!d2->name) return -1;
   return strcmp(d1->name, d2->name);
}

static void
_fill_apps_list(E_Config_Dialog_Data *cfdata, Evas_Object *il)
{
   Eina_List *desks = NULL, *l;
   Efreet_Desktop *desk = NULL;
   Evas *evas;
   const char *desktop_dir = e_user_desktop_dir_get();
   int n = 0;
   
   if (!desktop_dir) return;
   
   n = strlen(desktop_dir);
   
   if (!cfdata->desks)
     {
        desks = efreet_util_desktop_name_glob_list("*");
        EINA_LIST_FREE(desks, desk)
          {
             if (desk->no_display)
               {
                  efreet_desktop_free(desk);
                  continue;
               }
             if (!strncmp(desk->orig_path, desktop_dir, n))
                cfdata->desks = eina_list_append(cfdata->desks, desk);
             else
                efreet_desktop_free(desk);
          }
        cfdata->desks = eina_list_sort(cfdata->desks, -1, _cb_desks_sort);
     }

   evas = evas_object_evas_get(il);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(il);
   e_widget_ilist_clear(il);
   
   EINA_LIST_FOREACH(cfdata->desks, l, desk)
     {
        Evas_Object *icon = e_util_desktop_icon_add(desk, 24, evas);
        
        e_widget_ilist_append(il, icon, desk->name,
                              NULL, desk->orig_path,
                              desk->orig_path);
     }
   
   e_widget_ilist_go(il);
   e_widget_ilist_thaw(il);
   edje_thaw();
   evas_event_thaw(evas);
}

static void
_btn_cb_del(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   const Eina_List *l;
   const E_Ilist_Item *it;
   
   EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->obj.list), l, it)
     {
        const char *file;
        
        if (!it->selected) continue;
        file = e_widget_ilist_item_data_get(it);
        if (file) ecore_file_unlink(file);
     }
}

static void
_widget_list_selection_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   const Eina_List *l;
   const E_Ilist_Item *it;
   int selnum = 0;
   
   EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->obj.list), l, it)
     {
        if (!it->selected) continue;
        selnum++;
     }
   if (selnum == 0) e_widget_disabled_set(cfdata->obj.del, 1);
   else e_widget_disabled_set(cfdata->obj.del, 0);
}
