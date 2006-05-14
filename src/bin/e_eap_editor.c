#include "e.h"

typedef struct _E_App_Edit E_App_Edit;
typedef struct _E_App_Edit_CFData E_App_Edit_CFData;

struct _E_App_Edit
{      
   E_App       *eap;

   Evas_Object *img;
   Evas_Object *img_widget;
   int          img_set;
   
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
   int   startup_notify;
   int   wait_exit;
   /*- common -*/
   char *image;
   E_App_Edit *editor;
};

/* local subsystem functions */

static void           _e_eap_edit_fill_data(E_App_Edit_CFData *cdfata);
static void          *_e_eap_edit_create_data(E_Config_Dialog *cfd);
static void           _e_eap_edit_free_data(E_Config_Dialog *cfd, void *data);
static int            _e_eap_edit_basic_apply_data(E_Config_Dialog *cfd, void *data);
static int            _e_eap_edit_advanced_apply_data(E_Config_Dialog *cfd, void *data);
static Evas_Object   *_e_eap_edit_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, void *data);
static Evas_Object   *_e_eap_edit_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, void *data);
static void           _e_eap_edit_select_cb(Evas_Object *obj, char *file, void *data);
static void           _e_eap_edit_hilite_cb(Evas_Object *obj, char *file, void *data);

#define IFDUP(src, dst) if (src) dst = strdup(src); else dst = NULL

/* FIXME: this eap editor is half-done. first advanced mode needs to ALSO
 * cover basic config - image saving is broken, makign new icons is broken
 * along with e_apps.c etc. all in all - this is not usable.
 */

/* externally accessible functions */

EAPI void
e_eap_edit_show(E_Container *con, E_App *a)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   E_App_Edit *editor;

   if (!con) return;
   
   editor = E_NEW(E_App_Edit, 1);
   if (!editor) return;
   
   editor->eap = a;
   editor->img = NULL;
   e_object_ref(E_OBJECT(editor->eap));
   
   v = E_NEW(E_Config_Dialog_View, 1);
   if (v)
     {
	/* methods */
	v->create_cfdata           = _e_eap_edit_create_data;
	v->free_cfdata             = _e_eap_edit_free_data;
	v->basic.apply_cfdata      = _e_eap_edit_basic_apply_data;
	v->basic.create_widgets    = _e_eap_edit_basic_create_widgets;
	v->advanced.apply_cfdata   = _e_eap_edit_advanced_apply_data;
	v->advanced.create_widgets = _e_eap_edit_advanced_create_widgets;
	/* create config diaolg for NULL object/data */
	cfd = e_config_dialog_new(con, _("Eap Editor"), NULL, 0, v, editor);
     }
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
   IFDUP(cfdata->editor->eap->icon_class, cfdata->iclass);   
   cfdata->startup_notify = cfdata->editor->eap->startup_notify;
   cfdata->wait_exit = cfdata->editor->eap->wait_exit;
}

static void *
_e_eap_edit_create_data(E_Config_Dialog *cfd)
{
   E_App_Edit_CFData *cfdata;
   
   cfdata = E_NEW(E_App_Edit_CFData, 1);
   if (!cfdata) return NULL;
   cfdata->editor = cfd->data;
   cfdata->image = NULL;
   _e_eap_edit_fill_data(cfdata);
   return cfdata;
}

static void
_e_eap_edit_free_data(E_Config_Dialog *cfd, void *data)
{
   E_App_Edit_CFData *cfdata;
   
   cfdata = data;
   E_FREE(cfdata->name);
   E_FREE(cfdata->exe);
   E_FREE(cfdata->generic);
   E_FREE(cfdata->comment);
   E_FREE(cfdata->wname);
   E_FREE(cfdata->wclass);
   E_FREE(cfdata->wtitle);
   E_FREE(cfdata->wrole);
   E_FREE(cfdata->iclass);   
   E_FREE(cfdata->image);
   e_object_unref(E_OBJECT(cfdata->editor->eap));
   E_FREE(cfdata->editor);
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
   
   if (eap->name) evas_stringshare_del(eap->name);
   if (eap->exe) evas_stringshare_del(eap->exe);
   if (eap->image) evas_stringshare_del(eap->image);
   
   if (cfdata->name) eap->name = evas_stringshare_add(cfdata->name);
   if (cfdata->exe) eap->exe = evas_stringshare_add(cfdata->exe);
   if (cfdata->image) eap->image = evas_stringshare_add(cfdata->image);
   
   eap->startup_notify = cfdata->startup_notify;
   eap->wait_exit = cfdata->wait_exit;
   
   /* FIXME: hardcoded until the eap editor provides fields to change it */
   eap->width = 128;
   eap->height = 128;   

   if ((eap->name) && (eap->exe))
     e_app_fields_save(eap);
   
   return 1;
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
   
   if (eap->name) evas_stringshare_del(eap->name);
   if (eap->exe) evas_stringshare_del(eap->exe);
   if (eap->image) evas_stringshare_del(eap->image);
   
   if (eap->generic) evas_stringshare_del(eap->generic);
   if (eap->comment) evas_stringshare_del(eap->comment);
   if (eap->win_name) evas_stringshare_del(eap->win_name);
   if (eap->win_class) evas_stringshare_del(eap->win_class);   
   if (eap->win_title) evas_stringshare_del(eap->win_title);
   if (eap->win_role) evas_stringshare_del(eap->win_role);
   if (eap->icon_class) evas_stringshare_del(eap->icon_class);
   
   if (cfdata->startup_notify) eap->startup_notify = 1;   
   else eap->startup_notify = 0;
   if (cfdata->wait_exit) eap->wait_exit = 1;
   else eap->wait_exit = 0;
   
   if (cfdata->name) eap->name = evas_stringshare_add(cfdata->name);
   if (cfdata->exe) eap->exe = evas_stringshare_add(cfdata->exe);
   if (cfdata->image) eap->image = evas_stringshare_add(cfdata->image);
   
   if (cfdata->generic) eap->generic = evas_stringshare_add(cfdata->generic);
   if (cfdata->comment) eap->comment = evas_stringshare_add(cfdata->comment);
   if (cfdata->wname) eap->win_name = evas_stringshare_add(cfdata->wname);
   if (cfdata->wclass) eap->win_class = evas_stringshare_add(cfdata->wclass);
   if (cfdata->wtitle) eap->win_title = evas_stringshare_add(cfdata->wtitle);
   if (cfdata->wrole) eap->win_role = evas_stringshare_add(cfdata->wrole);
   if (cfdata->iclass) eap->icon_class = evas_stringshare_add(cfdata->iclass);

   /* FIXME: hardcoded until the eap editor provides fields to change it */
   eap->width = 128;
   eap->height = 128;
   
   if ((eap->name) && (eap->exe))
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
   
   ol = e_widget_table_add(evas, 0);
   
   o = e_widget_frametable_add(evas, _("Icon"), 0);
   
   if ((!editor->img) || (editor->img_set != 1))
     {
	editor->img = e_icon_add(evas);   
	if (eap->path)
	  {      
	     e_icon_file_key_set(editor->img, eap->path, "images/0");
	     e_icon_fill_inside_set(editor->img, 1);	
	  }
     }
   else if (editor->img_set)
     {
        editor->img = e_icon_add(evas);
	e_icon_file_set(editor->img, cfdata->image);
	e_icon_fill_inside_set(editor->img, 1);	
     }
   
   editor->img_widget = e_widget_iconsel_add(evas, editor->img, 48, 48, 
					     &(cfdata->image));
   e_widget_iconsel_select_callback_add(editor->img_widget, _e_eap_edit_select_cb, editor);
   e_widget_iconsel_hilite_callback_add(editor->img_widget, _e_eap_edit_hilite_cb, editor);
   e_widget_frametable_object_append(o, editor->img_widget,
				     0, 0, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(ol, o, 0, 0, 1, 1, 1 ,1, 1, 1);
   
   
   o = e_widget_frametable_add(evas, _("Basic Info"), 0);
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("App name")),
				     0, 0, 1, 1,
				     1, 1, 1, 1);
   
   entry = e_widget_entry_add(evas, &(cfdata->name));
   e_widget_min_size_set(entry, 100, 1);
   e_widget_frametable_object_append(o, entry,
				     1, 0, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Executable")),
				     0, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(cfdata->exe)),
				     1, 1, 1, 1,
				     1, 1, 1, 1);   
   e_widget_table_object_append(ol, o, 1, 0, 1, 1, 1 ,1, 1, 1);
      
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
   
   ol = _e_eap_edit_basic_create_widgets(cfd, evas, data);
   
   o = e_widget_frametable_add(evas, _("General"), 0);

   /*- general info -*/
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Generic Info")),
				     0, 0, 1, 1,
				     1, 1, 1, 1);
   
   entry = e_widget_entry_add(evas, &(cfdata->generic));
   e_widget_min_size_set(entry, 100, 1);   
   e_widget_frametable_object_append(o, entry,
				     1, 0, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Comment")),
				     0, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(cfdata->comment)),
				     1, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(ol, o, 0, 1, 1, 1, 1 ,1, 1, 1);
   
   
   /*- window info -*/
   o = e_widget_frametable_add(evas, _("Window"), 0);
   
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Window Name")),
				     0, 0, 1, 1,
				     1, 1, 1, 1);
   
   entry = e_widget_entry_add(evas, &(cfdata->wname));
   e_widget_min_size_set(entry, 100, 1);      
   e_widget_frametable_object_append(o, entry,
				     1, 0, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Window Class")),
				     0, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(cfdata->wclass)),
				     1, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Window Title")),
				     0, 2, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(cfdata->wtitle)),
				     1, 2, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Window Role")),
				     0, 3, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(cfdata->wrole)),
				     1, 3, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(ol, o, 0, 2, 1, 1, 1 ,1, 1, 1);
   
   /*- icon info -*/
   o = e_widget_frametable_add(evas, _("Icon Theme"), 0);
      
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Icon Class")),
				     0, 0, 1, 1,
				     1, 1, 1, 1);
   
   entry = e_widget_entry_add(evas, &(cfdata->iclass));
   e_widget_min_size_set(entry, 100, 1);   
   e_widget_frametable_object_append(o, entry,
				     1, 0, 1, 1,
				     1, 1, 1, 1);
   
   e_widget_table_object_append(ol, o, 1, 1, 1, 1, 1 ,1, 1, 1);
   
   
   /*- misc info -*/
   o = e_widget_frametable_add(evas, _("Misc"), 0);     
   e_widget_frametable_object_append(o, e_widget_check_add(evas, _("Startup Notify"), &(cfdata->startup_notify)),
				     0, 0, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_check_add(evas, _("Wait Exit"), &(cfdata->wait_exit)),
				     0, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(ol, o, 1, 2, 1, 1, 1 ,1, 1, 1);
      
   return ol;
}

void
_e_eap_edit_select_cb(Evas_Object *obj, char *file, void *data)
{
   E_App_Edit *editor;
   
   editor = data;
   editor->img_set = 1;
   printf("selected: %s\n", file);
}

void
_e_eap_edit_hilite_cb(Evas_Object *obj, char *file, void *data)
{
   E_App_Edit *editor;
   
   editor = data;
   editor->img_set = 1;   
   printf("hilited: %s\n", file);
}
