#ifndef E_VIEW_H
#define E_VIEW_H

#include "e.h"
#include "background.h"
#include "fs.h"
#include "text.h"

typedef struct _E_View                E_View;
typedef struct _E_Icon                E_Icon;

struct _E_View
{
   OBJ_PROPERTIES;
   
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
   
   Evas_Object            obj_bg;
   
   E_Background          *bg;
   
   int                    is_listing;
   int                    monitor_id;
   
   E_FS_Restarter        *restarter;
   
   Evas_List              icons;
   
   int                    is_desktop;
   int                    have_resort_queued;
   int                    sel_count;
   
   int                    changed;
};


struct _E_Icon
{
   OBJ_PROPERTIES;
   
   char   *file;
   
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
   } geom;
   
   int     changed;   
};


void      e_view_selection_update(E_View *v);
void      e_view_deselect_all(void);
void      e_view_deselect_all_except(E_Icon *not_ic);
Eevent   *e_view_get_current_event(void);
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
void      e_view_icon_exec(E_Icon *ic);
void      e_view_icon_free(E_Icon *ic);
void      e_view_icon_initial_show(E_Icon *ic);
void      e_view_icon_set_mime(E_Icon *ic, char *base, char *mime);
void      e_view_icon_set_link(E_Icon *ic, char *link);
E_Icon   *e_view_icon_new(void);
E_Icon   *e_view_find_icon_by_file(E_View *view, char *file);
void      e_view_icon_show(E_Icon *ic);
void      e_view_icon_hide(E_Icon *ic);
void      e_view_icon_apply_xy(E_Icon *ic);
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
void      e_view_free(E_View *v);
E_View   *e_view_new(void);
void      e_view_set_background(E_View *v);
void      e_view_set_dir(E_View *v, char *dir);
void      e_view_realize(E_View *v);
void      e_view_update(E_View *v);
void      e_view_init(void);

#endif
