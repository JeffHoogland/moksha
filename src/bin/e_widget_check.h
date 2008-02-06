/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_CHECK_H
#define E_WIDGET_CHECK_H

EAPI Evas_Object *e_widget_check_add(Evas *evas, const char *label, int *val);
EAPI void	  e_widget_check_checked_set(Evas_Object *check, int checked);
EAPI int	  e_widget_check_checked_get(Evas_Object *check);
EAPI Evas_Object *e_widget_check_icon_add(Evas *evas, const char *label, const char *icon, int icon_w, int icon_h, int *val);

#endif
#endif
