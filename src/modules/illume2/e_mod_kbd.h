#ifndef E_MOD_KBD_H
# define E_MOD_KBD_H

/* define keyboard object type */
# define E_ILLUME_KBD_TYPE 0xE1b0988

int e_mod_kbd_init(void);
int e_mod_kbd_shutdown(void);

E_Illume_Keyboard *e_mod_kbd_new(void);
void e_mod_kbd_enable(void);
void e_mod_kbd_disable(void);
void e_mod_kbd_show(void);
void e_mod_kbd_hide(void);
void e_mod_kbd_toggle(void);
void e_mod_kbd_fullscreen_set(E_Zone *zone, int fullscreen);
void e_mod_kbd_layout_set(E_Illume_Keyboard_Layout layout);

#endif
