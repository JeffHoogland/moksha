#include "e.h"

/* externally accessible functions */
void
e_pointer_container_set(E_Container *con)
{
   Ecore_X_Cursor cur;
   int w, h;
   Evas_Object *o;
   int *pix;

   E_OBJECT_CHECK(E_OBJECT(con));
   
   o = evas_object_image_add(con->bg_evas);
   if (ecore_x_cursor_color_supported_get())
     evas_object_image_file_set(o, e_path_find(path_images, "pointer.png"), NULL);
   else
     evas_object_image_file_set(o, e_path_find(path_images, "pointer_mono.png"), NULL);
   evas_object_image_size_get(o, &w, &h);
   pix = evas_object_image_data_get(o, 0);
   cur = ecore_x_cursor_new(con->win, pix, w, h, 0, 0);
   evas_object_image_data_set(o, pix);
   evas_object_del(o);
   ecore_x_window_cursor_set(con->manager->root, cur);
   ecore_x_window_cursor_set(con->win, cur);
   ecore_x_cursor_free(cur);
}

void
e_pointer_ecore_evas_set(Ecore_Evas *ee)
{
   Ecore_X_Window win;
   Ecore_X_Cursor cur;
   int w, h;
   Evas_Object *o;
   int *pix;
   Evas *e;
   
   e = ecore_evas_get(ee);
   win = ecore_evas_software_x11_window_get(ee);
   o = evas_object_image_add(e);
   if (ecore_x_cursor_color_supported_get())
     evas_object_image_file_set(o, e_path_find(path_images, "pointer.png"), NULL);
   else
     evas_object_image_file_set(o, e_path_find(path_images, "pointer_mono.png"), NULL);
   evas_object_image_size_get(o, &w, &h);
   pix = evas_object_image_data_get(o, 0);
   cur = ecore_x_cursor_new(win, pix, w, h, 0, 0);
   evas_object_image_data_set(o, pix);
   evas_object_del(o);
   ecore_x_window_cursor_set(win, cur);
   ecore_x_cursor_free(cur);
}
