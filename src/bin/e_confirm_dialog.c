/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/******** private function definitions **********/
static void _e_confirm_dialog_free(E_Confirm_Dialog *cd);
static void _e_confirm_dialog_delete(E_Win *win);
static void _e_confirm_dialog_yes(void *data, E_Dialog *dia);
static void _e_confirm_dialog_no(void *data, E_Dialog *dia);


/********** externally accesible functions ****************/
EAPI E_Confirm_Dialog *
e_confirm_dialog_show(const char *title, const char *icon, const char *text,
		      const char *button_text, const char *button2_text,
                      void (*func)(void *data), void (*func2)(void *data),
                      void *data, void *data2,
                      void (*del_func)(void *data), void *del_data)
{
   E_Confirm_Dialog *cd; 
   E_Dialog *dia;

   cd = E_OBJECT_ALLOC(E_Confirm_Dialog, E_CONFIRM_DIALOG_TYPE, _e_confirm_dialog_free);
   cd->yes.func = func;
   cd->yes.data = data;
   cd->no.func = func2;
   cd->no.data = data2;
   cd->del.func = del_func;
   cd->del.data = del_data;

   dia = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_confirm_dialog");
   if (!dia)
     {
	e_object_del(E_OBJECT(cd));
	return NULL;
     }
   dia->data = cd;
   cd->dia = dia;

   e_win_delete_callback_set(dia->win, _e_confirm_dialog_delete);

   if (title) e_dialog_title_set(dia, title);
   if (icon) e_dialog_icon_set(dia, icon, 64);
   if (text) e_dialog_text_set(dia, text);

   e_dialog_button_add(dia, !button_text ? _("Yes") : button_text, NULL, _e_confirm_dialog_yes, cd);
   e_dialog_button_add(dia, !button2_text ? _("No") : button2_text, NULL, _e_confirm_dialog_no, cd);

   e_dialog_button_focus_num(dia, 1);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);

   return cd;
}

/********* private function bodies ************/
static void
_e_confirm_dialog_free(E_Confirm_Dialog *cd)
{
   if ((cd->dia) && (cd->dia->win))
     _e_confirm_dialog_delete(cd->dia->win);
}

static void
_e_confirm_dialog_yes(void *data, E_Dialog *dia)
{
   E_Confirm_Dialog *cd;

   cd = data;
   if (cd->yes.func) cd->yes.func(cd->yes.data);
   _e_confirm_dialog_delete(cd->dia->win);
}

static void
_e_confirm_dialog_no(void *data, E_Dialog *dia)
{
   E_Confirm_Dialog *cd;

   cd = data;
   if (cd->no.func) cd->no.func(cd->no.data);
   _e_confirm_dialog_delete(cd->dia->win);
}

static void
_e_confirm_dialog_delete(E_Win *win)
{
   E_Dialog *dia;
   E_Confirm_Dialog *cd;

   dia = win->data;
   cd = dia->data;

   if (cd->del.func) cd->del.func(cd->del.data);
   e_util_defer_object_del(E_OBJECT(dia));
   free(cd);
}
