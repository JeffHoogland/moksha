/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_PREFIX_H
#define E_PREFIX_H

EAPI int         e_prefix_determine(char *argv0);
EAPI void        e_prefix_shutdown(void);
EAPI void        e_prefix_fallback(void);
EAPI const char *e_prefix_get(void);
EAPI const char *e_prefix_locale_get(void);
EAPI const char *e_prefix_bin_get(void);
EAPI const char *e_prefix_data_get(void);
EAPI const char *e_prefix_lib_get(void);
    
#endif
#endif
