#ifdef E_TYPEDEFS

#else
#ifndef E_FILEREG_H
#define E_FILEREG_H

EINTERN int e_filereg_init(void);
EINTERN int e_filereg_shutdown(void);

EAPI int e_filereg_register(const char * path);
EAPI void e_filereg_deregister(const char * path);
EAPI Eina_Bool e_filereg_file_protected(const char * path);

#endif
#endif
