#include "e.h"
#include "object.h"

typedef struct _E_Scrollbar           E_Scrollbar;

struct _E_Scrollbar
{
   E_Object o;

   /* I<---val--->|<==range==>|<-->I<-max */
   double val;
   double max;
   double range;

   char  *dir;
   
   Evas   evas;
   
   Ebits_Object bar;
   Ebits_Object base;
   
   int    visible;
   int    layer;
   int    direction;
   double x, y, w, h;
   
   int    mouse_down;
   int    down_x, down_y;
   int    mouse_x, mouse_y;
   int    scrolling_up, scrolling_down;
   
   struct {
      double x, y, w, h;      
   } bar_area;
   struct {
      double x, y, w, h;      
   } bar_pos;
   
   void  (*func_change) (void *_data, E_Scrollbar *sb, double val);
   void  *func_data;
};

E_Scrollbar *e_scrollbar_new(void);
void         e_scrollbar_add_to_evas(E_Scrollbar *sb, Evas evas);
void         e_scrollbar_show(E_Scrollbar *sb); 
void         e_scrollbar_hide(E_Scrollbar *sb); 
void         e_scrollbar_raise(E_Scrollbar *sb); 
void         e_scrollbar_lower(E_Scrollbar *sb); 
void         e_scrollbar_set_layer(E_Scrollbar *sb, int l); 
void         e_scrollbar_set_direction(E_Scrollbar *sb, int d); 
void         e_scrollbar_move(E_Scrollbar *sb, double x, double y); 
void         e_scrollbar_resize(E_Scrollbar *sb, double w, double h);
void         e_scrollbar_set_change_func(E_Scrollbar *sb,
					 void (*func_change) (void *_data, E_Scrollbar *sb, double val),
					 void *data);
void         e_scrollbar_set_value(E_Scrollbar *sb, double val);
void         e_scrollbar_set_range(E_Scrollbar *sb, double range);
void         e_scrollbar_set_max(E_Scrollbar *sb, double max);
double       e_scrollbar_get_value(E_Scrollbar *sb);
double       e_scrollbar_get_range(E_Scrollbar *sb);
double       e_scrollbar_get_max(E_Scrollbar *sb);
void         e_scrollbar_get_geometry(E_Scrollbar *sb, double *x, double *y, double *w, double *h);
