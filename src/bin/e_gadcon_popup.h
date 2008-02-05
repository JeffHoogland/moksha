/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Gadcon_Popup E_Gadcon_Popup;

#else
#ifndef E_GADCON_POPUP_H
#define E_GADCON_POPUP_H

#define E_GADCON_POPUP_TYPE 0xE0b0104e

struct _E_Gadcon_Popup
{
   E_Object             e_obj_inherit;

   E_Popup	       *win;
   E_Gadcon_Client     *gcc;
   Evas_Coord		w, h;
   Evas_Object	       *o_bg;

   int			pinned : 1;

   void			(*resize_func) (Evas_Object *obj, int *w, int *h);
};

EAPI E_Gadcon_Popup *e_gadcon_popup_new(E_Gadcon_Client *gcc, void (*resize_func) (Evas_Object *obj, int *w, int *h));
EAPI void e_gadcon_popup_content_set(E_Gadcon_Popup *pop, Evas_Object *o);
EAPI void e_gadcon_popup_show(E_Gadcon_Popup *pop);
EAPI void e_gadcon_popup_hide(E_Gadcon_Popup *pop);
EAPI void e_gadcon_popup_toggle_pinned(E_Gadcon_Popup *pop);

#endif
#endif
