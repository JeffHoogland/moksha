#ifndef E_CANVAS_H
#define E_CANVAS_H

void e_canvas_add(Ecore_Evas *ee);
void e_canvas_del(Ecore_Evas *ee);
void e_canvas_recache(void);
void e_canvas_cache_flush(void);
void e_canvas_cache_reload(void);
    
#endif
