#include "menu.h"
#include "border.h"
#include "desktops.h"
#include "debug.h"
#include "util.h"
#include "bordermenu.h"

static void e_bordermenu_cb_close(E_Menu *m, E_Menu_Item *mi, void  *data);
static void e_bordermenu_cb_remember_location(E_Menu *m, E_Menu_Item *mi, void  *data);

static void
e_bordermenu_cb_close(E_Menu *m, E_Menu_Item *mi, void  *data)
{
   E_Border *b;
   
   D_ENTER;
   
   b = data;

   if (b->win.client) e_icccm_delete(b->win.client);
   
   D_RETURN;
   UN(m);
   UN(mi);
}

static void
e_bordermenu_cb_remember_location(E_Menu *m, E_Menu_Item *mi, void  *data)
{
   E_Border *b;
   
   D_ENTER;
   
   b = data;
   
   if (mi->on) e_menu_item_set_state(mi, 0);
   else        e_menu_item_set_state(mi, 1);
   e_menu_set_state(m, mi);

   b->client.matched.matched = 1;
   b->client.matched.location.matched = mi->on;
   
   D_RETURN;
   UN(m);
}

void
e_bordermenu_do(E_Border *b)
{
   E_Menu *menu;
   E_Menu_Item *menuitem;
   
   D_ENTER;
   if (!b->menus)
     {
	menu = e_menu_new();
	b->menus = evas_list_append(b->menus, menu);
	e_menu_set_padding_icon(menu, 2);
	e_menu_set_padding_state(menu, 2);
	
	menuitem = e_menu_item_new("Close");
	/* e_menu_item_set_icon(menuitem, icon);   */
	/* e_menu_item_set_scale_icon(menuitem, 1);*/
	/* e_menu_item_set_separator(menuitem, 1);*/
	e_menu_item_set_callback(menuitem, e_bordermenu_cb_close, b);
	e_menu_add_item(menu, menuitem);
	
	menuitem = e_menu_item_new("");
	e_menu_item_set_separator(menuitem, 1);
	e_menu_item_set_check(menuitem, 1);
	e_menu_item_set_state(menuitem, b->client.matched.location.matched);
	e_menu_item_set_callback(menuitem, e_bordermenu_cb_remember_location, b);
	e_menu_add_item(menu, menuitem);
	
	menuitem = e_menu_item_new("Remember Location");
	e_menu_item_set_check(menuitem, 1);
	e_menu_item_set_state(menuitem, b->client.matched.location.matched);	
	e_menu_item_set_callback(menuitem, e_bordermenu_cb_remember_location, b);
	e_menu_add_item(menu, menuitem);
	e_menu_set_state(menu, menuitem);
     }

     {
	int pl, pr, pt, pb;
	int crx, cry, crw, crh;
	int mx, my;
	
	menu = b->menus->data;
	pl = pr = pt = pb = 0;
	if (b->bits.t) ebits_get_insets(b->bits.t, &pl, &pr, &pt, &pb);	
	crx = b->current.x + pl;
	cry = b->current.y + pt;
	crw = b->client.w;
	crh = b->client.h;
	ecore_pointer_xy_get(&mx, &my);
	if (mx + menu->current.w > crx + crw) mx = crx + crw - menu->current.w;
	if (my + menu->current.h > cry + crh) my = cry + crh - menu->current.h;
	if (mx < crx) mx = crx;
	if (my < cry) my = cry;
	e_menu_show_at_mouse(menu, mx, my, CurrentTime);
     }
   
   D_RETURN;
}
