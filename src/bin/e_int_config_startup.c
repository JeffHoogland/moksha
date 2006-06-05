#include "e.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void         _load_inits(E_Config_Dialog *cfd, Evas_Object *il);
void                _ilist_cb_init_selected(void *data);
static void         _init_file_added(void *data, Ecore_File_Monitor *monitor, Ecore_File_Event event, const char *path);

static Ecore_File_Monitor *_init_file_monitor;

struct _E_Config_Dialog_Data 
{
   int show_splash;
   char *init_default_theme;

   E_Config_Dialog *cfd;
   Evas_Object *il;
};

EAPI E_Config_Dialog *
e_int_config_startup(E_Container *con) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   
   cfd = e_config_dialog_new(con, _("Startup Settings"), NULL, 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   cfdata->show_splash = e_config->show_splash;
   if (e_config->init_default_theme)
     cfdata->init_default_theme = strdup(e_config->init_default_theme);
   else
     cfdata->init_default_theme = NULL;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   cfd->cfdata = cfdata;
   cfdata->cfd = cfd;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if (_init_file_monitor) 
     {
	ecore_file_monitor_del(_init_file_monitor);
	_init_file_monitor = NULL;
     }
   if (cfdata->init_default_theme) free(cfdata->init_default_theme);
   free(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   e_config->show_splash = cfdata->show_splash;
   if (e_config->init_default_theme)
     evas_stringshare_del(e_config->init_default_theme);

   if (!cfdata->init_default_theme[0])
     e_config->init_default_theme = NULL;
   else 
     {
	const char *f = ecore_file_get_file(cfdata->init_default_theme);
	e_config->init_default_theme = evas_stringshare_add(f);
     }
   
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *ot, *ob, *il, *im;
   char path[4096];
   
   ot = e_widget_table_add(evas, 0);
   il = e_widget_ilist_add(evas, 48, 48, &(cfdata->init_default_theme));
   cfdata->il = il;
   e_widget_ilist_selector_set(il, 1);
   e_widget_min_size_set(il, 180, 40);
   
   _load_inits(cfd, il);
   im = cfd->data;
   
   e_widget_table_object_append(ot, il, 0, 0, 1, 2, 1, 1, 1, 1);
   e_widget_table_object_append(ot, im, 1, 0, 1, 2, 1, 1, 1, 1);

   ob = e_widget_check_add(evas, _("Show Splash Screen At Boot"), &(cfdata->show_splash));
   e_widget_table_object_append(ot, ob, 0, 3, 2, 1, 1, 0, 0, 0);
   
   if (_init_file_monitor) 
     {
	ecore_file_monitor_del(_init_file_monitor);
	_init_file_monitor = NULL;
     }
   
   snprintf(path, sizeof(path), "%s/.e/e/init", e_user_homedir_get());
   _init_file_monitor = ecore_file_monitor_add(path, _init_file_added, cfdata);
   
   return ot;
}

static void
_load_inits(E_Config_Dialog *cfd, Evas_Object *il) 
{
   Evas *evas;
   Evas_Object *im, *o, *init_obj;
   Evas_List *init_dirs, *init;
   Ecore_Evas *eebuf;
   Evas *evasbuf;
   int i = 0;
   int selnum = -1;
   const char *s;
   char *c;
   char *homedir;
   
   if (!il) return;

   homedir = e_user_homedir_get();
   
   evas = evas_object_evas_get(il);
   eebuf = ecore_evas_buffer_new(1, 1);
   evasbuf = ecore_evas_get(eebuf);
   
   im = e_widget_preview_add(cfd->dia->win->evas, 320, 
			     (320 * e_zone_current_get(cfd->dia->win->container)->h) /
			     e_zone_current_get(cfd->dia->win->container)->w);

   
   /* Load inits */
   init_dirs = e_path_dir_list_get(path_init);
   for (init = init_dirs; init; init = init->next) 
     {
	E_Path_Dir *d;
	int detected;
	char *init_file;
	Ecore_List *inits;
	
	d = init->data;
	if (!ecore_file_is_dir(d->dir)) continue;
	inits = ecore_file_ls(d->dir);
	if (!inits) continue;
	
	detected = 0;
	if (homedir) 
	  {
	     if (!strncmp(d->dir, homedir, strlen(homedir))) 
	       {
		  e_widget_ilist_header_append(il, NULL, _("Personal"));
		  i++;
		  detected = 1;
	       }
	  }
	if (!detected) 
	  {
	     if (!strncmp(d->dir, e_prefix_data_get(), strlen(e_prefix_data_get())))
	       {
		  e_widget_ilist_header_append(il, NULL, _("System"));
		  i++;
		  detected = 1;
	       }
	  }
	if (!detected) 
	  {
	     e_widget_ilist_header_append(il, NULL, _("Other"));
	     i++;
	     detected = 1;
	  }

	while ((init_file = ecore_list_next(inits))) 
	  {
	     Evas_Object *ic = NULL;
	     char full_path[4096];
	     
	     snprintf(full_path, sizeof(full_path), "%s/%s", d->dir, init_file);
	     if (ecore_file_is_dir(full_path)) continue;
	     if (!e_util_edje_collection_exists(full_path, "init/splash")) continue;

	     if (!e_thumb_exists(c)) 
	       ic = e_thumb_generate_begin(full_path, 48, 48, cfd->dia->win->evas, &ic, NULL, NULL);
	     else 
	       ic = e_thumb_evas_object_get(full_path, cfd->dia->win->evas, 48, 48, 1);
	     
	     e_widget_ilist_append(il, ic, ecore_file_strip_ext(init_file), _ilist_cb_init_selected, cfd, full_path);
	     
	     if ((e_config->init_default_theme) && 
		 (!strcmp(e_config->init_default_theme, init_file))) 
	       {
		  E_Zone *z;
		  
		  z = e_zone_current_get(cfd->dia->win->container);
		  
		  selnum = i;
		  evas_object_del(im);
		  im = e_widget_preview_add(cfd->dia->win->evas, 320, (320 * z->h) / z->w);
		  e_widget_preview_edje_set(im, full_path, "init/splash");
	       }
	     i++;
	  }
	free(init_file);
	ecore_list_destroy(inits);
     }
   evas_list_free(init);
   if (init_dirs) e_path_dir_list_free(init_dirs);
   free(c);
   cfd->data = im;
   e_widget_ilist_go(il);
   if (selnum >= 0) 
     e_widget_ilist_selected_set(il, selnum);
   
   free(homedir);
}

void
_ilist_cb_init_selected(void *data) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_Data *cfdata;
   Evas *evas;
   const char *init;
   const char *f;
   
   cfd = data;
   cfdata = cfd->cfdata;
   evas = cfd->dia->win->evas;

   if (!cfdata->init_default_theme[0])
     init = e_path_find(path_init, "init.edj");
   else 
     {
	f = ecore_file_get_file(cfd->cfdata->init_default_theme);
	init = e_path_find(path_init, f);
     }
   e_widget_preview_edje_set(cfd->data, init, "init/splash");
}

static void 
_init_file_added(void *data, Ecore_File_Monitor *monitor, Ecore_File_Event event, const char *path) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_Data *cfdata;
   Evas *evas;
   Evas_Object *il, *ic;
   char *file, *noext;
   
   cfdata = data;
   if (!cfdata) return;
   
   il = cfdata->il;
   if (!il) return;
   
   cfd = cfdata->cfd;
   if (!cfd) return;
   
   evas = e_win_evas_get(cfd->dia->win);
   
   file = (char *)ecore_file_get_file((char *)path);
   noext = ecore_file_strip_ext(file);
   
   if (event == ECORE_FILE_EVENT_CREATED_FILE) 
     {
	if (e_util_edje_collection_exists((char *)path, "init/splash")) 
	  {
	     ic = edje_object_add(evas);
	     e_util_edje_icon_set(ic, "enlightenment/run");
	     e_widget_ilist_append(il, ic, noext, _ilist_cb_init_selected, cfd, (char *)path);
	  }
     }
   else if (event == ECORE_FILE_EVENT_DELETED_FILE)
     e_widget_ilist_remove_label(il, noext);
}
