#ifndef IB_ICONBAR_H
#define IB_ICONBAR_H

#include "e.h"
#include "config.h"
#include "exec.h"
#include "view.h"

#define SCROLL_W 16
#define SCREEN_W 1280

typedef struct _iconbar_icon E_Iconbar_Icon;

#ifndef E_ICONBAR_TYPEDEF
#define E_ICONBAR_TYPEDEF
typedef struct _E_Iconbar E_Iconbar;
#endif

#ifndef E_VIEW_TYPEDEF
#define E_VIEW_TYPEDEF
typedef struct _E_View                E_View;
#endif


struct _iconbar_icon
{
  OBJ_PROPERTIES;
  
  Evas_Object image;
  char *exec;
  int w, h;
  double x, y;
  
  int selected;
};

struct _E_Iconbar
{
  OBJ_PROPERTIES;
  
  char *name;
  E_View *v;
  Evas e;
  Evas_List icons;
  char *db;
  
  struct {
    Evas_Object clip;
    Evas_Object scroll;
    Evas_Object line_l;
    Evas_Object line_c;
    Evas_Object line_r;
    Evas_Object title;
  } obj;
  
  struct {
    char *title;
    char *vline;
    char *hline;
  } image;
  
  
  double start, speed, length;
  
  int scrolling, scroll_when_less;
  
  struct {
    int w;
    int h;
    int top;
    int left;
    int scroll_w;
    int title_w, title_h, line_w, line_h;
    
    int horizontal; /* 1 - horiz, 0 - vert */
    
    struct {
      int top;
      int left;
      int h;
      int w;
    } conf;
  } geom;
};


void            e_iconbar_init(void);
E_Iconbar      *e_iconbar_new(E_View *);
int             e_iconbar_config(E_Iconbar *);
void            e_iconbar_realize(E_Iconbar *);
void            e_iconbar_redraw(E_Iconbar *);
E_Iconbar_Icon *e_iconbar_new_icon(E_Iconbar *, char *, char *);
void            e_iconbar_fix_icons(E_Iconbar *);
void            e_iconbar_create_icons_from_db(E_Iconbar *);
void            e_iconbar_free(E_Iconbar *);
void            e_iconbar_update(E_Iconbar *);

void i_mouse_in(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y);
void i_mouse_out(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y);
void i_mouse_down(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y);

void s_mouse_move(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y);
void s_mouse_in(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y);
void s_mouse_out(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y);

#endif
