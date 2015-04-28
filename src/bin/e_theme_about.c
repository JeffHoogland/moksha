#include "e.h"

/* local subsystem functions */
static void
_cb_settings_theme(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Obj_Dialog *od = data;
   
   e_configure_registry_call("appearance/theme", od->win->container, NULL);
}
   
/* local subsystem globals */

/* externally accessible functions */

EAPI E_Theme_About *
e_theme_about_new(E_Container *con)
{
   E_Obj_Dialog *od;

   od = e_obj_dialog_new(con, _("About Theme"), "E", "_theme_about");
   if (!od) return NULL;
   e_obj_dialog_obj_theme_set(od, "base/theme", "e/theme/about");
   e_obj_dialog_obj_part_text_set(od, "e.text.label", _("Close"));
   e_obj_dialog_obj_part_text_set(od, "e.text.theme", _("Select Theme"));
   edje_object_signal_callback_add(od->bg_object, 
                                   "e,action,settings,theme", "",
                                   _cb_settings_theme, od);
   return (E_Theme_About *)od;
}

EAPI void
e_theme_about_show(E_Theme_About *about)
{
   e_obj_dialog_show((E_Obj_Dialog *)about);
   e_obj_dialog_icon_set((E_Obj_Dialog *)about, "preferences-desktop-theme");
}
