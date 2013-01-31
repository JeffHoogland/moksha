#ifdef E_TYPEDEFS
#else
#ifndef E_XKB_H
#define E_XKB_H

EAPI int e_xkb_init(void);
EAPI int e_xkb_shutdown(void);
EAPI void e_xkb_update(int);
EAPI void e_xkb_layout_next(void);
EAPI void e_xkb_layout_prev(void);
EAPI E_Config_XKB_Layout *e_xkb_layout_get(void);
EAPI void e_xkb_layout_set(const E_Config_XKB_Layout *cl);
EAPI const char *e_xkb_layout_name_reduce(const char *name);
EAPI void e_xkb_e_icon_flag_setup(Evas_Object *eicon, const char *name);
EAPI void e_xkb_flag_file_get(char *buf, size_t bufsize, const char *name);

EAPI Eina_Bool e_config_xkb_layout_eq(const E_Config_XKB_Layout *a, const E_Config_XKB_Layout *b);
EAPI void e_config_xkb_layout_free(E_Config_XKB_Layout *cl);
EAPI E_Config_XKB_Layout *e_config_xkb_layout_dup(const E_Config_XKB_Layout *cl);

extern EAPI int E_EVENT_XKB_CHANGED;

#endif
#endif
