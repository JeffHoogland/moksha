#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_KEYBIND_H
#define E_MOD_KEYBIND_H

#define E_XKB_NEXT_ACTION   "e_xkb_layout_next"
#define E_XKB_PREV_ACTION   "e_xkb_layout_prev"

int e_xkb_register_module_actions(void);
int e_xkb_unregister_module_actions(void);

int e_xkb_register_module_keybindings(void);
int e_xkb_unregister_module_keybindings(void);

#endif
#endif
