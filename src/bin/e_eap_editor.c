#include "e.h"

typedef struct _E_App_Edit E_App_Edit;
typedef struct _E_App_Edit_CFData E_App_Edit_CFData;

struct _E_App_Edit
{      
   E_App       *eap;

   Evas_Object *img;
   Evas_Object *img_widget;
   
   E_App_Edit_CFData *cfdata;
};

struct _E_App_Edit_CFData
{
   /*- BASIC -*/
   char *name;
   char *exe;
   /*- ADVANCED -*/
   char *generic;
   char *comment;
   char *wname;
   char *wclass;
   char *wtitle;
   char *wrole;
   char *iclass;
   char *path;
   int   startup_notify;
   int   wait_exit;
   /*- common -*/
   char *image;
   E_App_Edit *editor;
};

/* local subsystem functions */

static void           _e_eap_edit_save_cb(void *data, E_Dialog *dia);
static void           _e_eap_edit_cancel_cb(void *data, E_Dialog *dia);
static void           _e_eap_edit_browse_cb(void *data1, void *data2);
static void           _e_eap_edit_free(E_App_Edit *app);
static void           _e_eap_edit_selector_cb(E_Fileman *fileman, char *file, void *data);
static void           _e_eap_edit_fill_data(E_App_Edit_CFData *cdfata);
static void          *_e_eap_edit_create_data(E_Config_Dialog *cfd);
static void           _e_eap_edit_free_data(E_Config_Dialog *cfd, void *data);
static int            _e_eap_edit_basic_apply_data(E_Config_Dialog *cfd, void *data);
static int            _e_eap_edit_advanced_apply_data(E_Config_Dialog *cfd, void *data);
static Evas_Object   *_e_eap_edit_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, void *data);
static Evas_Object   *_e_eap_edit_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, void *data);
static void           _e_eap_edit_select_cb(Evas_Object *obj, char *file, void *data);

#define IFDUP(src, dst) if(src) dst = strdup(src); else dst = NULL

/* externally accessible functions */

void
e_eap_edit_show(E_Container *con, E_App *a)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View v;   
   E_App_Edit *editor;
   Evas_Object *o, *ol;

   if(!con)
     return;
   
   editor = E_NEW(E_App_Edit, 1);
   if (!editor) return;
   
   editor->eap = a;
   
   /* methods */
   v.create_cfdata           = _e_eap_edit_create_data;
   v.free_cfdata             = _e_eap_edit_free_data;
   v.basic.apply_cfdata      = _e_eap_edit_basic_apply_data;
   v.basic.create_widgets    = _e_eap_edit_basic_create_widgets;
   v.advanced.apply_cfdata   = _e_eap_edit_advanced_apply_data;
   v.advanced.create_widgets = _e_eap_edit_advanced_create_widgets;
   /* create config diaolg for NULL object/data */
   cfd = e_config_dialog_new(con, _("Eap Editor"), NULL, 0, &v, editor);
}

/* local subsystem functions */

static void 
_e_eap_edit_fill_data(E_App_Edit_CFData *cfdata)
{
   /*- BASIC -*/
   IFDUP(cfdata->editor->eap->name, cfdata->name);
   IFDUP(cfdata->editor->eap->exe, cfdata->exe);
   /*- ADVANCED -*/
   IFDUP(cfdata->editor->eap->generic, cfdata->generic);
   IFDUP(cfdata->editor->eap->comment, cfdata->comment);
   IFDUP(cfdata->editor->eap->win_name, cfdata->wname);
   IFDUP(cfdata->editor->eap->win_class, cfdata->wclass);
   IFDUP(cfdata->editor->eap->win_title, cfdata->wtitle);
   IFDUP(cfdata->editor->eap->win_role, cfdata->wrole);
   IFDUP(cfdata->editor->eap->path, cfdata->path);
}

static void *
_e_eap_edit_create_data(E_Config_Dialog *cfd)
{
   E_App_Edit_CFData *cfdata;
   
   cfdata = E_NEW(E_App_Edit_CFData, 1);
   if (!cfdata) return NULL;
   cfdata->editor = cfd->data;
   _e_eap_edit_fill_data(cfdata);
   return cfdata;
}

static void
_e_eap_edit_free_data(E_Config_Dialog *cfd, void *data)
{    
   free(data);
}

static int
_e_eap_edit_basic_apply_data(E_Config_Dialog *cfd, void *data)
{
   E_App_Edit *editor;
   E_App *eap;
   E_App_Edit_CFData *cfdata;
   
   cfdata = data;
   editor = cfdata->editor;
   eap = editor->eap;
   
   if(!(cfdata->path))
     return -1;
   
   E_FREE(eap->name);
   E_FREE(eap->exe);
   E_FREE(eap->image);
   
   IFDUP(cfdata->name, eap->name);
   IFDUP(cfdata->exe, eap->exe);
   IFDUP(cfdata->image, eap->image);
   
   e_app_fields_save(eap);
   
    return -1;
}

static int
_e_eap_edit_advanced_apply_data(E_Config_Dialog *cfd, void *data)
{
   E_App_Edit *editor;
   E_App *eap;
   E_App_Edit_CFData *cfdata;
   
   cfdata = data;
   editor = cfdata->editor;
   eap = editor->eap;
   
   if(!(cfdata->path))
     return -1;

   E_FREE(eap->generic);
   E_FREE(eap->comment);
   E_FREE(eap->win_name);
   E_FREE(eap->win_class);   
   E_FREE(eap->win_title);
   E_FREE(eap->win_role);
   E_FREE(eap->icon_class);
   E_FREE(eap->path);

   if(cfdata->startup_notify)
     eap->startup_notify = 1;   
   if(cfdata->wait_exit)
     eap->wait_exit = 1;
      
   IFDUP(cfdata->generic, eap->generic);
   IFDUP(cfdata->comment, eap->comment);
   IFDUP(cfdata->wname, eap->win_name);
   IFDUP(cfdata->wclass, eap->win_class);
   IFDUP(cfdata->wtitle, eap->win_title);
   IFDUP(cfdata->wrole, eap->win_role);
   IFDUP(cfdata->iclass, eap->icon_class);
   IFDUP(cfdata->path, eap->path);
      
   e_app_fields_save(eap);
   
   return 1;
}


static Evas_Object *
_e_eap_edit_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, void *data)
{
   E_App_Edit_CFData *cfdata;
   E_App_Edit *editor;
   E_App *eap;
   Evas_Object *ol, *o;
   Evas_Object *entry;   
   
   cfdata = data;
   editor = cfdata->editor;
   eap = editor->eap;
   
   ol = e_widget_list_add(evas, 0, 1);
   o = e_widget_frametable_add(evas, _("Icon"), 0);
   
   if(eap->path)
     {      
	editor->img = e_icon_add(evas);
	e_icon_file_key_set(editor->img, eap->path, "images/0");
	e_icon_fill_inside_set(editor->img, 1);	
     }
   else
     {
	editor->img = edje_object_add(evas);
	e_theme_edje_object_set(editor->img, "base/theme/icons/enlightenment/e", "icons/enlightenment/e");
     }
   
   editor->img_widget = e_widget_iconsel_add(evas, editor->img, 48, 48, 
					     &cfdata->image);
   e_widget_iconsel_select_callback_add(editor->img_widget, _e_eap_edit_select_cb, editor);
   e_widget_frametable_object_append(o, editor->img_widget,
				0, 0, 1, 1,
				1, 1, 1, 1);
   
  
   e_widget_list_object_append(ol, o, 1, 1, 0.5);
   
   o = e_widget_frametable_add(evas, _("Basic Info"), 0);
      
   e_widget_frametable_object_append(o, e_widget_label_add(evas, "App name"),
				     0, 0, 1, 1,
				     1, 1, 1, 1);
    
   entry = e_widget_entry_add(evas, &(cfdata->name));
   e_widget_min_size_set(entry, 100, 1);
   e_widget_frametable_object_append(o, entry,
				     1, 0, 1, 1,
				     1, 1, 1, 1);

   e_widget_frametable_object_append(o, e_widget_label_add(evas, "Executable"),
				     0, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(cfdata->exe)),
				     1, 1, 1, 1,
				     1, 1, 1, 1);   
   
   e_widget_list_object_append(ol, o, 1, 1, 0.5);
   
   return ol;
}

static Evas_Object *
_e_eap_edit_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, void *data)
{
   E_App_Edit_CFData *cfdata;
   E_App_Edit *editor;
   E_App *eap;
   Evas_Object *ol, *o;
   Evas_Object *entry;   
   
   cfdata = data;
   editor = cfdata->editor;
   eap = editor->eap;
   
   ol = e_widget_table_add(evas, _("Advanced"), 0);
   
   o = e_widget_frametable_add(evas, _("General"), 0);

   /*- general info -*/
   e_widget_frametable_object_append(o, e_widget_label_add(evas, "Generic Info"),
				0, 0, 1, 1,
				1, 1, 1, 1);
   entry = e_widget_entry_add(evas, &(cfdata->generic));
   e_widget_min_size_set(entry, 100, 1);   
   e_widget_frametable_object_append(o, entry,
				1, 0, 1, 1,
				1, 1, 1, 1);
   
   e_widget_frametable_object_append(o, e_widget_label_add(evas, "Comment"),
				0, 1, 1, 1,
				1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(cfdata->comment)),
				1, 1, 1, 1,
				1, 1, 1, 1);
   
   e_widget_table_object_append(ol, o, 0, 0, 1, 1, 1 ,1, 1, 1);
   
   
   /*- window info -*/
   
   o = e_widget_frametable_add(evas, _("Window"), 0);
   
   e_widget_frametable_object_append(o, e_widget_label_add(evas, "Window Name"),
				0, 0, 1, 1,
				1, 1, 1, 1);
   entry = e_widget_entry_add(evas, &(cfdata->wname));
   e_widget_min_size_set(entry, 100, 1);      
   e_widget_frametable_object_append(o, entry,
				1, 0, 1, 1,
				1, 1, 1, 1);
   
   e_widget_frametable_object_append(o, e_widget_label_add(evas, "Window Class"),
				0, 1, 1, 1,
				1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(cfdata->wclass)),
				1, 1, 1, 1,
				1, 1, 1, 1);
   
   e_widget_frametable_object_append(o, e_widget_label_add(evas, "Window Title"),
				0, 2, 1, 1,
				1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(cfdata->wtitle)),
				1, 2, 1, 1,
				1, 1, 1, 1);
   
   e_widget_frametable_object_append(o, e_widget_label_add(evas, "Window Role"),
				0, 3, 1, 1,
				1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(cfdata->wrole)),
				1, 3, 1, 1,
				1, 1, 1, 1);
   
   e_widget_table_object_append(ol, o, 0, 1, 1, 1, 1 ,1, 1, 1);
   
   /*- icon info -*/
   
   o = e_widget_frametable_add(evas, _("Icon"), 0);
      
   e_widget_frametable_object_append(o, e_widget_label_add(evas, "Icon Class"),
				0, 0, 1, 1,
				1, 1, 1, 1);
   entry = e_widget_entry_add(evas, &(cfdata->iclass));
   e_widget_min_size_set(entry, 100, 1);   
   e_widget_frametable_object_append(o, entry,
				1, 0, 1, 1,
				1, 1, 1, 1);
   
   e_widget_frametable_object_append(o, e_widget_label_add(evas, "Path"),
				0, 1, 1, 1,
				1, 1, 1, 1);
   
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(cfdata->path)),
				1, 1, 1, 1,
				1, 1, 1, 1);   
      
   e_widget_table_object_append(ol, o, 1, 0, 1, 1, 1 ,1, 1, 1);
   
   
   /*- misc info -*/
   
   o = e_widget_frametable_add(evas, _("Misc"), 0);     
   
   e_widget_frametable_object_append(o, e_widget_label_add(evas, "Startup notify"),
				0, 0, 1, 1,
				1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_check_add(evas, "", &(cfdata->startup_notify)),
				1, 0, 1, 1,
				1, 1, 1, 1);
   
   e_widget_frametable_object_append(o, e_widget_label_add(evas, "Wait exit"),
				0, 1, 1, 1,
				1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_check_add(evas, "", &(cfdata->wait_exit)),
				1, 1, 1, 1,
				1, 1, 1, 1);
   
   e_widget_table_object_append(ol, o, 1, 1, 1, 1, 1 ,1, 1, 1);
      
   return ol;
}

void
_e_eap_edit_select_cb(Evas_Object *obj, char *file, void *data)
{
   E_App_Edit *editor;
   
   editor = data;
   printf("selected: %s\n", file);
   
}
