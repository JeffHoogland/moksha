/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_UTILS_H
#define E_UTILS_H

EAPI void         e_util_container_fake_mouse_up_later(E_Container *con, int button);
EAPI void         e_util_container_fake_mouse_up_all_later(E_Container *con);
EAPI void         e_util_wakeup(void);
EAPI void         e_util_env_set(const char *var, const char *val);
EAPI E_Zone      *e_util_zone_current_get(E_Manager *man);
EAPI int          e_util_utils_installed(void);
EAPI int          e_util_app_installed(char *app);
EAPI int          e_util_glob_match(char *str, char *glob);
EAPI E_Container *e_util_container_number_get(int num);
EAPI E_Zone      *e_util_container_zone_number_get(int con_num, int zone_num);
EAPI int          e_util_head_exec(int head, char *cmd);
    
#endif
#endif
