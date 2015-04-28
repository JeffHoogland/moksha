#include "e.h"

/* local subsystem functions */

/* local subsystem globals */

/* externally accessible functions */
EAPI void
e_error_message_show_internal(char *txt)
{
   /* FIXME: maybe log these to a file and display them at some point */
   printf("<<<< Enlightenment Error >>>>\n%s\n", txt);
}

/* local subsystem functions */
