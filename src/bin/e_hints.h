/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_HINTS_H
#define E_HINTS_H

EAPI void e_hints_init(void);
EAPI void e_hints_client_list_set(void);
EAPI void e_hints_client_stacking_set(void);
EAPI void e_hints_active_window_set(E_Manager *man, Ecore_X_Window win);
EAPI void e_hints_window_name_set(Ecore_X_Window win, const char *name);
EAPI char *e_hints_window_name_get(Ecore_X_Window win);
EAPI void e_hints_desktop_config_set(void);
EAPI void e_hints_window_state_set(Ecore_X_Window win);
EAPI void e_hints_window_visible_set(Ecore_X_Window win);
EAPI int  e_hints_window_visible_isset(Ecore_X_Window win);
EAPI void e_hints_window_iconic_set(Ecore_X_Window win);
EAPI int  e_hints_window_iconic_isset(Ecore_X_Window win);
EAPI void e_hints_window_hidden_set(Ecore_X_Window win);
EAPI int  e_hints_window_hidden_isset(Ecore_X_Window win);
EAPI void e_hints_window_shaded_set(Ecore_X_Window win, int on);
EAPI int  e_hints_window_shaded_isset(Ecore_X_Window win);
EAPI void e_hints_window_shade_direction_set(Ecore_X_Window win, E_Direction dir);
EAPI E_Direction e_hints_window_shade_direction_get(Ecore_X_Window win);

EAPI void e_hints_window_maximized_set(Ecore_X_Window win, int on);
EAPI int  e_hints_window_maximized_isset(Ecore_X_Window win);
EAPI void e_hints_window_sticky_set(Ecore_X_Window win, int on);
EAPI int  e_hints_window_sticky_isset(Ecore_X_Window win);
EAPI void e_hints_window_icon_name_get(Ecore_X_Window win);

#endif
#endif
