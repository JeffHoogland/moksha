#include "e.h"

static void e_cb_mouse_in(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh);
static void e_cb_mouse_out(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh);
static void e_cb_mouse_down(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh);
static void e_cb_mouse_up(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh);
static void e_cb_mouse_move(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh);

static int mv_prev_x, mv_prev_y;

static void
e_cb_mouse_in(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh)
{
   E_Shelf *sh;
   
   sh = data;
   return;
   UN(o);
   UN(bt);
   UN(ox);
   UN(oy);
   UN(ow);
   UN(oh);
}

static void
e_cb_mouse_out(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh)
{
   E_Shelf *sh;
   
   sh = data;
   return;
   UN(o);
   UN(bt);
   UN(ox);
   UN(oy);
   UN(ow);
   UN(oh);
}

static void
e_cb_mouse_down(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh)
{
   E_Shelf *sh;
   
   sh = data;
   if (!strcmp(class, "Title_Bar"))
     {
	mv_prev_x = x;
	mv_prev_y = y;
	sh->state.moving = 1;
     }
   if (!strcmp(class, "Resize"))
     {
	mv_prev_x = x;
	mv_prev_y = y;
	sh->state.resizing = 1;
     }
   if (!strcmp(class, "Resize_Horizontal"))
     {
	mv_prev_x = x;
	mv_prev_y = y;
	sh->state.resizing = 1;
     }
   if (!strcmp(class, "Resize_Vertical"))
     {
	mv_prev_x = x;
	mv_prev_y = y;
	sh->state.resizing = 1;
     }
   if (!strcmp(class, "Menu"))
     {
     }
   if (!strcmp(class, "Close"))
     {
     }
   return;
   UN(o);
   UN(bt);
   UN(ox);
   UN(oy);
   UN(ow);
   UN(oh);
}

static void
e_cb_mouse_up(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh)
{
   E_Shelf *sh;
   
   sh = data;
   if (sh->state.moving) sh->state.moving = 0;
   if (sh->state.resizing) sh->state.resizing = 0;
   return;
   UN(o);
   UN(bt);
   UN(ox);
   UN(oy);
   UN(ow);
   UN(oh);
}

static void
e_cb_mouse_move(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh)
{
   E_Shelf *sh;
   
   sh = data;
   if (sh->state.moving)
     {
	e_shelf_move_by(sh, x - mv_prev_x, y - mv_prev_y);
	mv_prev_x = x;
	mv_prev_y = y;
     }
   if (sh->state.resizing)
     {
	if (sh->state.resizing == 1)
	  {
	     e_shelf_resize_by(sh, x - mv_prev_x, y - mv_prev_y);
	     mv_prev_x = x;
	     mv_prev_y = y;	     
	  }
     }
   return;
   UN(o);
   UN(bt);
   UN(ox);
   UN(oy);
   UN(ow);
   UN(oh);
}

void
e_shelf_free(E_Shelf *sh)
{
   IF_FREE(sh->name);
   FREE(sh);
}

E_Shelf *
e_shelf_new(void)
{
   E_Shelf *sh;

   sh = NEW(E_Shelf, 1);
   ZERO(sh, E_Shelf, 1);
   OBJ_INIT(sh, e_shelf_free);
   return sh;
}

void
e_shelf_set_name(E_Shelf *sh, char *name)
{
   IF_FREE(sh->name);
   sh->name = strdup(name);
}

void
e_shelf_set_view(E_Shelf *sh, E_View *v)
{
   sh->view = v;
}

void
e_shelf_realize(E_Shelf *sh)
{
   int pl, pr, pt, pb;
   
   sh->bit.border = ebits_load(PACKAGE_DATA_DIR"/data/config/appearance/default/shelves/default.bits.db");
   if (sh->bit.border)
     {
	ebits_add_to_evas(sh->bit.border, sh->view->evas);
	ebits_move(sh->bit.border, sh->x, sh->y);
	ebits_resize(sh->bit.border, sh->w, sh->h);
	ebits_set_layer(sh->bit.border, 9);
	if (sh->visible)
	  ebits_show(sh->bit.border);

#define HOOK_CB(_class)	\
ebits_set_bit_callback(sh->bit.border, _class, CALLBACK_MOUSE_IN, e_cb_mouse_in, sh); \
ebits_set_bit_callback(sh->bit.border, _class, CALLBACK_MOUSE_OUT, e_cb_mouse_out, sh); \
ebits_set_bit_callback(sh->bit.border, _class, CALLBACK_MOUSE_DOWN, e_cb_mouse_down, sh); \
ebits_set_bit_callback(sh->bit.border, _class, CALLBACK_MOUSE_UP, e_cb_mouse_up, sh); \
ebits_set_bit_callback(sh->bit.border, _class, CALLBACK_MOUSE_MOVE, e_cb_mouse_move, sh);
	HOOK_CB("Title_Bar");
	HOOK_CB("Resize");
	HOOK_CB("Resize_Horizontal");
	HOOK_CB("Resize_Vertical");
	HOOK_CB("Close");
	HOOK_CB("Iconify");
	HOOK_CB("Max_Size");
	HOOK_CB("Menu");
     }
   sh->obj.clipper = evas_add_rectangle(sh->view->evas);
   evas_set_layer(sh->view->evas, sh->obj.clipper, 9);
   evas_set_color(sh->view->evas, sh->obj.clipper, 255, 255, 255, 255);
	
   pl = pr = pt = pb = 0;
   if (sh->bit.border) ebits_get_insets(sh->bit.border, &pl, &pr, &pt, &pb);
   evas_move(sh->view->evas, sh->obj.clipper, sh->x + pl, sh->y + pt);
   evas_resize(sh->view->evas, sh->obj.clipper, sh->w - pl - pr, sh->h - pt - pb);
   
   if ((sh->visible) && (sh->icon_count > 0))
     evas_show(sh->view->evas, sh->obj.clipper);
}

void
e_shelf_show(E_Shelf *sh)
{
   if (sh->visible) return;
   
   sh->visible = 1;
}

void
e_shelf_hide(E_Shelf *sh)
{
   if (!sh->visible) return;

   sh->visible = 0;
}

void
e_shelf_move(E_Shelf *sh, int x, int y)
{
   int dx, dy;
   
   dx = x - sh->x;
   dy = y - sh->y;
   e_shelf_move_by(sh, dx, dy);
}

void
e_shelf_move_by(E_Shelf *sh, int dx, int dy)
{
   Evas_List l;
   
   sh->x += dx;
   sh->y += dy;
   if (sh->bit.border) ebits_move(sh->bit.border, sh->x, sh->y);
   if (sh->obj.clipper) 
     {
	int pl, pr, pt, pb;
	
	pl = pr = pt = pb = 0;
	if (sh->bit.border) ebits_get_insets(sh->bit.border, &pl, &pr, &pt, &pb);
	evas_move(sh->view->evas, sh->obj.clipper, sh->x + pl, sh->y + pt);
     }
   return;
   for (l = sh->view->icons; l; l = l->next)
     {
	E_Icon *icon;
	int x, y;
	
	icon = l->data;
	if (icon->shelf == sh)
	  {
	     e_icon_get_xy(icon, &x, &y);
	     e_icon_set_xy(icon, x + dx, y + dy);
	  }
     }
}

void
e_shelf_resize(E_Shelf *sh, int w, int h)
{
   sh->w = w;
   sh->h = h;
   if (sh->bit.border) ebits_resize(sh->bit.border, sh->w, sh->h);
   if (sh->obj.clipper) 
     {
	int pl, pr, pt, pb;
	
	pl = pr = pt = pb = 0;
	if (sh->bit.border) ebits_get_insets(sh->bit.border, &pl, &pr, &pt, &pb);
	evas_resize(sh->view->evas, sh->obj.clipper, sh->w - pl - pr, sh->h - pt - pb);
     }   
}

void
e_shelf_resize_by(E_Shelf *sh, int dw, int dh)
{
   e_shelf_resize(sh, sh->w + dw, sh->h + dh);
}

void
e_shelf_add_icon(E_Shelf *sh, E_Icon *icon)
{
   if (icon->shelf)
     e_shelf_del_icon(icon->shelf, icon);
   icon->shelf = sh;
   sh->icon_count++;
   if (sh->icon_count > 0)
     evas_show(sh->view->evas, sh->obj.clipper);
   evas_set_clip(sh->view->evas, icon->obj.icon, sh->obj.clipper);
   evas_set_clip(sh->view->evas, icon->obj.filename, sh->obj.clipper);
   evas_set_clip(sh->view->evas, icon->obj.sel1, sh->obj.clipper);
   evas_set_clip(sh->view->evas, icon->obj.sel2, sh->obj.clipper);
}

void
e_shelf_del_icon(E_Shelf *sh, E_Icon *icon)
{
   if (icon->shelf != sh) return;
   icon->shelf = NULL;
   if (sh->icon_count <= 0)
     evas_hide(sh->view->evas, sh->obj.clipper);
   evas_unset_clip(sh->view->evas, icon->obj.icon);
   evas_unset_clip(sh->view->evas, icon->obj.filename);
   evas_unset_clip(sh->view->evas, icon->obj.sel1);
   evas_unset_clip(sh->view->evas, icon->obj.sel2);
}
