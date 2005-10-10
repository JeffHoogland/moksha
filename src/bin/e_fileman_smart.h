/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_FM_SMART_H
#define E_FM_SMART_H

EAPI Evas_Object          *e_fm_add(Evas *evas);
EAPI void                  e_fm_dir_set(Evas_Object *object, const char *dir);
EAPI char                 *e_fm_dir_get(Evas_Object *object);
EAPI void                  e_fm_e_win_set(Evas_Object *object, E_Win *win);
EAPI E_Win                *e_fm_e_win_get(Evas_Object *object);
EAPI void                  e_fm_menu_set(Evas_Object *object, E_Menu *menu);
EAPI E_Menu               *e_fm_menu_get(Evas_Object *object);
EAPI void                  e_fm_scroll_horizontal(Evas_Object *object, double percent);
EAPI void                  e_fm_scroll_vertical(Evas_Object *object, double percent);

#endif
#endif
