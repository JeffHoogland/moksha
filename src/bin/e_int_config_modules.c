/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* PROTOTYPES - same all the time */
typedef struct _CFData CFData;

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, CFData *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata);
static int _advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);

static void _ilist_cb_change(void *data, Evas_Object *obj);
static void _ilist2_cb_change(void *data, Evas_Object *obj);

static void _module_load(void *data, void *data2);
static void _module_unload(void *data, void *data2);
static void _module_enable(void *data, void *data2);
static void _module_disable(void *data, void *data2);
static void _module_configure(void *data, void *data2);
static void _module_about(void *data, void *data2);

//static CFData cdata = NULL;

/* Actual config data we will be playing with whil the dialog is active */
struct _CFData
{
   E_Config_Dialog *cfd;
   struct {
      Evas_Object *configure, *enable, *disable, *about;
      Evas_Object *load, *unload, *loaded, *unloaded;
   } gui;
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
   cfd = e_config_dialog_new(con, _("Module Settings"), NULL, 0, &v, NULL);
   return cfd;   
}

static void
_module_load(void *data, void *data2)
{
   Evas_Object *ilist2;
   CFData *cfdata;
   E_Module *m;
   int i;
   char *v;
   
   ilist2 = data;
   cfdata = data2;
   i = e_widget_ilist_selected_get(ilist2);
   v = strdup(e_widget_ilist_selected_label_get(ilist2));
   m = e_module_find(v);
   if (!m) 
     { 
	m = e_module_new(v);
	cfdata->cfd->view_dirty = 1;
     }
}

static void
_module_unload(void *data, void *data2)
{
   Evas_Object *ilist;
   CFData *cfdata;
   E_Module *m;
   int i;
   char *v;
   
   ilist = data;
   cfdata = data2;
   
   i = e_widget_ilist_selected_get(ilist);
   v = strdup(e_widget_ilist_selected_label_get(ilist));
   m = e_module_find(v);
   if (m) 
     {   
	e_module_disable(m);
	e_object_del(E_OBJECT(m));
	e_config_save_queue();	
	cfdata->cfd->view_dirty = 1;
     }
}

static void
_module_enable(void *data, void *data2) /* this enables and disables :) */
{
   Evas_Object *obj;
   CFData *cfdata;
   E_Module *m;
   int i;
   char *v;
   
   obj = data;
   cfdata = data2;
   
   i = e_widget_ilist_selected_get(obj);
   v = strdup(e_widget_ilist_selected_label_get(obj));
   m = e_module_find(v);
   if (!m) 
     { 
	m = e_module_new(v);	
     }
   
   if (!m->enabled) 
     {
	e_module_enable(m);
	if (m->func.config) 
	  {
	     e_widget_disabled_set(cfdata->gui.configure, 0);
	  }
	e_widget_disabled_set(cfdata->gui.enable, 1);
	e_widget_disabled_set(cfdata->gui.disable, 0);	
     }
   cfdata->cfd->view_dirty = 1;
}

static void
_module_disable(void *data, void *data2) /* this enables and disables :) */
{
   Evas_Object *obj;
   CFData *cfdata;
   E_Module *m;
   int i;
   char *v;
   
   obj = data;
   cfdata = data2;
   
   i = e_widget_ilist_selected_get(obj);
   v = strdup(e_widget_ilist_selected_label_get(obj));
   m = e_module_find(v);
   if (m) 
     {
	if (m->enabled)
	  {
	     e_module_save(m);
	     e_module_disable(m);
	     cfdata->cfd->view_dirty = 1;
	  }
     }   
}

static void
_module_configure(void *data, void *data2)
{
   Evas_Object *obj;
   E_Module *m;
   int i;
   char *v;
   
   obj = data;
   i = e_widget_ilist_selected_get(obj);
   v = strdup(e_widget_ilist_selected_label_get(obj));
   m = e_module_find(v);
   if (m) 
     {   
	if (m->func.config) 
	  {
	     m->func.config(m);
	  }
	else 
	  {   
	     printf("Can't run config no module!!!\n");// Debug!!    
	  }
     }
}

static void
_module_about(void *data, void *data2)
{
   Evas_Object *obj;
   E_Module *m;
   int i;
   char *v;
   
   obj = data;
   i = e_widget_ilist_selected_get(obj);
   v = strdup(e_widget_ilist_selected_label_get(obj));
   m = e_module_find(v);
   if (m) 
     {   
	if (m->func.about) 
	  {
	     m->func.about(m);
	  }
	else 
	  {   
	     printf("Can't run about no module!!!\n");// Debug!!    
	  }
     }
}


/**--CREATE--**/
static void
_fill_data(CFData *cfdata)
{
   return;
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
   cfdata->cfd = cfd;
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
   Evas_Object *o, *ob, *of, *oc, *ilist;
   E_Radio_Group *rg;
   Evas_List *l;
   E_Module *m;
   
   cfd->hide_buttons = 1;
   cfd->cfdata = cfdata;
   
   o = e_widget_list_add(evas, 0, 1);
   of = e_widget_framelist_add(evas, "Modules", 1); 
   ilist = e_widget_ilist_add(evas, 16, 16, NULL);
   e_widget_on_change_hook_set(ilist, _ilist_cb_change, cfdata);
   
   /* Loaded Modules */
   for (l = e_config->modules; l; l = l->next) 
     {
	E_Config_Module *em;
	
	em = l->data;
	if (em->name) m = e_module_find(em->name);
	if (m) 
	  {
	     oc = e_icon_add(evas);
	     if (m->icon_file)
	       e_icon_file_set(oc, m->icon_file);
	     e_widget_ilist_append(ilist, oc, m->name, NULL, NULL, NULL);     
	  }
     }

   e_widget_min_size_set(ilist, 120, 120);
   e_widget_ilist_go(ilist);
   e_widget_framelist_object_append(of, ilist);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
     
   of = e_widget_frametable_add(evas, "Actions", 1);
      
   ob = e_widget_button_add(evas, "Enable", NULL, _module_enable, ilist, cfdata);
   cfdata->gui.enable = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 32, 32, 1, 1);
   
   ob = e_widget_button_add(evas, "Disable", NULL, _module_disable, ilist, cfdata);
   cfdata->gui.disable = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 32, 32, 1, 1);
   
   ob = e_widget_button_add(evas, "Configure", NULL, _module_configure, ilist, NULL);
   cfdata->gui.configure = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 32, 32, 1, 1);
   
   ob = e_widget_button_add(evas, "About", NULL, _module_about, ilist, NULL);
   cfdata->gui.about = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 3, 1, 1, 32, 32, 1, 1);

   e_widget_list_object_append(o, of, 1, 1, 0.5);
   e_dialog_resizable_set(cfd->dia, 1);
   return o;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata)
{
   /* generate the core widget layout for an advanced dialog */
   Evas_Object *o, *ob, *of, *oc, *ilist, *ilist2;
   E_Radio_Group *rg;
   Evas_List *l, *l2;
   E_Module *m;
   Ecore_List *modules;
   char full_path[PATH_MAX];
   int loaded;
   char *icon;
   char buf[PATH_MAX];
     
   cfd->hide_buttons = 1;
   cfd->cfdata = cfdata;
   
   o = e_widget_list_add(evas, 0, 1);
   
   of = e_widget_framelist_add(evas, "Loaded", 1);
   ilist = e_widget_ilist_add(evas, 16, 16, NULL);
   e_widget_on_change_hook_set(ilist, _ilist_cb_change, cfdata);
   
   /* Loaded Modules */
   for (l = e_config->modules; l; l = l->next) 
     {
	E_Config_Module *em;
	
	em = l->data;
	if (em->name) m = e_module_find(em->name);
	if (m) 
	  {
	     oc = e_icon_add(evas);
	     if (m->icon_file)
	       e_icon_file_set(oc, m->icon_file);
	     e_widget_ilist_append(ilist, oc, m->name, NULL, NULL, NULL);     
	  }
     }
  
   e_widget_ilist_go(ilist);
   e_widget_min_size_set(ilist, 120, 120);
   e_widget_framelist_object_append(of, ilist);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   ilist2 = e_widget_ilist_add(evas, 16, 16, NULL);
   e_widget_on_change_hook_set(ilist2, _ilist2_cb_change, cfdata);
   
   of = e_widget_frametable_add(evas, "Actions", 1);
   ob = e_widget_button_add(evas, "Load", NULL, _module_load, ilist2, cfdata);
   cfdata->gui.load = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 32, 32, 1, 1);
   
   ob = e_widget_button_add(evas, "Enable", NULL, _module_enable, ilist, cfdata);
   cfdata->gui.enable = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 32, 32, 1, 1);
   
   ob = e_widget_button_add(evas, "Disable", NULL, _module_disable, ilist, cfdata);
   cfdata->gui.disable = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 32, 32, 1, 1);
   
   ob = e_widget_button_add(evas, "Unload", NULL, _module_unload, ilist, cfdata);   
   cfdata->gui.unload = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 3, 1, 1, 32, 32, 1, 1);
   
   ob = e_widget_button_add(evas, "Configure", NULL, _module_configure, ilist, NULL);
   cfdata->gui.configure = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 4, 1, 1, 32, 32, 1, 1);
   e_widget_list_object_append(o, of, 1, 1, 0.5);  

   of = e_widget_framelist_add(evas, "Unloaded", 1);
   
   /* Get Modules; For each module, if it's not loaded, list it */
   l = NULL;
   for (l = e_path_dir_list_get(path_modules); l; l = l->next) 
     {
	E_Path_Dir *epd;
	
	epd = l->data;
	if ((ecore_file_exists(epd->dir)) && (ecore_file_is_dir(epd->dir))) 
	  {
	     modules = ecore_file_ls(epd->dir);
	     if (modules) 
	       {
		  char *mod;
		  while ((mod = ecore_list_next(modules))) 
		    {
		       snprintf(full_path, sizeof(full_path), "%s/%s", epd->dir, mod);
		       snprintf(buf, sizeof(buf), "%s/module_icon.png", mod);		       
		       if (ecore_file_is_dir(full_path)) 
			 {
			    E_Module *m;
			    m = e_module_find(mod);
			    if (m) 
			      {
				 loaded = 0;
				 for (l = e_config->modules; l; l = l->next) 
				   {
				      E_Config_Module *em;
	
				      em = l->data;
				      if (!strcmp(m->name, em->name)) 
					{
					   loaded = 1;
					   break; 
					}
				   }
			      }
			    if ((!m) || (!loaded)) 
			      {
				 oc = e_icon_add(evas);
				 icon = e_path_find(path_modules, buf);
				 e_icon_file_set(oc, icon);
				 e_widget_ilist_append(ilist2, oc, mod, NULL, NULL, NULL);
			      }
			 }
		    }
	       }
	  }
     }

   ecore_list_destroy(modules);
   
   e_widget_ilist_go(ilist2);
   e_widget_min_size_set(ilist2, 120, 120);
   e_widget_framelist_object_append(of, ilist2);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   e_dialog_resizable_set(cfd->dia, 1);   
   return o;
}

static void 
_ilist_cb_change(void *data, Evas_Object *obj) 
{
   E_Module *m;
   E_Config_Dialog *cfd;
   CFData *cfdata;
   int i;
   char *v;
   
   cfdata = data;
   cfd = cfdata->cfd;
   i = e_widget_ilist_selected_get(obj);
   v = strdup(e_widget_ilist_selected_label_get(obj));
   m = e_module_find(v);
   if (m) 
     {
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
     }

   if (cfd->view_type == E_CONFIG_DIALOG_CFDATA_TYPE_ADVANCED)
     {
	if (!m)
	  {
	     e_widget_disabled_set(cfdata->gui.load, 0);
	     e_widget_disabled_set(cfdata->gui.unload, 1);     
	  }
	else
	  {
	     e_widget_disabled_set(cfdata->gui.load, 1);
	     e_widget_disabled_set(cfdata->gui.unload, 0);
	  }
     }
}

static void 
_ilist2_cb_change(void *data, Evas_Object *obj) 
{
   E_Module *m;
   E_Config_Dialog *cfd;
   CFData *cfdata;
   int i;
   char *v;
   
   cfdata = data;
   cfd = cfdata->cfd;
   i = e_widget_ilist_selected_get(obj);
   v = strdup(e_widget_ilist_selected_label_get(obj));
   
   m = e_module_find(v);
   if (!m) 
     {
	e_widget_disabled_set(cfdata->gui.configure, 1);
	e_widget_disabled_set(cfdata->gui.enable, 1);
	e_widget_disabled_set(cfdata->gui.disable, 1);
	e_widget_disabled_set(cfdata->gui.unload, 1);     
	e_widget_disabled_set(cfdata->gui.load, 0);
     }
}
