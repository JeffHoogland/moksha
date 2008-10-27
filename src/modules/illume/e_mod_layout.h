#ifndef E_MOD_LAYOUT_H
#define E_MOD_LAYOUT_H

void _e_mod_layout_init(E_Module *m);
void _e_mod_layout_shutdown(void);
void _e_mod_layout_border_hide(E_Border *bd);
void _e_mod_layout_border_close(E_Border *bd);
void _e_mod_layout_border_show(E_Border *bd);

void _e_mod_layout_apply_all(void);

#endif
