#ifdef E_TYPEDEFS
#else
#ifndef E_CANVAS_H
#define E_CANVAS_H

EAPI void e_canvas_add(Ecore_Evas *ee);
EAPI void e_canvas_del(Ecore_Evas *ee);
EAPI void e_canvas_recache(void);
EAPI void e_canvas_cache_flush(void);
EAPI void e_canvas_cache_reload(void);
    
#endif
#endif
