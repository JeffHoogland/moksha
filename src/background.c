#include "e.h"
#include "debug.h"
#include "background.h"
#include "util.h"

static void
e_background_cleanup(E_Background * bg)
{
   Evas_List           l;

   D_ENTER;

   if (bg->layers)
     {
	for (l = bg->layers; l; l = l->next)
	  {
	     E_Background_Layer *bl;

	     bl = l->data;
	     if (bl->color_class)
		FREE(bl->color_class);
	     if (bl->file)
		FREE(bl->file);
	     if (bl->obj)
		evas_del_object(bg->evas, bl->obj);
	     FREE(bl);
	  }
	evas_list_free(bg->layers);
     }
   if (bg->file)
      FREE(bg->file);
   if (bg->base_obj)
      evas_del_object(bg->evas, bg->base_obj);

   e_object_cleanup(E_OBJECT(bg));

   D_RETURN;
}

E_Background       *
e_background_new(void)
{
   E_Background       *bg;

   D_ENTER;

   bg = NEW(E_Background, 1);
   ZERO(bg, E_Background, 1);

   e_object_init(E_OBJECT(bg), (E_Cleanup_Func) e_background_cleanup);

   D_RETURN_(bg);
}

E_Background       *
e_background_load(char *file)
{
   E_Background       *bg;
   E_DB_File          *db;
   int                 i, num;

   D_ENTER;

   db = e_db_open_read(file);
   if (!db)
      D_RETURN_(NULL);
   num = 0;
   e_db_int_get(db, "/type/bg", &num);
   if (num != 1)
     {
	e_db_close(db);
	e_db_flush();
	D_RETURN_(NULL);
     }
   e_db_int_get(db, "/layers/count", &num);

   bg = e_background_new();
   e_strdup(bg->file, file);
   for (i = 0; i < num; i++)
     {
	E_Background_Layer *bl;
	char                buf[PATH_MAX];

	bl = NEW(E_Background_Layer, 1);
	ZERO(bl, E_Background_Layer, 1);
	bg->layers = evas_list_append(bg->layers, bl);

	snprintf(buf, PATH_MAX, "/layers/%i/type", i);
	e_db_int_get(db, buf, (int *)&(bl->type));
	snprintf(buf, PATH_MAX, "/layers/%i/inlined", i);
	e_db_int_get(db, buf, &(bl->inlined));
	snprintf(buf, PATH_MAX, "/layers/%i/color_class", i);
	bl->color_class = e_db_str_get(db, buf);
	if (bl->inlined)
	  {
	     snprintf(buf, PATH_MAX, "%s:/layers/%i/image", file, i);
	     e_strdup(bl->file, buf);
	  }
	else
	  {
	     snprintf(buf, PATH_MAX, "/layers/%i/file", i);
	     bl->file = e_db_str_get(db, buf);
	  }
	snprintf(buf, PATH_MAX, "/layers/%i/scroll.x", i);
	e_db_float_get(db, buf, &(bl->scroll.x));
	snprintf(buf, PATH_MAX, "/layers/%i/scroll.y", i);
	e_db_float_get(db, buf, &(bl->scroll.y));
	snprintf(buf, PATH_MAX, "/layers/%i/pos.x", i);
	e_db_float_get(db, buf, &(bl->pos.x));
	snprintf(buf, PATH_MAX, "/layers/%i/pos.y", i);
	e_db_float_get(db, buf, &(bl->pos.y));
	snprintf(buf, PATH_MAX, "/layers/%i/size.w", i);
	e_db_float_get(db, buf, &(bl->size.w));
	snprintf(buf, PATH_MAX, "/layers/%i/size.h", i);
	e_db_float_get(db, buf, &(bl->size.h));
	snprintf(buf, PATH_MAX, "/layers/%i/size.orig.w", i);
	e_db_int_get(db, buf, &(bl->size.orig.w));
	snprintf(buf, PATH_MAX, "/layers/%i/size.orig.h", i);
	e_db_int_get(db, buf, &(bl->size.orig.h));
	snprintf(buf, PATH_MAX, "/layers/%i/fill.w", i);
	e_db_float_get(db, buf, &(bl->fill.w));
	snprintf(buf, PATH_MAX, "/layers/%i/fill.h", i);
	e_db_float_get(db, buf, &(bl->fill.h));
	snprintf(buf, PATH_MAX, "/layers/%i/fill.orig.w", i);
	e_db_int_get(db, buf, &(bl->fill.orig.w));
	snprintf(buf, PATH_MAX, "/layers/%i/fill.orig.h", i);
	e_db_int_get(db, buf, &(bl->fill.orig.h));
	snprintf(buf, PATH_MAX, "/layers/%i/angle", i);
	e_db_float_get(db, buf, (float *)&(bl->angle));
	snprintf(buf, PATH_MAX, "/layers/%i/fg.r", i);
	e_db_int_get(db, buf, &(bl->fg.r));
	snprintf(buf, PATH_MAX, "/layers/%i/fg.g", i);
	e_db_int_get(db, buf, &(bl->fg.g));
	snprintf(buf, PATH_MAX, "/layers/%i/fg.b", i);
	e_db_int_get(db, buf, &(bl->fg.b));
	snprintf(buf, PATH_MAX, "/layers/%i/fg.a", i);
	e_db_int_get(db, buf, &(bl->fg.a));
	snprintf(buf, PATH_MAX, "/layers/%i/bg.r", i);
	e_db_int_get(db, buf, &(bl->bg.r));
	snprintf(buf, PATH_MAX, "/layers/%i/bg.g", i);
	e_db_int_get(db, buf, &(bl->bg.g));
	snprintf(buf, PATH_MAX, "/layers/%i/bg.b", i);
	e_db_int_get(db, buf, &(bl->bg.b));
	snprintf(buf, PATH_MAX, "/layers/%i/bg.a", i);
	e_db_int_get(db, buf, &(bl->bg.a));
     }
   e_db_close(db);
   D_RETURN_(bg);
}

void
e_background_realize(E_Background * bg, Evas evas)
{
   Evas_List           l;
   int                 ww, hh, count;

   D_ENTER;

   if (bg->evas)
      D_RETURN;
   bg->evas = evas;
   if (!bg->evas)
      D_RETURN;
   for (count = 0, l = bg->layers; l; l = l->next, count++)
     {
	E_Background_Layer *bl;

	bl = l->data;
	if (bl->type == E_BACKGROUND_TYPE_IMAGE)
	  {
	     bl->obj = evas_add_image_from_file(bg->evas, bl->file);
	     evas_set_layer(bg->evas, bl->obj, 0);
	     evas_show(bg->evas, bl->obj);
#if 0				/* dont need this... do we? */
	     if (evas_get_image_alpha(bg->evas, bl->obj))
	       {
		  D("Adding rectangle to bg!\n");
		  bg->base_obj = evas_add_rectangle(bg->evas);
		  evas_lower(bg->evas, bg->base_obj);
		  evas_move(bg->evas, bg->base_obj, 0, 0);
		  evas_resize(bg->evas, bg->base_obj, 999999999, 999999999);
		  evas_set_color(bg->evas, bg->base_obj, 255, 255, 255, 255);
		  evas_show(bg->evas, bg->base_obj);
	       }
#endif
	  }
	else if (bl->type == E_BACKGROUND_TYPE_GRADIENT)
	  {
	  }
	else if (bl->type == E_BACKGROUND_TYPE_SOLID)
	  {
	  }
     }
   ww = bg->geom.w;
   hh = bg->geom.h;
   bg->geom.w = 0;
   bg->geom.h = 0;
   e_background_set_size(bg, ww, hh);

   D_RETURN;
}

void
e_background_set_scroll(E_Background * bg, int sx, int sy)
{
   Evas_List           l;

   D_ENTER;

   if ((bg->geom.sx == sx) && (bg->geom.sy == sy))
      D_RETURN;
   bg->geom.sx = sx;
   bg->geom.sy = sy;
   if (!bg->evas)
      D_RETURN;
   for (l = bg->layers; l; l = l->next)
     {
	E_Background_Layer *bl;

	bl = l->data;
	if (bl->type == E_BACKGROUND_TYPE_IMAGE)
	  {
	     evas_set_image_fill(bg->evas, bl->obj,
				 (double)bg->geom.sx * bl->scroll.x,
				 (double)bg->geom.sy * bl->scroll.y,
				 bl->fw, bl->fh);
	  }
     }
   D_RETURN;
}

void
e_background_set_size(E_Background * bg, int w, int h)
{
   Evas_List           l;

   D_ENTER;

   if ((bg->geom.w == w) && (bg->geom.h == h))
      D_RETURN;
   bg->geom.w = w;
   bg->geom.h = h;
   for (l = bg->layers; l; l = l->next)
     {
	E_Background_Layer *bl;
	double              x, y, w, h, fw, fh;
	int                 iw, ih;

	bl = l->data;
	iw = 0;
	ih = 0;
	if (bg->evas)
	   evas_get_image_size(bg->evas, bl->obj, &iw, &ih);
	w = bl->size.w * (double)bg->geom.w;
	h = bl->size.h * (double)bg->geom.h;
	if (bl->size.orig.w)
	   w = (double)iw     *bl->size.w;

	if (bl->size.orig.h)
	   h = (double)ih     *bl->size.h;

	fw = bl->fill.w * w;
	fh = bl->fill.h * h;
	if (bl->fill.orig.w)
	   fw = (double)iw    *bl->fill.w;

	if (bl->fill.orig.h)
	   fh = (double)ih    *bl->fill.h;

	x = ((double)bg->geom.w - w + 1) * bl->pos.x;
	y = ((double)bg->geom.h - h + 1) * bl->pos.y;
	bl->x = x;
	bl->y = y;
	bl->w = w;
	bl->h = h;
	bl->fw = fw;
	bl->fh = fh;
	if (bg->evas)
	  {
	     evas_move(bg->evas, bl->obj, bl->x, bl->y);
	     evas_resize(bg->evas, bl->obj, bl->w, bl->h);
	     if (bl->type == E_BACKGROUND_TYPE_IMAGE)
	       {
		  evas_set_image_fill(bg->evas, bl->obj,
				      (double)bg->geom.sx * bl->scroll.x,
				      (double)bg->geom.sy * bl->scroll.y,
				      bl->fw, bl->fh);
	       }
	     else if (bl->type == E_BACKGROUND_TYPE_GRADIENT)
	       {
		  evas_set_angle(bg->evas, bl->obj, bl->angle);
	       }
	     else if (bl->type == E_BACKGROUND_TYPE_SOLID)
	       {
	       }
	  }
     }
   D_RETURN;
}

void
e_background_set_color_class(E_Background * bg, char *cc, int r, int g, int b,
			     int a)
{
   Evas_List           l;

   D_ENTER;

   for (l = bg->layers; l; l = l->next)
     {
	E_Background_Layer *bl;

	bl = l->data;
	if ((bl->color_class) && (cc) && (!strcmp(bl->color_class, cc)))
	  {
	     if (bg->evas)
	       {
		  if ((l == bg->layers) && (bg->base_obj))
		     evas_set_color(bg->evas, bl->obj, r, g, b, 255);
		  else
		     evas_set_color(bg->evas, bl->obj, r, g, b, a);
	       }
	  }
     }

   D_RETURN;
}
