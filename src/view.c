#include "e.h"

E_View *
e_view_new(void)
{
   E_View *v;
   
   v = NEW(E_View, 1);
   ZERO(v, E_View, 1);
}

void
e_view_free(E_View *v)
{
}

void
e_view_init(void)
{
}
