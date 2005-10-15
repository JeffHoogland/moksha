/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#include <locale.h>
#include <libintl.h>

#define _(str) gettext(str)
#define d_(str, dom) dgettext(PACKAGE dom, str)

typedef struct _E_Language_Pack E_Language_Pack;

#else
#ifndef E_INTL_H
#define E_INTL_H

struct _E_Language_Pack
{
   int version;
   char *e_im_name;
   char *gtk_im_module;
   char *qt_im_module;
   char *xmodifiers;
   char *gtk_im_module_file;
   char *e_im_exec;
};

EAPI int		 e_intl_init(void);
EAPI int		 e_intl_shutdown(void);
/* Setting & Getting Language */
EAPI void		 e_intl_language_set(const char *lang);
EAPI const char		*e_intl_language_get(void);
EAPI const Evas_List	*e_intl_language_list(void);
/* Setting & Getting Input Method */
EAPI void                e_intl_input_method_set(const char *method);
EAPI const char         *e_intl_input_method_get(void);
EAPI const Evas_List    *e_intl_input_method_list(void);
    
#endif
#endif
