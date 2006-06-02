#include "e.h"

typedef struct _E_Entry_Dialog E_Entry_Dialog;
struct _E_Entry_Dialog 
{
   struct 
     {
	void *data;
	void (*func)(char *text, void *data);
     } ok;
   struct 
     {
	void *data;
	void (*func)(void *data);
     } cancel;
   E_Dialog *dia;
   Evas_Object *entry;
   char *text;
};

/* Private function definitions */
static void _e_entry_dialog_delete(E_Win *win);
static void _e_entry_dialog_ok(void *data, E_Dialog *dia);
static void _e_entry_dialog_cancel(void *data, E_Dialog *dia);

/* Externally accesible functions */
EAPI void
e_entry_dialog_show(const char *title, const char *icon, const char *text, 
		    const char *button_text, const char *button2_text, 
		    void (*func)(char *text, void *data), 
		    void (*func2)(void *data), void *data) 
{
   E_Entry_Dialog *ed;
   E_Dialog *dia;
   Evas_Object *o, *ob;
   int w, h;
   
   ed = E_NEW(E_Entry_Dialog, 1);
   ed->ok.func = func;
   ed->ok.data = data;
   ed->cancel.func = func2;
   ed->cancel.data = data;
   
   dia = e_dialog_new(e_container_current_get(e_manager_current_get()));
   if (!dia) 
     {
	E_FREE(ed);
	return;
     }
   dia->data = ed;
   ed->dia = dia;
   
   e_win_delete_callback_set(dia->win, _e_entry_dialog_delete);
   
   if (title) e_dialog_title_set(dia, title);
   if (icon) e_dialog_icon_set(dia, icon, 64);
   
   o = e_widget_list_add(dia->win->evas, 0, 0);
   if (text) 
     {
	ob = e_widget_label_add(dia->win->evas, text);
	e_widget_list_object_append(o, ob, 1, 0, 0.5);
     }
   
   ed->entry = e_widget_entry_add(dia->win->evas, &(ed->text));
   e_widget_list_object_append(o, ed->entry, 1, 1, 0.5);
   e_widget_min_size_get(o, &w, &h);
   e_dialog_content_set(dia, o, w, h);
   
   e_dialog_button_add(dia, !button_text ? _("Ok") : button_text, NULL, _e_entry_dialog_ok, ed);
   e_dialog_button_add(dia, !button2_text ? _("Cancel") : button2_text, NULL, _e_entry_dialog_cancel, ed);
   
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
}

/* Private Function Bodies */
static void
_e_entry_dialog_ok(void *data, E_Dialog *dia) 
{
   E_Entry_Dialog *ed;
   
   ed = data;
   if (ed->ok.func) ed->ok.func(ed->text, ed->ok.data);
   _e_entry_dialog_delete(ed->dia->win);
}

static void
_e_entry_dialog_cancel(void *data, E_Dialog *dia) 
{
   E_Entry_Dialog *ed;
   
   ed = data;
   if (ed->cancel.func) ed->cancel.func(ed->cancel.data);
   _e_entry_dialog_delete(ed->dia->win);   
}

static void
_e_entry_dialog_delete(E_Win *win) 
{
   E_Dialog *dia;
   E_Entry_Dialog *ed;
   
   dia = win->data;
   ed = dia->data;
   
   e_object_del(E_OBJECT(dia));
   free(ed);
}
