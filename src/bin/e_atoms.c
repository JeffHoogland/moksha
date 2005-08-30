/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* Atoms */
Ecore_X_Atom E_ATOM_MANAGED = 0;
Ecore_X_Atom E_ATOM_CONTAINER = 0;
Ecore_X_Atom E_ATOM_ZONE = 0;
Ecore_X_Atom E_ATOM_DESK = 0;
Ecore_X_Atom E_ATOM_MAPPED = 0;
Ecore_X_Atom E_ATOM_SHADE_DIRECTION = 0;
Ecore_X_Atom E_ATOM_HIDDEN = 0;
Ecore_X_Atom E_ATOM_BORDER_SIZE = 0;
Ecore_X_Atom E_ATOM_WINDOW_STATE = 0;
Ecore_X_Atom E_ATOM_WINDOW_STATE_CENTERED = 0;

/* externally accessible functions */
int
e_atoms_init(void)
{
   E_ATOM_MANAGED = ecore_x_atom_get("__E_WINDOW_MANAGED");
   E_ATOM_CONTAINER = ecore_x_atom_get("__E_WINDOW_CONTAINER");
   E_ATOM_ZONE = ecore_x_atom_get("__E_WINDOW_ZONE");
   E_ATOM_DESK = ecore_x_atom_get("__E_WINDOW_DESK");
   E_ATOM_MAPPED = ecore_x_atom_get("__E_WINDOW_MAPPED");
   E_ATOM_SHADE_DIRECTION = ecore_x_atom_get("__E_WINDOW_SHADE_DIRECTION");
   E_ATOM_HIDDEN = ecore_x_atom_get("__E_WINDOW_HIDDEN");
   E_ATOM_BORDER_SIZE = ecore_x_atom_get("__E_WINDOW_BORDER_SIZE");
   E_ATOM_WINDOW_STATE = ecore_x_atom_get("__E_ATOM_WINDOW_STATE");
   E_ATOM_WINDOW_STATE_CENTERED = ecore_x_atom_get("__E_ATOM_WINDOW_STATE_CENTERED");

   return 1;
}

int
e_atoms_shutdown(void)
{
   /* Nothing really to do here yet, just present for consistency right now */
   return 1;
}
