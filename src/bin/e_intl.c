/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "config.h"

/* TODO List:
 */

int
e_intl_init(void)
{
   setlocale(LC_ALL, "");
   bindtextdomain(PACKAGE, LOCALE_DIR);
   textdomain(PACKAGE);
//   XSetLocaleModifiers("");
   bind_textdomain_codeset(PACKAGE, "UTF-8");
   
   return 1;
}

int
e_intl_shutdown(void)
{
   return 1;
}
