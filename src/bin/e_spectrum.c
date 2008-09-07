/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

Evas_Smart *_e_spectrum_smart = NULL;

typedef struct _E_Spectrum E_Spectrum;

struct _E_Spectrum
{
  Evas_Object *o_spectrum;
  Evas_Object *o_event;
  Evas_Object *o_cursor;

  int iw, ih; /* square image width/height */
  E_Color_Component mode; 

  E_Color *cv;
  Ecore_Timer *draw_timer;
};

static int _e_spectrum_redraw(void *d);

static void
_e_spectrum_smart_add(Evas_Object *o)
{
  E_Spectrum *sp;
  sp = E_NEW(E_Spectrum, 1);

  if (!sp) return;

  evas_object_smart_data_set(o, sp);

  sp->mode = E_COLOR_COMPONENT_R;

  sp->o_spectrum = evas_object_image_add(evas_object_evas_get(o));
  sp->iw = sp->ih = 255;
  evas_object_image_size_set(sp->o_spectrum, sp->iw, sp->ih);
  evas_object_image_alpha_set(sp->o_spectrum, 1);

  evas_object_smart_member_add(sp->o_spectrum, o);
}

static void
_e_spectrum_smart_del(Evas_Object *o)
{
  E_Spectrum *sp;

  sp = evas_object_smart_data_get(o);
  if (!sp) return;

  evas_object_del(sp->o_spectrum);
  evas_object_del(sp->o_event);
  evas_object_del(sp->o_cursor);

  if (sp->draw_timer) ecore_timer_del(sp->draw_timer);

  E_FREE(sp);
}

static void
_e_spectrum_smart_show(Evas_Object *o)
{
  E_Spectrum *sp;

  sp = evas_object_smart_data_get(o);
  if (!sp) return;

  evas_object_show(sp->o_spectrum);
  evas_object_show(sp->o_event);
  evas_object_show(sp->o_cursor);
}

static void
_e_spectrum_smart_hide(Evas_Object *o)
{
  E_Spectrum *sp;

  sp = evas_object_smart_data_get(o);
  if (!sp) return;

  evas_object_hide(sp->o_spectrum);
  evas_object_hide(sp->o_event);
  evas_object_hide(sp->o_cursor);
}

static void
_e_spectrum_smart_move(Evas_Object *o, Evas_Coord x, Evas_Coord y)
{
  E_Spectrum *sp;

  sp = evas_object_smart_data_get(o);
  if (!sp) return;

  evas_object_move(sp->o_spectrum, x, y);
  evas_object_move(sp->o_event, x, y);
  evas_object_move(sp->o_cursor, x, y);
}

static void
_e_spectrum_smart_resize(Evas_Object *o, Evas_Coord w, Evas_Coord h)
{
  E_Spectrum *sp;

  sp = evas_object_smart_data_get(o);
  if (!sp) return;

  evas_object_image_fill_set(sp->o_spectrum, 0, 0, w, h);

  evas_object_resize(sp->o_spectrum, w, h);
  evas_object_resize(sp->o_event, w, h);
  evas_object_resize(sp->o_cursor, w, h);
}

static void
_e_spectrum_smart_color_set(Evas_Object *o, int r, int g, int b, int a)
{
  E_Spectrum *sp;

  sp = evas_object_smart_data_get(o);
  if (!sp) return;
  evas_object_color_set(sp->o_spectrum, r, g, b, a);
  evas_object_color_set(sp->o_event, r, g, b, a);
  evas_object_color_set(sp->o_cursor, r, g, b, a);
}

static void
_e_spectrum_smart_clip_set(Evas_Object *o, Evas_Object *o_clip)
{
  E_Spectrum *sp;

  sp = evas_object_smart_data_get(o);
  if (!sp) return;
  evas_object_clip_set(sp->o_spectrum, o_clip);
  evas_object_clip_set(sp->o_event, o_clip);
  evas_object_clip_set(sp->o_cursor, o_clip);
}

static void
_e_spectrum_smart_clip_unset(Evas_Object *o)
{
  E_Spectrum *sp;

  sp = evas_object_smart_data_get(o);
  if (!sp) return;
  evas_object_clip_unset(sp->o_spectrum);
  evas_object_clip_unset(sp->o_event);
  evas_object_clip_unset(sp->o_cursor);
}

static void
_e_spectrum_smart_init(void)
{
   if (_e_spectrum_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
	     "e_spectrum",
	       EVAS_SMART_CLASS_VERSION,
	       _e_spectrum_smart_add,
	       _e_spectrum_smart_del,
	       _e_spectrum_smart_move,
	       _e_spectrum_smart_resize,
	       _e_spectrum_smart_show,
	       _e_spectrum_smart_hide,
	       _e_spectrum_smart_color_set,
	       _e_spectrum_smart_clip_set,
	       _e_spectrum_smart_clip_unset,
	       NULL,
	       NULL
	  };
        _e_spectrum_smart = evas_smart_class_new(&sc);
     }
}

void
_e_spectrum_color_calc(E_Spectrum *sp, float vx, float vy, float vz, int *r, int *g, int *b)
{
   switch (sp->mode)
     {
      case E_COLOR_COMPONENT_R:
	 *r = 255 * vz;
	 *g = 255 * vy;
	 *b = 255 * vx;
	 break;
      case E_COLOR_COMPONENT_G:
	 *r = 255 * vx;
	 *g = 255 * vz;
	 *b = 255 * vy;
	 break;
      case E_COLOR_COMPONENT_B:
	 *r = 255 * vy;
	 *g = 255 * vx;
	 *b = 255 * vz;
	 break;
      case E_COLOR_COMPONENT_H:
	 evas_color_hsv_to_rgb(vz * 360.0, vy, vx, r, g, b);
	 break;
      case E_COLOR_COMPONENT_S:
	 evas_color_hsv_to_rgb(vx * 360.0, vz, vy, r, g, b);
	 break;
      case E_COLOR_COMPONENT_V:
	 evas_color_hsv_to_rgb(vy * 360.0, vx, vz, r, g, b);
	 break;
      default:
	 break;
     }
}

void
_e_spectrum_2d_color_at(E_Spectrum *sp, int x, int y, int *r, int *g, int *b)
{
  int rr, gg, bb;
  float h, s, v;

  if (!sp || !sp->cv) return;

  switch (sp->mode)
    {
     case E_COLOR_COMPONENT_R:
	rr = sp->cv->r;
	gg = (1 - (y / (double)(sp->ih))) * 255; 
	bb = (x / (double)(sp->iw)) * 255; 
	break;
     case E_COLOR_COMPONENT_G:
	rr = (x / (double)(sp->iw)) * 255; 
	gg = sp->cv->g;
	bb = (1 - (y / (double)(sp->ih))) * 255; 
	break;
     case E_COLOR_COMPONENT_B:
	rr = (1 - (y / (double)(sp->ih))) * 255; 
	gg = (x / (double)(sp->iw)) * 255; 
	bb = sp->cv->b;
	break;
     case E_COLOR_COMPONENT_H:
	h = sp->cv->h;
	s = 1 - (y / (double)(sp->ih)); 
	v = x / (double)(sp->iw); 
	evas_color_hsv_to_rgb(h, s, v, &rr, &gg, &bb);
	break;
     case E_COLOR_COMPONENT_S:
	s = sp->cv->s;
	v = 1 - (y / (double)(sp->ih)); 
	h = x / (double)(sp->iw) * 360; 
	evas_color_hsv_to_rgb(h, s, v, &rr, &gg, &bb);
	break;
     case E_COLOR_COMPONENT_V:
	v = sp->cv->v;
	h = (1 - (y / (double)(sp->ih))) * 360; 
	s = x / (double)(sp->iw); 
	evas_color_hsv_to_rgb(h, s, v, &rr, &gg, &bb);
	break;
     case E_COLOR_COMPONENT_MAX:
	break;
    }

  if (r) *r = rr;
  if (g) *g = gg;
  if (b) *b = bb;
}

static int
_e_spectrum_redraw(void *d)
{
  E_Spectrum *sp = d;
  int *data;
  int i, j;
  int r, g, b;
  float vx, vy, vz = 0;

  data = evas_object_image_data_get(sp->o_spectrum, 1);
  if (!data) 
    {
       sp->draw_timer = NULL;
       return 0;
    }

  switch (sp->mode)
    {
     case E_COLOR_COMPONENT_R:
	vz = (float)sp->cv->r / 255;
	break;
     case E_COLOR_COMPONENT_G:
	vz = (float)sp->cv->g / 255;
	break;
     case E_COLOR_COMPONENT_B:
	vz = (float)sp->cv->b / 255;
	break;
     case E_COLOR_COMPONENT_H:
	vz = sp->cv->h / 360;
	break;
     case E_COLOR_COMPONENT_S:
	vz = sp->cv->s;
	break;
     case E_COLOR_COMPONENT_V:
	vz = sp->cv->v;
	break;
     case E_COLOR_COMPONENT_MAX:
	break;
    }

  for (i = 0; i < sp->ih; i++)
    {
       vy = (float)i / sp->ih;
       for (j = 0; j < sp->iw; j++)
	 {
	    vx = (float)j / sp->iw;
	    _e_spectrum_color_calc(sp, vx, vy, vz, &r, &g, &b);
	    data[(i * sp->iw) + j] =
	      (sp->cv->a << 24) |
	      (((r * sp->cv->a) / 255) << 16) |
	      (((g * sp->cv->a) / 255) <<  8) |
	      (((b * sp->cv->a) / 255)      );
	 }
    }

  evas_object_image_data_set(sp->o_spectrum, data);
  evas_object_image_data_update_add(sp->o_spectrum, 0, 0, sp->iw, sp->ih);
  sp->draw_timer = NULL;
  return 0;
}

static void
_e_spectrum_update(E_Spectrum *sp)
{
  if (!sp || !sp->cv) return;

  if (sp->draw_timer) ecore_timer_del(sp->draw_timer);
  sp->draw_timer = ecore_timer_add(.001, _e_spectrum_redraw, sp);
}

Evas_Object *
e_spectrum_add(Evas *e)
{
   _e_spectrum_smart_init();
   return evas_object_smart_add(e, _e_spectrum_smart);
}

void
e_spectrum_mode_set(Evas_Object *o, E_Color_Component mode)
{
  E_Spectrum *sp;

  sp = evas_object_smart_data_get(o);
  if (!sp) return;

  if (sp->mode == mode) return;
  sp->mode = mode;
  _e_spectrum_update(sp);
}

E_Color_Component
e_spectrum_mode_get(Evas_Object *o)
{
  E_Spectrum *sp;

  sp = evas_object_smart_data_get(o);
  if (!sp) return -1;

  return sp->mode;
}

void
e_spectrum_color_value_set(Evas_Object *o, E_Color *cv)
{
  E_Spectrum *sp;

  sp = evas_object_smart_data_get(o);
  if (!sp) return;

  sp->cv = cv;
  _e_spectrum_update(sp);
}

void
e_spectrum_update(Evas_Object *o)
{
  E_Spectrum *sp;

  sp = evas_object_smart_data_get(o);
  if (!sp) return;

  _e_spectrum_update(sp);
}
