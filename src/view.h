#ifndef E_VIEW_H
#define E_VIEW_H

#include "e.h"
#include "background.h"
#include "scrollbar.h"
#include "fs.h"
#include "text.h"
#include "iconbar.h"
#include "object.h"

#ifndef E_VIEW_TYPEDEF
#define E_VIEW_TYPEDEF
typedef struct _E_View                E_View;
#endif

typedef struct _E_Icon                E_Icon;

#ifndef E_ICONBAR_TYPEDEF
#define E_ICONBAR_TYPEDEF
typedef struct _E_Iconbar E_Iconbar;
#endif



struct _E_View
{
   E_Object               o;
   
   char                  *dir;
   
   struct {
      Evas_Render_Method  render_method;
      int                 back_pixmap;
   } options;
   
   Evas                   evas;
   struct {
      Window              base;
      Window              main;
   } win;
   Pixmap                 pmap;
   struct {
      int                 w, h;
      int                 force;
   } size;
   struct {
      int                 x, y;
   } scroll;
   struct {
      int                 x, y;
   } location;
   struct {
      EfsdCmdId x, y, w, h;
      int       busy;
   } geom_get;
   struct {
      /* +-----------------+
       * |        Wt       |
       * |  +-----------+  |
       * |Wl|           |Wr|
       * |  |    [I] Is |  |
       * |  |    Ig     |  |
       * |  |   [txt]   |  |
       * |  |     Ib    |  |
       * |  +-----------+  |
       * |       Wb        |
       * +-----------------+
       */
      struct {
	 int l, r, t, b;
      } window;
      struct {
	 int s, g, b;
      } icon;
   } spacing;
   struct {
      int on;
      int x, y, w, h;
      struct {
	 int x, y;
      } down;
      struct {
	 struct {
	    int r, g, b, a;
	 } 
	 edge_l, edge_r, edge_t, edge_b, 
	 middle, 
	 grad_l, grad_r, grad_t, grad_b;
	 struct {
	    int l, r, t, b;
	 } grad_size;
      } config;
      struct {
	 Evas_Object clip;
	 Evas_Object edge_l;
	 Evas_Object edge_r;
	 Evas_Object edge_t;
	 Evas_Object edge_b;
	 Evas_Object middle;
	 Evas_Object grad_l;
	 Evas_Object grad_r;
	 Evas_Object grad_t;
	 Evas_Object grad_b;
      } obj;
   } select;
   struct {
      int started;
      Window win;
      int x, y;
      struct {
	 int x, y;
      } offset;
      int update;
   } drag;
   struct {
      int valid;
      double x1, x2, y1, y2;
   } extents;
   
   Evas_Object            obj_bg;
   
   E_Background          *bg;
   
   int getbg;

   struct {      
      E_Scrollbar           *h, *v;
   } scrollbar;
   
   int                    is_listing;
   int                    monitor_id;
   
   E_FS_Restarter        *restarter;
   
   Evas_List              icons;
   
   int                    is_desktop;
   int                    have_resort_queued;
   int                    sel_count;
   
   int                    changed;

   E_Iconbar              *iconbar;
};


struct _E_Icon
{
   E_Object o;
   
   char        *file;
   struct stat  stat;
   
   E_View *view;
   
   struct {
      char *icon;
      char *custom_icon;
      char *link;
      struct {
	 char *base;
	 char *type;
      } mime;
   } info;
   
   struct {
      Evas_Object  icon;
      Evas_Object  event1;
      Evas_Object  event2;
      E_Text      *text;
      struct {
	 struct {
	    Ebits_Object icon;
	    Ebits_Object text;
	 } over, under;
      } sel;
   } obj;
   
   struct {
      int hilited;
      int clicked;
      int selected;
      int running;
      int disabled;
      int visible;
      int just_selected;
      int just_executed;
   } state;
   
   struct {
      int x, y, w, h;
      struct {
	 int w, h;
      } icon;
      struct {
	 int w, h;
      } text;
   } geom, prev_geom;      
   
   struct {
      int write_xy;
   } q;
   
   int     changed;   
};


/**
 * e_view_init - View event handlers initialization.
 *
 * This function registers event handlers for the views.
 * Views are the windows in which e as a desktop shell
 * displays file icons.
 */
void      e_view_init(void);

void      e_view_selection_update(E_View *v);
void      e_view_deselect_all(void);
void      e_view_deselect_all_except(E_Icon *not_ic);
Ecore_Event   *e_view_get_current_event(void);
int       e_view_filter_file(E_View *v, char *file);
void      e_view_icons_get_extents(E_View *v, int *min_x, int *min_y, int *max_x, int *max_y);
void      e_view_icons_apply_xy(E_View *v);
void      e_view_scroll_to(E_View *v, int sx, int sy);
void      e_view_scroll_by(E_View *v, int sx, int sy);
void      e_view_scroll_to_percent(E_View *v, double psx, double psy);
void      e_view_get_viewable_percentage(E_View *v, double *vw, double *vh);
void      e_view_get_position_percentage(E_View *v, double *vx, double *vy);
void      e_view_icon_update_state(E_Icon *ic);
void      e_view_icon_invert_selection(E_Icon *ic);
void      e_view_icon_select(E_Icon *ic);
void      e_view_icon_deselect(E_Icon *ic);

/**
 * e_view_icon_exec - handles execution paths when user activates an icon
 * @ic:   The activated icon
 *
 * This function takes care of opening views when the user activates a
 * directory, launching commands when an executable is activated etc.
 */
void      e_view_icon_exec(E_Icon *ic);

void      e_view_icon_initial_show(E_Icon *ic);
void      e_view_icon_set_mime(E_Icon *ic, char *base, char *mime);
void      e_view_icon_set_link(E_Icon *ic, char *link);
E_Icon   *e_view_icon_new(void);
E_Icon   *e_view_find_icon_by_file(E_View *view, char *file);
void      e_view_icon_show(E_Icon *ic);
void      e_view_icon_hide(E_Icon *ic);
void      e_view_icon_apply_xy(E_Icon *ic);
void      e_view_icon_check_permissions(E_Icon *ic);
void      e_view_resort_alphabetical(E_View *v);
void      e_view_arrange(E_View *v);
void      e_view_resort(E_View *v);
void      e_view_queue_geometry_record(E_View *v);
void      e_view_geometry_record(E_View *v);    
void      e_view_queue_resort(E_View *v);
void      e_view_file_added(int id, char *file);
void      e_view_file_deleted(int id, char *file);
void      e_view_file_changed(int id, char *file);
void      e_view_file_moved(int id, char *file);
E_View   *e_view_find_by_monitor_id(int id);

/**
 * e_view_new - Creates a new view object
 * 
 * This function creates a new view and sets default
 * properties on it, such as colors and icon spacings.
 */
E_View   *e_view_new(void);

void      e_view_set_background(E_View *v);

/**
 * e_view_set_dir - Sets view to a given directory
 * @v     The view for which to set the directory
 * @dir   The directory to set the view to
 *
 * This function sets a view to a directory, loading the
 * view's metadata (view window coordinates etc) from that
 * directory, it also requests monitoring of the files in
 * the directory @dir from efsd.
 */
void      e_view_set_dir(E_View *v, char *dir);

/**
 * e_view_realize - Initializes a view's graphics and content
 * @v:    The view to initialize
 *
 * This function initializes a created view by loading
 * all the graphics, and sets the view to a given directory
 * by calling e_view_set_dir().
 */
void      e_view_realize(E_View *v);

void      e_view_update(E_View *v);

#endif
