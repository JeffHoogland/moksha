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
   Evas_Object *ob, *o;
    
   printf("t2\n");
   ob = e_widget_list_add(pg->evas, 1, 0);
   o = e_widget_button_add(pg->evas,
			   "Hello Another World", NULL,
			   NULL, NULL, NULL);
   e_widget_list_object_append(ob, o, 0, 0, 0.5);
   evas_object_show(o);
   e_wizard_page_show(ob);
   pg->data = ob;
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
