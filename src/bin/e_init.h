#ifndef E_INIT_H
#define E_INIT_H

int            e_init_init(void);
int            e_init_shutdown(void);
void           e_init_show(void);
void           e_init_hide(void);
void           e_init_title_set(const char *str);
void           e_init_version_set(const char *str);
void           e_init_status_set(const char *str);
Ecore_X_Window e_init_window_get(void);

#endif
