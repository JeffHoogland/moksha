#include "e.h"

void
e_icon_free(E_Icon *icon)
{
   IF_FREE(icon->file);
   IF_FREE(icon->icon);
   FREE(icon);
}

E_Icon *
e_icon_new(void)
{
   E_Icon *icon;

   icon = NEW(E_Icon, 1);
   ZERO(icon, E_Icon, 1);
   OBJ_INIT(icon, e_icon_free);
   return icon;
}

void
e_icon_calulcate_geometry(E_Icon *icon)
{
   if (!icon->view) return;
}

void
e_icon_realize(E_Icon *icon)
{
   int fx, fy;
   int iw, ih;
   double tw, th;
   
   icon->obj.icon = evas_add_image_from_file(icon->view->evas, icon->icon);
   icon->obj.filename = evas_add_text(icon->view->evas, "borzoib", 8, icon->file);
   evas_get_geometry(icon->view->evas, icon->obj.filename, NULL, NULL, &tw, &th);
   evas_get_image_size(icon->view->evas, icon->obj.icon, & iw, &ih);
   evas_set_color(icon->view->evas, icon->obj.filename, 0, 0, 0, 255);
   fx = icon->x + ((iw - tw) / 2);
   fy = icon->y + ih;
   evas_set_layer(icon->view->evas, icon->obj.icon, 10);
   evas_set_layer(icon->view->evas, icon->obj.filename, 10);
   evas_move(icon->view->evas, icon->obj.icon, icon->x, icon->y);
   evas_move(icon->view->evas, icon->obj.filename, fx, fy);
   if (icon->visible)
     {
	evas_show(icon->view->evas, icon->obj.icon);
	evas_show(icon->view->evas, icon->obj.filename);
     }
}
     
void
e_icon_unrealize(E_Icon *icon)
{
   if (icon->obj.icon) evas_del_object(icon->view->evas, icon->obj.icon);
   if (icon->obj.filename) evas_del_object(icon->view->evas, icon->obj.filename);
}

void
e_icon_update(E_Icon *icon)
{
   int fx, fy;
   int iw, ih;
   double tw, th;
   
   if (!icon->changed) return;
   evas_get_geometry(icon->view->evas, icon->obj.filename, NULL, NULL, &tw, &th);
   evas_get_image_size(icon->view->evas, icon->obj.icon, & iw, &ih);
   fx = icon->x + ((iw - tw) / 2);
   fy = icon->y + ih;
   evas_move(icon->view->evas, icon->obj.icon, icon->x, icon->y);
   evas_move(icon->view->evas, icon->obj.filename, fx, fy);
   icon->changed = 0;
   if (icon->view)
     {
	icon->view->changed = 1;
     }
   icon->changed = 0;
}
