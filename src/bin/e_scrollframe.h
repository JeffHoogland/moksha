/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_SCROLLFRAME_H
#define E_SCROLLFRAME_H

typedef enum _E_Scrollframe_Policy
{
   E_SCROLLFRAME_POLICY_OFF,
     E_SCROLLFRAME_POLICY_ON,
     E_SCROLLFRAME_POLICY_AUTO
}
E_Scrollframe_Policy;

EAPI Evas_Object *e_scrollframe_add             (Evas *evas);
EAPI void e_scrollframe_child_set               (Evas_Object *obj, Evas_Object *child);
EAPI void e_scrollframe_child_pos_set           (Evas_Object *obj, Evas_Coord x, Evas_Coord y);
EAPI void e_scrollframe_child_pos_get           (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
EAPI void e_scrollframe_child_viewport_size_get (Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);
EAPI void e_scrollframe_step_size_set           (Evas_Object *obj, Evas_Coord x, Evas_Coord y);
EAPI void e_scrollframe_step_size_get           (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
EAPI void e_scrollframe_page_size_set           (Evas_Object *obj, Evas_Coord x, Evas_Coord y);
EAPI void e_scrollframe_page_size_get           (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
EAPI void e_scrollframe_policy_set              (Evas_Object *obj, E_Scrollframe_Policy hbar, E_Scrollframe_Policy vbar);
EAPI void e_scrollframe_policy_get              (Evas_Object *obj, E_Scrollframe_Policy *hbar, E_Scrollframe_Policy *vbar);
    
#endif
#endif
