/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */

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
   
   return (E_Theme_About *)od;
}

EAPI void
e_theme_about_show(E_Theme_About *about)
{
   e_obj_dialog_show((E_Obj_Dialog *)about);
   e_obj_dialog_icon_set((E_Obj_Dialog *)about, "preferences-desktop-theme");
}
