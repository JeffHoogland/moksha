#include "e.h"

/* Atoms */
Ecore_X_Atom E_ATOM_MANAGED = 0;
Ecore_X_Atom E_ATOM_DESK = 0;
Ecore_X_Atom E_ATOM_ICONIC = 0;

/* externally accessible functions */
int
e_atoms_init(void)
{
   E_ATOM_MANAGED = ecore_x_atom_get("__E_WINDOW_MANAGED");
   E_ATOM_DESK = ecore_x_atom_get("__E_WINDOW_DESK");
   E_ATOM_ICONIC = ecore_x_atom_get("__E_WINDOW_ICONIC");
   return 1;
}

int
e_atoms_shutdown(void)
{
   /* Nothing really to do here yet, just present for consistency right now */
   return 1;
}
