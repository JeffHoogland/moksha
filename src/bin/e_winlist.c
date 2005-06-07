/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */

/* local subsystem globals */

/* externally accessible functions */
int
e_winlist_init(void)
{
   return 1;
}

int
e_winlist_shutdown(void)
{
   return 1;
}

/*
 * how to handle? on show, grab keyboard (and mouse) like menus
 * set "modifier keys active" if spawning event had modfiers active
 * if "modifier keys active" and if all modifier keys are released or is found not active on start = end
 * up/left == prev
 * down/right == next
 * escape = end
 * 1 - 9, 0 = select window 1 - 9, 10
 * local subsystem functions
 */
