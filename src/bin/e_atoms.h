#ifdef E_TYPEDEFS
#else
#ifndef E_ATOMS_H
#define E_ATOMS_H

/* an "overall" atom to see that we recognise the window */
extern EAPI Ecore_X_Atom E_ATOM_MANAGED;

/* basic window properties */
extern EAPI Ecore_X_Atom E_ATOM_DESK;
extern EAPI Ecore_X_Atom E_ATOM_ICONIC;

EAPI int    e_atoms_init(void);
EAPI int    e_atoms_shutdown(void);

#endif
#endif
