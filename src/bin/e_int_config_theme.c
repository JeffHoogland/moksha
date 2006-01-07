/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* PROTOTYPES - same all the time */
typedef struct _CFData CFData;
typedef struct _E_Cfg_Theme_Data E_Cfg_Theme_Data;

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, CFData *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);

/* Actual config data we will be playing with whil the dialog is active */
struct _CFData
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
   E_Config_Dialog_View v;
   
   /* methods */
   v.create_cfdata           = _create_data;
   v.free_cfdata             = _free_data;
   v.basic.apply_cfdata      = _basic_apply_data;
   v.basic.create_widgets    = _basic_create_widgets;
   v.advanced.apply_cfdata   = NULL;
   v.advanced.create_widgets = NULL; 
   /* create config diaolg for NULL object/data */
   cfd = e_config_dialog_new(con, _("Theme Selector"), NULL, 0, &v, NULL);
   //e_dialog_resizable_set(cfd->dia, 1);
   return cfd;
}

/**--CREATE--**/
static void
_fill_data(CFData *cfdata)
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
   CFData *cfdata;
   
   cfdata = E_NEW(CFData, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, CFData *cfdata)
{
   /* Free the cfdata */  
   free(cfdata->current_theme);
   free(cfdata);
}

/**--APPLY--**/
static int
_basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata)
{
   E_Zone *z;
   E_Desk *d;
   E_Action *a;
   
   z = e_zone_current_get(cfd->con);
   d = e_desk_current_get(z);
   /* Actually take our cfdata settings and apply them in real life */
   printf("set theme: %s\n", cfdata->theme);

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
   
   d = data;
   e_widget_image_object_set(d->cfd->data, e_thumb_evas_object_get(d->file, d->cfd->dia->win->evas, 160, 120, 1));
}


/**--GUI--**/
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata)
{
   /* generate the core widget layout for a basic dialog */
   Evas_Object *o, *fr, *im = NULL;
   Evas_Object *il;
   char buf[4096];
   char *homedir;
   Evas_Object *theme;   
   
   theme = edje_object_add(evas);
   
   _fill_data(cfdata);
   cfdata->theme = NULL;
   
   o = e_widget_table_add(evas, 0);
   
   il = e_widget_ilist_add(evas, 48, 48, &(cfdata->theme));
   homedir = e_user_homedir_get();
   if (homedir)
     {
	snprintf(buf, sizeof(buf), "%s/.e/e/themes", homedir);
	free(homedir);
     }

   if (ecore_file_is_dir(buf))
     {
	Ecore_List *themes;
	
	themes = ecore_file_ls(buf);
	if (themes)
	  {
	     /* add default theme */
	     ecore_list_prepend(themes, strdup("default.edj"));
	     ecore_list_goto_first(themes);
	     
	     char *themefile;
	     char fulltheme[PATH_MAX];
	     Evas_Object *o;
	     Ecore_Evas *eebuf;
	     Evas *evasbuf;
	     int i = 0;
	     
	     eebuf = ecore_evas_buffer_new(1, 1);
	     evasbuf = ecore_evas_get(eebuf);
	     o = edje_object_add(evasbuf);
		  
	     while ((themefile = ecore_list_next(themes)))
	       {
		  if (!strcmp(themefile, "default.edj"))
		    snprintf(fulltheme, sizeof(fulltheme), PACKAGE_DATA_DIR"/data/themes/default.edj");
		  else
		    snprintf(fulltheme, sizeof(fulltheme), "%s/%s", buf, themefile);
		  if (ecore_file_is_dir(fulltheme))
		    continue;
		  
		  /* minimum theme requirements */
		  if (edje_object_file_set(o, fulltheme, "desktop/background"))
		    {
		       Evas_Object *o = NULL;
		       char *noext, *ext;
		       E_Cfg_Theme_Data *cb_data;
		       
		       o = e_thumb_generate_begin(fulltheme, 48, 48, cfd->dia->win->evas, &o, NULL, NULL);
		       
		       ext = strrchr(themefile ,'.');
		       
		       if (!ext)
			 {
			    noext = strdup(themefile);
			 }
		       else
			 {
			    noext = malloc((ext - themefile + 1));
			    if (themefile)
			      {
				 memcpy(noext, themefile, ext - themefile);
				 noext[ext - themefile] = 0;
			      }
			 }
		       cb_data = E_NEW(E_Cfg_Theme_Data, 1);
		       cb_data->cfd = cfd;
		       cb_data->file = strdup(fulltheme);
		       cb_data->theme = strdup(themefile);
		       e_widget_ilist_append(il, o, noext, _e_config_theme_cb_standard, cb_data, themefile);
		       
		       if (!(strcmp(themefile, cfdata->current_theme)))
			 {
			    e_widget_ilist_selected_set(il, i);
			    im = e_widget_image_add_from_object(evas, theme, 320, 240);
			    e_widget_image_object_set(im, e_thumb_evas_object_get(fulltheme, evas, 160, 120, 1));
			 }
		       
		       free(noext);
		       i++;
		    }
	       }
	     
	     evas_object_del(o);
	     ecore_evas_free(eebuf);
	     ecore_list_destroy(themes);
	  }
     }

   e_widget_ilist_go(il);   
   e_widget_min_size_set(il, 180, 40);
   e_widget_table_object_append(o, il, 0, 0, 1, 2, 1, 1, 1, 1);
   fr = e_widget_framelist_add(evas, "Preview", 0);
   if (im == NULL)
     {
	theme = e_thumb_generate_begin(PACKAGE_DATA_DIR"/data/themes/default.edj",
	      160, 120, evas, &theme, NULL, NULL);
	im = e_widget_image_add_from_object(evas, theme,
	      320, 240);
     }
   cfd->data = im;
   e_widget_min_size_set(fr, 320, 240);
   e_widget_table_object_append(o, fr, 1, 0, 1, 2, 1, 1, 1, 1);   
   e_widget_framelist_object_append(fr, im);   
   
   return o;
}

