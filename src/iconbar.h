#ifndef IB_ICONBAR_H
#define IB_ICONBAR_H

#include "e.h"
#include "config.h"
#include "exec.h"
#include "view.h"

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
   OBJ_PROPERTIES;

   E_View       *view;
   Evas_List     icons;
  
   Ebits_Object *bit;
   struct {
      double x, y, w, h;
   } icon_area;
};

struct _E_Iconbar_Icon
{
   OBJ_PROPERTIES;

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
};

void            e_iconbar_init(void);
E_Iconbar      *e_iconbar_new(E_View *v);
void            e_iconbar_free(E_Iconbar *ib);
void            e_iconbar_icon_free(E_Iconbar_Icon *);
void            e_iconbar_realize(E_Iconbar *ib);
void            e_iconbar_fix(E_Iconbar *ib);
void            e_iconbar_file_add(E_View *v, char *file);
void            e_iconbar_file_delete(E_View *v, char *file);
void            e_iconbar_file_change(E_View *v, char *file);

#endif
