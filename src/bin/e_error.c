/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */

/* local subsystem globals */

/* externally accessible functions */
EAPI void
e_error_message_show_internal(char *txt)
{
   /* FIXME: maybe log these to a file and display them at some point */
   printf("<<<< Enlightenment Error >>>>\n"
	  "%s\n",
	  txt);
}

/* local subsystem functions */
