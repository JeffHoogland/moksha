/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* PROTOTYPES - same all the time */
typedef struct _CFData CFData;
typedef struct _E_Cfg_Bg_Data E_Cfg_Bg_Data;

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, CFData *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata);
static int _advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);

/* Actual config data we will be playing with whil the dialog is active */
struct _CFData
{
   /*- BASIC -*/
   char *file ;
   /*- ADVANCED -*/

};

struct _E_Cfg_Bg_Data
{
   E_Config_Dialog *cfd;
   char *file;
};

/* a nice easy setup function that does the dirty work */
E_Config_Dialog *
e_int_config_background(E_Container *con)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View v;
   
   /* methods */
   v.create_cfdata           = _create_data;
   v.free_cfdata             = _free_data;
   v.basic.apply_cfdata      = _basic_apply_data;
   v.basic.create_widgets    = _basic_create_widgets;
   v.advanced.apply_cfdata   = _advanced_apply_data;
   v.advanced.create_widgets = _advanced_create_widgets;
   /* create config diaolg for NULL object/data */
   cfd = e_config_dialog_new(con, _("Background Settings"), NULL, 0, &v, NULL);
   //e_dialog_resizable_set(cfd->dia, 1);
   return cfd;
}

/**--CREATE--**/
static void
_fill_data(CFData *cfdata)
{
   /* TODO: get debfault bg */

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
   free(cfdata);
}

/**--APPLY--**/
static int
_basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata)
{
   E_Zone *z;
   E_Desk *d;
   
   z = e_zone_current_get(cfd->con);
   d = e_desk_current_get(z);
   /* Actually take our cfdata settings and apply them in real life */
   printf("file: %s\n", cfdata->file);
   //e_bg_add(cfd->con, z, 0, 0, cfdata->file);
   if (e_config->desktop_default_background) evas_stringshare_del(e_config->desktop_default_background);
   e_config->desktop_default_background = evas_stringshare_add(cfdata->file);
   e_bg_update();
   e_config_save_queue();
   return 1; /* Apply was OK */
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata)
{
   /* Actually take our cfdata settings and apply them in real life */

   return 1; /* Apply was OK */
}

void
_e_config_bg_cb_standard(void *data)
{
   E_Cfg_Bg_Data *d;
   
   d = data;
   e_widget_image_object_set(d->cfd->data, e_thumb_evas_object_get(d->file, d->cfd->dia->win->evas, 160, 120, 1));
}


/**--GUI--**/
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata)
{
   /* generate the core widget layout for a basic dialog */
   Evas_Object *o, *ob, *fr, *im = NULL;
   Evas_Object *il;
   char buf[4096];
   char *homedir;
   Evas_Object *bg;   
   
   _fill_data(cfdata);
   
   o = e_widget_table_add(evas, 0);
   
   cfdata->file = NULL;
   il = e_widget_ilist_add(evas, 48, 48, &(cfdata->file));
   //e_widget_ilist_selector_set(il, 1);
   homedir = e_user_homedir_get();
   if (homedir)
     {
	snprintf(buf, sizeof(buf), "%s/.e/e/backgrounds", homedir);
	free(homedir);
     }
   
   if ((ecore_file_exists(buf)) && (ecore_file_is_dir(buf)))
     {
	Ecore_List *bgs;
	
	bgs = ecore_file_ls(buf);
	if (bgs)
	  {
	     char *bg;
	     char fullbg[PATH_MAX];
	     Evas_Object *o;
	     Ecore_Evas *eebuf;
	     Evas *evasbuf;
	     Evas_List *l;
	     int i = 0;
	     
	     eebuf = ecore_evas_buffer_new(1, 1);
	     evasbuf = ecore_evas_get(eebuf);
	     o = edje_object_add(evasbuf);
		  
	     while ((bg = ecore_list_next(bgs)))
	       {
		  snprintf(fullbg, sizeof(fullbg), "%s/%s", buf, bg);
		  if (ecore_file_is_dir(fullbg))
		    continue;
		  
		  /* minimum theme requirements */
		  if (edje_object_file_set(o, fullbg, "desktop/background"))
		    {
		       Evas_Object *o = NULL;
		       char *noext, *ext;
		       E_Cfg_Bg_Data *cb_data;
		       
		       o = e_thumb_generate_begin(fullbg, 48, 48, cfd->dia->win->evas, &o, NULL, NULL);
		       
		       ext = strrchr(bg ,'.');
		       
		       if (!ext)
			 {
			    noext = strdup(bg);
			 }
		       else
			 {
			    noext = malloc((ext - bg + 1));
			    if (bg)
			      {
				 memcpy(noext, bg, ext - bg);
				 noext[ext - bg] = 0;
			      }
			 }
		       cb_data = E_NEW(E_Cfg_Bg_Data, 1);
		       cb_data->cfd = cfd;
		       cb_data->file = strdup(fullbg);
		       e_widget_ilist_append(il, o, noext, _e_config_bg_cb_standard, cb_data, fullbg);
		       
		       if ((e_config->desktop_default_background) && !(strcmp(e_config->desktop_default_background, fullbg)))
			 {
			    e_widget_ilist_selected_set(il, i);
			    bg = edje_object_add(evas);
			    edje_object_file_set(bg, e_config->desktop_default_background, "desktop/background");
			    im = e_widget_image_add_from_object(evas, bg, 160, 120);
			 }
		       
		       free(noext);
		       i++;
		    }
	       }
	     
	     evas_object_del(o);
	     ecore_evas_free(eebuf);
	     ecore_list_destroy(bgs);
	  }
     }
   e_widget_ilist_go(il);   
   e_widget_min_size_set(il, 240, 320);
   e_widget_table_object_append(o, il, 0, 0, 1, 2, 1, 1, 1, 1);
   fr = e_widget_framelist_add(evas, "Preview", 0);
   if (im == NULL)
     {
	bg = edje_object_add(evas);
	e_theme_edje_object_set(bg, "base/theme/background", "desktop/background");
	im = e_widget_image_add_from_object(evas, bg, 160, 120);
     }
   cfd->data = im;
   e_widget_min_size_set(fr, 180, 150);
   e_widget_table_object_append(o, fr, 1, 0, 1, 1, 1, 1, 1, 1);   
   e_widget_framelist_object_append(fr, im);   
   
   return o;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata)
{
   /* generate the core widget layout for an advanced dialog */
   Evas_Object *o, *ob, *of;
   
   _fill_data(cfdata);
   
   o = e_widget_list_add(evas, 0, 0);
   
   return o;
}
