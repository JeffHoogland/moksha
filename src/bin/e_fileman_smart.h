/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Event_Fm_Reconfigure E_Event_Fm_Reconfigure;

#else
#ifndef E_FM_SMART_H
#define E_FM_SMART_H

struct _E_Event_Fm_Reconfigure  
{
   Evas_Object *object;
   Evas_Coord w, h;   
};

EAPI Evas_Object          *e_fm_add(Evas *evas);
EAPI void                  e_fm_dir_set(Evas_Object *object, const char *dir);
EAPI char                 *e_fm_dir_get(Evas_Object *object);
EAPI void                  e_fm_e_win_set(Evas_Object *object, E_Win *win);
EAPI E_Win                *e_fm_e_win_get(Evas_Object *object);
EAPI void                  e_fm_menu_set(Evas_Object *object, E_Menu *menu);
EAPI E_Menu               *e_fm_menu_get(Evas_Object *object);
EAPI void                  e_fm_scroll_horizontal(Evas_Object *object, double percent);
EAPI void                  e_fm_scroll_vertical(Evas_Object *object, double percent);
EAPI void                  e_fm_geometry_virtual_get(Evas_Object *object, Evas_Coord *w, Evas_Coord *h);
EAPI void                  e_fm_reconfigure_callback_add(Evas_Object *object, void (*func)(void *data, Evas_Object *obj, E_Event_Fm_Reconfigure *ev), void *data);
    
extern int E_EVENT_FM_RECONFIGURE;
#endif
#endif
