/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* Atoms */
Ecore_X_Atom E_ATOM_MANAGED = 0;
Ecore_X_Atom E_ATOM_DESK = 0;
Ecore_X_Atom E_ATOM_ICONIC = 0;
Ecore_X_Atom E_ATOM_MAPPED = 0;

/* externally accessible functions */
int
e_atoms_init(void)
{
   E_ATOM_MANAGED = ecore_x_atom_get("__E_WINDOW_MANAGED");
   E_ATOM_DESK = ecore_x_atom_get("__E_WINDOW_DESK");
   E_ATOM_ICONIC = ecore_x_atom_get("__E_WINDOW_ICONIC");
   E_ATOM_MAPPED = ecore_x_atom_get("__E_WINDOW_MAPPED");
   return 1;
}

int
e_atoms_shutdown(void)
{
   /* Nothing really to do here yet, just present for consistency right now */
   return 1;
}
