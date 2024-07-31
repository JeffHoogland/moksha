#ifdef E_TYPEDEFS
#else
#ifndef E_CANVAS_H
#define E_CANVAS_H

EAPI void        e_canvas_add(Ecore_Evas *ee);
EAPI void        e_canvas_del(Ecore_Evas *ee);
EAPI void        e_canvas_recache(void);
EAPI void        e_canvas_cache_flush(void);
EAPI void        e_canvas_cache_reload(void);
EAPI void        e_canvas_idle_flush(void);
EAPI void        e_canvas_rehint(void);
EAPI Ecore_Evas *e_canvas_new(Ecore_X_Window win, int x, int y, int w, int h, int direct_resize, int override, Ecore_X_Window *win_ret);

EAPI const Eina_List *e_canvas_list(void);
#endif
#endif
