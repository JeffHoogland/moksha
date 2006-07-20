/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_App_Edit E_App_Edit;

struct _E_App_Edit
{
   E_App       *eap;

   Evas_Object *img;
   Evas_Object *img_widget;
   int          img_set;

   E_Config_Dialog_Data *cfdata;
};

struct _E_Config_Dialog_Data
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
   int   width;
   int   height;
   E_App_Edit *editor;
};

/* local subsystem functions */

static void           _e_eap_edit_fill_data(E_Config_Dialog_Data *cdfata);
static void          *_e_eap_edit_create_data(E_Config_Dialog *cfd);
static void           _e_eap_edit_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *data);
static int            _e_eap_edit_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *data);
static int            _e_eap_edit_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *data);
static Evas_Object   *_e_eap_edit_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *data);
static Evas_Object   *_e_eap_edit_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *data);
static void           _e_eap_edit_select_cb(Evas_Object *obj, char *file, void *data);
static void           _e_eap_edit_hilite_cb(Evas_Object *obj, char *file, void *data);

#define IFDUP(src, dst) if (src) dst = strdup(src); else dst = NULL

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
_e_eap_edit_fill_data(E_Config_Dialog_Data *cfdata)
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
   /*- COMMON -*/
   IFDUP(cfdata->editor->eap->image, cfdata->image);
   cfdata->height = cfdata->editor->eap->height;
   cfdata->width = cfdata->editor->eap->width;
   if (cfdata->image) cfdata->editor->img_set = 1;
}

static void *
_e_eap_edit_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;
   cfdata->editor = cfd->data;
   cfdata->image = NULL;
   _e_eap_edit_fill_data(cfdata);
   return cfdata;
}

static void
_e_eap_edit_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *data)
{
   E_FREE(data->name);
   E_FREE(data->exe);
   E_FREE(data->generic);
   E_FREE(data->comment);
   E_FREE(data->wname);
   E_FREE(data->wclass);
   E_FREE(data->wtitle);
   E_FREE(data->wrole);
   E_FREE(data->iclass);
   E_FREE(data->image);
   if (data->editor->eap->image) evas_stringshare_del(data->editor->eap->image);
   data->editor->eap->width = 0;
   data->editor->eap->height = 0;
   e_object_unref(E_OBJECT(data->editor->eap));
   if (data->editor)
     {
	if (data->editor->img) evas_object_del(data->editor->img);
	if (data->editor->img_widget) evas_object_del(data->editor->img_widget);
	free(data->editor);
     }
   free(data);
}

static int
_e_eap_edit_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *data)
{
   E_App_Edit *editor;
   E_App *eap;

   editor = data->editor;
   eap = editor->eap;

   if (eap->name) evas_stringshare_del(eap->name);
   if (eap->exe) evas_stringshare_del(eap->exe);
   if (eap->image) evas_stringshare_del(eap->image);

   if (data->name) eap->name = evas_stringshare_add(data->name);
   if (data->exe) eap->exe = evas_stringshare_add(data->exe);
   if (data->image) eap->image = evas_stringshare_add(data->image);

   eap->startup_notify = data->startup_notify;
   eap->wait_exit = data->wait_exit;

   /* FIXME: hardcoded until the eap editor provides fields to change it */
   if (data->width) eap->width = data->width;
   else eap->width = 128;
   if (data->height) eap->height = data->height;
   else eap->height = 128;

   if ((eap->name) && (eap->exe))
     e_app_fields_save(eap);

   return 1;
}

static int
_e_eap_edit_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *data)
{
   E_App_Edit *editor;
   E_App *eap;

   editor = data->editor;
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

   if (data->startup_notify) eap->startup_notify = 1;
   else eap->startup_notify = 0;
   if (data->wait_exit) eap->wait_exit = 1;
   else eap->wait_exit = 0;

   if (data->name) eap->name = evas_stringshare_add(data->name);
   if (data->exe) eap->exe = evas_stringshare_add(data->exe);
   if (data->image) eap->image = evas_stringshare_add(data->image);

   if (data->generic) eap->generic = evas_stringshare_add(data->generic);
   if (data->comment) eap->comment = evas_stringshare_add(data->comment);
   if (data->wname) eap->win_name = evas_stringshare_add(data->wname);
   if (data->wclass) eap->win_class = evas_stringshare_add(data->wclass);
   if (data->wtitle) eap->win_title = evas_stringshare_add(data->wtitle);
   if (data->wrole) eap->win_role = evas_stringshare_add(data->wrole);
   if (data->iclass) eap->icon_class = evas_stringshare_add(data->iclass);

   /* FIXME: hardcoded until the eap editor provides fields to change it */
   if (data->width) eap->width = data->width;
   else eap->width = 128;
   if (data->height) eap->height = data->height;
   else eap->height = 128;

   if ((eap->name) && (eap->exe))
     e_app_fields_save(eap);
   return 1;
}


static Evas_Object *
_e_eap_edit_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *data)
{
   E_App_Edit *editor;
   E_App *eap;
   Evas_Object *ol, *o;
   Evas_Object *entry;

   editor = data->editor;
   eap = editor->eap;

   ol = e_widget_table_add(evas, 0);

   o = e_widget_frametable_add(evas, _("Icon"), 0);

   if ((editor->img_set) && (data->image))
     {
	if (editor->img) evas_object_del(editor->img);
        editor->img = e_icon_add(evas);
	e_icon_file_set(editor->img, data->image);
	e_icon_fill_inside_set(editor->img, 1);
     }
   else if (!editor->img)
     {
	editor->img = e_icon_add(evas);
	if (eap->path)
	  {
	     e_icon_file_key_set(editor->img, eap->path, "images/0");
	     e_icon_fill_inside_set(editor->img, 1);
	  }
     }

   if (editor->img_widget) evas_object_del(editor->img_widget);
   editor->img_widget = e_widget_iconsel_add(evas, editor->img, 48, 48,
					     &(data->image));
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

   entry = e_widget_entry_add(evas, &(data->name));
   e_widget_min_size_set(entry, 100, 1);
   e_widget_frametable_object_append(o, entry,
				     1, 0, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Executable")),
				     0, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(data->exe)),
				     1, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(ol, o, 1, 0, 1, 1, 1 ,1, 1, 1);

   return ol;
}

static Evas_Object *
_e_eap_edit_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *data)
{
   E_App_Edit *editor;
   E_App *eap;
   Evas_Object *ol, *o;
   Evas_Object *entry;

   editor = data->editor;
   eap = editor->eap;

   ol = _e_eap_edit_basic_create_widgets(cfd, evas, data);

   o = e_widget_frametable_add(evas, _("General"), 0);

   /*- general info -*/
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Generic Info")),
				     0, 0, 1, 1,
				     1, 1, 1, 1);

   entry = e_widget_entry_add(evas, &(data->generic));
   e_widget_min_size_set(entry, 100, 1);
   e_widget_frametable_object_append(o, entry,
				     1, 0, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Comment")),
				     0, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(data->comment)),
				     1, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(ol, o, 0, 1, 1, 1, 1 ,1, 1, 1);


   /*- window info -*/
   o = e_widget_frametable_add(evas, _("Window"), 0);

   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Window Name")),
				     0, 0, 1, 1,
				     1, 1, 1, 1);

   entry = e_widget_entry_add(evas, &(data->wname));
   e_widget_min_size_set(entry, 100, 1);
   e_widget_frametable_object_append(o, entry,
				     1, 0, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Window Class")),
				     0, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(data->wclass)),
				     1, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Window Title")),
				     0, 2, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(data->wtitle)),
				     1, 2, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Window Role")),
				     0, 3, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(data->wrole)),
				     1, 3, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(ol, o, 0, 2, 1, 1, 1 ,1, 1, 1);

   /*- icon info -*/
   o = e_widget_frametable_add(evas, _("Icon Theme"), 0);

   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Icon Class")),
				     0, 0, 1, 1,
				     1, 1, 1, 1);

   entry = e_widget_entry_add(evas, &(data->iclass));
   e_widget_min_size_set(entry, 100, 1);
   e_widget_frametable_object_append(o, entry,
				     1, 0, 1, 1,
				     1, 1, 1, 1);

   e_widget_table_object_append(ol, o, 1, 1, 1, 1, 1 ,1, 1, 1);


   /*- misc info -*/
   o = e_widget_frametable_add(evas, _("Misc"), 0);
   e_widget_frametable_object_append(o, e_widget_check_add(evas, _("Startup Notify"), &(data->startup_notify)),
				     0, 0, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_check_add(evas, _("Wait Exit"), &(data->wait_exit)),
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
