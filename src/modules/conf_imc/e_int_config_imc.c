/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_int_config_imc.h"
#include "e_int_config_imc_import.h"

typedef struct _E_Imc_Update_Data E_Imc_Update_Data;

static void        *_create_data             (E_Config_Dialog *cfd);
static void         _free_data               (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data        (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _advanced_apply_data     (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets    (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

/* Basic Callbacks */
static int	    _basic_list_sort_cb	     (const void *d1, const void *d2);
static void	    _e_imc_disable_change_cb (void *data, Evas_Object *obj);
static void	    _e_imc_list_change_cb    (void *data, Evas_Object *obj);
static void	    _e_imc_setup_cb          (void *data, void *data2);

/* Advanced Callbacks */
static void         _cb_dir                  (void *data, Evas_Object *obj);
static void         _cb_button_up            (void *data1, void *data2);
static void	    _cb_new		     (void *data, void *data2);

static void	    _cb_files_changed	     (void *data, Evas_Object *obj, void *event_info);
static void         _cb_files_selection_change(void *data, Evas_Object *obj, void *event_info);
static void         _cb_files_files_changed  (void *data, Evas_Object *obj, void *event_info);
static void	    _cb_files_selected       (void *data, Evas_Object *obj, void *event_info);
static void         _cb_files_files_deleted  (void *data, Evas_Object *obj, void *event_info);
static void         _e_imc_adv_setup_cb      (void *data, void *data2);

static void         _e_imc_change_enqueue    (E_Config_Dialog_Data *cfdata);
static void         _e_imc_entry_change_cb   (void *data, Evas_Object *obj);
static void	    _e_imc_form_fill         (E_Config_Dialog_Data *cfdata);
static const char*  _e_imc_file_name_new_get (void);
static Eina_Bool    _change_hash_free_cb     (const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata __UNUSED__);
static Eina_Bool    _change_hash_apply_cb    (const Eina_Hash *hash __UNUSED__, const void *key, void *data, void *fdata __UNUSED__);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;

   /* Maluable Widgets */
   Evas_Object *o_personal;
   Evas_Object *o_system;
   Evas_Object *o_up_button;
   Evas_Object *o_fm; /* File manager */
   Evas_Object *o_frame; /* scrollpane for file manager*/

   const char *imc_current;
   Eina_Hash *imc_basic_map;

   int imc_disable; /* 0=enable, 1=disable */
   int fmdir; /* 0=Local, 1=System*/
   struct
     {
	int dirty;
	char *e_im_name;
	char *e_im_exec;
	char *e_im_setup_exec;

	char *gtk_im_module;
	char *qt_im_module;
	char *xmodifiers;
     } imc;

   Eina_Hash *imc_change_map;

   struct
     {
	Evas_Object *imc_basic_list;
	Evas_Object *imc_basic_disable;
	Evas_Object *imc_basic_setup;

	Evas_Object *imc_advanced_disable;
	Evas_Object *imc_advanced_setup;

	Evas_Object *e_im_name;
	Evas_Object *e_im_exec;
	Evas_Object *e_im_setup_exec;
	Evas_Object *gtk_im_module;
	Evas_Object *qt_im_module;
	Evas_Object *xmodifiers;
     } gui;

   E_Win *win_import;
};

EAPI E_Config_Dialog *
e_int_config_imc(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_imc_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   v->advanced.apply_cfdata   = _advanced_apply_data;
   v->basic.create_widgets    = _basic_create_widgets;
   v->basic.apply_cfdata      = _basic_apply_data;

   cfd = e_config_dialog_new(con,
			     _("Input Method Settings"),
			    "E", "_config_imc_dialog",
			     "preferences-imc", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   if (e_config->input_method)
     cfdata->imc_current = eina_stringshare_add(e_config->input_method);

   if (cfdata->imc_current)
     {
	const char *path;

	path = e_intl_imc_system_path_get();
	if (!strncmp(cfdata->imc_current, path, strlen(path)))
	  cfdata->fmdir = 1;
	cfdata->imc_disable = 0;
     }
   else
     cfdata->imc_disable = 1;
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

static Eina_Bool
_change_hash_free_cb(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata __UNUSED__)
{
   E_Input_Method_Config *imc;

   imc = data;
   e_intl_input_method_config_free(imc);
   return 1;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->win_import)
     e_int_config_imc_import_del(cfdata->win_import);
   eina_stringshare_del(cfdata->imc_current);

   if (cfdata->imc_basic_map)
     {
	eina_hash_foreach(cfdata->imc_basic_map, _change_hash_free_cb, NULL);
	eina_hash_free(cfdata->imc_basic_map);
     }

   if (cfdata->imc_change_map)
     {
	eina_hash_foreach(cfdata->imc_change_map, _change_hash_free_cb, NULL);
	eina_hash_free(cfdata->imc_change_map);
     }
   cfdata->imc_change_map = NULL;

   E_FREE(cfdata->imc.e_im_name);
   E_FREE(cfdata->imc.e_im_exec);
   E_FREE(cfdata->imc.e_im_setup_exec);
   E_FREE(cfdata->imc.gtk_im_module);
   E_FREE(cfdata->imc.qt_im_module);
   E_FREE(cfdata->imc.xmodifiers);
   E_FREE(cfdata);
}

/*** Start Basic Dialog Logic ***/
static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->imc_current)
     {
	if (e_config->input_method)
	  {
	     eina_stringshare_del(e_config->input_method);
	     e_config->input_method = NULL;
	  }

	if (!cfdata->imc_disable)
	  e_config->input_method = eina_stringshare_ref(cfdata->imc_current);

	e_intl_input_method_set(e_config->input_method);
     }

   e_config_save_queue();
   return 1;
}

static int
_basic_list_sort_cb(const void *d1, const void *d2)
{
   if (!d1) return 1;
   if (!d2) return -1;

   return (strcmp((const char*)d1, (const char*)d2));
}

static void
_e_imc_disable_change_cb(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
}

void
_e_imc_setup_button_toggle(Evas_Object *button, E_Input_Method_Config *imc)
{
   if (imc)
     {
	int flag;

	flag = (!imc->e_im_setup_exec) || (!imc->e_im_setup_exec[0]);
	e_widget_disabled_set(button, flag);
     }
   else
     e_widget_disabled_set(button, 1);
}

static void
_e_imc_list_change_cb(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;
   E_Input_Method_Config *imc;

   cfdata = data;
   e_widget_check_checked_set(cfdata->gui.imc_basic_disable, 0);

   if (cfdata->imc_current)
     {
	imc = eina_hash_find(cfdata->imc_basic_map, cfdata->imc_current);
	_e_imc_setup_button_toggle(cfdata->gui.imc_basic_setup, imc);
     }
}

static void
_e_imc_setup_cb(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (cfdata->imc_current && cfdata->imc_basic_map)
     {
	E_Input_Method_Config *imc;

	imc = eina_hash_find(cfdata->imc_basic_map, cfdata->imc_current);

	if ((imc) && (imc->e_im_setup_exec))
	  {
	     Ecore_Exe *exe;
	     const char *cmd;

	     cmd = imc->e_im_setup_exec;

	     e_util_library_path_strip();
	     exe = ecore_exe_run(cmd, NULL);
	     e_util_library_path_restore();

	     if (!exe)
	       {
   		  e_util_dialog_show(_("Run Error"),
		_( "Enlightenment was unable to fork a child process:<br>"
	   	   "<br>"
		   "%s<br>"),
		   cmd);
	       }
	  }
     }
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;
   int i;
   Eina_List *imc_basic_list;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_frametable_add(evas, _("Input Method Selector"), 0);

   /* Disable imc checkbox */
   ob = e_widget_check_add(evas, _("Use No Input Method"), &(cfdata->imc_disable));
   cfdata->gui.imc_basic_disable = ob;
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 0, 1, 0);

   /* Configure imc button */
   ob = e_widget_button_add(evas, _("Setup Selected Input Method"), "configure", _e_imc_setup_cb, cfdata, NULL);
   cfdata->gui.imc_basic_setup = ob;
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 1, 1, 1, 0);

   /* Input method List */
   ob = e_widget_ilist_add(evas, 16, 16, &(cfdata->imc_current));
   e_widget_on_change_hook_set(ob, _e_imc_list_change_cb, cfdata);
   e_widget_min_size_set(ob, 175, 175);
   cfdata->gui.imc_basic_list = ob;

   evas_event_freeze(evas_object_evas_get(ob));
   edje_freeze();
   e_widget_ilist_freeze(ob);

   imc_basic_list = e_intl_input_method_list();
   /* Sort basic input method list */
   imc_basic_list = eina_list_sort(imc_basic_list,
				   eina_list_count(imc_basic_list),
				   _basic_list_sort_cb);

   if (cfdata->imc_basic_map)
     {
	eina_hash_foreach(cfdata->imc_basic_map, _change_hash_free_cb, NULL);
	eina_hash_free(cfdata->imc_basic_map);
	cfdata->imc_basic_map = NULL;
     }

   i = 0;
   while (imc_basic_list)
     {
	E_Input_Method_Config *imc;
	Eet_File *imc_ef;
	char *imc_path;

	imc_path = imc_basic_list->data;
	imc_ef = eet_open(imc_path, EET_FILE_MODE_READ);
	if (imc_ef)
	  {
	     imc = e_intl_input_method_config_read(imc_ef);
	     eet_close(imc_ef);

	     if (imc && imc->e_im_name)
	       {
		  Evas_Object *icon;

		  icon = NULL;
		  if (imc->e_im_setup_exec)
		    {
		       Efreet_Desktop *desktop;
		       desktop = efreet_util_desktop_exec_find(imc->e_im_setup_exec);
		       if (desktop)
			 icon = e_util_desktop_icon_add(desktop, 48, evas);
		    }

		  e_widget_ilist_append(cfdata->gui.imc_basic_list, icon, imc->e_im_name, NULL, NULL, imc_path);
		  if (cfdata->imc_current && !strncmp(imc_path, cfdata->imc_current, eina_stringshare_strlen(cfdata->imc_current)))
		    e_widget_ilist_selected_set(cfdata->gui.imc_basic_list, i);
		  i++;

		  if (!cfdata->imc_basic_map)
		    cfdata->imc_basic_map = eina_hash_string_superfast_new(NULL);
		  eina_hash_add(cfdata->imc_basic_map, imc_path, imc);
	       }
	  }
	free(imc_path);
	imc_basic_list = eina_list_remove_list(imc_basic_list, imc_basic_list);
     }

   _e_imc_setup_button_toggle(cfdata->gui.imc_basic_setup, eina_hash_find(cfdata->imc_basic_map, cfdata->imc_current));

   e_widget_ilist_go(ob);
   e_widget_ilist_thaw(ob);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(ob));

   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 1, 1, 1);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   e_dialog_resizable_set(cfd->dia, 1);
   return o;
}
/*** End Basic Dialog Logic ***/

/*** Start Advanced Dialog Logic ***/
static Eina_Bool
_change_hash_apply_cb(const Eina_Hash *hash __UNUSED__, const void *key, void *data, void *fdata __UNUSED__)
{
   E_Input_Method_Config *imc;
   Eet_File *ef;

   imc = data;

   if (ecore_file_exists(key))
     {
	ef = eet_open(key, EET_FILE_MODE_WRITE);
	if (ef)
	  {
	     e_intl_input_method_config_write(ef, imc);
	     eet_close(ef);
	  }
     }

   e_intl_input_method_config_free(imc);

   return 1;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* inherit basic apply functionality */
   _basic_apply_data(cfd, cfdata);

   /* Save all file changes */
   if (cfdata->imc_current)
     _e_imc_change_enqueue(cfdata);

   if (cfdata->imc_change_map)
     {
	eina_hash_foreach(cfdata->imc_change_map, _change_hash_apply_cb, NULL);
	eina_hash_free(cfdata->imc_change_map);
     }
   cfdata->imc_change_map = NULL;
   return 1;
}

/** Start Button Callbacks **/

/* Radio Toggled */
static void
_cb_dir(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;
   const char *path;

   cfdata = data;
   if (cfdata->fmdir == 1)
     path = e_intl_imc_system_path_get();
   else
     path = e_intl_imc_personal_path_get();
   e_fm2_path_set(cfdata->o_fm, path, "/");
}

/* Directory Navigator */
static void
_cb_button_up(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (cfdata->o_fm)
     e_fm2_parent_go(cfdata->o_fm);
   if (cfdata->o_frame)
     e_widget_scrollframe_child_pos_set(cfdata->o_frame, 0, 0);
}

/* Entry chagned */
static void
_e_imc_entry_change_cb(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   cfdata->imc.dirty = 1;
}

/* Creating a new IMC */
static void
_cb_new(void *data, void *data2)
{
   E_Input_Method_Config *imc_new;
   Eet_File *ef;
   const char *file;
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   imc_new = E_NEW(E_Input_Method_Config, 1);
   imc_new->version = E_INTL_INPUT_METHOD_CONFIG_VERSION;

   file = _e_imc_file_name_new_get();

   if (file)
     {
	ef = eet_open(file, EET_FILE_MODE_WRITE);
	if (ef)
	  {
	     e_intl_input_method_config_write(ef, imc_new);
	     eet_close(ef);
	     e_int_config_imc_update(cfdata->cfd, file);
	  }
     }
   free(imc_new);
}

static void
_e_imc_adv_setup_cb(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (cfdata->imc.e_im_setup_exec)
     {
	     Ecore_Exe *exe;
	     char *cmd;

	     cmd = cfdata->imc.e_im_setup_exec;

	     e_util_library_path_strip();
	     exe = ecore_exe_run(cmd, NULL);
	     e_util_library_path_restore();

	     if (!exe)
	       {
   		  e_util_dialog_show(_("Run Error"),
		_( "Enlightenment was unable to fork a child process:<br>"
	   	   "<br>"
		   "%s<br>"),
		   cmd);
	       }
     }
}

/** End Button Callbacks **/

/** Start IMC FM2 Callbacks **/
static void
_cb_files_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (!cfdata->o_fm) return;
   if (!e_fm2_has_parent_get(cfdata->o_fm))
     {
	if (cfdata->o_up_button)
	  e_widget_disabled_set(cfdata->o_up_button, 1);
     }
   else
     {
	if (cfdata->o_up_button)
	  e_widget_disabled_set(cfdata->o_up_button, 0);
     }
   if (cfdata->o_frame)
     e_widget_scrollframe_child_pos_set(cfdata->o_frame, 0, 0);
}

static void
_cb_files_selection_change(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *selected;
   E_Fm2_Icon_Info *ici;
   const char *realpath;
   char buf[4096];

   cfdata = data;
   if (!cfdata->o_fm) return;
   selected = e_fm2_selected_list_get(cfdata->o_fm);
   if (!selected) return;

   if (cfdata->imc_current)
     {
	_e_imc_change_enqueue(cfdata);
	eina_stringshare_del(cfdata->imc_current);
	cfdata->imc_current = NULL;
     }

   ici = selected->data;
   realpath = e_fm2_real_path_get(cfdata->o_fm);
   if (!strcmp(realpath, "/"))
     snprintf(buf, sizeof(buf), "/%s", ici->file);
   else
     snprintf(buf, sizeof(buf), "%s/%s", realpath, ici->file);
   eina_list_free(selected);
   if (ecore_file_is_dir(buf)) return;
   cfdata->imc_current = eina_stringshare_add(buf);
   _e_imc_form_fill(cfdata);
   if (cfdata->o_frame)
     e_widget_change(cfdata->o_frame);
}

static void
_cb_files_selected(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
}

static void
_cb_files_files_changed(void *data, Evas_Object *obj, void *event_info)
{
   const char *buf;
   const char *p;
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (!cfdata->imc_current) return;
   if (!cfdata->o_fm) return;
   p = e_fm2_real_path_get(cfdata->o_fm);
   if (p)
     {
	if (strncmp(p, cfdata->imc_current, strlen(p))) return;
     }

   buf = e_intl_imc_personal_path_get();
   if (!strncmp(cfdata->imc_current, buf, strlen(buf)))
     p = cfdata->imc_current + strlen(buf) + 1;
   else
     {
	buf = e_intl_imc_system_path_get();
	if (!strncmp(cfdata->imc_current, buf, strlen(buf)))
	  p = cfdata->imc_current + strlen(buf) + 1;
     }
   if (!p) return;
   e_fm2_select_set(cfdata->o_fm, p, 1);
   e_fm2_file_show(cfdata->o_fm, p);
}

static void
_cb_files_files_deleted(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *sel, *all, *n;
   E_Fm2_Icon_Info *ici, *ic;

   cfdata = data;
   if (!cfdata->imc_current) return;
   if (!cfdata->o_fm) return;

   all = e_fm2_all_list_get(cfdata->o_fm);
   if (!all) return;
   sel = e_fm2_selected_list_get(cfdata->o_fm);
   if (!sel) return;

   ici = sel->data;

   all = eina_list_data_find_list(all, ici);
   n = eina_list_next(all);
   if (!n)
     {
	n = eina_list_prev(all);
	if (!n) return;
     }

   ic = n->data;
   if (!ic) return;

   e_fm2_select_set(cfdata->o_fm, ic->file, 1);
   e_fm2_file_show(cfdata->o_fm, ic->file);

   eina_list_free(n);

   evas_object_smart_callback_call(cfdata->o_fm, "selection_change", cfdata);
}
/** End IMC FM2 Callbacks **/

/** Start IMC Utility Functions **/
/* When updating the selection call this to fill in the form */
static void
_e_imc_form_fill(E_Config_Dialog_Data *cfdata)
{
   Eet_File *imc_file;
   E_Input_Method_Config *imc;
   int imc_free;

   if (!cfdata->imc_current)
     {
	e_widget_disabled_set(cfdata->gui.imc_advanced_setup, 1);
	return;
     }

   imc_free = 0;
   imc = eina_hash_find(cfdata->imc_change_map, cfdata->imc_current);

   if (!imc)
     {
	imc_free = 1;
	imc_file = eet_open(cfdata->imc_current, EET_FILE_MODE_READ);
	if (imc_file)
	  {
	     imc = e_intl_input_method_config_read(imc_file);
	     eet_close(imc_file);
	  }
     }

   _e_imc_setup_button_toggle(cfdata->gui.imc_advanced_setup, imc);

   if (imc)
     {
	e_widget_entry_text_set(cfdata->gui.e_im_name, imc->e_im_name);
	e_widget_entry_text_set(cfdata->gui.e_im_exec, imc->e_im_exec);
	e_widget_entry_text_set(cfdata->gui.e_im_setup_exec, imc->e_im_setup_exec);
	e_widget_entry_text_set(cfdata->gui.gtk_im_module, imc->gtk_im_module);
	e_widget_entry_text_set(cfdata->gui.qt_im_module, imc->qt_im_module);
	e_widget_entry_text_set(cfdata->gui.xmodifiers, imc->xmodifiers);

	e_widget_entry_readonly_set(cfdata->gui.e_im_name, cfdata->fmdir);
	e_widget_entry_readonly_set(cfdata->gui.e_im_exec, cfdata->fmdir);
	e_widget_entry_readonly_set(cfdata->gui.e_im_setup_exec, cfdata->fmdir);
	e_widget_entry_readonly_set(cfdata->gui.gtk_im_module, cfdata->fmdir);
	e_widget_entry_readonly_set(cfdata->gui.qt_im_module, cfdata->fmdir);
	e_widget_entry_readonly_set(cfdata->gui.xmodifiers, cfdata->fmdir);
	if (imc_free) e_intl_input_method_config_free(imc);
     }
   e_widget_check_checked_set(cfdata->gui.imc_advanced_disable, 0);
   cfdata->imc.dirty = 0;
}

/* Remember changes in memory until we click apply */
static void
_e_imc_change_enqueue(E_Config_Dialog_Data *cfdata)
{
   if (cfdata->imc.dirty)
     {
	E_Input_Method_Config *imc_update;
	E_Input_Method_Config *imc_update_old;

	imc_update = E_NEW(E_Input_Method_Config, 1);

	imc_update->version = E_INTL_INPUT_METHOD_CONFIG_VERSION;

	/* TODO: need to only add if the string is not empty */
	imc_update->e_im_name = eina_stringshare_add(cfdata->imc.e_im_name);
	imc_update->e_im_exec = eina_stringshare_add(cfdata->imc.e_im_exec);
	imc_update->e_im_setup_exec = eina_stringshare_add(cfdata->imc.e_im_setup_exec);
	imc_update->gtk_im_module = eina_stringshare_add(cfdata->imc.gtk_im_module);
        imc_update->qt_im_module = eina_stringshare_add(cfdata->imc.qt_im_module);
        imc_update->xmodifiers = eina_stringshare_add(cfdata->imc.xmodifiers);

	/* look for changes to this file and remove them */
	imc_update_old = eina_hash_find(cfdata->imc_change_map, cfdata->imc_current);
	if (imc_update_old)
	  {
	     eina_hash_del(cfdata->imc_change_map, cfdata->imc_current, NULL);
	     e_intl_input_method_config_free(imc_update_old);

	  }
	if (!cfdata->imc_change_map)
	  cfdata->imc_change_map = eina_hash_string_superfast_new(NULL);
	eina_hash_add(cfdata->imc_change_map, cfdata->imc_current, imc_update);
     }
}

/* Get a new filename, and create the new file */
static const char*
_e_imc_file_name_new_get(void)
{
   char path[4096];
   int i;

   for (i = 0; i < 32; i++)
     {
	snprintf(path, sizeof(path), "%s/new_input_method-%02d.imc",
		 e_intl_imc_personal_path_get(), i);
	if (!ecore_file_exists(path))
	  return eina_stringshare_add(path);
     }

   return NULL;
}
/** End IMC Utility Functions **/

/** Import Dialog **/
EAPI void
e_int_config_imc_import_done(E_Config_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = dia->cfdata;
   cfdata->win_import = NULL;
}

static void
_cb_import(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data1;
   if (cfdata->win_import)
     e_win_raise(cfdata->win_import);
   else
     cfdata->win_import = e_int_config_imc_import(cfdata->cfd);
}

EAPI void
e_int_config_imc_update(E_Config_Dialog *dia, const char *file)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = dia->cfdata;
   cfdata->fmdir = 1;
   eina_stringshare_del(cfdata->imc_current);
   cfdata->imc_current = eina_stringshare_add(file);
   e_widget_radio_toggle_set(cfdata->o_personal, 1);

   if (cfdata->o_fm)
     e_fm2_path_set(cfdata->o_fm, e_intl_imc_personal_path_get(), "/");
   _e_imc_form_fill(cfdata);
   if (cfdata->o_frame)
     e_widget_change(cfdata->o_frame);
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ot, *of, *il, *ol;
   const char *path;
   E_Fm2_Config fmc;
   E_Radio_Group *rg;

   ot = e_widget_table_add(evas, 0);
   ol = e_widget_table_add(evas, 0);
   il = e_widget_table_add(evas, 1);

   rg = e_widget_radio_group_new(&(cfdata->fmdir));

   o = e_widget_radio_add(evas, _("Personal"), 0, rg);
   e_widget_table_object_append(il, o, 0, 0, 1, 1, 1, 1, 0, 0);
   e_widget_on_change_hook_set(o, _cb_dir, cfdata);
   cfdata->o_personal = o;

   o = e_widget_radio_add(evas, _("System"), 1, rg);
   e_widget_table_object_append(il, o, 1, 0, 1, 1, 1, 1, 0, 0);
   e_widget_on_change_hook_set(o, _cb_dir, cfdata);
   cfdata->o_system = o;

   e_widget_table_object_append(ol, il, 0, 0, 1, 1, 0, 0, 0, 0);

   o = e_widget_button_add(evas, _("Go up a Directory"), "go-up", _cb_button_up, cfdata, NULL);
   cfdata->o_up_button = o;
   e_widget_table_object_append(ol, o, 0, 1, 1, 1, 0, 0, 0, 0);

   if (cfdata->fmdir == 1)
     path = e_intl_imc_system_path_get();
   else
     path = e_intl_imc_personal_path_get();

   o = e_fm2_add(evas);
   cfdata->o_fm = o;
   memset(&fmc, 0, sizeof(E_Fm2_Config));
   fmc.view.mode = E_FM2_VIEW_MODE_LIST;
   fmc.view.open_dirs_in_place = 1;
   fmc.view.selector = 1;
   fmc.view.single_click = 0;
   fmc.view.no_subdir_jump = 0;
   fmc.icon.list.w = 16;
   fmc.icon.list.h = 16;
   fmc.icon.fixed.w = 1;
   fmc.icon.fixed.h = 1;
   fmc.icon.extension.show = 0;
   fmc.icon.key_hint = NULL;
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 0;
   fmc.list.sort.dirs.last = 1;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(o, &fmc);
   e_fm2_icon_menu_flags_set(o, E_FM2_MENU_NO_SHOW_HIDDEN);

   evas_object_smart_callback_add(o, "dir_changed",
				  _cb_files_changed, cfdata);
   evas_object_smart_callback_add(o, "selection_change",
				  _cb_files_selection_change, cfdata);
   evas_object_smart_callback_add(o, "selected",
				  _cb_files_selected, cfdata);
   evas_object_smart_callback_add(o, "changed",
				  _cb_files_files_changed, cfdata);
   evas_object_smart_callback_add(o, "files_deleted",
				  _cb_files_files_deleted, cfdata);
   cfdata->o_frame = NULL;
   e_fm2_path_set(o, path, "/");

   of = e_widget_scrollframe_pan_add(evas, o,
				     e_fm2_pan_set,
				     e_fm2_pan_get,
				     e_fm2_pan_max_get,
				     e_fm2_pan_child_size_get);
   cfdata->o_frame = of;
   e_widget_min_size_set(of, 160, 160);
   e_widget_table_object_append(ol, of, 0, 2, 1, 1, 1, 1, 1, 1);
   e_widget_table_object_append(ot, ol, 0, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_list_add(evas, 0, 0);

   /* Disable imc checkbox */
   /* il( o[Check], ol( o[Button], o[Button] ) ) */
   il = e_widget_list_add(evas, 0, 1);

   o = e_widget_check_add(evas, _("Use No Input Method"), &(cfdata->imc_disable));
   e_widget_on_change_hook_set(o, _e_imc_disable_change_cb, cfdata);
   cfdata->gui.imc_advanced_disable = o;
   e_widget_list_object_append(il, o, 1, 0, 0.5);

   ol = e_widget_list_add(evas, 1, 1);

   o = e_widget_button_add(evas, _("New"), "document-new", _cb_new, cfdata, NULL);
   e_widget_list_object_append(ol, o, 1, 0, 0.5);
   o = e_widget_button_add(evas, _("Import..."), "preferences-imc", _cb_import, cfdata, NULL);
   e_widget_list_object_append(ol, o, 1, 0, 0.5);
   e_widget_list_object_append(il, ol, 1, 0, 0.5);
   e_widget_list_object_append(of, il, 1, 0, 0.0);

   ol = e_widget_frametable_add(evas, _("Input Method Parameters"), 0);
   e_widget_frametable_content_align_set(ol, 0.0, 0.0);

   o = e_widget_label_add(evas, _("Name"));
   e_widget_frametable_object_append(ol, o, 0, 0, 1, 1, 1, 1, 0, 0);
   o = e_widget_entry_add(evas, &(cfdata->imc.e_im_name), NULL, NULL, NULL);
   e_widget_on_change_hook_set(o, _e_imc_entry_change_cb, cfdata);
   cfdata->gui.e_im_name = o;
   e_widget_frametable_object_append(ol, o, 1, 0, 1, 1, 1, 1, 1, 0);

   o = e_widget_label_add(evas, _("Execute Command"));
   e_widget_frametable_object_append(ol, o, 0, 1, 1, 1, 1, 1, 0, 0);
   o = e_widget_entry_add(evas, &(cfdata->imc.e_im_exec), NULL, NULL, NULL);
   e_widget_on_change_hook_set(o, _e_imc_entry_change_cb, cfdata);
   cfdata->gui.e_im_exec = o;
   e_widget_frametable_object_append(ol, o, 1, 1, 1, 1, 1, 1, 1, 0);

   o = e_widget_label_add(evas, _("Setup Command"));
   e_widget_frametable_object_append(ol, o, 0, 2, 1, 1, 1, 1, 0, 0);
   o = e_widget_entry_add(evas, &(cfdata->imc.e_im_setup_exec), NULL, NULL, NULL);
   e_widget_on_change_hook_set(o, _e_imc_entry_change_cb, cfdata);
   cfdata->gui.e_im_setup_exec = o;
   e_widget_frametable_object_append(ol, o, 1, 2, 1, 1, 1, 1, 1, 0);

   e_widget_list_object_append(of, ol, 0, 1, 0.5);

   ol = e_widget_frametable_add(evas, _("Exported Environment Variables"), 0);
   e_widget_frametable_content_align_set(ol, 0.0, 0.0);

   o = e_widget_label_add(evas, "GTK_IM_MODULE");
   e_widget_frametable_object_append(ol, o, 0, 0, 1, 1, 1, 1, 0, 0);
   o = e_widget_entry_add(evas, &(cfdata->imc.gtk_im_module), NULL, NULL, NULL);
   e_widget_on_change_hook_set(o, _e_imc_entry_change_cb, cfdata);
   cfdata->gui.gtk_im_module = o;
   e_widget_frametable_object_append(ol, o, 1, 0, 1, 1, 1, 1, 1, 0);

   o = e_widget_label_add(evas, "QT_IM_MODULE");
   e_widget_frametable_object_append(ol, o, 0, 1, 1, 1, 1, 1, 0, 0);
   o = e_widget_entry_add(evas, &(cfdata->imc.qt_im_module), NULL, NULL, NULL);
   e_widget_on_change_hook_set(o, _e_imc_entry_change_cb, cfdata);
   cfdata->gui.qt_im_module = o;
   e_widget_frametable_object_append(ol, o, 1, 1, 1, 1, 1, 1, 1, 0);

   o = e_widget_label_add(evas, "XMODIFIERS");
   e_widget_frametable_object_append(ol, o, 0, 2, 1, 1, 1, 1, 0, 0);
   o = e_widget_entry_add(evas, &(cfdata->imc.xmodifiers), NULL, NULL, NULL);
   e_widget_on_change_hook_set(o, _e_imc_entry_change_cb, cfdata);
   cfdata->gui.xmodifiers = o;
   e_widget_frametable_object_append(ol, o, 1, 2, 1, 1, 1, 1, 1, 0);

   e_widget_list_object_append(of, ol, 0, 1, 0.5);
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);

   o = e_widget_button_add(evas, _("Setup Selected Input Method"), "configure", _e_imc_adv_setup_cb, cfdata, NULL);
   e_widget_table_object_append(ot, o, 0, 1, 1, 1, 1, 1, 1, 0);
   cfdata->gui.imc_advanced_setup = o;

   e_dialog_resizable_set(cfd->dia, 1);

   _e_imc_form_fill(cfdata);
   return ot;
}
