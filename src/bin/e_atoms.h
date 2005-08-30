/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_ATOMS_H
#define E_ATOMS_H

/* an "overall" atom to see that we recognise the window */
extern EAPI Ecore_X_Atom E_ATOM_MANAGED;

/* basic window properties */
extern EAPI Ecore_X_Atom E_ATOM_CONTAINER;
extern EAPI Ecore_X_Atom E_ATOM_ZONE;
extern EAPI Ecore_X_Atom E_ATOM_DESK;
extern EAPI Ecore_X_Atom E_ATOM_MAPPED;
extern EAPI Ecore_X_Atom E_ATOM_SHADE_DIRECTION;
extern EAPI Ecore_X_Atom E_ATOM_HIDDEN;
extern EAPI Ecore_X_Atom E_ATOM_BORDER_SIZE;

/* extra e window states */
/* if we add more states, we need to fix
 * * e_hints_window_e_state_get()
 * * e_hints_window_e_state_set()
 * * _e_win_state_update() + e_win
 */
extern EAPI Ecore_X_Atom E_ATOM_WINDOW_STATE;
extern EAPI Ecore_X_Atom E_ATOM_WINDOW_STATE_CENTERED;

EAPI int    e_atoms_init(void);
EAPI int    e_atoms_shutdown(void);

#endif
#endif
