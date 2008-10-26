/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

static int focus_mode = 1;

EAPI int
wizard_page_init(E_Wizard_Page *pg)
{
   return 1;
}
EAPI int
wizard_page_shutdown(E_Wizard_Page *pg)
{
   return 1;
}
EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;
   
   o = e_widget_list_add(pg->evas, 1, 0);
   e_wizard_title_set(_("Window Focus"));
   
   of = e_widget_framelist_add(pg->evas, _("Focus mode"), 0);

   rg = e_widget_radio_group_new(&focus_mode);
   
   ob = e_widget_radio_add(pg->evas, _("Click to focus windows"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   evas_object_show(ob);
   ob = e_widget_radio_add(pg->evas, _("Mouse over focuses windows"), 1, rg);
   e_widget_framelist_object_append(of, ob);
   evas_object_show(ob);
   
   e_widget_list_object_append(o, of, 0, 0, 0.5);
   evas_object_show(ob);
   evas_object_show(of);

   e_wizard_page_show(o);
   pg->data = of;
   return 1; /* 1 == show ui, and wait for user, 0 == just continue */
}
EAPI int
wizard_page_hide(E_Wizard_Page *pg)
{
   evas_object_del(pg->data);
   return 1;
}
EAPI int
wizard_page_apply(E_Wizard_Page *pg)
{
   if (!focus_mode)
     {
	// FIXME: click to focus
     }
   else
     {
	// FIXME: sloppy focus
     }
   return 1;
}
