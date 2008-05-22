#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data 
{
   Evas_Object *o_scroll, *o_fm;
   E_Fm2_Config fmc;
};

/* private function prototypes */
static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_focus_steal(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_wid_cb_selected(void *data, Evas_Object *obj, void *event);
static void _e_wid_cb_dir_changed(void *data, Evas_Object *obj, void *event);
static void _e_wid_cb_changed(void *data, Evas_Object *obj, void *event);
static void _e_wid_cb_file_deleted(void *data, Evas_Object *obj, void *event);

EAPI Evas_Object *
e_widget_flist_add(Evas *evas) 
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;

   wd = E_NEW(E_Widget_Data, 1);
   if (!wd) return NULL;

   obj = e_widget_add(evas);
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_focus_hook_set(obj, _e_wid_focus_hook);
   e_widget_data_set(obj, wd);

   o = e_scrollframe_add(evas);
   wd->o_scroll = o;
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _e_wid_focus_steal, obj);

   o = e_fm2_add(evas);
   wd->o_fm = o;
   memset(&wd->fmc, 0, sizeof(E_Fm2_Config));
   wd->fmc.view.mode = E_FM2_VIEW_MODE_LIST;
   wd->fmc.view.open_dirs_in_place = 1;
   wd->fmc.view.selector = 1;
   wd->fmc.view.single_click = 0;
   wd->fmc.view.no_subdir_jump = 0;
   wd->fmc.icon.list.w = 48;
   wd->fmc.icon.list.h = 48;
   wd->fmc.icon.fixed.w = 1;
   wd->fmc.icon.fixed.h = 1;
   wd->fmc.icon.extension.show = 0;
   wd->fmc.icon.key_hint = NULL;
   wd->fmc.list.sort.no_case = 1;
   wd->fmc.list.sort.dirs.first = 0;
   wd->fmc.list.sort.dirs.last = 0;
   wd->fmc.selection.single = 1;
   wd->fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(wd->o_fm, &wd->fmc);
   e_fm2_icon_menu_flags_set(wd->o_fm, E_FM2_MENU_NO_SHOW_HIDDEN);

   evas_object_smart_callback_add(wd->o_fm, "dir_changed", 
                                  _e_wid_cb_dir_changed, obj);
   evas_object_smart_callback_add(wd->o_fm, "selection_change", 
                                  _e_wid_cb_selected, obj);
   evas_object_smart_callback_add(wd->o_fm, "changed", 
                                  _e_wid_cb_changed, obj);
   evas_object_smart_callback_add(wd->o_fm, "files_deleted", 
                                  _e_wid_cb_file_deleted, obj);

   e_scrollframe_child_set(wd->o_scroll, o);
   e_scrollframe_extern_pan_set(wd->o_scroll, o, e_fm2_pan_set, 
                                e_fm2_pan_get, e_fm2_pan_max_get, 
                                e_fm2_pan_child_size_get);

   e_widget_sub_object_add(obj, o);
   evas_object_show(o);
   evas_object_smart_callback_add(o, "selected", 
                                  _e_wid_cb_selected, obj);

   evas_object_resize(obj, 32, 32);
   e_widget_min_size_set(obj, 32, 32);
   return obj;
}

EAPI void 
e_widget_flist_path_set(Evas_Object *obj, const char *dev, const char *path) 
{
   E_Widget_Data *wd = NULL;

   wd = e_widget_data_get(obj);
   e_fm2_path_set(wd->o_fm, dev, path);
   e_scrollframe_child_pos_set(wd->o_scroll, 0, 0);
}

EAPI Evas_List *
e_widget_flist_all_list_get(Evas_Object *obj) 
{
   E_Widget_Data *wd = NULL;

   wd = e_widget_data_get(obj);
   return e_fm2_all_list_get(wd->o_fm);
}

EAPI Evas_List *
e_widget_flist_selected_list_get(Evas_Object *obj) 
{
   E_Widget_Data *wd = NULL;

   wd = e_widget_data_get(obj);
   return e_fm2_selected_list_get(wd->o_fm);
}

EAPI const char *
e_widget_flist_real_path_get(Evas_Object *obj) 
{
   E_Widget_Data *wd = NULL;

   wd = e_widget_data_get(obj);
   return e_fm2_real_path_get(wd->o_fm);
}

EAPI int 
e_widget_flist_has_parent_get(Evas_Object *obj) 
{
   E_Widget_Data *wd = NULL;

   wd = e_widget_data_get(obj);
   return e_fm2_has_parent_get(wd->o_fm);
}

EAPI void 
e_widget_flist_select_set(Evas_Object *obj, const char *file, int select) 
{
   E_Widget_Data *wd = NULL;

   wd = e_widget_data_get(obj);
   e_fm2_select_set(wd->o_fm, file, select);
}

EAPI void 
e_widget_flist_file_show(Evas_Object *obj, const char *file) 
{
   E_Widget_Data *wd = NULL;

   wd = e_widget_data_get(obj);
   e_fm2_file_show(wd->o_fm, file);
}

EAPI void 
e_widget_flist_parent_go(Evas_Object *obj) 
{
   E_Widget_Data *wd = NULL;

   wd = e_widget_data_get(obj);
   e_fm2_parent_go(wd->o_fm);
   e_scrollframe_child_pos_set(wd->o_scroll, 0, 0);
}

EAPI void 
e_widget_flist_refresh(Evas_Object *obj) 
{
   E_Widget_Data *wd = NULL;

   wd = e_widget_data_get(obj);
   e_fm2_refresh(wd->o_fm);
}

/* private functions */
static void 
_e_wid_del_hook(Evas_Object *obj) 
{
   E_Widget_Data *wd = NULL;

   wd = e_widget_data_get(obj);
   E_FREE(wd);
}

static void 
_e_wid_focus_hook(Evas_Object *obj) 
{
   E_Widget_Data *wd = NULL;

   wd = e_widget_data_get(obj);
   if (e_widget_focus_get(obj)) 
     {
        edje_object_signal_emit(e_scrollframe_edje_object_get(wd->o_scroll), 
                                "e,state,focused", "e");
        evas_object_focus_set(wd->o_fm, 1);
     }
   else 
     {
        edje_object_signal_emit(e_scrollframe_edje_object_get(wd->o_scroll), 
                                "e,state,unfocused", "e");
        evas_object_focus_set(wd->o_fm, 0);
     }
}

static void 
_e_wid_focus_steal(void *data, Evas *evas, Evas_Object *obj, void *event) 
{
   e_widget_focus_steal(data);
}

static void 
_e_wid_cb_selected(void *data, Evas_Object *obj, void *event) 
{
   evas_object_smart_callback_call(data, "selection_change", event);
}

static void 
_e_wid_cb_dir_changed(void *data, Evas_Object *obj, void *event) 
{
   evas_object_smart_callback_call(data, "dir_changed", event);
}

static void 
_e_wid_cb_changed(void *data, Evas_Object *obj, void *event) 
{
   evas_object_smart_callback_call(data, "changed", event);
}

static void 
_e_wid_cb_file_deleted(void *data, Evas_Object *obj, void *event) 
{
   evas_object_smart_callback_call(data, "files_deleted", event);
}
