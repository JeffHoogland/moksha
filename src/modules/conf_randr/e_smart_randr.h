#ifdef E_TYPEDEFS
#else
# ifndef E_SMART_RANDR_H
#  define E_SMART_RANDR_H

Evas_Object *e_smart_randr_add(Evas *evas);
void e_smart_randr_virtual_size_calc(Evas_Object *obj);
void e_smart_randr_monitors_create(Evas_Object *obj);
void e_smart_randr_min_size_get(Evas_Object *obj, Evas_Coord *mw, Evas_Coord *mh);
Eina_Bool e_smart_randr_changed_get(Evas_Object *obj);
void e_smart_randr_changes_apply(Evas_Object *obj);
Eina_List *e_smart_randr_monitors_get(Evas_Object *obj);

# endif
#endif
