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
extern EAPI Ecore_X_Atom E_ATOM_DESKTOP_FILE;
/* extra e window states */
/* if we add more states, we need to fix
 * * e_hints_window_e_state_get()
 * * e_hints_window_e_state_set()
 * * _e_win_state_update() + e_win
 */
extern EAPI Ecore_X_Atom E_ATOM_WINDOW_STATE;
extern EAPI Ecore_X_Atom E_ATOM_WINDOW_STATE_CENTERED;

extern EAPI Ecore_X_Atom E_ATOM_ZONE_GEOMETRY;

EINTERN int    e_atoms_init(void);
EINTERN int    e_atoms_shutdown(void);

#endif
#endif
