/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Event_Fm_Reconfigure      E_Event_Fm_Reconfigure;
typedef struct _E_Event_Fm_Directory_Change E_Event_Fm_Directory_Change;

#else
#ifndef E_FM_SMART_H
#define E_FM_SMART_H

struct _E_Event_Fm_Reconfigure  
{
   Evas_Object *object;
   Evas_Coord x, y, w, h;   
};

struct _E_Event_Fm_Directory_Change
{
   Evas_Object *object;
   Evas_Coord w, h;  
};

EAPI int                   e_fm_init(void);
EAPI int                   e_fm_shutdown(void);
EAPI Evas_Object          *e_fm_add(Evas *evas);
EAPI void                  e_fm_dir_set(Evas_Object *object, const char *dir);
EAPI char                 *e_fm_dir_get(Evas_Object *object);
EAPI void                  e_fm_e_win_set(Evas_Object *object, E_Win *win);
EAPI E_Win                *e_fm_e_win_get(Evas_Object *object);
EAPI void                  e_fm_menu_set(Evas_Object *object, E_Menu *menu);
EAPI E_Menu               *e_fm_menu_get(Evas_Object *object);

EAPI void                  e_fm_scroll_set(Evas_Object *object, Evas_Coord x, Evas_Coord y);
EAPI void                  e_fm_scroll_get(Evas_Object *object, Evas_Coord *x, Evas_Coord *y);
EAPI void                  e_fm_scroll_max_get(Evas_Object *object, Evas_Coord *x, Evas_Coord *y);

EAPI void                  e_fm_scroll_horizontal(Evas_Object *object, double percent);
EAPI void                  e_fm_scroll_vertical(Evas_Object *object, double percent);

EAPI void                  e_fm_geometry_virtual_get(Evas_Object *object, Evas_Coord *w, Evas_Coord *h);
EAPI void                  e_fm_reconfigure_callback_add(Evas_Object *object, void (*func)(void *data, Evas_Object *obj, E_Event_Fm_Reconfigure *ev), void *data);
EAPI int                   e_fm_freeze(Evas_Object *freeze);
EAPI int                   e_fm_thaw(Evas_Object *freeze);
EAPI void                  e_fm_selector_enable(Evas_Object *object, void (*func)(Evas_Object *object, char *file, void *data), void *data);
EAPI void                  e_fm_background_set(Evas_Object *object, Evas_Object *bg);

EAPI Evas_Object          *e_fm_icon_create(void *data);
EAPI void                  e_fm_icon_destroy(Evas_Object *obj, void *data);

extern int E_EVENT_FM_RECONFIGURE;
extern int E_EVENT_FM_DIRECTORY_CHANGE;
#endif
#endif
