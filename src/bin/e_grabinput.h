/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_GRABINPUT_H
#define E_GRABINPUT_H

EAPI int            e_grabinput_init(void);
EAPI int            e_grabinput_shutdown(void);
EAPI void           e_grabinput_get(Ecore_X_Window mouse_win, int confine_mouse, Ecore_X_Window key_win);
EAPI void           e_grabinput_release(Ecore_X_Window mouse_win, Ecore_X_Window key_win);
    
#endif
#endif
