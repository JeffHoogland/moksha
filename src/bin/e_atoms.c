#include "e.h"

/* atom globals */
int _e_atom_wm_name = 0;
int _e_atom_wm_class = 0;
int _e_atom_wm_hints = 0;
int _e_atom_wm_size_hints = 0;
int _e_atom_wm_protocols = 0;
int _e_atom_wm_icon_name = 0;
int _e_atom_wm_client_machine = 0;
int _e_atom_motif_wm_hints = 0;
int _e_atom_netwm_pid = 0;
int _e_atom_netwm_desktop = 0;

/* externally accessible functions */
int
e_atoms_init(void)
{
   _e_atom_wm_name = ecore_x_atom_get("WM_NAME");
   _e_atom_wm_class = ecore_x_atom_get("WM_CLASS");
   _e_atom_wm_hints = ecore_x_atom_get("WM_HINTS");
   _e_atom_wm_size_hints = ecore_x_atom_get("WM_SIZE_HINTS");
   _e_atom_wm_protocols = ecore_x_atom_get("WM_PROTOCOLS");
   _e_atom_wm_icon_name = ecore_x_atom_get("WM_ICON_NAME");
   _e_atom_wm_client_machine = ecore_x_atom_get("WM_CLIENT_MACHINE");
   _e_atom_motif_wm_hints = ecore_x_atom_get("_MOTIF_WM_HINTS");
   _e_atom_netwm_pid = ecore_x_atom_get("_NET_WM_PID");
   _e_atom_netwm_desktop = ecore_x_atom_get("_NET_WM_DESKTOP");
   return 1;
}

int
e_atoms_shutdown(void)
{
   /* Nothing really to do here yet, just present for consistency right now */
   return 1;
}
