#ifdef E_TYPEDEFS
#else
#ifndef E_XKB_H
#define E_XKB_H

EAPI int e_xkb_init(void);
EAPI int e_xkb_shutdown(void);
EAPI void e_xkb_update(int);
EAPI void e_xkb_layout_next(void);
EAPI void e_xkb_layout_prev(void);
EAPI void e_xkb_layout_set(const char *name);
EAPI const char *e_xkb_layout_name_reduce(const char *name);
EAPI void e_xkb_e_icon_flag_setup(Evas_Object *eicon, const char *name);
EAPI void e_xkb_flag_file_get(char *buf, size_t bufsize, const char *name);

extern EAPI int E_EVENT_XKB_CHANGED;

#endif
#endif
