#ifdef E_TYPEDEFS
# ifdef E_INTERNAL
#  if E_INTERNAL

#   ifdef HAVE_GETTEXT
#define _(str) gettext(str)
#define d_(str, dom) dgettext(PACKAGE dom, str)
#define P_(str, str_p, n) ngettext(str, str_p, n)
#define dP_(str, str_p, n, dom) dngettext(PACKAGE dom, str, str_p, n)
#   else
#define _(str) (str)
#define d_(str, dom) (str)
#define P_(str, str_p, n) (str_p)
#define dP_(str, str_p, n, dom) (str_p)
#   endif
/* These macros are used to just mark strings for translation, this is useful
 * for string lists which are not dynamically allocated
 */
#define N_(str) (str)
#define NP_(str, str_p) str, str_p
#  endif
# endif
typedef struct _E_Locale_Parts E_Locale_Parts;

# else
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

EINTERN int		 e_intl_init(void);
EINTERN int		 e_intl_shutdown(void);
EINTERN int		 e_intl_post_init(void);
EINTERN int		 e_intl_post_shutdown(void);
/* Setting & Getting Language */
EAPI void		 e_intl_language_set(const char *lang);
EAPI const char		*e_intl_language_get(void);
EAPI const char		*e_intl_language_alias_get(void);
EAPI Eina_List		*e_intl_language_list(void);
/* Setting & Getting Input Method */
EAPI void                e_intl_input_method_set(const char *method);
EAPI const char         *e_intl_input_method_get(void);
EAPI Eina_List		*e_intl_input_method_list(void);
EAPI const char		*e_intl_imc_personal_path_get(void);
EAPI const char		*e_intl_imc_system_path_get(void);

/* Getting locale */
EAPI E_Locale_Parts	*e_intl_locale_parts_get(const char *locale);
EAPI void		 e_intl_locale_parts_free(E_Locale_Parts *locale_parts);
EAPI char               *e_intl_locale_parts_combine(E_Locale_Parts *locale_parts, int mask);
EAPI char		*e_intl_locale_charset_canonic_get(const char *charset);

# endif
#endif
