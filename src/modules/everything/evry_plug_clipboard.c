#include "e_mod_main.h"

static Evry_Action *act;
static Ecore_X_Window clipboard_win = 0;

static int
_action(Evry_Action *act)
{
   const Evry_Item *it = act->it1.item;

   ecore_x_selection_primary_set(clipboard_win, it->label, strlen(it->label));
   ecore_x_selection_clipboard_set(clipboard_win, it->label, strlen(it->label));

   return 1;
}

static int
_check_item(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   return (it && it->label && (strlen(it->label) > 0));
}

Eina_Bool
evry_plug_clipboard_init(void)
{
   if (!evry_api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   Ecore_X_Window win = ecore_x_window_new(0, 0, 0, 1, 1);
   if (!win) return EINA_FALSE;

//FIXME: Icon name doesn't follow FDO Spec
   act = EVRY_ACTION_NEW("Copy to Clipboard",
			 EVRY_TYPE_TEXT, 0,
			 "everything-clipboard",
			 _action, _check_item);
   act->remember_context = EINA_TRUE;
   evry_action_register(act, 10);

   clipboard_win = win;

   return EINA_TRUE;
}

void
evry_plug_clipboard_shutdown(void)
{
   ecore_x_window_free(clipboard_win);
   evry_action_free(act);
}
