#ifdef E_TYPEDEFS
#else
# ifndef E_SMART_RANDR_H
#  define E_SMART_RANDR_H

Evas_Object *e_smart_randr_add(Evas *evas);
void e_smart_randr_virtual_size_calc(Evas_Object *obj);
void e_smart_randr_monitors_create(Evas_Object *obj);

# endif
#endif
