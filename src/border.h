#ifndef E_BORDER_H
#define E_BORDER_H

#include "e.h"
#include "observer.h"
#include "debug.h"
#include "text.h"

#ifndef E_DESKTOPS_TYPEDEF
#define E_DESKTOPS_TYPEDEF
typedef struct _E_Desktop             E_Desktop;
#endif

typedef struct _E_Grab                E_Grab;
typedef struct _E_Border              E_Border;

struct _E_Grab
{
   int              button;
   Ecore_Event_Key_Modifiers mods;
   int              any_mod;
   int              remove_after;
   int              allow;
};

struct _E_Border
{
   E_Observee obs;

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
	 E_Text *l, *r, *t, *b;
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
      char *machine;
      char *icon_name;
      int pid;
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
      int initial_state;
      int is_desktop;
      int w, h;
      int no_place;
      struct {
	 int requested;
	 int x, y;
	 int gravity;
      } pos;
      int desk;
      struct {
	 int x, y;
      } area;
      int internal;
      struct {
	 int matched;
	 struct {
	    int matched;
	    int ignore;
	 } prog_location;
	 struct {
	    int matched;
	    char *style;
	 } border;
	 struct {
	    int matched;
	    int x, y;
	 } location;
	 struct {
	    int matched;
	    int x, y;
	 } desk_area;
	 struct {
	    int matched;
	    int w, h;
	 } size;
	 struct {
	    int matched;
	    int desk;
	 } desktop;
	 struct {
	    int matched;
	    int sticky;
	 } sticky;
	 struct {
	    int matched;
	    int layer;
	 } layer;
      } matched;
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
   E_Grab   *click_grab;
   E_Desktop *desk;
   
   char *border_style;
   char *border_file;

   int first_expose;
   
   int hold_changes;
   
   Evas_List menus;
   
   int changed;
};

/**
 * e_border_init - Border handling initialization.
 *
 * This function registers the border event handlers
 * against ecore.
 */
void      e_border_init(void);

E_Border *e_border_new(void);

void      e_border_update_borders(void);
void      e_border_apply_border(E_Border *b);
void      e_border_reshape(E_Border *b);
void      e_border_release(E_Border *b);    
E_Border *e_border_adopt(Window win, int use_client_pos);
void      e_border_adopt_children(Window win);
void      e_border_remove_mouse_grabs(E_Border *b);
void      e_border_remove_click_grab(E_Border *b);
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
E_Border *e_border_current_focused(void);
void      e_border_focus_grab_ended(void);
void      e_border_raise_next(void);
void      e_border_send_pointer(E_Border *b);
int       e_border_viewable(E_Border *b);
void      e_border_print_pos(char *buf, E_Border *b);
void      e_border_print_size(char *buf, E_Border *b);
void      e_border_set_gravity(E_Border *b, int gravity);

#endif
