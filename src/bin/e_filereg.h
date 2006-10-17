/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_FILEREG_H
#define E_FILEREG_H

EAPI int e_filereg_init(void);
EAPI int e_filereg_shutdown(void);

EAPI int e_filereg_register(const char * path);
EAPI void e_filereg_deregister(const char * path);
EAPI Evas_Bool e_filereg_file_protected(const char * path);

#endif
#endif


