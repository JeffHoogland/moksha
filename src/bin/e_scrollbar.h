#ifdef E_TYPEDEFS

typedef enum _E_Scrollbar_Direction 
{
   E_SCROLLBAR_HOR = 0,
   E_SCROLLBAR_VERT = 1
} E_Scrollbar_Direction;

#else
#ifndef E_SCROLLBAR_H
#define E_SCROLLBAR_H

EAPI Evas_Object *e_scrollbar_add(Evas *evas);
EAPI void e_scrollbar_direction_set_(Evas_Object *object, E_Scrollbar_Direction dir);
EAPI E_Scrollbar_Direction e_scrollbar_direction_get(Evas_Object *object);
   
#endif
#endif
