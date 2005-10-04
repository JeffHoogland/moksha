/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef enum _E_Scrollbar_Direction
{
   E_SCROLLBAR_HORIZONTAL,
   E_SCROLLBAR_VERTICAL
} E_Scrollbar_Direction;

#else
#ifndef E_SCROLLBAR_H
#define E_SCROLLBAR_H

EAPI Evas_Object          *e_scrollbar_add(Evas *evas);
EAPI void                  e_scrollbar_direction_set(Evas_Object *object,
						     E_Scrollbar_Direction dir);
EAPI E_Scrollbar_Direction e_scrollbar_direction_get(Evas_Object *object);

#endif
#endif
