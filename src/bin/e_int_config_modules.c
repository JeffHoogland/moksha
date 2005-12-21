/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
    
/* PROTOTYPES - same all the time */
typedef struct _CFData CFData;
typedef struct _E_Cfg_Mod_Data E_Cfg_Mod_Data;

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, CFData *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata);
static int _advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);

void _e_config_module_list(Evas_List **b,char *dir, int loaded);
void _e_config_mod_cb_standard(void *data);
void *_module_load(void *data, void *data2);
void *_module_unload(void *data, void *data2);

/* Actual config data we will be playing with whil the dialog is active */
struct _CFData
{
   /*- BASIC -*/
   int mode;
   /*- ADVANCED -*/
   Evas_List *mods, *umods;
   E_Module *cur_mod;
   Evas_Object *mod_name;
   struct {
      Evas_Object *configure, *enable, *disable, *about;
      Evas_Object *load, *unload, *loaded, *unloaded;
   } gui;
};

struct _E_Cfg_Mod_Data
{
   E_Config_Dialog *cfd;
   int loaded;
   E_Module *mod;
   char *mod_name;// use this for unloaded mods
};

/* a nice easy setup function that does the dirty work */
E_Config_Dialog *
e_int_config_modules(E_Container *con)
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
   cfd = e_config_dialog_new(con, _("Modules Settings"), NULL, 0, &v, NULL);
   return cfd;   
}

/* FIXME : redo this to setup list of loaded and unloaded modules in one pass (easy):)*/
/* FIXME: this leaks ecore list of files - also same fn dealing with 2 lists is a bit evil */
void 
_e_config_module_list(Evas_List **b, char *dir, int loaded)
{
   Evas_List *l; 
   char fullpath[PATH_MAX];
   
   l = *b;
   if ((ecore_file_exists(dir)) && (ecore_file_is_dir(dir)))
     {
	Ecore_List *mods;
	
	mods = ecore_file_ls(dir);
	if (mods)
	  {	     	     
	     char *mod;
	     int i = 0;
	     
	     while ((mod = ecore_list_next(mods)))
	       {
		  snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, mod);
		  if (ecore_file_is_dir(fullpath))
		    {
		       E_Module *m;
		       
		       m = e_module_find(mod);		       
		       if ((!m) && (!loaded))
			 l = evas_list_append(l, mod);
		       else if ((m) && (loaded))
			 l = evas_list_append(l, m);
		    }
	       }
	  }     
     }
   *b = l;
}

void
_e_config_mod_cb_standard(void *data)
{
   E_Cfg_Mod_Data *d;
   E_Config_Dialog *cfd;
   CFData *cfdata;
   E_Module *m;
   
   d = data;
   cfd = d->cfd;
   cfdata = cfd->cfdata;
   
   m = d->mod;
   cfd->data = m;
   
   if (m->enabled)
     {
	e_widget_disabled_set(cfdata->gui.enable, 1);
	e_widget_disabled_set(cfdata->gui.disable, 0);
	if (m->func.config) 
	  e_widget_disabled_set(cfdata->gui.configure, 0);
	else
	  e_widget_disabled_set(cfdata->gui.configure, 1);
     }
   else
     {
	e_widget_disabled_set(cfdata->gui.configure, 1);
	e_widget_disabled_set(cfdata->gui.enable, 0);
	e_widget_disabled_set(cfdata->gui.disable, 1);
     }
   
   if (m->func.about) 
     e_widget_disabled_set(cfdata->gui.about, 0);
   else
     e_widget_disabled_set(cfdata->gui.about, 1);
   
   /* Load / Unload menu */
   if (cfd->view_type == E_CONFIG_DIALOG_CFDATA_TYPE_ADVANCED)
     {
	if (!d->loaded)/* unloaded module was clicked */
	  {
	     cfd->data=d->mod_name;
	     e_widget_disabled_set(cfdata->gui.load, 0);
	     e_widget_disabled_set(cfdata->gui.unload, 1);     
	     e_widget_disabled_set(cfdata->gui.loaded, 1);
	  }
	else /* this is a loaded module */
	  {
	     cfd->data=d->mod;
	     e_widget_disabled_set(cfdata->gui.load, 1);
	     e_widget_disabled_set(cfdata->gui.unload, 0);
	  }
     }
}

void *
_module_load(void *data, void *data2)
{
   E_Cfg_Mod_Data *d;
   E_Config_Dialog *cfd;
   CFData *cfdata;
   Evas_Object *ob;
   
   cfd = data;   
   d = cfd->data;   
   cfdata = cfd->cfdata;
   e_module_new(d->mod_name);
   
   cfd->view_dirty = 1;
}

void *
_module_unload(void *data, void *data2)
{
   E_Cfg_Mod_Data *d;
   E_Module *m;
   E_Config_Dialog *cfd;
   CFData *cfdata;
   
   cfd = data;
   d = cfd->data;
   m = d->mod;
   cfdata = cfd->cfdata;
    
   e_module_disable(m);
   e_object_del(E_OBJECT(m));
   e_config_save_queue();
   cfd->view_dirty = 1;
}

void *
_module_enable(void *data, void *data2) /* this enables and disables :) */
{
   E_Config_Dialog *cfd;
   CFData *cfdata;
   E_Module *m;
   E_Cfg_Mod_Data *d;
   
   cfd = data;
   d = cfd->data;
   
   if (!d->loaded)
     {
	m = e_module_new(d->mod_name);
	cfd->view_dirty = 1;
     }
   else
     {
	m = d->mod;
     }
   cfdata = cfd->cfdata;

   if (m->enabled)
     {
	e_module_save(m);
	e_module_disable(m);
	
	e_widget_disabled_set(cfdata->gui.configure, 1);
	e_widget_disabled_set(cfdata->gui.enable, 0);
	e_widget_disabled_set(cfdata->gui.disable, 1);
     }
   else
     {
	e_module_enable(m);
	if (m->func.config)
	  e_widget_disabled_set(cfdata->gui.configure, 0);
	e_widget_disabled_set(cfdata->gui.enable, 1);
	e_widget_disabled_set(cfdata->gui.disable, 0);
     }

   /* this forces a redraw of the window so that the buttons update their
    * state properly... probably fix this with a better method that doesn't
    * unselect the current selection
    */
   cfd->view_dirty = 1;
}

void *
_module_configure(void *data, void *data2)
{
   E_Cfg_Mod_Data *d;
   E_Config_Dialog *cfd;
   CFData *cfdata;
   E_Module *m;
   
   cfd = data;
   d = cfd->data;
   m = d->mod;
   cfdata = cfd->cfdata;
   if (m->func.config)
     {
	m->func.config(m);
     }
   else   
     printf("Can't run config no module!!!\n");// Debug!!    
}

void *
_module_about(void *data, void *data2)
{
   E_Config_Dialog *cfd;
   CFData *cfdata;
   E_Module *m;
   
   cfd = data;
   m = cfd->data;
   cfdata = cfd->cfdata;
   if (m->func.about)
     {
	m->func.about(m);
     }
   else   
     printf("Can't run about no module!!!\n");// Debug!!    
}


/**--CREATE--**/
static void
_fill_data(CFData *cfdata)
{
   char buf[4096];
   char fullpath[PATH_MAX];
   Evas_List *l = NULL;
   
   cfdata->umods = NULL;
   cfdata->mods = NULL;
   //e_module_list();
   
   /* We could use e_module_list() but this method gives us alphabetical order */
   for (l = e_path_dir_list_get(path_modules); l; l = l->next)
     {
	E_Path_Dir *epd;
	
	epd = l->data;
	_e_config_module_list(&(cfdata->mods), epd->dir, 1);
     }
   
   for (l = e_path_dir_list_get(path_modules); l; l = l->next)
     {
	E_Path_Dir *epd;
	
	epd = l->data;
	_e_config_module_list(&(cfdata->umods), epd->dir, 0);
     }
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
   return 1; /* Apply was OK */
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata)
{
   return 1; /* Apply was OK */
}

/**--GUI--**/
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata)
{
   /* generate the core widget layout for a basic dialog */
   Evas_Object *o, *ob, *of, *sob;
   E_Radio_Group *rg;
   Evas_List *l;
   E_Module *m;
   
   _fill_data(cfdata);
   cfd->hide_buttons = 1;
   
   o = e_widget_list_add(evas, 0, 1);
   of = e_widget_framelist_add(evas, "Modules", 1); 
   ob = e_widget_ilist_add(evas, 16, 16, NULL);
   for(l = cfdata->mods; l; l = l->next)
     {
	E_Cfg_Mod_Data *cb_data;
	
	m = l->data;
	sob = e_icon_add(evas);
	if (m->icon_file)
	  e_icon_file_set(sob, m->icon_file);
	/*else if (mod->edje_icon_file)
	 * {
	 *  if (mod->edje_icon_key)
	 */
	cb_data = E_NEW(E_Cfg_Mod_Data, 1);
	cb_data->loaded = 1;
	cb_data->cfd = cfd;
	cb_data->mod = m;
	e_widget_ilist_append(ob, sob, m->name, _e_config_mod_cb_standard, cb_data, m->name);
     }
   e_widget_min_size_set(ob, 120, 120);
   e_widget_ilist_go(ob);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_frametable_add(evas, "Actions", 1);
   
   ob = e_widget_button_add(evas, "Enable", NULL, _module_enable, cfd, NULL);
   cfdata->gui.enable = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 32, 32, 1, 1);
   
   ob = e_widget_button_add(evas, "Disable", NULL, _module_enable, cfd, NULL);
   cfdata->gui.disable = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 32, 32, 1, 1);
   
   ob = e_widget_button_add(evas, "Configure", NULL, _module_configure, cfd, NULL);
   cfdata->gui.configure=ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 32, 32, 1, 1);
   
   ob = e_widget_button_add(evas, "About", NULL, _module_about, cfd, NULL);
   cfdata->gui.about=ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 3, 1, 1, 32, 32, 1, 1);

   e_widget_list_object_append(o, of, 1, 1, 0.5);
   return o;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata)
{
   /* generate the core widget layout for an advanced dialog */
   Evas_Object *o, *ob, *of, *sob;
   E_Radio_Group *rg;
   Evas_List *l;
   E_Module *m;
     
   _fill_data(cfdata);
   cfd->hide_buttons = 1;
   
   o = e_widget_list_add(evas, 0, 1);
   
   of = e_widget_framelist_add(evas, "Loaded", 1);
   ob = e_widget_ilist_add(evas, 16, 16, NULL);
   for(l = cfdata->mods; l; l = l->next)
   {
      E_Cfg_Mod_Data *cb_data;
      
      m = l->data;
      sob = e_icon_add(evas);
      if (m->icon_file)
	e_icon_file_set(sob, m->icon_file);
      cb_data = E_NEW(E_Cfg_Mod_Data, 1);
      cb_data->cfd = cfd;
      cb_data->loaded = 1;
      cb_data->mod = m;
      e_widget_ilist_append(ob, sob, m->name, _e_config_mod_cb_standard, cb_data, m->name);
   }
   cfdata->gui.loaded = ob;
   e_widget_ilist_go(ob);
   e_widget_min_size_set(ob, 120, 120);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_frametable_add(evas, "Actions", 1);
   ob = e_widget_button_add(evas, "Load", NULL, _module_load, cfd, NULL);
   cfdata->gui.load = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 32, 32, 1, 1);
   
   ob = e_widget_button_add(evas, "Enable", NULL, _module_enable, cfd, NULL);
   cfdata->gui.enable = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 32, 32, 1, 1);
   
   ob = e_widget_button_add(evas, "Disable", NULL, _module_enable, cfd, NULL);
   cfdata->gui.disable = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 32, 32, 1, 1);
   
   ob = e_widget_button_add(evas, "Unload", NULL, _module_unload, cfd, NULL);   
   cfdata->gui.unload = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 3, 1, 1, 32, 32, 1, 1);
   
   ob = e_widget_button_add(evas, "Configure", NULL, _module_configure, cfd, NULL);
   cfdata->gui.configure = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 4, 1, 1, 32, 32, 1, 1);
   e_widget_list_object_append(o, of, 1, 1, 0.5);  

   of = e_widget_framelist_add(evas, "Unloaded", 1);
   ob = e_widget_ilist_add(evas, 16, 16, NULL);
   for (l = cfdata->umods; l; l = l->next)
     {
	char *mod;
	char *icon;
	char buf[PATH_MAX];
	E_Cfg_Mod_Data *cb_data;
	
	mod = l->data;
	sob = e_icon_add(evas);
	snprintf(buf, sizeof(buf), "%s/module_icon.png", mod);
	icon = e_path_find(path_modules, buf);
	e_icon_file_set(sob,icon);
	cb_data = E_NEW(CFData,1);
	cb_data->cfd = cfd;
	cb_data->loaded = 0;
	cb_data->mod_name = strdup(mod);
	e_widget_ilist_append(ob, sob, mod, _e_config_mod_cb_standard, cb_data, mod);
     }
   cfdata->gui.unloaded = ob;
   e_widget_ilist_go(ob);
   e_widget_min_size_set(ob, 120, 120);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   return o;
}

