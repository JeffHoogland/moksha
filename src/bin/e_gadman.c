/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void _e_gadman_free(E_Gadman *gm);

/* externally accessible functions */
int
e_gadman_init(void)
{
   return 1;
}

int
e_gadman_shutdown(void)
{
   return 1;
}

E_Gadman *
e_gadman_new(E_Container *con)
{
   E_Gadman    *gm;

   gm = E_OBJECT_ALLOC(E_Gadman, _e_gadman_free);
   if (!gm) return NULL;
   gm->container = con;
   return gm;
}

/* local subsystem functions */
static void
_e_gadman_free(E_Gadman *gm)
{
   free(gm);
}
