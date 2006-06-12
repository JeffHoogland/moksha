/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_FM_H
#define E_WIDGET_FM_H

EAPI Evas_Object     *e_widget_fileman_add(Evas *evas, char **val);
EAPI void             e_widget_fileman_select_callback_add(Evas_Object *obj, void (*func) (Evas_Object *obj, char *file, void *data), void *data);
EAPI void             e_widget_fileman_hilite_callback_add(Evas_Object *obj, void (*func) (Evas_Object *obj, char *file, void *data), void *data);
EAPI void             e_widget_fileman_dir_set(Evas_Object *obj, const char *dir);

#endif
#endif
