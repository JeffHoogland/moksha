#ifndef IB_ICONBAR_H
#define IB_ICONBAR_H

#include "e.h"
#include "config.h"
#include "exec.h"
#include "view.h"
#include "object.h"
#include "resist.h"

typedef struct _E_Iconbar_Icon E_Iconbar_Icon;
#ifndef E_ICONBAR_TYPEDEF
#define E_ICONBAR_TYPEDEF
typedef struct _E_Iconbar E_Iconbar;
#endif

#ifndef E_VIEW_TYPEDEF
#define E_VIEW_TYPEDEF
typedef struct _E_View                E_View;
#endif

struct _E_Iconbar
{
   E_Object      o;    

   E_View       *view;
   Evas_List     icons;
   
   Evas_Object   clip;
   
   int           has_been_scrolled;
   float         scroll;
  
   Ebits_Object *bit;
   struct {
      double x, y, w, h;
   } icon_area;
};

struct _E_Iconbar_Icon
{
   E_Object      o;    

   E_Iconbar   *iconbar;
   
   Evas_Object  image;
   
   char        *image_path;
   char        *exec;
   
   int          hilited;
   struct {
      Evas_Object  image;
      char        *timer;
      double       start;
   } hi;
   
   int         wait;
   float       wait_timeout;
   
   pid_t       launch_pid;
   int         launch_id;
   void       *launch_id_cb;
};

void            e_iconbar_init(void);
E_Iconbar      *e_iconbar_new(E_View *v);
void            e_iconbar_icon_free(E_Iconbar_Icon *);
void            e_iconbar_realize(E_Iconbar *ib);
void            e_iconbar_fix(E_Iconbar *ib);
double          e_iconbar_get_length(E_Iconbar *ib);
void            e_iconbar_file_add(E_View *v, char *file);
void            e_iconbar_file_delete(E_View *v, char *file);
void            e_iconbar_file_change(E_View *v, char *file);
void            e_iconbar_save_out_final(E_Iconbar *ib);
E_Rect *	e_iconbar_get_resist_rect(E_Iconbar *ib);
   
#endif
