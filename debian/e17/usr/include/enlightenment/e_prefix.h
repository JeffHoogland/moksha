#ifdef E_TYPEDEFS

#else
#ifndef E_PREFIX_H
#define E_PREFIX_H

EAPI int         e_prefix_determine(char *argv0);
EINTERN void     e_prefix_shutdown(void);
EAPI void        e_prefix_fallback(void);
EAPI const char *e_prefix_get(void);
EAPI const char *e_prefix_locale_get(void);
EAPI const char *e_prefix_bin_get(void);
EAPI const char *e_prefix_data_get(void);
EAPI const char *e_prefix_lib_get(void);

EAPI size_t      e_prefix_data_concat_len(char *dst, size_t size, const char *path, size_t path_len);
EAPI size_t      e_prefix_data_concat(char *dst, size_t size, const char *path);
EAPI size_t      e_prefix_data_snprintf(char *dst, size_t size, const char *fmt, ...) EINA_PRINTF(3, 4);

#define e_prefix_data_concat_static(dst, path) e_prefix_data_concat_len(dst, sizeof(dst), path,  (sizeof(path) > 0) ? sizeof(path) - 1 : 0)


#endif
#endif
