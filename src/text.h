#ifndef E_TEXT_H
#define E_TEXT_H

#include "e.h"

typedef struct _E_Text                E_Text;

struct _E_Text
{
   char *text;
   char *class;
   char *state;
   
   int visible;
   double x, y, w, h;
   struct {
      double w, h;
   } min, max;
   struct {
      int r, g, b, a;
   } color;
   int layer;   
   
   Evas evas;
   Evas_Object obj;
};

E_Text *e_text_new(Evas evas, char *text, char *class);
void    e_text_free(E_Text *t);
void    e_text_set_text(E_Text *t);
void    e_text_set_layer(E_Text *t, int l);
void    e_text_set_clip(E_Text *t, Evas_Object clip);
void    e_text_unset_clip(E_Text *t);
void    e_text_raise(E_Text *t);
void    e_text_lower(E_Text *t);
void    e_text_show(E_Text *t);
void    e_text_hide(E_Text *t);
void    e_text_set_color(E_Text *t, int r, int g, int b, int a);
void    e_text_move(E_Text *t, double x, double y);
void    e_text_resize(E_Text *t, double w, double h);
void    e_text_get_geometry(E_Text *t, double *x, double *y, double *w, double *h);
void    e_text_get_min_size(E_Text *t, double *w, double *h);
void    e_text_get_max_size(E_Text *t, double *w, double *h);
void    e_text_set_state(E_Text *t, char *state);

#endif
