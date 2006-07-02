/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Path_Pair E_Path_Pair;
typedef struct _CFPath_Change_Data CFPath_Change_Data;

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static void _ilist_update(Evas_Object *obj, E_Path *old, E_Path *new);
static void _ilist_path_cb_change(void *data);

struct _E_Path_Pair
{
   E_Path	*path;
   const char	*path_description;
};

struct _CFPath_Change_Data
{
   E_Path		*path;
   E_Config_Dialog_Data	*cfdata;
};
     
struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;

   /* Current data */
   E_Path		*cur_path;

   Evas_List		*pcd_list;
   E_Path_Pair		*paths_available;
   struct
     {
	Evas_Object	*path_list;
	   
	Evas_Object	*default_list; /* Read Only */
	Evas_Object	*user_list; /* Editable */
     }
   gui;
};

EAPI E_Config_Dialog *
e_int_config_paths(E_Container *con)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->basic.create_widgets    = _basic_create_widgets;
   v->basic.apply_cfdata      = _basic_apply_data;
   
   cfd = e_config_dialog_new(con, _("Search Path Configuration"), "enlightenment/directories", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->paths_available = E_NEW(E_Path_Pair, 11);
   cfdata->paths_available[0].path =			path_data;
   cfdata->paths_available[0].path_description =	_("Data");
   cfdata->paths_available[1].path =			path_images;
   cfdata->paths_available[1].path_description =	_("Images");
   cfdata->paths_available[2].path =			path_fonts;
   cfdata->paths_available[2].path_description =	_("Fonts");
   cfdata->paths_available[3].path =			path_themes;
   cfdata->paths_available[3].path_description =	_("Themes");
   cfdata->paths_available[4].path =			path_init;
   cfdata->paths_available[4].path_description =	_("Init");
   cfdata->paths_available[5].path =			path_icons;
   cfdata->paths_available[5].path_description =	_("Icons");
   cfdata->paths_available[6].path =			path_modules;
   cfdata->paths_available[6].path_description =	_("Modules");
   cfdata->paths_available[7].path =			path_backgrounds;
   cfdata->paths_available[7].path_description =	_("Backgrounds");
   cfdata->paths_available[8].path =			path_input_methods;
   cfdata->paths_available[8].path_description =	_("Input Methods");
   cfdata->paths_available[9].path =			path_messages;
   cfdata->paths_available[9].path_description =	_("Messages");
   cfdata->paths_available[10].path =			 NULL;
   cfdata->paths_available[10].path_description =	 NULL;
   
   return;
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

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   while (cfdata->pcd_list)
     {
	free(cfdata->pcd_list->data);
	cfdata->pcd_list = evas_list_remove_list(cfdata->pcd_list, cfdata->pcd_list);
     }
   free(cfdata->paths_available);
   free(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{	 
   _ilist_update(	cfdata->gui.user_list,
			cfdata->cur_path,
			NULL);
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;
   int i;

   o = e_widget_table_add(evas, 1);
  
   of = e_widget_framelist_add(evas, _("E Paths"), 0);
   ob = e_widget_ilist_add(evas, 0, 0, NULL);
   cfdata->gui.path_list = ob;
   e_widget_min_size_set(ob, 100, 100);

   /* Fill In Ilist */
   for (i = 0; cfdata->paths_available[i].path; i++)
     {
	CFPath_Change_Data *pcd;
	pcd = E_NEW(CFPath_Change_Data, 1);
	
	pcd->path = cfdata->paths_available[i].path;
	pcd->cfdata = cfdata;
	cfdata->pcd_list = evas_list_append(cfdata->pcd_list, pcd);
	e_widget_ilist_append(ob, NULL, cfdata->paths_available[i].path_description, _ilist_path_cb_change, pcd, NULL);
     }

   e_widget_ilist_go(ob);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(o, of, 0, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Default Directories"), 0);
   ob = e_widget_ilist_add(evas, 0, 0, NULL);
   cfdata->gui.default_list = ob;
   e_widget_min_size_set(ob, 100, 100);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(o, of, 1, 0, 2, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("User Defined Directories"), 0);
   e_widget_framelist_content_align_set(of, 0.5, 0.5);
   ob = e_widget_config_list_add(evas, e_widget_entry_add, _("New Directory"), 2);
   e_widget_disabled_set(ob, 1);
   cfdata->gui.user_list = ob;
   e_widget_framelist_object_append(of, ob);
   
   e_widget_table_object_append(o, of, 0, 1, 3, 1, 1, 1, 1, 1);
   
   return o;
}

static void
_ilist_path_cb_change(void *data)
{
   CFPath_Change_Data *pcd;
   Evas_List *default_list;
   int i;
   
   pcd = data;
   default_list = pcd->path->default_dir_list;

   /* Update Default List */   
   e_widget_ilist_clear(pcd->cfdata->gui.default_list);
   for (i = 0; i < evas_list_count(default_list); i++)
     {
	const char *dir;

	dir = ((E_Path_Dir *)evas_list_nth(default_list, i))->dir;
	e_widget_ilist_append(pcd->cfdata->gui.default_list, NULL, dir, NULL, NULL, NULL);
     }
   e_widget_ilist_go(pcd->cfdata->gui.default_list);

   _ilist_update(	pcd->cfdata->gui.user_list,
			pcd->cfdata->cur_path,
			pcd->path);

   pcd->cfdata->cur_path = pcd->path;
}

static void 
_ilist_update(Evas_Object *obj, E_Path *old, E_Path *new)
{
   /* Save current data to old path */
   if (old)
     {
	int i;
	
	e_path_user_path_clear(old);
	for (i = 0; i < e_widget_config_list_count(obj); i++)
	  {
	     const char *dir;
	     dir = e_widget_config_list_nth_get(obj, i);
	     e_path_user_path_append(old, dir);
	  }
     }

   /* Fill list with selected data */
   if (new)
     {
	Evas_List *l;
	
	e_widget_disabled_set(obj, 0);
	e_widget_config_list_clear(obj);
	for (l = *(new->user_dir_list); l; l = l->next)
	  {
	     E_Path_Dir *epd;
	     epd = l->data;
	     e_widget_config_list_append(obj, epd->dir);
	  }
     }
}

