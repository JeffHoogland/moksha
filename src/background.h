#ifndef E_BACKGROUND_H
#define E_BACKGROUND_H

#include "object.h"

typedef struct _E_Background E_Background;
typedef struct _E_Background_Layer E_Background_Layer;

typedef enum
{
   E_BACKGROUND_TYPE_IMAGE = 0,
   E_BACKGROUND_TYPE_GRADIENT = 1,
   E_BACKGROUND_TYPE_SOLID = 2
}
E_Background_Type;

struct _E_Background
{
   E_Object            o;

   Evas *                evas;
   char               *file;

   struct
   {
      int                 sx, sy;
      int                 w, h;
   }
   geom;

   Evas_List *           layers;

   Evas_Object *         base_obj;
};

struct _E_Background_Layer
{
   E_Background_Type   type;
   int                 inlined;
   struct
   {
      float               x, y;
   }
   scroll;
   struct
   {
      float               x, y;
   }
   pos;
   struct
   {
      float               w, h;
      struct
      {
	 int                 w, h;
      }
      orig;
   }
   size               , fill;
   char               *color_class;
   char               *file;
   double              angle;
   struct
   {
      int                 r, g, b, a;
   }
   fg                 , bg;

   double              x, y, w, h, fw, fh;

   Evas_Object *         obj;
};

E_Background       *e_background_new(void);
E_Background       *e_background_load(char *file);

void                e_background_realize(E_Background * bg, Evas * evas);
void                e_background_set_scroll(E_Background * bg, int sx, int sy);
void                e_background_set_size(E_Background * bg, int w, int h);
void                e_background_set_color_class(E_Background * bg, char *cc,
						 int r, int g, int b, int a);

#endif
