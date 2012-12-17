#ifdef E_TYPEDEFS
#else
# ifndef E_SMART_RANDR_H
#  define E_SMART_RANDR_H

Evas_Object *e_smart_randr_add(Evas *evas);
void e_smart_randr_current_size_set(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
void e_smart_randr_monitors_create(Evas_Object *obj);

# endif
#endif
