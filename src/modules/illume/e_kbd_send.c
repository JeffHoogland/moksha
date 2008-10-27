#include <e.h>
#include "e_kbd_send.h"

static const char *
_string_to_keysym(const char *str)
{
   int glyph, ok;
   
   /* utf8 -> glyph id (unicode - ucs4) */
   glyph = 0;
   ok = evas_string_char_next_get(str, 0, &glyph);
   if (glyph <= 0) return NULL;
   /* glyph id -> keysym */
   if (glyph > 0xff) glyph |= 0x1000000;
   return ecore_x_keysym_string_get(glyph);
}

EAPI void
e_kbd_send_string_press(const char *str, Kbd_Mod mod)
{
   const char *key = NULL;
   
   key = _string_to_keysym(str);
   if (!key) return;
   e_kbd_send_keysym_press(key, mod);
}

EAPI void
e_kbd_send_keysym_press(const char *key, Kbd_Mod mod)
{
   if (mod & KBD_MOD_CTRL) ecore_x_test_fake_key_down("Control_L");
   if (mod & KBD_MOD_ALT) ecore_x_test_fake_key_down("Alt_L");
   if (mod & KBD_MOD_WIN) ecore_x_test_fake_key_down("Super_L");
   ecore_x_test_fake_key_press(key);
   if (mod & KBD_MOD_WIN) ecore_x_test_fake_key_up("Super_L");
   if (mod & KBD_MOD_ALT) ecore_x_test_fake_key_up("Alt_L");
   if (mod & KBD_MOD_CTRL) ecore_x_test_fake_key_up("Control_L");
}
