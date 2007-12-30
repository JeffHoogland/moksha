#include "e.h"

/* Private function definitions */
static void _e_entry_dialog_free(E_Entry_Dialog *dia);
static void _e_entry_dialog_ok(void *data, E_Dialog *dia);
static void _e_entry_dialog_cancel(void *data, E_Dialog *dia);
static void _e_entry_dialog_delete(E_Win *win);
static void _e_entry_cb_key_down(void *data, Evas_Object *obj, void *event_info);

/* Externally accesible functions */
EAPI E_Entry_Dialog *
e_entry_dialog_show(const char *title, const char *icon, const char *text,
		    const char *initial_text,
		    const char *button_text, const char *button2_text,
		    void (*ok_func)(char *text, void *data), 
		    void (*cancel_func)(void *data), void *data) 
{
   E_Entry_Dialog *ed;
   E_Dialog *dia;
   Evas_Object *o, *ob;
   int w, h;

   ed = E_OBJECT_ALLOC(E_Entry_Dialog, E_ENTRY_DIALOG_TYPE, _e_entry_dialog_free);
   ed->ok.func = ok_func;
   ed->ok.data = data;
   ed->cancel.func = cancel_func;
   ed->cancel.data = data;
   if (initial_text)
     ed->text = strdup(initial_text);
   
   dia = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_entry_dialog");
   if (!dia) 
     {
	e_object_del(E_OBJECT(ed));
	return NULL;
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
   
   ed->entry = e_widget_entry_add(dia->win->evas, &(ed->text), NULL, NULL, NULL);
   evas_object_smart_callback_add(ed->entry, "key_down", _e_entry_cb_key_down, ed);
   e_widget_list_object_append(o, ed->entry, 1, 1, 0.5);
   e_widget_min_size_get(o, &w, &h);
   e_dialog_content_set(dia, o, w, h);
   
   e_dialog_button_add(dia, !button_text ? _("OK") : button_text, NULL, _e_entry_dialog_ok, ed);
   e_dialog_button_add(dia, !button2_text ? _("Cancel") : button2_text, NULL, _e_entry_dialog_cancel, ed);
   
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
   e_widget_focus_set(ed->entry, 1);
   return ed;
}

/* Private Function Bodies */
static void
_e_entry_dialog_free(E_Entry_Dialog *ed)
{
   e_object_del(E_OBJECT(ed->dia));
   evas_object_smart_callback_del(ed->entry, "key_down", _e_entry_cb_key_down);
   E_FREE(ed->text);
   free(ed);
}
    
static void
_e_entry_dialog_ok(void *data, E_Dialog *dia) 
{
   E_Entry_Dialog *ed;
   
   ed = data;
   e_object_ref(E_OBJECT(ed));
   if (ed->ok.func) ed->ok.func(ed->text, ed->ok.data);
   e_object_del(E_OBJECT(ed));
   e_object_unref(E_OBJECT(ed));
}

static void
_e_entry_dialog_cancel(void *data, E_Dialog *dia) 
{
   E_Entry_Dialog *ed;
   
   ed = data;
   e_object_ref(E_OBJECT(ed));
   if (ed->cancel.func) ed->cancel.func(ed->cancel.data);
   e_object_del(E_OBJECT(ed));
   e_object_unref(E_OBJECT(ed));
}

static void
_e_entry_dialog_delete(E_Win *win) 
{
   E_Dialog *dia;
   E_Entry_Dialog *ed;
   
   dia = win->data;
   ed = dia->data;
   e_object_del(E_OBJECT(ed));
}

static void 
_e_entry_cb_key_down(void *data, Evas_Object *obj, void *event_info) 
{
   Evas_Event_Key_Down *ev;
   E_Entry_Dialog *ed;
   
   ev = event_info;
   if (strcmp(ev->keyname, "Return")) return;
   if (!(ed = data)) return;
   e_object_ref(E_OBJECT(ed));
   if (ed->ok.func) ed->ok.func(ed->text, ed->ok.data);
   e_object_del(E_OBJECT(ed));
   e_object_unref(E_OBJECT(ed));
}
