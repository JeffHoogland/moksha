#ifndef E_ICONS_H
#define E_ICONS_H

#include "view.h"
#include "text.h"

#ifndef E_ICON_TYPEDEF
#define E_ICON_TYPEDEF
typedef struct _E_Icon    E_Icon;
#endif

#ifndef E_VIEW_TYPEDEF
#define E_VIEW_TYPEDEF
typedef struct _E_View    E_View;
#endif

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


E_Icon   *e_icon_new(void);
void      e_icon_update_state(E_Icon *ic);
void      e_icon_invert_selection(E_Icon *ic);
void      e_icon_select(E_Icon *ic);
void      e_icon_deselect(E_Icon *ic);

/**
 * e_icon_exec - handles execution paths when user activates an icon
 * @ic:   The activated icon
 *
 * This function takes care of opening views when the user activates a
 * directory, launching commands when an executable is activated etc.
 */
void      e_icon_exec(E_Icon *ic);

void      e_icon_initial_show(E_Icon *ic);
void      e_icon_set_mime(E_Icon *ic, char *base, char *mime);
void      e_icon_set_link(E_Icon *ic, char *link);
E_Icon   *e_icon_find_by_file(E_View *view, char *file);
void      e_icon_show(E_Icon *ic);
void      e_icon_hide(E_Icon *ic);
void      e_icon_apply_xy(E_Icon *ic);
void      e_icon_check_permissions(E_Icon *ic);

#endif
