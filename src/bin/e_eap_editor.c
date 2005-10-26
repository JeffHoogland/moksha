#include "e.h"

/* local subsystem functions */

static void _e_eap_edit_save_cb(void *data, E_Dialog *dia);
static void _e_eap_edit_cancel_cb(void *data, E_Dialog *dia);
static void _e_eap_edit_browse_cb(void *data1, void *data2);
static void _e_eap_edit_free(E_App_Edit *app);
static void _e_eap_edit_selector_cb(E_Fileman *fileman, char *file, void *data);
  
#define IFDUP(src, dst) if(src) dst = strdup(src); else dst = NULL

/* externally accessible functions */
E_App_Edit *
e_eap_edit_show(E_Container *con, E_App *a)
{
   E_Manager *man;
   E_App_Edit *app;
   Evas_Object *o, *ol;
   
   app = E_OBJECT_ALLOC(E_App_Edit, E_EAP_EDIT_TYPE, _e_eap_edit_free);
   if (!app) return NULL;
   app->dia = e_dialog_new(con);
   if (!app->dia)
     {
	free(app);
	return NULL;
     }
   
   app->con = con;
   e_dialog_title_set(app->dia, _("Eap Editor"));
   app->evas = e_win_evas_get(app->dia->win);

   app->eap = a;
   
   ol = e_widget_list_add(app->evas, 0, 0);
   o = e_widget_table_add(app->evas, _("Eap Info"), 0);   
   
   IFDUP(a->name, app->data.name);
   IFDUP(a->generic, app->data.generic);
   IFDUP(a->comment, app->data.comment);
   IFDUP(a->exe, app->data.exe);
   IFDUP(a->win_name, app->data.wname);
   IFDUP(a->win_class, app->data.wclass);
   IFDUP(a->win_title, app->data.wtitle);
   IFDUP(a->win_role, app->data.wrole);
   IFDUP(a->path, app->data.path);
   
   if(a->path)
     {
	app->img = edje_object_add(app->evas);	
	edje_object_file_set(app->img, a->path, "icon");
	
     }
   else
     {
	app->img = edje_object_add(app->evas);
	e_theme_edje_object_set(app->img, "base/theme/icons/enlightenment/e", "icons/enlightenment/e");
     }

   app->img_widget = e_widget_image_add_from_object(app->evas, app->img, 48, 48);
   e_widget_table_object_append(o, app->img_widget,
				0, 0, 1, 1,
				1, 1, 1, 1);
   
   e_widget_table_object_append(o, e_widget_button_add(app->evas, "Set Icon",
						       NULL, _e_eap_edit_browse_cb,
						       app, NULL),
				1, 0, 1, 1,
				0, 0, 0, 0);
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "App name"),
				     0, 1, 1, 1,
				     1, 1, 1, 1);
     {
	Evas_Object *entry;
	entry = e_widget_entry_add(app->evas, &(app->data.name));
	e_widget_min_size_set(entry, 100, 1);
	e_widget_table_object_append(o, entry,
				     1, 1, 1, 1,
				     1, 1, 1, 1);
     }
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Generic Info"),
				     0, 2, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_entry_add(app->evas, &(app->data.generic)),
				     1, 2, 1, 1,
				     1, 1, 1, 1);
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Comment"),
				     0, 3, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_entry_add(app->evas, &(app->data.comment)),
				     1, 3, 1, 1,
				     1, 1, 1, 1);
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Executable"),
				     0, 4, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_entry_add(app->evas, &(app->data.exe)),
				     1, 4, 1, 1,
				     1, 1, 1, 1);
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Window Name"),
				     0, 5, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_entry_add(app->evas, &(app->data.wname)),
				     1, 5, 1, 1,
				     1, 1, 1, 1);   
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Window Class"),
				     0, 6, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_entry_add(app->evas, &(app->data.wclass)),
				     1, 6, 1, 1,
				     1, 1, 1, 1);
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Window Title"),
				     0, 7, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_entry_add(app->evas, &(app->data.wtitle)),
				     1, 7, 1, 1,
				     1, 1, 1, 1);
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Window Role"),
				     0, 8, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_entry_add(app->evas, &(app->data.wrole)),
				     1, 8, 1, 1,
				     1, 1, 1, 1);   
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Icon Class"),
				     0, 9, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_entry_add(app->evas, &(app->data.iclass)),
				     1, 9, 1, 1,
				     1, 1, 1, 1);   
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Path"),
				     0, 10, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_entry_add(app->evas, &(app->data.path)),
				     1, 10, 1, 1,
				     1, 1, 1, 1);   
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Startup notify"),
				     0, 11, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_check_add(app->evas, "", &(app->data.startup_notify)),
				     1, 11, 1, 1,
				     1, 1, 1, 1);
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Wait exit"),
				     0, 12, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_check_add(app->evas, "", &(app->data.wait_exit)),
				     1, 12, 1, 1,
				     1, 1, 1, 1);
   
   e_widget_list_object_append(ol, o, 1, 1, 0.5);
   
     {
	Evas_Coord minw, minh;
	e_widget_min_size_get(ol, &minw, &minh);
	e_dialog_content_set(app->dia, ol, minw, minh);
     }

   e_dialog_button_add(app->dia, _("Cancel"), NULL, _e_eap_edit_cancel_cb, app);
   e_dialog_button_add(app->dia, _("Save"), NULL, _e_eap_edit_save_cb, app);
   
   e_dialog_show(app->dia);
}

/* local subsystem functions */

static void 
_e_eap_edit_save_cb(void *data, E_Dialog *dia)
{
   E_App_Edit *editor;
   E_App *eap;
   
   editor = data;
   eap = editor->eap;
   
   if(!(editor->data.path))
     return;

   printf("path : %s\n", editor->data.path);
   
   E_FREE(eap->name);
   E_FREE(eap->generic);
   E_FREE(eap->comment);
   E_FREE(eap->exe);
   E_FREE(eap->win_name);
   E_FREE(eap->win_class);   
   E_FREE(eap->win_title);
   E_FREE(eap->win_role);
   E_FREE(eap->icon_class);
   E_FREE(eap->path);
   E_FREE(eap->image);
   if(editor->data.startup_notify)
     eap->startup_notify = 1;   
   if(editor->data.wait_exit)
     eap->wait_exit = 1;
   
   IFDUP(editor->data.name, eap->name);
   IFDUP(editor->data.generic, eap->generic);
   IFDUP(editor->data.comment, eap->comment);
   IFDUP(editor->data.exe, eap->exe);
   IFDUP(editor->data.wname, eap->win_name);
   IFDUP(editor->data.wclass, eap->win_class);
   IFDUP(editor->data.wtitle, eap->win_title);
   IFDUP(editor->data.wrole, eap->win_role);
   IFDUP(editor->data.iclass, eap->icon_class);
   IFDUP(editor->data.path, eap->path);
   IFDUP(editor->data.image, eap->image);
   
   printf("image is: %s\n", eap->image);

   printf("eap path: %s\n", eap->path);
   
   e_app_fields_save(eap);
   
   _e_eap_edit_free(editor);
}

static void 
_e_eap_edit_cancel_cb(void *data, E_Dialog *dia)
{
   _e_eap_edit_free(data);
}

static void 
_e_eap_edit_browse_cb(void *data1, void *data2)
{
   E_Fileman *fileman;
   E_App_Edit *editor;
   
   editor = data1;
   fileman = e_fileman_new (editor->dia->win->container);
   e_fileman_selector_enable(fileman, _e_eap_edit_selector_cb, editor);
   e_win_resize(fileman->win, 640, 300);
   e_fileman_show (fileman);
}

/* need to make sure we can load the image */
static void 
_e_eap_edit_selector_cb(E_Fileman *fileman, char *file, void *data)
{
   E_App_Edit *editor;
   char *ext;
   Evas_Object *img;
   
   ext = strrchr(file, '.');
   if(!ext)
     return;
   if(strcasecmp(ext, ".png") && strcasecmp(ext, ".jpg") &&
      strcasecmp(ext, ".jpeg"))
     return;

   editor = data;
   E_FREE(editor->eap->image);
   editor->data.image = strdup(file);
   e_widget_sub_object_del(editor->img_widget, editor->img);
   evas_object_del(editor->img);
   editor->img = evas_object_image_add(editor->evas);
   evas_object_image_file_set(editor->img, file, NULL);
   evas_object_image_fill_set(editor->img, 0, 0, 48, 48);
   evas_object_resize(editor->img, 48, 48);
   evas_object_show(editor->img);
   e_widget_resize_object_set(editor->img_widget, editor->img);
   e_widget_sub_object_add(editor->img_widget, editor->img);
   e_widget_min_size_set(editor->img_widget, 48, 48);   
   e_widget_change(editor->img_widget);
   
   e_object_del(fileman);
}

static void _e_eap_edit_free(E_App_Edit *app)
{
   e_object_del(E_OBJECT(app->eap));
   e_object_del(E_OBJECT(app->dia));
   E_FREE(app);
}
