#ifndef E_BORDER_H
#define E_BORDER_H

#include "e.h"

struct _E_Grab
{
   int              button;
   Ev_Key_Modifiers mods;
   int              any_mod;
   int              remove_after;
   int              allow;
};

struct _E_Border
{
   OBJ_PROPERTIES;

   struct {
      Window main;
      Window l, r, t, b;
      Window input;
      Window container;
      Window client;
   } win;
   struct {
      Evas l, r, t, b;
   } evas;
   struct {
      struct {
	 Evas_Object l, r, t, b;
      } title;
      struct {
	 Evas_Object l, r, t, b;
      } title_clip;
   } obj;   
   struct {
      Pixmap l, r, t, b;
   } pixmap;
   struct {
      int new;
      Ebits_Object l, r, t, b;
   } bits;
   
   struct {
      struct {
	 int x, y, w, h;
	 int visible;
	 int dx, dy;
      } requested;
      int   x, y, w, h;
      int   visible;
      int   selected;
      int   select_lost_from_grab;
      int   shaded;
      int   has_shape;
      int   shape_changes;
      int   shaped_client;
   } current, previous;
   
   struct {
      struct {
	 int w, h;
	 double aspect;
      } base, min, max, step;
      int layer;
      char *title;
      char *name;
      char *class;
      char *command;
      Window group;
      int takes_focus;
      int sticky;
      Colormap colormap;
      int fixed;
      int arrange_ignore;
      int hidden;
      int iconified;
      int titlebar;
      int border;
      int handles;
      int w, h;
      struct {
	 int requested;
	 int x, y;
	 int gravity;
      } pos;
   } client;
   
   struct {
      int move, resize;
   } mode;
   
   struct {
      int x, y, w, h;
      int is;
   } max;
   
   int ignore_unmap;
   int shape_changed;
   int placed;
   
   Evas_List grabs;
   E_Desktop *desk;
   
   char *border_file;

   int first_expose;
   
   int changed;
};


void      e_border_apply_border(E_Border *b);
void      e_border_reshape(E_Border *b);
E_Border *e_border_adopt(Window win, int use_client_pos);
E_Border *e_border_new(void);
void      e_border_free(E_Border *b);
void      e_border_remove_mouse_grabs(E_Border *b);
void      e_border_attach_mouse_grabs(E_Border *b);
void      e_border_remove_all_mouse_grabs(void);
void      e_border_attach_all_mouse_grabs(void);
void      e_border_redo_grabs(void);
E_Border *e_border_find_by_window(Window win);
void      e_border_set_bits(E_Border *b, char *file);
void      e_border_set_color_class(E_Border *b, char *class, int rr, int gg, int bb, int aa);
void      e_border_adjust_limits(E_Border *b);
void      e_border_update(E_Border *b);
void      e_border_set_layer(E_Border *b, int layer);
void      e_border_raise(E_Border *b);
void      e_border_lower(E_Border *b);
void      e_border_raise_above(E_Border *b, E_Border *above);
void      e_border_lower_below(E_Border *b, E_Border *below);
void      e_border_init(void);
void      e_border_adopt_children(Window win);
E_Border *e_border_current_focused(void);
void      e_border_focus_grab_ended(void);


#endif
