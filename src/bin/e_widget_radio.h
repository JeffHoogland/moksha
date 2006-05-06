/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_RADIO_H
#define E_WIDGET_RADIO_H

typedef struct _E_Radio_Group E_Radio_Group;

EAPI E_Radio_Group *e_widget_radio_group_new(int *val);
EAPI Evas_Object *e_widget_radio_add(Evas *evas, char *label, int valnum, E_Radio_Group *group);
EAPI void e_widget_radio_toggle_set(Evas_Object *obj, int toggle);

#endif
#endif
