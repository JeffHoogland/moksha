#include "Evry.h"


static Evry_Action *act;
static Ecore_X_Window clipboard_win = 0;

static int
_action(Evry_Action *act __UNUSED__, const Evry_Item *it, const Evry_Item *it2 __UNUSED__, const char *input __UNUSED__)
{
   ecore_x_selection_primary_set(clipboard_win, it->label, strlen(it->label));
   ecore_x_selection_clipboard_set(clipboard_win, it->label, strlen(it->label));
}

static int
_check_item(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   return (it && it->label && (strlen(it->label) > 0));
}

static Eina_Bool
_init(void)
{
   Ecore_X_Window win = ecore_x_window_new(0, 0, 0, 1, 1);
   if (!win) return EINA_FALSE;

   act = E_NEW(Evry_Action, 1);
   act->name = "Copy to Clipboard";
   act->is_default = EINA_TRUE;
   act->type_in1 = "TEXT";
   act->action = &_action;
   act->check_item = &_check_item;
   act->icon = "edit-copy";
   evry_action_register(act);

   clipboard_win = win;

   return EINA_TRUE;
}

static void
_shutdown(void)
{
   ecore_x_window_free(clipboard_win);
   evry_action_unregister(act);
   E_FREE(act);
}

EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);

