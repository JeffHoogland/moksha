/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "e.h"

static Evas_List *_e_iconic_borders = NULL;

int
e_iconify_init(void)
{
   /* FIXME: initialize some ecore events for iconify/uniconify
    * (for things like modules to upate state
    */
   return 1;
}

int
e_iconify_shutdown(void)
{
   evas_list_free(_e_iconic_borders);
   return 1;
}

Evas_List *
e_iconify_clients_list_get(void)
{
   return _e_iconic_borders;
}

int
e_iconify_border_iconfied(E_Border *bd)
{
   if (evas_list_find(_e_iconic_borders, bd)) return 1;
   else return 0;
}

void
e_iconify_border_add(E_Border *bd)
{
   E_OBJECT_CHECK(bd);

   /* FIXME send iconify event for this border */
   _e_iconic_borders = evas_list_append(_e_iconic_borders, bd);
   
}

void
e_iconify_border_remove(E_Border *bd)
{
   /* FIXME send uniconify event for this border */
   _e_iconic_borders = evas_list_remove(_e_iconic_borders, bd);
}


