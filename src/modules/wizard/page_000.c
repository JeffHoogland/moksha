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
static int
_next_page(void *data)
{
   e_wizard_button_next_enable_set(1);
   e_wizard_next();
   return 0;
}
EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   e_wizard_title_set(_("Enlightenment"));
   e_wizard_button_next_enable_set(0);
   ecore_timer_add(2.0, _next_page, NULL);
   return 1;
}
EAPI int
wizard_page_hide(E_Wizard_Page *pg)
{
   return 1;
}
EAPI int
wizard_page_apply(E_Wizard_Page *pg)
{
   return 1;
}
