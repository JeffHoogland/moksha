#include "e.h"

static Evas_List views = NULL;

void
e_view_free(E_View *v)
{
   views = evas_list_remove(views, v);
   FREE(v);
}

E_View *
e_view_new(void)
{
   E_View *v;
   
   v = NEW(E_View, 1);
   ZERO(v, E_View, 1);
   OBJ_INIT(v, e_view_free);
   
   views = evas_list_append(views, v);
   return v;   
}

void
e_view_init(void)
{
}
