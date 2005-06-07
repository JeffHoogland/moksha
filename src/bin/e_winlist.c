/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */

/* local subsystem globals */
static E_Popup *winlist = NULL;

/* externally accessible functions */
int
e_winlist_init(void)
{
   return 1;
}

int
e_winlist_shutdown(void)
{
   e_winlist_hide();
   return 1;
}

/*
 * how to handle? on show, grab keyboard (and mouse) like menus
 * set "modifier keys active" if spawning event had modfiers active
 * if "modifier keys active" and if all modifier keys are released or is found not active on start = end
 * up/left == prev
 * down/right == next
 * escape = end
 * 1 - 9, 0 = select window 1 - 9, 10
 * local subsystem functions
 */

void
e_winlist_show(E_Zone *zone)
{
   int x, y, w, h;
   Evas_Object *o;
   
   if (winlist) return;

   /* FIXME: should be config */
   w = zone->w / 2;
   if (w > 400) w = 400;
   h = zone->h / 2;
   if (h > 800) h = 800;
   else if (h < 400) h = 400;
   
   winlist = e_popup_new(zone, x, y, w, h); 
   if (!winlist) return;
   e_popup_layer_set(winlist, 255);
   o = edje_object_add(winlist->evas);
   /* FIXME: need theme stuff */
   e_theme_edje_object_set(o, "base/theme/winlist",
			   "widgets/winlist/main");
   evas_object_move(o, 0, 0);
   evas_object_resize(o, w, h);
   evas_object_show(o);
   e_popup_edje_bg_object_set(winlist, o);
   /* FIXME: bg obj needs to be stored */
   /* FIXME: create and swallow box */
   /* FIXME: fill box with current clients */
   /* FIXME: configure list with current focused window */
   /* FIXME: grab mouse and keyboard */
   
   e_popup_show(winlist);
}

void
e_winlist_hide(void)
{
   if (!winlist) return;
   e_popup_hide(winlist);
   e_object_del(E_OBJECT(winlist));
   winlist = NULL;
}

void
e_winlist_next(void)
{
}

void
e_winlist_prev(void)
{
}

void
e_winlist_modifiers_set(int mod)
{
}
