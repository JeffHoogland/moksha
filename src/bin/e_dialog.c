/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Dialog_Button E_Dialog_Button;

struct _E_Dialog_Button
{
   E_Dialog *dialog;
   Evas_Object *obj, *obj_icon;
   char *label;
   char *icon;
   void (*func) (void *data, E_Dialog *dia);
   void *data;
};

/* local subsystem functions */
static void _e_dialog_free(E_Dialog *dia);
static void _e_dialog_cb_button_clicked(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_dialog_button_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event);
static void _e_dialog_button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event);
static void _e_dialog_cb_delete(E_Win *win);
static void _e_dialog_cb_key_down(void *data, Evas *e, Evas_Object *obj, void *event);


/* local subsystem globals */

/* externally accessible functions */

E_Dialog *
e_dialog_new(E_Container *con)
{
   E_Dialog *dia;
   E_Manager *man;
   Evas_Object *o;
   Evas_Modifier_Mask mask;
   
   if (!con)
     {
	man = e_manager_current_get();
	if (!man) return NULL;
	con = e_container_current_get(man);
	if (!con) con = e_container_number_get(man, 0);
	if (!con) return NULL;
     }
   dia = E_OBJECT_ALLOC(E_Dialog, E_DIALOG_TYPE, _e_dialog_free);
   if (!dia) return NULL;
   dia->win = e_win_new(con);
   if (!dia->win)
     {
	free(dia);
	return NULL;
     }
   ecore_x_netwm_window_type_set(dia->win->evas_win, ECORE_X_WINDOW_TYPE_DIALOG);
   e_win_delete_callback_set(dia->win, _e_dialog_cb_delete);
   dia->win->data = dia;
   e_win_name_class_set(dia->win, "E", "_dialog");
   o = edje_object_add(e_win_evas_get(dia->win));
   dia->bg_object = o;
   e_theme_edje_object_set(o, "base/theme/dialog",
			   "widgets/dialog/main");
   evas_object_move(o, 0, 0);
   evas_object_show(o);
   
   o = edje_object_add(e_win_evas_get(dia->win));
   dia->text_object = o;
   e_theme_edje_object_set(o, "base/theme/dialog",
			   "widgets/dialog/text");
   edje_object_part_swallow(dia->bg_object, "content_swallow", o);
   evas_object_show(o);
   
   o = e_box_add(e_win_evas_get(dia->win));
   dia->box_object = o;
   e_box_orientation_set(o, 1);
   e_box_homogenous_set(o, 1);
   e_box_align_set(o, 0.5, 0.5);
   edje_object_part_swallow(dia->bg_object, "buttons_swallow", o);
   evas_object_show(o);

   o = evas_object_rectangle_add(e_win_evas_get(dia->win));
   dia->event_object = o;
   mask = 0;
   evas_object_key_grab(o, "Tab", mask, ~mask, 0);
   mask = evas_key_modifier_mask_get(e_win_evas_get(dia->win), "Shift");
   evas_object_key_grab(o, "Tab", mask, ~mask, 0);
   mask = 0;
   evas_object_key_grab(o, "Return", mask, ~mask, 0);
   mask = 0;
   evas_object_key_grab(o, "space", mask, ~mask, 0);
   
   evas_object_event_callback_add(o, EVAS_CALLBACK_KEY_DOWN, _e_dialog_cb_key_down, dia);

   dia->focused = NULL;
 
   return dia;
}

void
e_dialog_button_add(E_Dialog *dia, char *label, char *icon, void (*func) (void *data, E_Dialog *dia), void *data)
{
   E_Dialog_Button *db;
   Evas_Coord mw, mh;
   
   db = E_NEW(E_Dialog_Button, 1);
   db->dialog = dia;
   if (label) db->label = strdup(label);
   if (icon) db->icon = strdup(icon);
   db->func = func;
   db->data = data;
   db->obj = edje_object_add(e_win_evas_get(dia->win));
   e_theme_edje_object_set(db->obj, "base/theme/dialog",
			   "widgets/dialog/button");
   edje_object_signal_callback_add(db->obj, "click", "",
				   _e_dialog_cb_button_clicked, db);
   edje_object_part_text_set(db->obj, "button_text", db->label);
   
   evas_object_event_callback_add(db->obj, EVAS_CALLBACK_MOUSE_IN, _e_dialog_button_cb_mouse_in, dia);
   evas_object_event_callback_add(db->obj, EVAS_CALLBACK_MOUSE_DOWN, _e_dialog_button_cb_mouse_down, db);
   
   if (icon)
     {
	db->obj_icon = edje_object_add(e_win_evas_get(dia->win));
	e_util_edje_icon_set(db->obj_icon, icon);
	edje_object_part_swallow(db->obj, "icon_swallow", db->obj_icon);
	edje_object_signal_emit(db->obj, "icon_visible", "");
	edje_object_message_signal_process(db->obj);
	evas_object_show(db->obj_icon);
     }
   edje_object_calc_force(db->obj);
   edje_object_size_min_calc(db->obj, &mw, &mh);
   e_box_pack_end(dia->box_object, db->obj);
   e_box_pack_options_set(db->obj,
			  1, 1, /* fill */
			  0, 1, /* expand */
			  0.5, 0.5, /* align */
			  mw, mh, /* min */
			  9999, mh /* max */
			  );
   evas_object_show(db->obj);
   
   dia->buttons = evas_list_append(dia->buttons, db);
}

int
e_dialog_button_focus_num(E_Dialog *dia, int button)
{
   E_Dialog_Button *db = NULL;

   if (button < 0)
     return 0;
   
   db = evas_list_nth(dia->buttons, button);
    
   if (!db) 
     return 0;

   if (dia->focused)
     {
	E_Dialog_Button *focused;

	focused = dia->focused->data;
	if (focused)
	  edje_object_signal_emit(focused->obj, "unfocus", "");      
     }

   dia->focused = evas_list_nth_list(dia->buttons, button);
   edje_object_signal_emit(db->obj, "focus", "");

   return 1;
}

int
e_dialog_button_focus_button(E_Dialog *dia, E_Dialog_Button *button)
{
   E_Dialog_Button *db = NULL;
   
   if (!button)
     return 0;
   
   db = evas_list_find(dia->buttons, button);
 
   if (!db)
     return 0;
   
   if (dia->focused)
     {
	E_Dialog_Button *focused;

	focused = dia->focused->data;
	if (focused)
	  edje_object_signal_emit(focused->obj, "unfocus", "");      
     }

   dia->focused = evas_list_find_list(dia->buttons, button);
   edje_object_signal_emit(db->obj, "focus", "");

   return 1;
}

void
e_dialog_title_set(E_Dialog *dia, char *title)
{
   e_win_title_set(dia->win, title);
}

void
e_dialog_text_set(E_Dialog *dia, char *text)
{
   edje_object_part_text_set(dia->text_object, "text", text);
}

void
e_dialog_icon_set(E_Dialog *dia, char *icon, Evas_Coord size)
{
   if (icon)
     {
	dia->icon_object = edje_object_add(e_win_evas_get(dia->win));
	e_util_edje_icon_set(dia->icon_object, icon);
	edje_extern_object_min_size_set(dia->icon_object, size, size);
	edje_object_part_swallow(dia->bg_object, "icon_swallow", dia->icon_object);
	evas_object_show(dia->icon_object);
     }
}

void
e_dialog_show(E_Dialog *dia)
{
   Evas_Coord mw, mh;
   Evas_Object *o;
   
   o = dia->text_object;
   edje_object_size_min_calc(o, &mw, &mh);
   edje_extern_object_min_size_set(o, mw, mh);
   edje_object_part_swallow(dia->bg_object, "content_swallow", o);

   o = dia->box_object;
   e_box_min_size_get(o, &mw, &mh);
   edje_extern_object_min_size_set(o, mw, mh);
   edje_object_part_swallow(dia->bg_object, "buttons_swallow", o);
   
   edje_object_size_min_calc(dia->bg_object, &mw, &mh);
   evas_object_resize(dia->bg_object, mw, mh);
   e_win_resize(dia->win, mw, mh);
   e_win_size_min_set(dia->win, mw, mh);
   e_win_size_max_set(dia->win, mw, mh);
   e_win_show(dia->win);
}

/* local subsystem functions */
static void
_e_dialog_free(E_Dialog *dia)
{
   while (dia->buttons)
     {
	E_Dialog_Button *db;
	
	db = dia->buttons->data;
	dia->buttons = evas_list_remove_list(dia->buttons, dia->buttons);
	E_FREE(db->label);
	E_FREE(db->icon);
	evas_object_del(db->obj);
	if (db->obj_icon) evas_object_del(db->obj_icon);
	free(db);
     }
   if (dia->text_object) evas_object_del(dia->text_object);
   if (dia->icon_object) evas_object_del(dia->icon_object);
   if (dia->box_object) evas_object_del(dia->box_object);
   if (dia->bg_object) evas_object_del(dia->bg_object);
   if (dia->event_object) evas_object_del(dia->event_object);
   e_object_del(E_OBJECT(dia->win));
   free(dia);
}

static void
_e_dialog_cb_button_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Dialog_Button *db;
   
   db = data;
   if (db->func) 
   {
      edje_object_signal_emit(db->obj, "focus", "");
      db->func(db->data, db->dialog);
   }
   else
     e_object_del(E_OBJECT(db->dialog));
}

static void
_e_dialog_button_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event)
{        
   edje_object_signal_emit(obj, "enter", "");  
}

static void
_e_dialog_button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event)
{        
   E_Dialog *dia;
   E_Dialog_Button *db;
   
   db = data;
   dia = db->dialog;
   
   e_dialog_button_focus_button(dia, db);
}

/* TODO: Implement shift-tab and left arrow */
static void
_e_dialog_cb_key_down(void *data, Evas *e, Evas_Object *obj, void *event)
{
   Evas_Event_Key_Down *ev;
   E_Dialog *dia;

   ev = event;
   dia = data;

   if (!strcmp(ev->keyname, "Tab"))
     {
	if (dia->focused && dia->buttons)
	  {
	     E_Dialog_Button *db;
	     
	     db = dia->focused->data;	 
	     edje_object_signal_emit(db->obj, "unfocus", "");	     
	     if (evas_key_modifier_is_set(evas_key_modifier_get(e_win_evas_get(dia->win)), "Shift"))
	       {
		  if (dia->focused->prev) dia->focused = dia->focused->prev;
		  else dia->focused = evas_list_last(dia->buttons);
	       }
	     else
	       {
		  if (dia->focused->next) dia->focused = dia->focused->next;
		  else dia->focused = dia->buttons;	 
	       }
	     db = evas_list_data(dia->focused);
	     edje_object_signal_emit(db->obj, "focus", "");
	     edje_object_signal_emit(db->obj, "enter", "");
	     
	  }
       	else
	  {
	     E_Dialog_Button *db;

	     dia->focused = dia->buttons;

	     db = dia->focused->data;
	     edje_object_signal_emit(db->obj, "focus", "");
	     edje_object_signal_emit(db->obj, "enter", "");	     
	  }
     }
   else if (((!strcmp(ev->keyname, "Return")) || 
	     (!strcmp(ev->keyname, "space"))) && dia->focused)
     {
	E_Dialog_Button *db;

	db = evas_list_data(dia->focused);
	edje_object_signal_emit(db->obj, "click", "");      
     }
}

static void
_e_dialog_cb_delete(E_Win *win)
{
   E_Dialog *dia;
   
   dia = win->data;
   e_object_del(E_OBJECT(dia));
}
