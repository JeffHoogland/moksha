#include "e.h"

void
e_background_free(E_Background *bg)
{
   IF_FREE(bg->image);
   if ((bg->evas) && (bg->obj))
     evas_del_object(bg->evas, bg->obj);
   FREE(bg);
}

E_Background *
e_background_new(void)
{
   E_Background *bg;
   
   bg = NEW(E_Background, 1);
   ZERO(bg, E_Background, 1);
   OBJ_INIT(bg, e_background_free);
   
   return bg;
}

void
e_background_realize(E_Background *bg, Evas evas)
{
   Evas_Object o;
   int iw, ih;
   
   if (bg->evas) return;
   /* FIXME: boring for now - just a scaled image */
   bg->evas = evas;
   bg->obj = evas_add_image_from_file(bg->evas, bg->image);
   evas_set_layer(bg->evas, bg->obj, 0);
   evas_move(bg->evas, bg->obj, 0, 0);
   evas_resize(bg->evas, bg->obj, bg->geom.w, bg->geom.h);
   evas_set_image_fill(bg->evas, bg->obj, 0, 0, bg->geom.w, bg->geom.h);   
   evas_show(bg->evas, bg->obj);
   o = evas_add_image_from_file(bg->evas, PACKAGE_DATA_DIR"/data/images/e_logo.png");
   evas_get_image_size(bg->evas, o, &iw, &ih);
   evas_move(bg->evas, o, (bg->geom.w - iw) / 2, (bg->geom.h - ih) / 2);
   evas_show(bg->evas, o);
}
