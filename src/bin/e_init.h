#ifndef E_INIT_H
#define E_INIT_H

EAPI int            e_init_init(void);
EAPI int            e_init_shutdown(void);
EAPI void           e_init_show(void);
EAPI void           e_init_hide(void);
EAPI void           e_init_title_set(const char *str);
EAPI void           e_init_version_set(const char *str);
EAPI void           e_init_status_set(const char *str);
EAPI Ecore_X_Window e_init_window_get(void);

#endif
