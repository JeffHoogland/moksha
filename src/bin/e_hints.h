/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_HINTS_H
#define E_HINTS_H

EAPI void e_hints_init(void);
EAPI void e_hints_e16_comms_pretend(E_Manager *man);
EAPI void e_hints_manager_init(E_Manager *man);
EAPI void e_hints_client_list_set(void);
EAPI void e_hints_client_stacking_set(void);

EAPI void e_hints_active_window_set(E_Manager *man, E_Border *bd);

EAPI void e_hints_window_init(E_Border *bd);
EAPI void e_hints_window_state_set(E_Border *bd);
EAPI void e_hints_window_state_get(E_Border *bd);
EAPI void e_hints_window_type_set(E_Border *bd);
EAPI void e_hints_window_type_get(E_Border *bd);

EAPI void e_hints_window_state_update(E_Border *bd, Ecore_X_Window_State state,
				      Ecore_X_Window_State_Action action);

EAPI void e_hints_window_visible_set(E_Border *bd);
EAPI void e_hints_window_iconic_set(E_Border *bd);
EAPI void e_hints_window_hidden_set(E_Border *bd);

EAPI void e_hints_window_shade_direction_set(E_Border *bd, E_Direction dir);
EAPI E_Direction e_hints_window_shade_direction_get(E_Border *bd);

EAPI void e_hints_window_size_set(E_Border *bd);
EAPI void e_hints_window_size_unset(E_Border *bd);
EAPI int  e_hints_window_size_get(E_Border *bd);

EAPI void e_hints_window_shaded_set(E_Border *bd, int on);
EAPI void e_hints_window_maximized_set(E_Border *bd, int horizontal, int vertical);
EAPI void e_hints_window_fullscreen_set(E_Border *bd, int on);
EAPI void e_hints_window_sticky_set(E_Border *bd, int on);
EAPI void e_hints_window_stacking_set(E_Border *bd, E_Stacking stacking);

EAPI void e_hints_window_desktop_set(E_Border *bd);

EAPI void e_hints_window_e_state_set(E_Border *bd);
EAPI void e_hints_window_e_state_get(E_Border *bd);

EAPI void e_hints_window_qtopia_soft_menu_get(E_Border *bd);
EAPI void e_hints_window_qtopia_soft_menus_get(E_Border *bd);

EAPI void e_hints_window_virtual_keyboard_state_get(E_Border *bd);
EAPI void e_hints_window_virtual_keyboard_get(E_Border *bd);

EAPI void e_hints_openoffice_gnome_fake(Ecore_X_Window root);
EAPI void e_hints_openoffice_kde_fake(Ecore_X_Window root);

extern EAPI Ecore_X_Atom _QTOPIA_SOFT_MENU;
extern EAPI Ecore_X_Atom _QTOPIA_SOFT_MENUS;

#endif
#endif
