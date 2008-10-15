/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void _e_obj_dialog_free(E_Obj_Dialog *od);
static void _e_obj_dialog_cb_delete(E_Win *win);
static void _e_obj_dialog_cb_close(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_obj_dialog_cb_resize(E_Win *win);

/* local subsystem globals */

/* externally accessible functions */

EAPI E_Obj_Dialog *
e_obj_dialog_new(E_Container *con, char *title, char *class_name, char *class_class)
{
   E_Obj_Dialog *od;
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
   od = E_OBJECT_ALLOC(E_Obj_Dialog, E_OBJ_DIALOG_TYPE, _e_obj_dialog_free);
   if (!od) return NULL;
   od->win = e_win_new(con);
   if (!od->win)
     {
	free(od);
	return NULL;
     }
   e_win_delete_callback_set(od->win, _e_obj_dialog_cb_delete);
   e_win_resize_callback_set(od->win, _e_obj_dialog_cb_resize);
   od->win->data = od;
   e_win_dialog_set(od->win, 1);
   e_win_name_class_set(od->win, class_name, class_class);
   e_win_title_set(od->win, title);
   
   o = edje_object_add(e_win_evas_get(od->win));
   od->bg_object = o;
   
   e_win_centered_set(od->win, 1);
   od->cb_delete = NULL;
   
   return od;
}

EAPI void
e_obj_dialog_cb_delete_set(E_Obj_Dialog *od, void (*func)(E_Obj_Dialog *od))
{
   od->cb_delete = func;
}

EAPI void
e_obj_dialog_icon_set(E_Obj_Dialog *od, char *icon)
{
   E_OBJECT_CHECK(od);
   E_OBJECT_TYPE_CHECK(od, E_OBJ_DIALOG_TYPE);
   if (od->win->border->internal_icon)
     {
	eina_stringshare_del(od->win->border->internal_icon);
	od->win->border->internal_icon = NULL;
     }
   if (icon)
     od->win->border->internal_icon = eina_stringshare_add(icon);
}

EAPI void
e_obj_dialog_show(E_Obj_Dialog *od)
{
   Evas_Coord mw, mh;
   
   E_OBJECT_CHECK(od);
   E_OBJECT_TYPE_CHECK(od, E_OBJ_DIALOG_TYPE);

   edje_object_size_min_calc(od->bg_object, &mw, &mh);
   evas_object_resize(od->bg_object, mw, mh);
   e_win_resize(od->win, mw, mh);
   e_win_size_min_set(od->win, mw, mh);
   e_win_size_max_set(od->win, mw, mh);
   
   e_win_show(od->win);
}

EAPI void
e_obj_dialog_obj_part_text_set(E_Obj_Dialog *od, char *part, char *text)
{
   E_OBJECT_CHECK(od);
   E_OBJECT_TYPE_CHECK(od, E_OBJ_DIALOG_TYPE);
   edje_object_part_text_set(od->bg_object, part, text);
}

EAPI void
e_obj_dialog_obj_theme_set(E_Obj_Dialog *od, char *theme_cat, char *theme_obj)
{
   E_OBJECT_CHECK(od);
   E_OBJECT_TYPE_CHECK(od, E_OBJ_DIALOG_TYPE);
   
   e_theme_edje_object_set(od->bg_object, theme_cat, theme_obj);
   evas_object_move(od->bg_object, 0, 0);
   evas_object_show(od->bg_object);
   edje_object_signal_callback_add(od->bg_object, "e,action,close", "",
				   _e_obj_dialog_cb_close, od);
}

/* local subsystem functions */
static void
_e_obj_dialog_free(E_Obj_Dialog *od)
{
   if (od->bg_object) evas_object_del(od->bg_object);
   e_object_del(E_OBJECT(od->win));
   free(od);
}

static void
_e_obj_dialog_cb_delete(E_Win *win)
{
   E_Obj_Dialog *od;
   
   od = win->data;
   if (od->cb_delete)
     od->cb_delete(od);
   e_object_del(E_OBJECT(od));
}

static void
_e_obj_dialog_cb_close(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Obj_Dialog *od;
   
   od = data;
   if (od->cb_delete)
     od->cb_delete(od);
   e_util_defer_object_del(E_OBJECT(od));
}

static void
_e_obj_dialog_cb_resize(E_Win *win)
{
   E_Obj_Dialog *od;
   
   od = win->data;
   evas_object_resize(od->bg_object, od->win->w, od->win->h);
}
