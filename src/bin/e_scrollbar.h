/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Scrollbar_Drag_Handler E_Scrollbar_Drag_Handler;
typedef enum _E_Scrollbar_Direction E_Scrollbar_Direction;

#else
#ifndef E_SCROLLBAR_H
#define E_SCROLLBAR_H

struct _E_Scrollbar_Drag_Handler
{
   void *data;
   struct {
      void (*drag)(Evas_Object *obj, double value, void *data);
   } cb;
};

enum _E_Scrollbar_Direction
{
   E_SCROLLBAR_HORIZONTAL = 0,
   E_SCROLLBAR_VERTICAL = 1
};

EAPI Evas_Object          *e_scrollbar_add(Evas *evas);
EAPI void                  e_scrollbar_direction_set(Evas_Object *object, E_Scrollbar_Direction dir);
EAPI E_Scrollbar_Direction e_scrollbar_direction_get(Evas_Object *object);
EAPI void                  e_scrollbar_callback_drag_add (Evas_Object *object, void (*func)(Evas_Object *obj, double value, void *data), void *data);
EAPI void                  e_scrollbar_value_set (Evas_Object *object, double value);
EAPI double                e_scrollbar_value_get (Evas_Object *object);
EAPI void                  e_scrollbar_drag_resize(Evas_Object *object, int percent);

#endif
#endif
