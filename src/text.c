#include "text.h"

E_Text *
e_text_new(Evas evas, char *text, char *class)
{
   E_Text *t;
   
   t = NEW(E_Text, 1);
   ZERO(t, E_Text, 1);
   t->state = strdup("normal");
   if (class) t->class = strdup(class);
   else t->class = strdup("");
   if (text) t->text = strdup(text);
   else t->text = strdup("");
   t->evas = evas;
   t->obj = evas_add_text(t->evas, "borzoib", 8, t->text);
   evas_set_color(t->evas, t->obj, 0, 0, 0, 255);
   return t;
}

void    e_text_free(E_Text *t)
{
   IF_FREE(t->state);
   IF_FREE(t->class);
   IF_FREE(t->text);
   
   if ((t->evas) && (t->obj))
     {
	evas_del_object(t->evas, t->obj);
     }
   FREE(t);
}

void    e_text_set_text(E_Text *t){}
void    e_text_set_layer(E_Text *t, int l){}
void    e_text_set_clip(E_Text *t, Evas_Object clip){}
void    e_text_unset_clip(E_Text *t){}
void    e_text_raise(E_Text *t){}
void    e_text_lower(E_Text *t){}
void    e_text_show(E_Text *t){}
void    e_text_hide(E_Text *t){}
void    e_text_set_color(E_Text *t, int r, int g, int b, int a){}
void    e_text_move(E_Text *t, double x, double y){}
void    e_text_resize(E_Text *t, double w, double h){}
void    e_text_get_geometry(E_Text *t, double *x, double *y, double *w, double *h){}
void    e_text_get_min_size(E_Text *t, double *w, double *h){}
void    e_text_get_max_size(E_Text *t, double *w, double *h){}
void    e_text_set_state(E_Text *t, char *state){}
			    
