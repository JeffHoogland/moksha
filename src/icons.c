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
   e_icon_set_icon(icon, PACKAGE_DATA_DIR"/data/icons/directory/default.db:/icon/selected");
}

static void
e_icon_out_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Icon *icon;
   
   icon = _data;
   e_icon_set_icon(icon, PACKAGE_DATA_DIR"/data/icons/directory/default.db:/icon/normal");
}

static void
e_icon_down_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Icon *icon;
   
   icon = _data;
   e_icon_set_icon(icon, PACKAGE_DATA_DIR"/data/icons/directory/default.db:/icon/clicked");
}

static void
e_icon_up_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Icon *icon;
   
   icon = _data;
   e_icon_set_icon(icon, PACKAGE_DATA_DIR"/data/icons/directory/default.db:/icon/selected");
}

void
e_icon_free(E_Icon *icon)
{
   IF_FREE(icon->file);
   IF_FREE(icon->current.icon);
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
   icon->obj.sel1 = evas_add_rectangle(icon->view->evas);
   icon->obj.sel2 = evas_add_rectangle(icon->view->evas);
   evas_set_layer(icon->view->evas, icon->obj.sel1, 11);
   evas_set_layer(icon->view->evas, icon->obj.sel2, 11);
   evas_set_color(icon->view->evas, icon->obj.sel1, 0, 0, 0, 0);
   evas_set_color(icon->view->evas, icon->obj.sel2, 0, 0, 0, 0);
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
   printf("e_icon_set_filename(%s)\n", file);
   icon->file = strdup(file);
   icon->changed = 1;
   if (icon->view) icon->view->changed = 1;
}

void
e_icon_update(E_Icon *icon)
{
   if (!icon->changed) return;
   if (((icon->current.icon) && (icon->previous.icon) &&
       (strcmp(icon->current.icon, icon->previous.icon))) ||
	(!icon->current.icon) || (!icon->previous.icon))
     {
	if (icon->obj.filename) evas_del_object(icon->view->evas, icon->obj.filename);
	icon->obj.filename = NULL;
	if (icon->obj.icon) evas_del_object(icon->view->evas, icon->obj.icon);
	icon->obj.icon = NULL;
     }
   if (!icon->obj.filename)
     {
	icon->obj.filename = evas_add_text(icon->view->evas, "borzoib", 8, icon->file);
	evas_set_layer(icon->view->evas, icon->obj.filename, 10);
	icon->previous.x = icon->current.x - 1;
	icon->previous.visible = icon->current.visible - 1;
     }
   if (!icon->obj.icon)
     {
	icon->obj.icon = evas_add_image_from_file(icon->view->evas, icon->current.icon);
	evas_set_layer(icon->view->evas, icon->obj.icon, 10);
	icon->previous.x = icon->current.x - 1;
	icon->previous.visible = icon->current.visible - 1;
     }
   if ((icon->previous.x != icon->current.x) ||
       (icon->previous.y != icon->current.y))
     {
	int fx, fy;
	int iw, ih;
	double tw, th;
   
	evas_get_geometry(icon->view->evas, icon->obj.filename, NULL, NULL, &tw, &th);
	evas_get_image_size(icon->view->evas, icon->obj.icon, & iw, &ih);
	fx = icon->current.x + ((iw - tw) / 2);
	fy = icon->current.y + ih;
	evas_move(icon->view->evas, icon->obj.icon, icon->current.x, icon->current.y);
	evas_move(icon->view->evas, icon->obj.filename, fx, fy);
	evas_move(icon->view->evas, icon->obj.sel1, icon->current.x, icon->current.y);
	evas_resize(icon->view->evas, icon->obj.sel1, iw, ih);
	evas_move(icon->view->evas, icon->obj.sel2, fx, fy);
	evas_resize(icon->view->evas, icon->obj.sel2, tw, th);
	evas_set_color(icon->view->evas, icon->obj.filename, 0, 0, 0, 255);
     }
   if (icon->current.visible != icon->previous.visible)
     {
	if (icon->current.visible)
	  {
	     evas_show(icon->view->evas, icon->obj.icon);
	     evas_show(icon->view->evas, icon->obj.filename);
	     evas_show(icon->view->evas, icon->obj.sel1);
	     evas_show(icon->view->evas, icon->obj.sel2);
	  }
	else
	  {
	     evas_hide(icon->view->evas, icon->obj.icon);
	     evas_hide(icon->view->evas, icon->obj.filename);
	     evas_hide(icon->view->evas, icon->obj.sel1);
	     evas_hide(icon->view->evas, icon->obj.sel2);
	  }
     }
   
   IF_FREE(icon->previous.icon);
   icon->previous = icon->current;
   if (icon->current.icon) icon->previous.icon = strdup(icon->current.icon);
   
   icon->changed = 0;
}
