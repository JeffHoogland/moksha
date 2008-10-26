/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

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
   Eina_List *l;
   int i, sel = -1;
   
   o = e_widget_list_add(pg->evas, 1, 0);
   e_wizard_title_set(_("Launcher Bar"));
   
   of = e_widget_framelist_add(pg->evas, _("Select applications"), 0);
	
   ob = e_widget_ilist_add(pg->evas, 32 * e_scale, 32 * e_scale, NULL);
   e_widget_min_size_set(ob, 140 * e_scale, 140 * e_scale);
	
   e_widget_ilist_freeze(ob);
   e_widget_ilist_go(ob);
   e_widget_ilist_thaw(ob);
   
   if (sel >= 0) e_widget_ilist_selected_set(ob, sel);
   
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
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
   return 1;
}
