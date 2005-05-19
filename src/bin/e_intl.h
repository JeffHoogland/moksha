/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#include <locale.h>
#include <libintl.h>

#define _(str) gettext(str)
#define d_(str, dom) dgettext(PACKAGE dom, str)

#else
#ifndef E_INTL_H
#define E_INTL_H

EAPI int e_intl_init(void);
EAPI int e_intl_shutdown(void);
EAPI void e_intl_language_set(const char *lang);
EAPI const char *e_intl_language_get(void);
EAPI const Evas_List *e_intl_language_list(void);
    
#endif
#endif
