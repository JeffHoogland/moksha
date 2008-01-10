/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#if E_INTERNAL
#define _(str) gettext(str)
#define d_(str, dom) dgettext(PACKAGE dom, str)
#endif

/* This macro is used to just mark string for translation, this is useful
 * for string lists which are not dynamically allocated 
 */
#define N_(str) (str)

typedef struct _E_Locale_Parts E_Locale_Parts;

#else
#ifndef E_INTL_H
#define E_INTL_H

#define E_INTL_LOC_CODESET   1 << 0
#define E_INTL_LOC_REGION    1 << 1
#define E_INTL_LOC_MODIFIER  1 << 2
#define E_INTL_LOC_LANG      1 << 3

struct _E_Locale_Parts
{
   int mask;
   const char *lang;
   const char *region;
   const char *codeset;
   const char *modifier;
};

EAPI int		 e_intl_init(void);
EAPI int		 e_intl_shutdown(void);
EAPI int		 e_intl_post_init(void);
EAPI int		 e_intl_post_shutdown(void);
/* Setting & Getting Language */
EAPI void		 e_intl_language_set(const char *lang);
EAPI const char		*e_intl_language_get(void);
EAPI const char		*e_intl_language_alias_get(void);
EAPI Evas_List		*e_intl_language_list(void);
/* Setting & Getting Input Method */
EAPI void                e_intl_input_method_set(const char *method);
EAPI const char         *e_intl_input_method_get(void);
EAPI Evas_List		*e_intl_input_method_list(void);
EAPI const char		*e_intl_imc_personal_path_get(void);
EAPI const char		*e_intl_imc_system_path_get(void);

/* Getting locale */
EAPI E_Locale_Parts	*e_intl_locale_parts_get(const char *locale);
EAPI void		 e_intl_locale_parts_free(E_Locale_Parts *locale_parts);
EAPI char               *e_intl_locale_parts_combine(E_Locale_Parts *locale_parts, int mask);
EAPI char		*e_intl_locale_charset_canonic_get(const char *charset);
#endif
#endif
