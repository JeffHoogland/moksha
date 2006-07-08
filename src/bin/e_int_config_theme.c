/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* PROTOTYPES - same all the time */
typedef struct _E_Cfg_Theme_Data E_Cfg_Theme_Data;

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

/* Actual config data we will be playing with whil the dialog is active */
struct _E_Config_Dialog_Data
{
   /*- BASIC -*/
   char *theme;
   char *current_theme;
   /*- ADVANCED -*/
};

struct _E_Cfg_Theme_Data
{
   E_Config_Dialog *cfd;
   char *file;
   char *theme;
};

/* a nice easy setup function that does the dirty work */
EAPI E_Config_Dialog *
e_int_config_theme(E_Container *con)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   
   /* methods */
   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->basic.apply_cfdata      = _basic_apply_data;
   v->basic.create_widgets    = _basic_create_widgets;
   v->override_auto_apply = 1;
   
   /* create config diaolg for NULL object/data */
   cfd = e_config_dialog_new(con, _("Theme Selector"), "enlightenment/themes", 0, v, NULL);
   return cfd;
}

/**--CREATE--**/
static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   /* get current theme */
   E_Config_Theme * c;
   c = e_theme_config_get("theme");
   cfdata->current_theme = strdup(c->file);
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   /* Create cfdata - cfdata is a temporary block of config data that this
    * dialog will be dealing with while configuring. it will be applied to
    * the running systems/config in the apply methods
    */
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Free the cfdata */  
   free(cfdata->current_theme);
   free(cfdata);
}

/**--APPLY--**/
static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_Action *a;
   
   /* Actually take our cfdata settings and apply them in real life */
   e_theme_config_set("theme", cfdata->theme);
   e_config_save_queue();

   a = e_action_find("restart");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);   
   return 1; /* Apply was OK */
}

void
_e_config_theme_cb_standard(void *data)
{
   E_Cfg_Theme_Data *d;
   E_Config_Dialog_Data *cfdata;
   const char *tmp;
   
   d = data;
   e_widget_preview_edje_set(d->cfd->data, d->file, "desktop/background");
   
   cfdata = d->cfd->cfdata;
   if (cfdata->current_theme) 
     {
	tmp = ecore_file_get_file(d->file);
	if (!strcmp(tmp, cfdata->current_theme)) 
	  {
	     e_dialog_button_disable_num_set(d->cfd->dia, 0, 1);
	     e_dialog_button_disable_num_set(d->cfd->dia, 1, 1);	
	  }
     }
}

/**--GUI--**/
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for a basic dialog */
   Evas_Object *o, *o2, *im = NULL;
   Evas_Object *il;
   char *homedir;
   Evas_Object *theme;
   char fulltheme[PATH_MAX];
   int i = 0, selnum = -1;
   Ecore_Evas *eebuf;
   Evas *evasbuf;
   Evas_List *l, *paths;
   
   theme = edje_object_add(evas);
   
   _fill_data(cfdata);
   cfdata->theme = NULL;
   
   o = e_widget_table_add(evas, 0);
   
   il = e_widget_ilist_add(evas, 48, 48, &(cfdata->theme));

   eebuf = ecore_evas_buffer_new(1, 1);
   evasbuf = ecore_evas_get(eebuf);
   o2 = edje_object_add(evasbuf);

   paths = e_path_dir_list_get(path_themes);
   for (l = paths; l; l = l->next)
     {
	Ecore_List *themes;
	char *themefile;
	E_Path_Dir *d;
	int detected;
	
	d = l->data;
	if (!ecore_file_is_dir(d->dir)) continue;
	themes = ecore_file_ls(d->dir);
	if (!themes) continue;
	
	detected = 0;
	homedir = e_user_homedir_get();
	if (homedir)
	  {
	     if (!strncmp(d->dir, homedir, strlen(homedir)))
	       {
		  e_widget_ilist_header_append(il, NULL, _("Personal"));
		  i++;
		  detected = 1;
	       }
	     free(homedir);
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
	
	while ((themefile = ecore_list_next(themes)))
	  {
	     snprintf(fulltheme, sizeof(fulltheme), "%s/%s", d->dir, themefile);
	     if (ecore_file_is_dir(fulltheme)) continue;
	     
	     /* minimum theme requirements */
	     if (edje_object_file_set(o2, fulltheme, "desktop/background"))
	       {
		  Evas_Object *o3 = NULL;
		  char *noext;
		  E_Cfg_Theme_Data *cb_data;
		  
		  o3 = e_thumb_icon_add(cfd->dia->win->evas);
		  e_thumb_icon_file_set(o3, fulltheme, "desktop/background");
		  e_thumb_icon_size_set(o3, 64,
					 (64 * e_zone_current_get(cfd->dia->win->container)->h) /
					 e_zone_current_get(cfd->dia->win->container)->w);
		  e_thumb_icon_begin(o3);
		  
		  noext = ecore_file_strip_ext(themefile);
		  
		  cb_data = E_NEW(E_Cfg_Theme_Data, 1);
		  cb_data->cfd = cfd;
		  cb_data->file = strdup(fulltheme);
		  cb_data->theme = strdup(themefile);
		  
		  e_widget_ilist_append(il, o3, noext,
					_e_config_theme_cb_standard, cb_data,
					themefile);
		  if (!(strcmp(themefile, cfdata->current_theme)))
		    {
		       selnum = i;
		       im = e_widget_preview_add(evas, 320, 
						 (320 * e_zone_current_get(cfd->dia->win->container)->h) /
						 e_zone_current_get(cfd->dia->win->container)->w);
		       e_widget_preview_edje_set(im, fulltheme, "desktop/background");
		    }
		  free(noext);
		  i++;
	       }
	  }
	free(themefile);
	ecore_list_destroy(themes);
     }
   if (paths) e_path_dir_list_free(paths);
   evas_object_del(o2);
   ecore_evas_free(eebuf);

   e_widget_ilist_go(il);   
   e_widget_min_size_set(il, 180, 40);
   e_widget_table_object_append(o, il, 0, 0, 1, 2, 1, 1, 1, 1);
   if (im == NULL)
     {
	snprintf(fulltheme, sizeof(fulltheme), PACKAGE_DATA_DIR"/data/themes/default.edj");
	im = e_widget_preview_add(evas, 320, 
				  (320 * e_zone_current_get(cfd->dia->win->container)->h) /
				  e_zone_current_get(cfd->dia->win->container)->w);
	e_widget_preview_edje_set(im, fulltheme, "desktop/background");
     }
   cfd->data = im;
   
   e_widget_table_object_append(o, im, 1, 0, 1, 2, 1, 1, 1, 1);   

   if (selnum >= 0) e_widget_ilist_selected_set(il, selnum);
   
   return o;
}

