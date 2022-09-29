#include "e_mod_main.h"
#ifdef HAVE_ELEMENTARY
# include <Elementary.h>
static Evas_Object * clipboard_win = NULL;
#endif

static Evry_Action *act;

static int
_action(Evry_Action *action)
{
   const Evry_Item *it = action->it1.item;
#ifdef HAVE_ELEMENTARY
   elm_cnp_selection_set(clipboard_win,
                        ELM_SEL_TYPE_PRIMARY,
                        ELM_SEL_FORMAT_TEXT,
                        it->label, strlen(it->label));
   elm_cnp_selection_set(clipboard_win,
                        ELM_SEL_TYPE_CLIPBOARD,
                        ELM_SEL_FORMAT_TEXT,
                        it->label, strlen(it->label));
#endif
   return 1;
}

static int
_check_item(Evry_Action *action __UNUSED__, const Evry_Item *it)
{
   return it && it->label && (strlen(it->label) > 0);
}

Eina_Bool
evry_plug_clipboard_init(void)
{
   if (!evry_api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;
#ifdef HAVE_ELEMENTARY
   Evas_Object *win = elm_win_add(NULL, NULL, ELM_WIN_BASIC);
   //ecore_x_icccm_name_class_set(win, "evry", "clipboard");
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
#endif
   return EINA_FALSE;
}

void
evry_plug_clipboard_shutdown(void)
{
#ifdef HAVE_ELEMENTARY
   if (clipboard_win) evas_object_del(clipboard_win);
   clipboard_win = NULL;
   evry_action_free(act);
#endif
}

