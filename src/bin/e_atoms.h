#ifndef E_ATOMS_H
#define E_ATOMS_H

/* atom globals */
extern int _e_atom_wm_name;
extern int _e_atom_wm_class;
extern int _e_atom_wm_hints;
extern int _e_atom_wm_size_hints;
extern int _e_atom_wm_protocols;
extern int _e_atom_wm_icon_name;
extern int _e_atom_wm_client_machine;
extern int _e_atom_motif_wm_hints;
extern int _e_atom_netwm_pid;
extern int _e_atom_netwm_desktop;

EAPI int    e_atoms_init(void);
EAPI int    e_atoms_shutdown(void);

#endif
