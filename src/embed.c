#include "embed.h"

typedef struct _Embed Embed_Private;

struct _Embed
{
   Ebits_Object o;
   Evas evas;
   Evas_Object image_obj;
   Evas_Object clip_obj;
   int clip_x, clip_y;
   E_Text *text_obj;
};

static void
e_embed_text_func_show(void *_data)
{
   Embed_Private *em;
   
   em = _data;
   if (em->clip_obj) evas_show(em->evas, em->clip_obj);
   e_text_show(em->text_obj);
}

static void
e_embed_text_func_hide(void *_data)
{
   Embed_Private *em;
   
   em = _data;
   if (em->clip_obj) evas_hide(em->evas, em->clip_obj);
   e_text_hide(em->text_obj);
}

static void
e_embed_text_func_move(void *_data, double x, double y)
{
   Embed_Private *em;
   
   em = _data;
   if (em->clip_obj) evas_move(em->evas, em->clip_obj, x, y);
   e_text_move(em->text_obj, x, y);
}

static void
e_embed_text_func_resize(void *_data, double w, double h)
{
   Embed_Private *em;
   
   em = _data;
   if (em->clip_obj) evas_resize(em->evas, em->clip_obj, w, h);
   e_text_resize(em->text_obj, w, h);
}

static void
e_embed_text_func_raise(void *_data)
{
   Embed_Private *em;
   
   em = _data;
   if (em->clip_obj) evas_raise(em->evas, em->clip_obj);
   e_text_raise(em->text_obj);
}

static void
e_embed_text_func_lower(void *_data)
{
   Embed_Private *em;
   
   em = _data;
   if (em->clip_obj) evas_lower(em->evas, em->clip_obj);
   e_text_lower(em->text_obj);
}

static void
e_embed_text_func_set_layer(void *_data, int l)
{
   Embed_Private *em;
   
   em = _data;
   if (em->clip_obj) evas_set_layer(em->evas, em->clip_obj, l);
   e_text_set_layer(em->text_obj, l);
}

static void
e_embed_text_func_set_clip(void *_data, Evas_Object clip)
{
   Embed_Private *em;
   
   em = _data;
   if (em->clip_obj)
     {
	if (clip)
	  e_text_set_clip(em->clip_obj, clip);
	else
	  e_text_unset_clip(em->clip_obj);	
     }
   else
     {
	if (clip)
	  e_text_set_clip(em->text_obj, clip);
	else
	  e_text_unset_clip(em->text_obj);
     }
}

static void
e_embed_text_func_set_color_class(void *_data, char *cc, int r, int g, int b, int a)
{
   UN(_data);
   UN(cc);
   UN(r);
   UN(g);
   UN(b);
   UN(a);
}

static void
e_embed_text_func_get_min_size(void *_data, double *w, double *h)
{
   Embed_Private *em;
   
   em = _data;
   e_text_get_min_size(em->text_obj, w, h);
   if (em->clip_x) *w = 0;
   if (em->clip_y) *h = 0;
}

static void
e_embed_text_func_get_max_size(void *_data, double *w, double *h)
{
   Embed_Private *em;
   
   em = _data;
   e_text_get_max_size(em->text_obj, w, h);
   if (em->clip_x) *w = 999999999;
   if (em->clip_y) *h = 999999999;
}

/***/

Embed
e_embed_text(Ebits_Object o, char *bit_name, Evas evas, E_Text *text_obj, int clip_x, int clip_y)
{
   Embed_Private *em;
   
   em = NEW(Embed_Private, 1);
   ZERO(em, Embed_Private, 1);
   em->o = o;
   em->evas = evas;
   em->text_obj = text_obj;
   em->clip_x = clip_x;
   em->clip_y = clip_y;
   if ((clip_x) || (clip_y))
     {
	em->clip_obj = evas_add_rectangle(em->evas);
	evas_set_color(em->evas, em->clip_obj, 255, 255, 255, 255);
	e_text_set_clip(em->text_obj, em->clip_obj);
	evas_show(em->evas, em->clip_obj);
     }
   ebits_set_named_bit_replace(o, bit_name, 
			       e_embed_text_func_show,
			       e_embed_text_func_hide,
			       e_embed_text_func_move,
			       e_embed_text_func_resize,
			       e_embed_text_func_raise,
			       e_embed_text_func_lower,
			       e_embed_text_func_set_layer,
			       e_embed_text_func_set_clip,
			       e_embed_text_func_set_color_class,
			       e_embed_text_func_get_min_size,
			       e_embed_text_func_get_max_size,
			       em);   
   return em;
}

/*****************************************************************************/

static void
e_embed_image_func_show(void *_data)
{
   Embed_Private *em;
   
   em = _data;
   evas_show(em->evas, em->image_obj);
}

static void
e_embed_image_func_hide(void *_data)
{
   Embed_Private *em;
   
   em = _data;
   evas_hide(em->evas, em->image_obj);
}

static void
e_embed_image_func_move(void *_data, double x, double y)
{
   Embed_Private *em;
   
   em = _data;
   evas_move(em->evas, em->image_obj, x, y);
}

static void
e_embed_image_func_resize(void *_data, double w, double h)
{
   Embed_Private *em;
   
   em = _data;
   evas_resize(em->evas, em->image_obj, w, h);
   evas_set_image_fill(em->evas, em->image_obj, 0, 0, w, h);
}

static void
e_embed_image_func_raise(void *_data)
{
   Embed_Private *em;
   
   em = _data;
   evas_raise(em->evas, em->image_obj);
}

static void
e_embed_image_func_lower(void *_data)
{
   Embed_Private *em;
   
   em = _data;
   evas_lower(em->evas, em->image_obj);
}

static void
e_embed_image_func_set_layer(void *_data, int l)
{
   Embed_Private *em;
   
   em = _data;
   evas_set_layer(em->evas, em->image_obj, l);
}

static void
e_embed_image_func_set_clip(void *_data, Evas_Object clip)
{
   Embed_Private *em;
   
   em = _data;
   if (clip)
     evas_set_clip(em->evas, em->image_obj, clip);
   else
     evas_unset_clip(em->evas, em->image_obj);
}

static void
e_embed_image_func_set_color_class(void *_data, char *cc, int r, int g, int b, int a)
{
   Embed_Private *em;
   
   em = _data;
   if ((cc) && (!strcmp(cc, "icon")))
     evas_set_color(em->evas, em->image_obj, r, g, b, a);
}

static void
e_embed_image_func_get_min_size(void *_data, double *w, double *h)
{
   Embed_Private *em;
   int iw, ih;
   
   em = _data;
   evas_get_image_size(em->evas, em->image_obj, &iw, &ih);
   if (w) *w = iw;
   if (h) *h = ih;
}

static void
e_embed_image_func_get_max_size(void *_data, double *w, double *h)
{
   Embed_Private *em;
   int iw, ih;
   
   em = _data;
   evas_get_image_size(em->evas, em->image_obj, &iw, &ih);
   if (w) *w = iw;
   if (h) *h = ih;
}

/***/

Embed
e_embed_image_object(Ebits_Object o, char *bit_name, Evas evas, Evas_Object image_obj)
{
   Embed_Private *em;
   
   em = NEW(Embed_Private, 1);
   ZERO(em, Embed_Private, 1);
   em->o = o;
   em->evas = evas;
   em->image_obj = image_obj;
   ebits_set_named_bit_replace(o, bit_name, 
			       e_embed_image_func_show,
			       e_embed_image_func_hide,
			       e_embed_image_func_move,
			       e_embed_image_func_resize,
			       e_embed_image_func_raise,
			       e_embed_image_func_lower,
			       e_embed_image_func_set_layer,
			       e_embed_image_func_set_clip,
			       e_embed_image_func_set_color_class,
			       e_embed_image_func_get_min_size,
			       e_embed_image_func_get_max_size,
			       em);
   return em;
}

/*****/

void
e_embed_free(Embed emb)
{
   Embed_Private *em;
   
   em = emb;
   if (em->clip_obj) evas_del_object(em->evas, em->clip_obj);
   FREE(em);
}
