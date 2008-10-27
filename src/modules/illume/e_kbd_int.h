#ifndef E_KBD_INT_H
#define E_KBD_INT_H

#include "e_kbd_buf.h"

typedef enum _E_Kbd_Int_Type
{
   E_KBD_INT_TYPE_UNKNOWN = 0,
   E_KBD_INT_TYPE_ALPHA = (1 << 0),
   E_KBD_INT_TYPE_NUMERIC = (1 << 1),
   E_KBD_INT_TYPE_PIN = (1 << 2),
   E_KBD_INT_TYPE_PHONE_NUMBER = (1 << 3),
   E_KBD_INT_TYPE_HEX = (1 << 4),
   E_KBD_INT_TYPE_TERMINAL = (1 << 5),
   E_KBD_INT_TYPE_PASSWORD = (1 << 6)
} E_Kbd_Int_Type;

typedef struct _E_Kbd_Int           E_Kbd_Int;
typedef struct _E_Kbd_Int_Key       E_Kbd_Int_Key;
typedef struct _E_Kbd_Int_Key_State E_Kbd_Int_Key_State;
typedef struct _E_Kbd_Int_Layout    E_Kbd_Int_Layout;
typedef struct _E_Kbd_Int_Match     E_Kbd_Int_Match;

struct _E_Kbd_Int
{
   E_Win               *win;
   const char          *themedir, *syskbds, *sysdicts;
   Evas_Object         *base_obj, *layout_obj, *event_obj, *icon_obj, *box_obj;
   Eina_List           *layouts;
   Eina_List           *matches;
   Ecore_Event_Handler *client_message_handler;
   struct {
      char             *directory;
      const char       *file;
      int               w, h;
      int               fuzz;
      E_Kbd_Int_Type    type;
      Eina_List        *keys;
      E_Kbd_Int_Key    *pressed;
      int               state;
   } layout;
   struct {
      Evas_Coord        x, y, cx, cy;
      int               lx, ly, clx, cly;
      Ecore_Timer      *hold_timer;
      unsigned char     down : 1;
      unsigned char     stroke : 1;
      unsigned char     zoom : 1;
   } down;
   struct {
      E_Popup          *popup;
      Evas_Object      *base_obj, *scrollframe_obj, *ilist_obj;
   } layoutlist;
   struct {
      E_Popup          *popup;
      Evas_Object      *base_obj, *scrollframe_obj, *ilist_obj;
      Eina_List        *matches;
   } matchlist;
   struct {
      E_Popup          *popup;
      Evas_Object      *base_obj, *scrollframe_obj, *ilist_obj;
      Eina_List        *matches;
   } dictlist;
   struct {
      E_Popup          *popup;
      Evas_Object      *base_obj, *layout_obj, *sublayout_obj;
      E_Kbd_Int_Key    *pressed;
   } zoomkey;
                     
   E_Kbd_Buf           *kbuf;
};

struct _E_Kbd_Int_Key
{
   int x, y, w, h;
   
   Eina_List *states;
   Evas_Object *obj, *zoom_obj, *icon_obj, *zoom_icon_obj;

   unsigned char pressed : 1;
   unsigned char selected : 1;
   
   unsigned char is_shift : 1;
   unsigned char is_ctrl : 1;
   unsigned char is_alt : 1;
   unsigned char is_capslock : 1;
};

struct _E_Kbd_Int_Key_State
{
   int state;
   const char *label, *icon;
   const char *out;
};

struct _E_Kbd_Int_Layout
{
   const char *path;
   const char *dir;
   const char *icon;
   const char *name;
   int type;
};

struct _E_Kbd_Int_Match
{
   E_Kbd_Int   *ki;
   const char  *str;
   Evas_Object *obj;
};

EAPI E_Kbd_Int *e_kbd_int_new(const char *themedir, const char *syskbds, const char *sysdicts);
EAPI void e_kbd_int_free(E_Kbd_Int *ki);

#endif
