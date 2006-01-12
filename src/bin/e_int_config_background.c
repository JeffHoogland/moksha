/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define BG_SET_DEFAULT_DESK 0
#define BG_SET_THIS_DESK 1
#define BG_SET_ALL_DESK 2

/* PROTOTYPES - same all the time */
typedef struct _CFData CFData;
typedef struct _E_Cfg_Bg_Data E_Cfg_Bg_Data;

static void        *_create_data(E_Config_Dialog *cfd);
static void        _free_data(E_Config_Dialog *cfd, CFData *cfdata);
static int         _basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);
static int         _advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);
static void        _load_bgs(Evas *evas, E_Config_Dialog *cfd, Evas_Object *il);

/* Actual config data we will be playing with whil the dialog is active */
struct _CFData
{
   /*- BASIC -*/
   char *file ;
   char *current_file;
   /*- ADVANCED -*/
   int bg_method;
};

struct _E_Cfg_Bg_Data
{
   E_Config_Dialog *cfd;
   char *file;
};

/* a nice easy setup function that does the dirty work */
EAPI E_Config_Dialog *
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
   return cfd;
}

/**--CREATE--**/
static void
_fill_data(CFData *cfdata)
{
   cfdata->bg_method = BG_SET_DEFAULT_DESK;
   if (e_config->desktop_default_background) 
     cfdata->current_file = strdup(e_config->desktop_default_background);
   else 
     cfdata->current_file = NULL;
   /* TODO: get default bg */
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
   if (cfdata->current_file) free(cfdata->current_file);
   free(cfdata);
}

/**--APPLY--**/
static int
_basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata)
{
   while (e_config->desktop_backgrounds)
     {
	E_Config_Desktop_Background *cfbg;
	
	cfbg = e_config->desktop_backgrounds->data;
	e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x, cfbg->desk_y);
     }
   if (e_config->desktop_default_background) evas_stringshare_del(e_config->desktop_default_background);
   e_config->desktop_default_background = evas_stringshare_add(cfdata->file);
   e_bg_update();
   e_config_save_queue();
   if (cfdata->current_file) free(cfdata->current_file);
   cfdata->current_file = strdup(cfdata->file);
   return 1; /* Apply was OK */
}

void
_e_config_bg_cb_standard(void *data)
{
   E_Cfg_Bg_Data *d;
   CFData *cfdata;
   
   d = data;
   cfdata = d->cfd->cfdata;
   e_widget_image_object_set
     (d->cfd->data,
      e_thumb_evas_object_get(d->file, d->cfd->dia->win->evas, 200, 160, 1));
   if (cfdata->current_file) 
     {
	if (!strcmp(d->file, cfdata->current_file)) 
	  {
	     e_dialog_button_disable_num_set(d->cfd->dia, 0, 1);
	     e_dialog_button_disable_num_set(d->cfd->dia, 1, 1);	
	  }
     }
}

/**--GUI--**/
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata)
{
   Evas_Object *o, *fr, *il;
   Evas_Object *im = NULL;
   
   _fill_data(cfdata);
   
   o = e_widget_table_add(evas, 0);
   
   cfdata->file = NULL;
   il = e_widget_ilist_add(evas, 48, 48, &(cfdata->file));
   e_widget_ilist_selector_set(il, 1);
   e_widget_min_size_set(il, 240, 200);

   _load_bgs(evas, cfd, il);
   im = cfd->data;
   
   e_widget_focus_set(il, 1);
   e_widget_ilist_go(il);   
   e_widget_table_object_append(o, il, 0, 0, 1, 2, 1, 1, 1, 1);

   fr = e_widget_framelist_add(evas, "Preview", 0);
   e_widget_min_size_set(fr, 180, 160);
   e_widget_table_object_append(o, fr, 1, 0, 1, 1, 1, 1, 1, 1);   
   e_widget_framelist_object_append(fr, im);   
   
   return o;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata) 
{
   E_Zone *z;
   E_Desk *d;
   int x, y;

   if (!cfdata->file) return 0;
   switch (cfdata->bg_method) 
     {
      case BG_SET_DEFAULT_DESK:
	while (e_config->desktop_backgrounds)
	  {
	     E_Config_Desktop_Background *cfbg;
	     
	     cfbg = e_config->desktop_backgrounds->data;
	     e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x, cfbg->desk_y);
	  }
	if (e_config->desktop_default_background) evas_stringshare_del(e_config->desktop_default_background);
	e_config->desktop_default_background = evas_stringshare_add(cfdata->file);
	e_bg_update();
	e_config_save_queue();
	break;
      case BG_SET_THIS_DESK:
	z = e_zone_current_get(cfd->con);   
	d = e_desk_current_get(z);
	e_desk_xy_get(d, &x, &y);
	e_bg_del(-1, -1, -1, -1);
	e_bg_del(z->container->num, z->num, x, y);
	e_bg_add(z->container->num, z->num, x, y, cfdata->file);
	e_bg_update();
        e_config_save_queue();
	break;
      case BG_SET_ALL_DESK:
	while (e_config->desktop_backgrounds)
	  {
	     E_Config_Desktop_Background *cfbg;
	     
	     cfbg = e_config->desktop_backgrounds->data;
	     e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x, cfbg->desk_y);
	  }
	e_bg_add(-1, -1, -1, -1, cfdata->file);
	e_bg_update();
        e_config_save_queue();
	break;
     }
   if (cfdata->current_file) free(cfdata->current_file);
   cfdata->current_file = strdup(cfdata->file);	
   return 1; /* Apply was OK */   
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata) 
{
   Evas_Object *o, *fr, *il;
   Evas_Object *im = NULL;
   Evas_Object *oc;
   E_Radio_Group *rg;
   
   _fill_data(cfdata);
   
   o = e_widget_table_add(evas, 0);
   
   cfdata->file = NULL;
   il = e_widget_ilist_add(evas, 48, 48, &(cfdata->file));
   e_widget_ilist_selector_set(il, 1);
   e_widget_min_size_set(il, 240, 200);

   _load_bgs(evas, cfd, il);
   im = cfd->data;
   
   e_widget_focus_set(il, 1);
   e_widget_ilist_go(il);   
   e_widget_table_object_append(o, il, 0, 0, 1, 2, 1, 1, 1, 1);

   fr = e_widget_framelist_add(evas, "Preview", 0);
   e_widget_min_size_set(fr, 180, 160);
   e_widget_table_object_append(o, fr, 1, 0, 1, 1, 1, 1, 1, 1);   
   e_widget_framelist_object_append(fr, im);   
   
   rg = e_widget_radio_group_new(&(cfdata->bg_method));
   
   fr = e_widget_framelist_add(evas, "Set Background For", 0);
   e_widget_min_size_set(fr, 180, 160);
   
   oc = e_widget_radio_add(evas, _("Default Desktop"), BG_SET_DEFAULT_DESK, rg);
   e_widget_framelist_object_append(fr, oc);   
   oc = e_widget_radio_add(evas, _("This Desktop"), BG_SET_THIS_DESK, rg);
//   e_widget_disabled_set(oc, 1);
   e_widget_framelist_object_append(fr, oc);   
   oc = e_widget_radio_add(evas, _("All Desktops"), BG_SET_ALL_DESK, rg);
//   e_widget_disabled_set(oc, 1);
   e_widget_framelist_object_append(fr, oc);   

   e_widget_table_object_append(o, fr, 1, 1, 1, 1, 1, 1, 1, 1);   
   
   return o;
}

static void
_load_bgs(Evas *evas, E_Config_Dialog *cfd, Evas_Object *il) 
{
   Evas_Object *im = NULL;
   Evas_Object *bg = NULL;
   char buf[4096];
   char *homedir;
   
   homedir = e_user_homedir_get();
   if (homedir)
     {
	snprintf(buf, sizeof(buf), "%s/.e/e/backgrounds", homedir);
	free(homedir);
     }
   
   if (ecore_file_is_dir(buf))
     {
	Ecore_List *bgs;
	
	bgs = ecore_file_ls(buf);
	if (bgs)
	  {
	     char *bgfile;
	     char fullbg[PATH_MAX];
	     Evas_Object *o, *otmp;
	     int i = 0;
	     
	     while ((bgfile = ecore_list_next(bgs)))
	       {
		  snprintf(fullbg, sizeof(fullbg), "%s/%s", buf, bgfile);
		  if (ecore_file_is_dir(fullbg)) continue;
		  
		  /* minimum theme requirements */
		  if (e_util_edje_collection_exists(fullbg, "desktop/background"))
		    {
		       Evas_Object *o = NULL;
		       char *noext;
		       E_Cfg_Bg_Data *cb_data;
		       
		       o = e_thumb_generate_begin(fullbg, 48, 48, evas, &o, NULL, NULL);
		       noext = ecore_file_strip_ext(bgfile);
		       /* FIXME: cb_data is leaked - not freed ever */
		       cb_data = E_NEW(E_Cfg_Bg_Data, 1);
		       cb_data->cfd = cfd;
		       cb_data->file = strdup(fullbg);
		       e_widget_ilist_append(il, o, noext, _e_config_bg_cb_standard, cb_data, cb_data->file);
		       
		       if ((e_config->desktop_default_background) && 
			   (!strcmp(e_config->desktop_default_background, fullbg)))
			 {			    
			    e_widget_ilist_selected_set(il, i);
			    bg = edje_object_add(evas);
			    edje_object_file_set(bg, e_config->desktop_default_background, "desktop/background");
			    im = e_widget_image_add_from_object(evas, bg, 200, 160);
			    e_widget_image_object_set(im, e_thumb_evas_object_get(fullbg, evas, 200, 160, 1));
			 }		       
		       free(noext);
		       i++;
		    }
	       }
	     free(bgfile);
	     ecore_list_destroy(bgs);
	  }
     }

   if (im == NULL)
     {
	bg = edje_object_add(evas);
	e_theme_edje_object_set(bg, "base/theme/background", "desktop/background");
	im = e_widget_image_add_from_object(evas, bg, 200, 160);
     }
   cfd->data = im;
}


