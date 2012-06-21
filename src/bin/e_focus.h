#ifdef E_TYPEDEFS
#else
#ifndef E_FOCUS_H
#define E_FOCUS_H

EINTERN int e_focus_init(void);
EINTERN int e_focus_shutdown(void);
EAPI void e_focus_idler_before(void);

EAPI void e_focus_event_mouse_in(E_Border* bd);
EAPI void e_focus_event_mouse_out(E_Border* bd);
EAPI void e_focus_event_mouse_down(E_Border* bd);
EAPI void e_focus_event_mouse_up(E_Border* bd);
EAPI void e_focus_event_focus_in(E_Border *bd);
EAPI void e_focus_event_focus_out(E_Border *bd);
EAPI void e_focus_setup(E_Border *bd);
EAPI void e_focus_setdown(E_Border *bd);

#endif
#endif
