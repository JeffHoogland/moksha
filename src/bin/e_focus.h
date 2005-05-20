/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_FOCUS_H
#define E_FOCUS_H

EAPI int e_focus_init(void);
EAPI int e_focus_shutdown(void);
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
