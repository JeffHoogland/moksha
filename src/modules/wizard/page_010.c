/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

static char *lang = NULL;

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
    
   o = e_widget_list_add(pg->evas, 1, 0);
   of = e_widget_framelist_add(pg->evas, _("Choose Language"), 0);
   ob = e_widget_ilist_add(pg->evas, 16, 16, &lang);
   e_widget_min_size_set(ob, 175, 175);
   e_widget_framelist_object_append(of, ob);
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
   printf("a2\n");
   return 1;
}
