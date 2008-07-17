/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Win E_Win;

#else
#ifndef E_WIN_H
#define E_WIN_H

#define E_WIN_TYPE 0xE0b01011

struct _E_Win
{
   E_Object             e_obj_inherit;
   
   int                  x, y, w, h;
   int                  engine;
   E_Container         *container;
   E_Border            *border;
   Ecore_Evas          *ecore_evas;
   Evas                *evas;
   Ecore_X_Window       evas_win;
   unsigned char        placed : 1;
   int                  min_w, min_h, max_w, max_h, base_w, base_h;
   int                  step_x, step_y;
   double               min_aspect, max_aspect;
   void               (*cb_move) (E_Win *win);
   void               (*cb_resize) (E_Win *win);
   void               (*cb_delete) (E_Win *win);
   void                *data;

   struct {
      unsigned char     centered : 1;
      unsigned char     dialog : 1;
      unsigned char     no_remember : 1;
   } state;
   
   E_Pointer           *pointer;
};

EAPI int    e_win_init               (void);
EAPI int    e_win_shutdown           (void);
EAPI E_Win *e_win_new                (E_Container *con);
EAPI void   e_win_show               (E_Win *win);
EAPI void   e_win_hide               (E_Win *win);
EAPI void   e_win_move               (E_Win *win, int x, int y);
EAPI void   e_win_resize             (E_Win *win, int w, int h);
EAPI void   e_win_move_resize        (E_Win *win, int x, int y, int w, int h);
EAPI void   e_win_raise              (E_Win *win);
EAPI void   e_win_lower              (E_Win *win);
EAPI void   e_win_placed_set         (E_Win *win, int placed);
EAPI Evas  *e_win_evas_get           (E_Win *win);
EAPI void   e_win_shaped_set         (E_Win *win, int shaped);
EAPI void   e_win_avoid_damage_set   (E_Win *win, int avoid);
EAPI void   e_win_borderless_set     (E_Win *win, int borderless);
EAPI void   e_win_layer_set          (E_Win *win, int layer);
EAPI void   e_win_sticky_set         (E_Win *win, int sticky);
EAPI void   e_win_move_callback_set  (E_Win *win, void (*func) (E_Win *win));
EAPI void   e_win_resize_callback_set(E_Win *win, void (*func) (E_Win *win));
EAPI void   e_win_delete_callback_set(E_Win *win, void (*func) (E_Win *win));
EAPI void   e_win_size_min_set       (E_Win *win, int w, int h);
EAPI void   e_win_size_max_set       (E_Win *win, int w, int h);
EAPI void   e_win_size_base_set      (E_Win *win, int w, int h);
EAPI void   e_win_step_set           (E_Win *win, int x, int y);
EAPI void   e_win_name_class_set     (E_Win *win, const char *name, const char *class);
EAPI void   e_win_title_set          (E_Win *win, const char *title);
EAPI void   e_win_border_icon_set    (E_Win *win, const char *icon);
EAPI void   e_win_border_icon_key_set(E_Win *win, const char *key);
EAPI void   e_win_centered_set       (E_Win *win, int centered);
EAPI void   e_win_dialog_set         (E_Win *win, int dialog);
EAPI void   e_win_no_remember_set    (E_Win *win, int no_remember);

EAPI E_Win *e_win_evas_object_win_get(Evas_Object *obj);
    
#endif
#endif
