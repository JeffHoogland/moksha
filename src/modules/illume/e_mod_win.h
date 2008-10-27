#ifndef E_MOD_WIN_H
#define E_MOD_WIN_H

#include "e_slipshelf.h"

void _e_mod_win_init(E_Module *m);
void _e_mod_win_shutdown(void);

void _e_mod_win_cfg_update(void);
void _e_mod_win_slipshelf_cfg_update(void);

void e_mod_win_cfg_kbd_start(void);
void e_mod_win_cfg_kbd_stop(void);
void e_mod_win_cfg_kbd_update(void);    

extern E_Slipshelf *slipshelf;

#endif
