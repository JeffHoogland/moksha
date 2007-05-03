/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Font_Default E_Font_Default;
typedef struct _E_Font_Fallback E_Font_Fallback;
typedef struct _E_Font_Available E_Font_Available;
typedef struct _E_Font_Properties E_Font_Properties;

#else
#ifndef E_FONT_H
#define E_FONT_H

struct _E_Font_Default
{
   const char     *text_class;
   const char     *font;
   Evas_Font_Size  size;
};

struct _E_Font_Fallback
{
   const char *name;
};

struct _E_Font_Available
{
   const char *name;
};

struct _E_Font_Properties 
{
   const char *name;
   Evas_List *styles;
};

EAPI int		e_font_init(void);
EAPI int		e_font_shutdown(void);
EAPI void		e_font_apply(void);
EAPI Evas_List         *e_font_available_list(void);
EAPI void		e_font_available_list_free(Evas_List *available);
EAPI Evas_Hash         *e_font_available_list_parse(Evas_List *list);
EAPI void               e_font_available_hash_free(Evas_Hash *hash);
EAPI E_Font_Properties *e_font_fontconfig_name_parse(const char *font);
EAPI const char        *e_font_fontconfig_name_get(const char *name, const char *style);
EAPI void               e_font_properties_free(E_Font_Properties *efp);

/* global font fallbacks */
EAPI void		e_font_fallback_clear(void);
EAPI void		e_font_fallback_append(const char *font);
EAPI void		e_font_fallback_prepend(const char *font);
EAPI void		e_font_fallback_remove(const char *font);
EAPI Evas_List         *e_font_fallback_list(void);

/* setup edje text classes */
EAPI void		e_font_default_set(const char *text_class, const char *font, Evas_Font_Size size);
EAPI E_Font_Default    *e_font_default_get(const char *text_class);
EAPI void		e_font_default_remove(const char *text_class);
EAPI Evas_List         *e_font_default_list(void);
EAPI const char        *e_font_default_string_get(const char *text_class, Evas_Font_Size *size_ret);
    
#endif
#endif
