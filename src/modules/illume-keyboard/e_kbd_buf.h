#ifndef E_KBD_BUF_H
#define E_KBD_BUF_H

#include "e_kbd_dict.h"

typedef struct _E_Kbd_Buf           E_Kbd_Buf;
typedef struct _E_Kbd_Buf_Key       E_Kbd_Buf_Key;
typedef struct _E_Kbd_Buf_Keystroke E_Kbd_Buf_Keystroke;
typedef struct _E_Kbd_Buf_Layout    E_Kbd_Buf_Layout;

struct _E_Kbd_Buf
{
   const char       *sysdicts;
   Eina_List        *keystrokes;
   Eina_List        *string_matches;
   const char       *actual_string;
   E_Kbd_Buf_Layout *layout;
   struct {
      void        (*func) (void *data);
      const void   *data;
// FIXME: just faking delayed lookup with timer
      Ecore_Timer  *faket;
   } lookup;
   struct {
      E_Kbd_Dict         *sys;
      E_Kbd_Dict         *personal;
      E_Kbd_Dict         *data;
      Ecore_File_Monitor *data_monitor;
      Ecore_Timer        *data_reload_delay;
   } dict;
};

struct _E_Kbd_Buf_Key
{
   int         x, y, w, h;
   const char *key, *key_shift, *key_capslock;
};

struct _E_Kbd_Buf_Keystroke
{
   const char       *key;
   int               x, y;
   E_Kbd_Buf_Layout *layout;
   unsigned char     shift : 1;
   unsigned char     capslock : 1;
};

struct _E_Kbd_Buf_Layout {
   int        ref;
   int        w, h;
   int        fuzz;
   Eina_List *keys;
};

EAPI E_Kbd_Buf *e_kbd_buf_new(const char *sysdicts, const char *dicts);
EAPI void e_kbd_buf_free(E_Kbd_Buf *kb);
EAPI void e_kbd_buf_dict_set(E_Kbd_Buf *kb, const char *dict);
EAPI void e_kbd_buf_clear(E_Kbd_Buf *kb);
EAPI void e_kbd_buf_layout_clear(E_Kbd_Buf *kb);
EAPI void e_kbd_buf_layout_size_set(E_Kbd_Buf *kb, int w, int h);
EAPI void e_kbd_buf_layout_fuzz_set(E_Kbd_Buf *kb, int fuzz);
EAPI void e_kbd_buf_layout_key_add(E_Kbd_Buf *kb, const char *key, const char *key_shift, const char *key_capslock, int x, int y, int w, int h);
EAPI void e_kbd_buf_pressed_key_add(E_Kbd_Buf *kb, const char *key, int shift, int capslock);
EAPI void e_kbd_buf_pressed_point_add(E_Kbd_Buf *kb, int x, int y, int shift, int capslock);
EAPI const char *e_kbd_buf_actual_string_get(E_Kbd_Buf *kb);
EAPI const Eina_List *e_kbd_buf_string_matches_get(E_Kbd_Buf *kb);
EAPI void e_kbd_buf_backspace(E_Kbd_Buf *kb);
EAPI void e_kbd_buf_lookup(E_Kbd_Buf *kb, void (*func) (void *data), const void *data);
EAPI void e_kbd_buf_lookup_cancel(E_Kbd_Buf *kb);
EAPI void e_kbd_buf_word_use(E_Kbd_Buf *kb, const char *word);
    
#endif
