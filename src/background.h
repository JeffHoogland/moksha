#ifndef E_BACKGROUND_H
#define E_BACKGROUND_H

#include "e.h"

struct _E_Background
{
   OBJ_PROPERTIES;
   
   Evas  evas;
   char *file;
   
   struct {
      int sx, sy;
      int w, h;
   } geom;
   
   Evas_List layers;
   
   Evas_Object base_obj;
};

void           e_background_free(E_Background *bg);
E_Background  *e_background_new(void);
E_Background  *e_background_load(char *file);
void           e_background_realize(E_Background *bg, Evas evas);
void           e_background_set_scroll(E_Background *bg, int sx, int sy);
void           e_background_set_size(E_Background *bg, int w, int h);
void           e_background_set_color_class(E_Background *bg, char *cc, int r, int g, int b, int a);

#endif 
