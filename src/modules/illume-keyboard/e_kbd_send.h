#ifndef E_KBD_SEND_H
#define E_KBD_SEND_H

typedef enum _Kbd_Mod
{
   KBD_MOD_SHIFT = (1 << 0),
   KBD_MOD_CTRL  = (1 << 1),
   KBD_MOD_ALT   = (1 << 2),
   KBD_MOD_WIN   = (1 << 3)
} Kbd_Mod;

EAPI void e_kbd_send_string_press(const char *str, Kbd_Mod mod);
EAPI void e_kbd_send_keysym_press(const char *key, Kbd_Mod mod);

#endif
