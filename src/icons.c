#include "e.h"

static void e_icon_in_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y);
static void e_icon_out_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y);
static void e_icon_down_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y);
static void e_icon_up_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y);

static void
e_icon_in_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Icon *icon;
   
   icon = _data;
   icon->current.state.hilited = 1;
   icon->changed = 1;
   icon->view->changed = 1;
}

static void
e_icon_out_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Icon *icon;
   
   icon = _data;
   icon->current.state.hilited = 0;
   icon->changed = 1;
   icon->view->changed = 1;
}

static void
e_icon_down_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Icon *icon;
   
   icon = _data;
   icon->current.state.clicked = 1;
   icon->changed = 1;
   icon->view->changed = 1;
}

static void
e_icon_up_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Icon *icon;
   
   icon = _data;
   icon->current.state.clicked = 0;
   if (icon->current.state.selected)
     icon->current.state.selected = 0;
   else
     icon->current.state.selected = 1;
   icon->changed = 1;
   icon->view->changed = 1;
}

void
e_icon_free(E_Icon *icon)
{
   IF_FREE(icon->file);
   IF_FREE(icon->info.icon.normal);
   IF_FREE(icon->info.icon.selected);
   IF_FREE(icon->info.icon.clicked);
   IF_FREE(icon->info.link);
   IF_FREE(icon->info.mime.base);
   IF_FREE(icon->info.mime.type);
   IF_FREE(icon->previous.icon);
   FREE(icon);
}

E_Icon *
e_icon_new(void)
{
   E_Icon *icon;

   icon = NEW(E_Icon, 1);
   ZERO(icon, E_Icon, 1);
   OBJ_INIT(icon, e_icon_free);
   icon->info.icon.normal = strdup(PACKAGE_DATA_DIR"/data/icons/file/default.db:/icon/normal");   
   icon->previous.state.clicked = -1;
   return icon;
}

void
e_icon_place_grid(E_Icon *icon)
{
   int x, y;
   
   if (icon->view->options.arrange.grid.dir == 0) /* h */
     {
	int gw;
	
	if (icon->view->options.arrange.grid.w > 0)
	  gw = icon->view->size.w / icon->view->options.arrange.grid.w;
	else gw = 1;
	y = icon->view->options.arrange.grid.next_pos / gw;
	x = icon->view->options.arrange.grid.next_pos - (y * gw);
	x *= icon->view->options.arrange.grid.w;
	y *= icon->view->options.arrange.grid.h;
	x += (icon->view->options.arrange.grid.w - icon->current.w) / 2;
	y += (icon->view->options.arrange.grid.h - icon->current.h);
	e_icon_set_xy(icon, x, y);
	icon->view->options.arrange.grid.next_pos++;
     }
   else /* v */
     {
     }
}

void
e_icon_pre_show(E_Icon *icon)
{
   int x, y;
   
   if (icon->info.ready) return;
   e_icon_update(icon);
   icon->info.ready = 1;
   if (icon->info.coord.have)
     {
	x = icon->info.coord.x;
	y = icon->info.coord.y;
	e_icon_set_xy(icon, x, y);
     }
   else
     {
	if (icon->view->options.arrange.method == 0) /* grid */
	  {
	     /* need to redo whole grid... */
	     if ((icon->current.w > icon->view->options.arrange.grid.w) ||
		 (icon->current.h > icon->view->options.arrange.grid.h))
	       {		  
		  Evas_List l;
		  
		  icon->view->options.arrange.grid.next_pos = 0;
		  icon->view->options.arrange.grid.w = icon->current.w;
		  icon->view->options.arrange.grid.h = icon->current.h;
		  for (l = icon->view->icons; l; l = l->next)
		    {
		       E_Icon *ic;
		       
		       ic = l->data;
		       if (ic->info.ready)
			 e_icon_place_grid(ic);
		    }
	       }
	     else
	       e_icon_place_grid(icon);
	  }
     }
   e_icon_show(icon);   
}

void
e_icon_calulcate_geometry(E_Icon *icon)
{
   int iw, ih, tw, th;
   double dtw, dth;
   
   if (!icon->view) return;
   dtw = 0; dth = 0; iw = 0; ih = 0;
   evas_get_geometry(icon->view->evas, icon->obj.filename, NULL, NULL, &dtw, &dth);
   tw = (int)dtw; 
   th = (int)dth;
   evas_get_image_size(icon->view->evas, icon->obj.icon, &iw, &ih);
   if (tw < iw)
     {
	icon->current.ix = icon->current.x;
	icon->current.iy = icon->current.y;
	icon->current.tx = icon->current.x + ((iw - tw) / 2);
	icon->current.ty = icon->current.y + ih;
	icon->current.w = iw;
	icon->current.h = ih + th;
	icon->current.iw = iw;
	icon->current.ih = ih;
	icon->current.tw = tw;
	icon->current.th = th;
     }
   else
     {
	icon->current.ix = icon->current.x + ((tw - iw) / 2);
	icon->current.iy = icon->current.y;
	icon->current.tx = icon->current.x;
	icon->current.ty = icon->current.y + ih;
	icon->current.w = tw;
	icon->current.h = ih + th;
	icon->current.iw = iw;
	icon->current.ih = ih;
	icon->current.tw = tw;
	icon->current.th = th;
     }
   if (INTERSECTS(0, 0, icon->view->size.w, icon->view->size.h,
		  icon->current.x, icon->current.y, icon->current.w, icon->current.h))
     icon->current.viewable = 1;
   else
     icon->current.viewable = 0;
}

void
e_icon_realize(E_Icon *icon)
{
   icon->obj.sel1 = evas_add_rectangle(icon->view->evas);
   icon->obj.sel2 = evas_add_rectangle(icon->view->evas);
   evas_set_color(icon->view->evas, icon->obj.sel1, 0, 0, 0, 0);
   evas_set_color(icon->view->evas, icon->obj.sel2, 0, 0, 0, 0);
   evas_set_layer(icon->view->evas, icon->obj.sel1, 11);
   evas_set_layer(icon->view->evas, icon->obj.sel2, 11);
   evas_callback_add(icon->view->evas, icon->obj.sel1, CALLBACK_MOUSE_IN, e_icon_in_cb, icon);
   evas_callback_add(icon->view->evas, icon->obj.sel1, CALLBACK_MOUSE_OUT, e_icon_out_cb, icon);
   evas_callback_add(icon->view->evas, icon->obj.sel1, CALLBACK_MOUSE_DOWN, e_icon_down_cb, icon);
   evas_callback_add(icon->view->evas, icon->obj.sel1, CALLBACK_MOUSE_UP, e_icon_up_cb, icon);
   evas_callback_add(icon->view->evas, icon->obj.sel2, CALLBACK_MOUSE_IN, e_icon_in_cb, icon);
   evas_callback_add(icon->view->evas, icon->obj.sel2, CALLBACK_MOUSE_OUT, e_icon_out_cb, icon);
   evas_callback_add(icon->view->evas, icon->obj.sel2, CALLBACK_MOUSE_DOWN, e_icon_down_cb, icon);
   evas_callback_add(icon->view->evas, icon->obj.sel2, CALLBACK_MOUSE_UP, e_icon_up_cb, icon);
}
     
void
e_icon_unrealize(E_Icon *icon)
{
   if (icon->obj.icon) evas_del_object(icon->view->evas, icon->obj.icon);
   if (icon->obj.filename) evas_del_object(icon->view->evas, icon->obj.filename);
   if (icon->obj.sel1) evas_del_object(icon->view->evas, icon->obj.sel1);
   if (icon->obj.sel2) evas_del_object(icon->view->evas, icon->obj.sel2);   
   icon->obj.icon = NULL;
   icon->obj.filename = NULL;
   icon->obj.sel1 = NULL;
   icon->obj.sel2 = NULL;
   if (icon->view) icon->view->changed = 1;
}

void
e_icon_set_icon(E_Icon *icon, char *file)
{
   IF_FREE(icon->current.icon);
   icon->current.icon = strdup(file);
   icon->changed = 1;
   if (icon->view) icon->view->changed = 1;
}

void
e_icon_show(E_Icon *icon)
{
   if (!icon->current.visible)
     {
	icon->current.visible = 1;
	icon->changed = 1;
	if (icon->view) icon->view->changed = 1;
     }
}

void
e_icon_hide(E_Icon *icon)
{
   if (icon->current.visible)
     {
	icon->current.visible = 0;
	icon->changed = 1;
	if (icon->view) icon->view->changed = 1;
     }
}

void
e_icon_set_xy(E_Icon *icon, int x, int y)
{
   icon->current.x = x;
   icon->current.y = y;
   icon->changed = 1;
   if (icon->view) icon->view->changed = 1;
}

void
e_icon_get_xy(E_Icon *icon, int *x, int *y)
{
   if (x) *x = icon->current.x;
   if (y) *y = icon->current.y;
}

void
e_icon_set_filename(E_Icon *icon, char *file)
{
   IF_FREE(icon->file);
   icon->file = strdup(file);
   icon->changed = 1;
   if (icon->view) icon->view->changed = 1;
}

void
e_icon_update(E_Icon *icon)
{
   int obj_new = 0;
   
   if (!icon->changed) return;
   if (icon->current.state.clicked)
     {
	if (icon->info.icon.clicked)
	  icon->current.icon = icon->info.icon.clicked;
	else if (icon->info.icon.selected)
	  icon->current.icon = icon->info.icon.selected;
	else 
	  icon->current.icon = icon->info.icon.normal;
     }
   else if (icon->current.state.selected)
     {
	     if (icon->info.icon.selected)
	  icon->current.icon = icon->info.icon.selected;
	else 
	  icon->current.icon = icon->info.icon.normal;
     }
   else
     {
	icon->current.icon = icon->info.icon.normal;	     
     }
   if ((!icon->current.state.selected) && (icon->obj.sel_icon))
     {
	ebits_hide(icon->obj.sel_icon);
	ebits_free(icon->obj.sel_icon);
	icon->obj.sel_icon = NULL;
     }
   if (!icon->obj.filename)
     {
	icon->obj.filename = evas_add_text(icon->view->evas, "borzoib", 8, icon->file);
	evas_set_layer(icon->view->evas, icon->obj.filename, 10);
	icon->previous.x = icon->current.x - 1;
	icon->previous.visible = icon->current.visible - 1;
	obj_new = 1;
     }
   if (((icon->previous.icon) && (icon->current.icon) && 
	(strcmp(icon->current.icon, icon->previous.icon))) ||
       ((!icon->previous.icon) && (icon->current.icon)))
     {
	int iw, ih;
	
	if (!icon->obj.icon)
	  {
	     icon->obj.icon = evas_add_image_from_file(icon->view->evas, icon->current.icon);
	     evas_set_layer(icon->view->evas, icon->obj.icon, 10);
	     obj_new = 1;
	  }
	else
	  evas_set_image_file(icon->view->evas, icon->obj.icon, icon->current.icon);
	evas_get_image_size(icon->view->evas, icon->obj.icon, &iw, &ih);
	evas_set_image_fill(icon->view->evas, icon->obj.icon, 0, 0, iw, ih);
	evas_resize(icon->view->evas, icon->obj.icon, iw, ih);
	icon->previous.x = icon->current.x - 1;	     
     }
   if ((!icon->obj.sel_icon) && (icon->current.state.selected))
     {
	icon->obj.sel_icon = ebits_load(PACKAGE_DATA_DIR"/data/config/appearance/default/selections/file.bits.db");
	if (icon->obj.sel_icon)
	  {
	     ebits_add_to_evas(icon->obj.sel_icon, icon->view->evas);	     
	     ebits_set_layer(icon->obj.sel_icon, 9);
	     ebits_set_color_class(icon->obj.sel_icon, "Selected BG", 100, 200, 255, 255);
	     obj_new = 1;
	  }
     }
   if (obj_new)
     {
	if (icon->shelf)
	  {
	     E_Shelf *sh;
	     
	     sh = icon->shelf;
	     e_shelf_del_icon(sh, icon);
	     e_shelf_add_icon(sh, icon);
	  }
     }
   if ((icon->previous.x != icon->current.x) ||
       (icon->previous.y != icon->current.y) ||
       (icon->current.visible != icon->previous.visible) ||
       (obj_new))
     {
	e_icon_calulcate_geometry(icon);
	if (icon->current.viewable)
	  {
	     evas_move(icon->view->evas, icon->obj.icon, icon->current.ix, icon->current.iy);
	     evas_move(icon->view->evas, icon->obj.filename, icon->current.tx, icon->current.ty);
	     evas_move(icon->view->evas, icon->obj.sel1, icon->current.ix, icon->current.iy);
	     evas_resize(icon->view->evas, icon->obj.sel1, icon->current.iw, icon->current.ih);
	     evas_move(icon->view->evas, icon->obj.sel2, icon->current.tx, icon->current.ty);
	     evas_resize(icon->view->evas, icon->obj.sel2, icon->current.tw, icon->current.th);
	     evas_set_color(icon->view->evas, icon->obj.filename, 0, 0, 0, 255);
	     if (icon->obj.sel_icon)
	       {
		  int pl, pr, pt, pb;
		  
		  pl = pr = pt = pb = 0;
		  ebits_get_insets(icon->obj.sel_icon, &pl, &pr, &pt, &pb);
		  ebits_move(icon->obj.sel_icon, icon->current.ix - pl, icon->current.iy - pt);
		  ebits_resize(icon->obj.sel_icon, icon->current.iw + pl + pr, icon->current.ih + pt + pb);
	       }
	  }	
     }
   if ((icon->current.visible != icon->previous.visible) || (obj_new) ||
       (icon->current.viewable != icon->previous.viewable))
     {
	if ((icon->current.visible) && (icon->current.viewable))
	  {
	     evas_show(icon->view->evas, icon->obj.icon);
	     evas_show(icon->view->evas, icon->obj.filename);
	     evas_show(icon->view->evas, icon->obj.sel1);
	     evas_show(icon->view->evas, icon->obj.sel2);
	     if (icon->obj.sel_icon) ebits_show(icon->obj.sel_icon);
	  }
	else
	  {
	     evas_hide(icon->view->evas, icon->obj.icon);
	     evas_hide(icon->view->evas, icon->obj.filename);
	     evas_hide(icon->view->evas, icon->obj.sel1);
	     evas_hide(icon->view->evas, icon->obj.sel2);
	     if (icon->obj.sel_icon) ebits_hide(icon->obj.sel_icon);
	  }
     }
   IF_FREE(icon->previous.icon);
   icon->previous = icon->current;
   icon->previous.icon = strdup(icon->current.icon);
   icon->changed = 0;
}
