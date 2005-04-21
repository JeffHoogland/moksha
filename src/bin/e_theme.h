/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_THEME_H
#define E_THEME_H

EAPI int       e_theme_init(void);
EAPI int       e_theme_shutdown(void);

EAPI const char *e_theme_file_get(char *category);
EAPI void        e_theme_file_set(char *category, char *file);
    
#endif
#endif
