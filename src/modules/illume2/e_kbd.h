#ifndef E_KBD_H
#define E_KBD_H

#define E_KBD_TYPE 0xE1b0988

typedef enum _E_Kbd_Layout E_Kbd_Layout;
enum _E_Kbd_Layout
{
   E_KBD_LAYOUT_NONE,
   E_KBD_LAYOUT_DEFAULT,
   E_KBD_LAYOUT_ALPHA,
   E_KBD_LAYOUT_NUMERIC,
   E_KBD_LAYOUT_PIN,
   E_KBD_LAYOUT_PHONE_NUMBER,
   E_KBD_LAYOUT_HEX,
   E_KBD_LAYOUT_TERMINAL,
   E_KBD_LAYOUT_PASSWORD, 
   E_KBD_LAYOUT_IP, 
   E_KBD_LAYOUT_HOST, 
   E_KBD_LAYOUT_FILE, 
   E_KBD_LAYOUT_URL, 
   E_KBD_LAYOUT_KEYPAD, 
   E_KBD_LAYOUT_J2ME
};

typedef struct _E_Kbd E_Kbd;
struct _E_Kbd 
{
   E_Object e_obj_inherit;
   E_Border *border;
   Ecore_Timer *timer;
   Ecore_Animator *animator;
   E_Kbd_Layout layout;
   Eina_List *waiting_borders;

   double start, len;
   int h, adjust, adjust_start, adjust_end;
   unsigned char visible : 1;
   unsigned char disabled : 1;
   unsigned char fullscreen : 1;
};

int e_kbd_init(void);
int e_kbd_shutdown(void);

E_Kbd *e_kbd_new(void);
void e_kbd_all_enable(void);
void e_kbd_all_disable(void);
void e_kbd_show(E_Kbd *kbd);
void e_kbd_hide(E_Kbd *kbd);
void e_kbd_enable(E_Kbd *kbd);
void e_kbd_disable(E_Kbd *kbd);
void e_kbd_layout_set(E_Kbd *kbd, E_Kbd_Layout layout);
void e_kbd_fullscreen_set(E_Zone *zone, int fullscreen);

#endif
