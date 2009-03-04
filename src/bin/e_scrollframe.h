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
EAPI void e_scrollframe_extern_pan_set          (Evas_Object *obj, Evas_Object *pan, void (*pan_set) (Evas_Object *obj, Evas_Coord x, Evas_Coord y), void (*pan_get) (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y), void (*pan_max_get) (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y), void (*pan_child_size_get) (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y));
EAPI int  e_scrollframe_custom_theme_set        (Evas_Object *obj, char *custom_category, char *custom_group);
EAPI int  e_scrollframe_custom_edje_file_set    (Evas_Object *obj, char *file, char *group);
EAPI void e_scrollframe_child_pos_set           (Evas_Object *obj, Evas_Coord x, Evas_Coord y);
EAPI void e_scrollframe_child_pos_get           (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
EAPI void e_scrollframe_child_region_show       (Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h);
EAPI void e_scrollframe_child_viewport_size_get (Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);
EAPI void e_scrollframe_step_size_set           (Evas_Object *obj, Evas_Coord x, Evas_Coord y);
EAPI void e_scrollframe_step_size_get           (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
EAPI void e_scrollframe_page_size_set           (Evas_Object *obj, Evas_Coord x, Evas_Coord y);
EAPI void e_scrollframe_page_size_get           (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
EAPI void e_scrollframe_policy_set              (Evas_Object *obj, E_Scrollframe_Policy hbar, E_Scrollframe_Policy vbar);
EAPI void e_scrollframe_policy_get              (Evas_Object *obj, E_Scrollframe_Policy *hbar, E_Scrollframe_Policy *vbar);
EAPI Evas_Object *e_scrollframe_edje_object_get (Evas_Object *obj);
EAPI void e_scrollframe_single_dir_set          (Evas_Object *obj, Evas_Bool single_dir);
EAPI Evas_Bool e_scrollframe_single_dir_get     (Evas_Object *obj);
EAPI void e_scrollframe_thumbscroll_force       (Evas_Object *obj, Evas_Bool forced);
    
#endif
#endif
