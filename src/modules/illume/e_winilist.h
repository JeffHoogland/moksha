#ifndef E_WINILIST_H
#define E_WINILIST_H

EAPI int e_winilist_init(void);
EAPI int e_winilist_shutdown(void);

EAPI Evas_Object *e_winilist_add(Evas *e);
EAPI void e_winilist_border_select_callback_set(Evas_Object *obj, void (*func) (void *data, E_Border *bd), void *data);
EAPI void e_winilist_special_append(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data1, void *data2), void *data1, void *data2);
EAPI void e_winilist_special_prepend(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data1, void *data2), void *data1, void *data2);
EAPI void e_winilist_optimial_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);
    
#endif
