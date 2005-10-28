#include "e.h"

/*- DESCRIPTION -*/
/* An E_Object that brings up a file dialog window with a list of files, some
 * buttons, and a "Places" frame where the user can add his favorite places.
 * When the user selects a file, it will trigger a callback.
 */ 

static void _e_file_dialog_button1_click(void *data, E_Dialog *dia);
static void _e_file_dialog_button2_click(void *data, E_Dialog *dia);    
static void _e_file_dialog_free(E_File_Dialog *dia);

E_File_Dialog *
e_file_dialog_new(E_Container *con)
{
   E_File_Dialog *dia;
   Evas_Coord     w, h, ew, eh;
   E_Manager     *man;
   Evas          *evas;
   Evas_Object   *table, *ol;
   
   if (!con)
     {
	man = e_manager_current_get();
	if (!man) return NULL;
	con = e_container_current_get(man);
	if (!con) con = e_container_number_get(man, 0);
	if (!con) return NULL;
     }
   
   dia = E_OBJECT_ALLOC(E_File_Dialog, E_FILE_DIALOG_TYPE, _e_file_dialog_free);
   if(!dia) return NULL;
   dia->dia = e_dialog_new(con);
   if(!dia->dia)
     {
	free(dia);
	return NULL;
     }
   
   dia->con = con;
   dia->file = NULL;
   evas = dia->dia->win->evas;
   
   ol = e_widget_list_add(evas, 0, 1);
   
   table = e_widget_frametable_add(evas, "Places", 0);
   
   e_widget_frametable_object_append(table, e_widget_button_add(evas, "Home", "fileman/home", NULL,
								NULL, NULL),
				     0, 0, 1, 1, 1, 0, 1, 0);
   
   e_widget_frametable_object_append(table, e_widget_button_add(evas, " Desktop", "fileman/desktop", NULL,
								NULL, NULL),
				     0, 1, 1, 1, 1, 0, 1, 0);
   
   e_widget_frametable_object_append(table, e_widget_button_add(evas, " Icons", "fileman/folder", NULL,
								NULL, NULL),
				     0, 2, 1, 1, 1, 0, 1, 0);
   
   e_widget_list_object_append(ol, table, 1, 1, 0.0);
   
   table = e_widget_frametable_add(evas, "Select File", 0);
   
   e_widget_frametable_object_append(table, e_widget_fileman_add(evas, dia->file),
				     0, 0, 4, 4, 1, 1, 1, 1);
   
   e_widget_list_object_append(ol, table, 1, 1, 0.0);
   
   e_widget_min_size_get(ol, &w, &h);
   e_dialog_content_set(dia->dia, ol, w, h);
   
   e_dialog_button_add(dia->dia, "Ok", NULL, _e_file_dialog_button1_click, dia);
   e_dialog_button_add(dia->dia, "Cancel", NULL, _e_file_dialog_button2_click, dia);
   
   return dia;
}

void
e_file_dialog_show(E_File_Dialog *dia)
{
   e_dialog_show(dia->dia);
}

void
e_file_dialog_title_set(E_File_Dialog *dia, const char *title)
{
   e_dialog_title_set(dia->dia, title);
}


/* local subsystem functions */

static void
_e_file_dialog_button1_click(void *data, E_Dialog *dia)
{
   // TODO: Save or Ok ... clicked. Call user cb.
}

static void
_e_file_dialog_button2_click(void *data, E_Dialog *dia)
{
     _e_file_dialog_free(data);     	
}

static void
_e_file_dialog_free(E_File_Dialog *dia)
{
   e_object_del(E_OBJECT(dia->dia));
   free(dia);
}
