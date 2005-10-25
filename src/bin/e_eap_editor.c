#include "e.h"

/* local subsystem functions */

static void _e_eap_edit_save_cb(void *data, E_Dialog *dia);
static void _e_eap_edit_cancel_cb(void *data, E_Dialog *dia);
static void _e_eap_edit_browse_cb(void *data1, void *data2);
static void _e_eap_edit_free(E_App_Edit *app);


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
	entry = e_widget_entry_add(app->evas, NULL);
	e_widget_min_size_set(entry, 100, 1);
	e_widget_table_object_append(o, entry,
				     1, 1, 1, 1,
				     1, 1, 1, 1);
     }
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Generic Info"),
				     0, 2, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_entry_add(app->evas, NULL),
				     1, 2, 1, 1,
				     1, 1, 1, 1);
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Comments"),
				     0, 3, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_entry_add(app->evas, NULL),
				     1, 3, 1, 1,
				     1, 1, 1, 1);
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Executable"),
				     0, 4, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_entry_add(app->evas, NULL),
				     1, 4, 1, 1,
				     1, 1, 1, 1);
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Window Name"),
				     0, 5, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_entry_add(app->evas, NULL),
				     1, 5, 1, 1,
				     1, 1, 1, 1);   
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Window Class"),
				     0, 6, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_entry_add(app->evas, NULL),
				     1, 6, 1, 1,
				     1, 1, 1, 1);
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Path"),
				     0, 7, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_entry_add(app->evas, NULL),
				     1, 7, 1, 1,
				     1, 1, 1, 1);   
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Startup notify"),
				     0, 8, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_check_add(app->evas, "", NULL),
				     1, 8, 1, 1,
				     1, 1, 1, 1);
   
   e_widget_table_object_append(o, e_widget_label_add(app->evas, "Wait exit"),
				     0, 9, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(o, e_widget_check_add(app->evas, "", NULL),
				     1, 9, 1, 1,
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
   E_App_Edit *app;
   E_App *a;
   
   app = data;
   a = app->eap;
}

static void 
_e_eap_edit_cancel_cb(void *data, E_Dialog *dia)
{
   _e_eap_edit_free(data);
}

static void 
_e_eap_edit_browse_cb(void *data1, void *data2)
{}

static void _e_eap_edit_free(E_App_Edit *app)
{
   e_object_del(E_OBJECT(app->eap));
   e_object_del(E_OBJECT(app->dia));
   E_FREE(app);
}
