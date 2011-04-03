#include "e.h"
#include "e_mod_main.h"

struct _E_Config_Dialog_Data
{
   Evas_Object *o_list, *o_up;
   int fmdir;
   char *bg;
};

/* local function prototypes */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _cb_dir_changed(void *data, Evas_Object *obj, void *event);
static void _cb_sel_changed(void *data, Evas_Object *obj, void *event);
static void _cb_files_changed(void *data, Evas_Object *obj, void *event);
static void _cb_files_deleted(void *data, Evas_Object *obj, void *event);
static void _cb_radio_changed(void *data, Evas_Object *obj, void *event);
static void _cb_dir_up(void *data1, void *data2);

E_Config_Dialog *
e_int_config_desklock_fsel(E_Config_Dialog *parent) 
{
   E_Container *con;
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (parent) 
     con = parent->con;
   else
     con = e_container_current_get(e_manager_current_get());

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic_only = 1;
   v->normal_win = 1;

   cfd = e_config_dialog_new(con, _("Select a Background..."), 
                             "E", "_desklock_fsel_dialog", 
                             "enlightenment/background", 0, v, parent);

   return cfd;
}

/* local functions */
static void *
_create_data(E_Config_Dialog *cfd __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->fmdir = 0;
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   const char *bg_file = NULL;

   if (cfdata->bg) bg_file = strdup(cfdata->bg);
   E_FREE(cfdata->bg);
   E_FREE(cfdata);
   e_int_config_desklock_fsel_done(cfd->data, bg_file);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *ow, *ot, *rt;
   E_Radio_Group *rg;
   char path[PATH_MAX];

   /* Content */
   o = e_widget_list_add(evas, 0, 0);

   rg = e_widget_radio_group_new(&(cfdata->fmdir));
   ot = e_widget_table_add(evas, 0);
   rt = e_widget_table_add(evas, 1);

   ow = e_widget_radio_add(evas, _("Personal"), 0, rg);
   evas_object_smart_callback_add(ow, "changed", _cb_radio_changed, cfdata);
   e_widget_table_object_append(rt, ow, 0, 0, 1, 1, 1, 1, 0, 0);
   ow = e_widget_radio_add(evas, _("System"), 1, rg);
   evas_object_smart_callback_add(ow, "changed", _cb_radio_changed, cfdata);
   e_widget_table_object_append(rt, ow, 1, 0, 1, 1, 1, 1, 0, 0);
   e_widget_table_object_append(ot, rt, 0, 0, 1, 1, 0, 0, 0, 0);
   ow = e_widget_button_add(evas, _("Go up a Directory"), "go-up", 
                            _cb_dir_up, cfdata, NULL);
   cfdata->o_up = ow;
   e_widget_table_object_append(ot, ow, 0, 1, 1, 1, 0, 0, 0, 0);

   if (cfdata->fmdir == 1)
     e_prefix_data_concat_static(path, "data/backgrounds");
   else
     e_user_dir_concat_static(path, "backgrounds");

   cfdata->o_list = e_widget_flist_add(evas);
   evas_object_smart_callback_add(cfdata->o_list, "dir_changed", 
                                  _cb_dir_changed, cfdata);
   evas_object_smart_callback_add(cfdata->o_list, "selection_change", 
                                  _cb_sel_changed, cfdata);
   evas_object_smart_callback_add(cfdata->o_list, "changed", 
                                  _cb_files_changed, cfdata);
   evas_object_smart_callback_add(cfdata->o_list, "files_deleted", 
                                  _cb_files_deleted, cfdata);
   e_widget_flist_path_set(cfdata->o_list, path, "/");
   e_widget_size_min_set(cfdata->o_list, 200, 160);
   e_widget_table_object_append(ot, cfdata->o_list, 0, 2, 1, 1, 1, 1, 1, 1);
   e_widget_list_object_append(o, ot, 1, 1, 0.5);

   return o;
}

static void 
_cb_dir_changed(void *data, Evas_Object *obj __UNUSED__, void *event __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = data)) return;
   e_widget_disabled_set(cfdata->o_up, 
                         !e_widget_flist_has_parent_get(cfdata->o_list));
}

static void 
_cb_sel_changed(void *data, Evas_Object *obj __UNUSED__, void *event __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *sel;
   E_Fm2_Icon_Info *ici;
   const char *realpath;
   char buff[PATH_MAX];

   if (!(cfdata = data)) return;
   if (!(sel = e_widget_flist_selected_list_get(cfdata->o_list))) return;
   ici = sel->data;
   realpath = e_widget_flist_real_path_get(cfdata->o_list);
   if (!strcmp(realpath, "/"))
     snprintf(buff, sizeof(buff), "/%s", ici->file);
   else
     snprintf(buff, sizeof(buff), "%s/%s", realpath, ici->file);
   eina_list_free(sel);
   if (ecore_file_is_dir(buff)) return;
   E_FREE(cfdata->bg);
   cfdata->bg = strdup(buff);
}

static void 
_cb_files_changed(void *data, Evas_Object *obj __UNUSED__, void *event __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;
   const char *p = NULL;
   char buff[PATH_MAX];
   size_t len;

   if (!(cfdata = data)) return;
   if (!cfdata->bg) return;
   p = e_widget_flist_real_path_get(cfdata->o_list);
   if (p) 
     {
        if (strncmp(p, cfdata->bg, strlen(p))) return;
     }
   else return;

   len = e_user_dir_concat_static(buff, "backgrounds");
   if (!strncmp(cfdata->bg, buff, len))
     p = cfdata->bg + len + 1;
   else 
     {
        len = e_prefix_data_concat_static(buff, "data/backgrounds");
        if (!strncmp(cfdata->bg, buff, len))
          p = cfdata->bg + len + 1;
        else
          p = cfdata->bg;
     }
   e_widget_flist_select_set(cfdata->o_list, p, 1);
   e_widget_flist_file_show(cfdata->o_list, p);
}

static void 
_cb_files_deleted(void *data, Evas_Object *obj __UNUSED__, void *event __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *sel, *all, *n;
   E_Fm2_Icon_Info *ici, *ic;

   if (!(cfdata = data)) return;
   if (!cfdata->bg) return;
   if (!(all = e_widget_flist_all_list_get(cfdata->o_list))) return;
   if (!(sel = e_widget_flist_selected_list_get(cfdata->o_list))) return;
   if (!(ici = sel->data)) return;
   all = eina_list_data_find_list(all, ici);
   n = eina_list_next(all);
   if (!n) 
     {
        if (!(n = eina_list_prev(all))) return;
     }
   if (!(ic = n->data)) return;
   e_widget_flist_select_set(cfdata->o_list, ic->file, 1);
   e_widget_flist_file_show(cfdata->o_list, ic->file);
   eina_list_free(n);
   evas_object_smart_callback_call(cfdata->o_list, "selection_change", cfdata);
}

static void 
_cb_radio_changed(void *data, Evas_Object *obj __UNUSED__, void *event __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;
   char path[PATH_MAX];

   if (!(cfdata = data)) return;
   if (cfdata->fmdir == 1)
     e_prefix_data_concat_static(path, "data/backgrounds");
   else
     e_user_dir_concat_static(path, "backgrounds");
   e_widget_flist_path_set(cfdata->o_list, path, "/");
}

static void 
_cb_dir_up(void *data1, void *data2 __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = data1)) return;
   e_widget_flist_parent_go(cfdata->o_list);
}
