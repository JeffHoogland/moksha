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
static void _e_dialog_cb_delete(E_Win *win);
    
/* local subsystem globals */

/* externally accessible functions */

E_Dialog *
e_dialog_new(E_Container *con)
{
   E_Dialog *dia;
   E_Manager *man;
   Evas_Object *o;
   
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
			  1, 1, /* expand */
			  0.5, 0.5, /* align */
			  mw, mh, /* min */
			  9999, mh /* max */
			  );
   evas_object_show(db->obj);
   
   dia->buttons = evas_list_append(dia->buttons, db);
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
e_dialog_icon_set(E_Dialog *dia, char *icon)
{
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
   e_object_del(E_OBJECT(dia->win));
   free(dia);
}

static void
_e_dialog_cb_button_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Dialog_Button *db;
   
   db = data;
   if (db->func)
     db->func(db->data, db->dialog);
   else
     e_object_del(E_OBJECT(db->dialog));
}

static void
_e_dialog_cb_delete(E_Win *win)
{
   E_Dialog *dia;
   
   dia = win->data;
   e_object_del(E_OBJECT(dia));
}
