#include "iconbar.h"

static Evas_List           iconbars = NULL;

static E_Config_Base_Type *cf_iconbar      = NULL;
static E_Config_Base_Type *cf_iconbar_icon = NULL;

static void ib_bits_show(void *data);
static void ib_bits_hide(void *data);
static void ib_bits_move(void *data, double x, double y);
static void ib_bits_resize(void *data, double w, double h);
static void ib_bits_raise(void *data);
static void ib_bits_lower(void *data);
static void ib_bits_set_layer(void *data, int l);
static void ib_bits_set_clip(void *data, Evas_Object clip);
static void ib_bits_set_color_class(void *data, char *cc, int r, int g, int b, int a);
static void ib_bits_get_min_size(void *data, double *w, double *h);
static void ib_bits_get_max_size(void *data, double *w, double *h);

static void ib_mouse_in(void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y);
static void ib_mouse_out(void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y);
static void ib_mouse_down(void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y);
static void ib_mouse_up(void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y);

void
e_iconbar_init()
{
   cf_iconbar_icon = e_config_type_new();
   E_CONFIG_NODE(cf_iconbar_icon, "exec",  E_CFG_TYPE_STR, NULL, E_Iconbar_Icon, exec, 0, 0, "");
   E_CONFIG_NODE(cf_iconbar_icon, "image", E_CFG_TYPE_KEY, NULL, E_Iconbar_Icon, image_path, 0, 0, "");
   
   cf_iconbar = e_config_type_new();
   E_CONFIG_NODE(cf_iconbar, "icons", E_CFG_TYPE_LIST, cf_iconbar_icon, E_Iconbar, icons, 0, 0, NULL);
}

E_Iconbar *
e_iconbar_new(E_View *v)
{
   Evas_List l;
   char buf[PATH_MAX];
   E_Iconbar *ib;
   
   sprintf(buf, "%s/.e_iconbar.db", v->dir);
   
   ib = e_config_load(buf, "", cf_iconbar);
   if (!ib) return NULL;
   
   OBJ_INIT(ib, e_iconbar_free);
   ib->view = v;
   
   for (l = ib->icons; l; l = l->next)
     {
	E_Iconbar_Icon *ic;
	
	ic = l->data;
	OBJ_INIT(ic, e_iconbar_icon_free);
	ic->iconbar = ib;
     }
   sprintf(buf, "%s/.e_iconbar.bits.db", v->dir);
   
   ib->bit = ebits_load(buf);
   if (!ib->bit)
     {
	OBJ_UNREF(ib);
	return NULL;
     }
   
   return ib;
}

void
e_iconbar_free(E_Iconbar *ib)
{
   iconbars = evas_list_remove(iconbars, ib);

   ib->view->changed = 1;
   if (ib->bit) ebits_free(ib->bit);
   if (ib->icons)
     {
	Evas_List l;
  
	for (l = ib->icons; l; l = l->next)
	  {
	     E_Iconbar_Icon *ic;
	     
	     ic = l->data;
	     OBJ_UNREF(ic);
	  }
	evas_list_free(ib->icons);
     }
  FREE(ib);
}

void
e_iconbar_icon_free(E_Iconbar_Icon *ic)
{
   if (ic->image) evas_del_object(ic->iconbar->view->evas, ic->image);
   IF_FREE(ic->image_path);
   IF_FREE(ic->exec);
   FREE(ic);
}

void
e_iconbar_realize(E_Iconbar *ib)
{
   Evas_List l;
   
   for (l = ib->icons; l; l = l->next)
     {
	E_Iconbar_Icon *ic;
	char buf[PATH_MAX];
	
	ic = l->data;
	sprintf(buf, "%s/.e_iconbar.db:%s", ib->view->dir, ic->image_path);
	ic->image = evas_add_image_from_file(ib->view->evas, buf);
	evas_set_color(ib->view->evas, ic->image, 255, 255, 255, 128);
	evas_callback_add(ib->view->evas, ic->image, CALLBACK_MOUSE_IN, ib_mouse_in, ic);
	evas_callback_add(ib->view->evas, ic->image, CALLBACK_MOUSE_OUT, ib_mouse_out, ic);
	evas_callback_add(ib->view->evas, ic->image, CALLBACK_MOUSE_DOWN, ib_mouse_down, ic);
	evas_callback_add(ib->view->evas, ic->image, CALLBACK_MOUSE_UP, ib_mouse_up, ic);
     }
   ebits_add_to_evas(ib->bit, ib->view->evas);
   ebits_set_named_bit_replace(ib->bit, "Icons",
			       ib_bits_show,
			       ib_bits_hide,
			       ib_bits_move,
			       ib_bits_resize,
			       ib_bits_raise,
			       ib_bits_lower,
			       ib_bits_set_layer,
			       ib_bits_set_clip,
			       ib_bits_set_color_class,
			       ib_bits_get_min_size,
			       ib_bits_get_max_size,
			       ib);
   ebits_set_layer(ib->bit, 10000);
   e_iconbar_fix(ib);
}

void
e_iconbar_fix(E_Iconbar *ib)
{
   Evas_List l;
   double ix, iy, aw, ah;
   
   ebits_move(ib->bit, 0, 0);
   ebits_resize(ib->bit, ib->view->size.w, ib->view->size.h);
   ebits_show(ib->bit);
   ib->view->changed = 1;
   
   ix = ib->icon_area.x;
   iy = ib->icon_area.y;
   aw = ib->icon_area.w;
   ah = ib->icon_area.h;
   
   if (aw > ah) /* horizontal */
     {
     }
   else /* vertical */
     {
     }
   
   for (l = ib->icons; l; l = l->next)
     {
	E_Iconbar_Icon *ic;
	int iw, ih;
	double w, h;
	double ox, oy;
	
	ic = l->data;
	evas_get_image_size(ic->iconbar->view->evas, ic->image, &iw, &ih);
	w = iw;
	h = ih;
	ox = 0;
	oy = 0;
	if (aw > ah) /* horizontal */
	  {
	     if (h > ah)
	       {
		  w = (ah * w) / h;
		  h = ah;
	       }
	     ox = 0;
	     oy = (ah - h) / 2;
	     evas_move(ic->iconbar->view->evas, ic->image, ix + ox, iy + oy);
	     evas_resize(ic->iconbar->view->evas, ic->image, w, h);
	     ix += w;
	  }
	else /* vertical */
	  {
	     if (w > aw)
	       {
		  h = (aw * h) / w;
		  w = aw;
	       }
	     ox = (aw - w) / 2;
	     oy = 0;
	     evas_move(ic->iconbar->view->evas, ic->image, ix + ox, iy + oy);
	     evas_resize(ic->iconbar->view->evas, ic->image, w, h);
	     iy += h;
	  }
     }
}

static void
ib_bits_show(void *data)
{
   E_Iconbar *ib;
   Evas_List l;
   
   ib = (E_Iconbar *)data;
   for (l = ib->icons; l; l = l->next)
     {
	E_Iconbar_Icon *ic;
	
	ic = l->data;
	evas_show(ic->iconbar->view->evas, ic->image);
     }
}

static void
ib_bits_hide(void *data)
{
   E_Iconbar *ib;
   Evas_List l;
   
   ib = (E_Iconbar *)data;
   for (l = ib->icons; l; l = l->next)
     {
	E_Iconbar_Icon *ic;
	
	ic = l->data;
	evas_hide(ic->iconbar->view->evas, ic->image);
     }
}

static void
ib_bits_move(void *data, double x, double y)
{
   E_Iconbar *ib;
   Evas_List l;
   
   ib = (E_Iconbar *)data;
   ib->icon_area.x = x;
   ib->icon_area.y = y;
}

static void
ib_bits_resize(void *data, double w, double h)
{
   E_Iconbar *ib;
   Evas_List l;
   
   ib = (E_Iconbar *)data;
   ib->icon_area.w = w;
   ib->icon_area.h = h;
}

static void
ib_bits_raise(void *data)
{
   E_Iconbar *ib;
   Evas_List l;
   
   ib = (E_Iconbar *)data;
   for (l = ib->icons; l; l = l->next)
     {
	E_Iconbar_Icon *ic;
	
	ic = l->data;
	evas_raise(ic->iconbar->view->evas, ic->image);
     }
}

static void
ib_bits_lower(void *data)
{
   E_Iconbar *ib;
   Evas_List l;
   
   ib = (E_Iconbar *)data;
   for (l = ib->icons; l; l = l->next)
     {
	E_Iconbar_Icon *ic;
	
	ic = l->data;
	evas_lower(ic->iconbar->view->evas, ic->image);
     }
}

static void
ib_bits_set_layer(void *data, int lay)
{
   E_Iconbar *ib;
   Evas_List l;
   
   ib = (E_Iconbar *)data;
   for (l = ib->icons; l; l = l->next)
     {
	E_Iconbar_Icon *ic;
	
	ic = l->data;
	evas_set_layer(ic->iconbar->view->evas, ic->image, lay);
     }
}

static void
ib_bits_set_clip(void *data, Evas_Object clip)
{
}

static void
ib_bits_set_color_class(void *data, char *cc, int r, int g, int b, int a)
{
}

static void
ib_bits_get_min_size(void *data, double *w, double *h)
{
   *w = 0;
   *h = 0;
}

static void
ib_bits_get_max_size(void *data, double *w, double *h)
{
   *w = 999999;
   *h = 999999;
}

static void
ib_mouse_in(void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Iconbar_Icon *ic;
   
   ic = (E_Iconbar_Icon *)data;
   evas_set_color(ic->iconbar->view->evas, ic->image, 255, 255, 255, 255);
   ic->iconbar->view->changed = 1;
}

static void
ib_mouse_out(void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Iconbar_Icon *ic;
   
   ic = (E_Iconbar_Icon *)data;
   evas_set_color(ic->iconbar->view->evas, ic->image, 255, 255, 255, 128);
   ic->iconbar->view->changed = 1;
}

static void
ib_mouse_down(void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Iconbar_Icon *ic;
   
   ic = (E_Iconbar_Icon *)data;
   e_exec_run(ic->exec);
}

static void
ib_mouse_up(void *data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
}
