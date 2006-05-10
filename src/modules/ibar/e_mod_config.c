/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

struct _E_Config_Dialog_Data
{
   char *dir;
   int   show_label;
};

/* Protos */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

void 
_config_ibar_module(Config_Item *ci)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   v = E_NEW(E_Config_Dialog_View, 1);

   /* Dialog Methods */
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = NULL;
   v->advanced.create_widgets = NULL;
   
   /* Create The Dialog */
   cfd = e_config_dialog_new(e_container_current_get(e_manager_current_get()),
			     _("IBar Configuration"), NULL, 0, v, ci);
   ibar_config->config_dialog = cfd;
}

static void 
_fill_data(Config_Item *ci, E_Config_Dialog_Data *cfdata)
{
   if (ci->dir)
     cfdata->dir = strdup(ci->dir);
   else
     cfdata->dir = strdup("");
   cfdata->show_label = ci->show_label;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;
   Config_Item *ci;
   
   ci = cfd->data;
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(ci, cfdata);
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->dir) free(cfdata->dir);
   ibar_config->config_dialog = NULL;
   free(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ol, *ob;
   Ecore_List *dirs;
   char *home, buf[4096], *file;
   int selnum = -1;
   
   o = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_framelist_add(evas, _("Selected Bar Source"), 0);
   
   ol = e_widget_tlist_add(evas, &(cfdata->dir));
   e_widget_min_size_set(ol, 160, 160);
   e_widget_framelist_object_append(of, ol);

   home = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar", home);
   dirs = ecore_file_ls(buf);
   
   if (dirs)
     {
	int i;
	
	i = 0;
	while ((file = ecore_list_next(dirs)))
	  {
	     if (file[0] == '.') continue;
	     snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar/%s", home, file);
	     if (ecore_file_is_dir(buf))
	       {
		  e_widget_tlist_append(ol, file, NULL, NULL, file);
		  if ((cfdata->dir) && (!strcmp(cfdata->dir, file)))
		    selnum = i;
		  i++;
	       }
	  }
	ecore_list_destroy(dirs);
     }
   free(home);
   e_widget_tlist_go(ol);
   if (selnum >= 0)
     e_widget_tlist_selected_set(ol, selnum);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   ob = e_widget_check_add(evas, _("Show Icon Label"), &(cfdata->show_label));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   
   return o;
}

static int 
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Config_Item *ci;
   
   ci = cfd->data;
   if (ci->dir) evas_stringshare_del(ci->dir);
   ci->dir = NULL;
   if (cfdata->dir) ci->dir = evas_stringshare_add(cfdata->dir);
   ci->show_label = cfdata->show_label;
   _ibar_config_update();
   e_config_save_queue();
   return 1;
}
