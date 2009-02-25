/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void _e_border_cb_border_menu_end(void *data, E_Menu *m);
static void _e_border_menu_cb_locks(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_remember(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_border(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_close(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_iconify(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_kill(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_maximize_pre(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_maximize(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_maximize_vertically(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_maximize_horizontally(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_unmaximize(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_shade(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_icon_edit(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_application_add_pre(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_more_pre(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_prop(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_stick(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_stacking_pre(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_on_top(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_normal(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_below(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_fullscreen(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_skip_winlist(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_skip_pager(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_skip_taskbar(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_sendto_pre(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_sendto(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_pin(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_unpin(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_raise(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_lower(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_state_pre(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_skip_pre(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_fav_add(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_kbdshrtct_add(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_ibar_add_pre(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_ibar_add(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_border_pre(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_iconpref_e(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_iconpref_netwm(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_iconpref_user(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_default_icon(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_border_menu_cb_netwm_icon(void *data, E_Menu *m, E_Menu_Item *mi);

EAPI void
e_int_border_menu_create(E_Border *bd)
{
   E_Menu *m;
   E_Menu_Item *mi;

   if (bd->border_menu) return;

   m = e_menu_new();
   e_menu_category_set(m, "border");
   e_menu_category_data_set("border", bd);
   e_object_data_set(E_OBJECT(m), bd);
   bd->border_menu = m;
   e_menu_post_deactivate_callback_set(m, _e_border_cb_border_menu_end, NULL);

   if (!bd->internal)
     {
	if (bd->desktop)
	  {
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, _("Edit Icon"));
	     e_menu_item_callback_set(mi, _e_border_menu_cb_icon_edit, bd);
             e_util_desktop_menu_item_icon_add(bd->desktop, 16, mi);

	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, _("Add Application..."));
	     e_menu_item_submenu_pre_callback_set(mi, _e_border_menu_cb_application_add_pre, bd);
	     e_menu_item_icon_edje_set(mi,
	     e_theme_edje_file_get("base/theme/borders",
				   "e/widgets/border/default/application_add"),
				   "e/widgets/border/default/application_add");
	  }
	else if (bd->client.icccm.class) 
	  {
             /* icons with no class useless to borders */
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, _("Create Icon"));
	     e_menu_item_callback_set(mi, _e_border_menu_cb_icon_edit, bd);
	  }
	mi = e_menu_item_new(m);
	e_menu_item_separator_set(mi, 1);
     }

   if (!bd->sticky)
     {
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Send to Desktop"));
	e_menu_item_submenu_pre_callback_set(mi, _e_border_menu_cb_sendto_pre, bd);
	e_menu_item_icon_edje_set(mi,
				  e_theme_edje_file_get("base/theme/borders",
							"e/widgets/border/default/sendto"),
				  "e/widgets/border/default/sendto");
     }

   if ((bd->client.netwm.state.stacking != E_STACKING_BELOW) ||
       (!bd->user_skip_winlist) || (!bd->borderless))
     {
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Pin to Desktop"));
	e_menu_item_callback_set(mi, _e_border_menu_cb_pin, bd);
        e_menu_item_icon_edje_set(mi,
			          e_theme_edje_file_get("base/theme/borders",
						        "e/widgets/border/default/stick"),
			          "e/widgets/border/default/stick");
     }
   if ((bd->client.netwm.state.stacking == E_STACKING_BELOW) &&
       (bd->user_skip_winlist) && (bd->borderless))
     {
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Unpin from Desktop"));
	e_menu_item_callback_set(mi, _e_border_menu_cb_unpin, bd);
        e_menu_item_icon_edje_set(mi,
			          e_theme_edje_file_get("base/theme/borders",
						        "e/widgets/border/default/stick"),
			          "e/widgets/border/default/stick");
     }

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("More..."));
   e_menu_item_submenu_pre_callback_set(mi, _e_border_menu_cb_more_pre, bd);
   e_menu_item_icon_edje_set(mi,
			     e_theme_edje_file_get("base/theme/borders",
						   "e/widgets/border/default/basic"),
						   "e/widgets/border/default/basic");

   mi = e_menu_item_new(m);
   e_menu_item_separator_set(mi, 1);

   if (!(((bd->client.icccm.min_w == bd->client.icccm.max_w) &&
	  (bd->client.icccm.min_h == bd->client.icccm.max_h)) ||
	 (bd->lock_user_maximize)))
     {
	/* Only allow to change layer for windows in "normal" layers */
	if ((!bd->lock_user_maximize) && (!bd->shaded) &&
	    ((bd->layer == 50) || (bd->layer == 100) || (bd->layer == 150)))
	  {
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, _("Maximize"));
	     e_menu_item_submenu_pre_callback_set(mi, _e_border_menu_cb_maximize_pre, bd);
	     e_menu_item_icon_edje_set(mi,
				       e_theme_edje_file_get("base/theme/borders",
							     "e/widgets/border/default/maximize"),
				       "e/widgets/border/default/maximize");
	  }
     }

   if (!bd->internal)
     {
	if ((!bd->lock_user_iconify) && (!bd->fullscreen))
	  {
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, _("Iconify"));
	     e_menu_item_callback_set(mi, _e_border_menu_cb_iconify, bd);
	     e_menu_item_icon_edje_set(mi,
				       e_theme_edje_file_get("base/theme/borders",
							     "e/widgets/border/default/minimize"),
				       "e/widgets/border/default/minimize");
	  }
	/* Only allow to change layer for windows in "normal" layers */
	if ((!bd->lock_user_stacking) &&
	    ((bd->layer == 50) || (bd->layer == 100) || (bd->layer == 150)))
	  {
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, _("Lower"));
	     e_menu_item_callback_set(mi, _e_border_menu_cb_lower, bd);
	     e_menu_item_icon_edje_set(mi,
				       e_theme_edje_file_get("base/theme/borders",
							     "e/widgets/border/default/stack_on_top"),
				       "e/widgets/border/default/stack_below");

	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, _("Raise"));
	     e_menu_item_callback_set(mi, _e_border_menu_cb_raise, bd);
	     e_menu_item_icon_edje_set(mi,
				       e_theme_edje_file_get("base/theme/borders",
							     "e/widgets/border/default/stack_on_top"),
				       "e/widgets/border/default/stack_on_top");
	  }
     }

   mi = e_menu_item_new(m);
   e_menu_item_separator_set(mi, 1);

   if ((!bd->lock_close) && (!bd->internal))
     {
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Kill"));
	e_menu_item_callback_set(mi, _e_border_menu_cb_kill, bd);
	e_menu_item_icon_edje_set(mi,
				  e_theme_edje_file_get("base/theme/borders",
							"e/widgets/border/default/kill"),
							"e/widgets/border/default/kill");
     }

   if (!bd->lock_close)
     {
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Close"));
	e_menu_item_callback_set(mi, _e_border_menu_cb_close, bd);
	e_menu_item_icon_edje_set(mi,
				  e_theme_edje_file_get("base/theme/borders",
							"e/widgets/border/default/close"),
							"e/widgets/border/default/close");
     }
}

EAPI void
e_int_border_menu_show(E_Border *bd, Evas_Coord x, Evas_Coord y, int key, Ecore_X_Time timestamp)
{
   e_int_border_menu_create(bd);
   if (key)
     e_menu_activate_key(bd->border_menu, bd->zone, x, y, 1, 1, 
                         E_MENU_POP_DIRECTION_DOWN);
   else
     e_menu_activate_mouse(bd->border_menu, bd->zone, x, y, 1, 1,
			   E_MENU_POP_DIRECTION_DOWN, timestamp);
}

EAPI void
e_int_border_menu_del(E_Border *bd)
{
   int was_menu = 0;

   if (bd->border_menu)
     {
	e_object_del(E_OBJECT(bd->border_menu));
	bd->border_menu = NULL;
	was_menu = 1;
     }
}

static void
_e_border_cb_border_menu_end(void *data, E_Menu *m)
{
   E_Border *bd;

   bd = e_object_data_get(E_OBJECT(m));
   if (bd)
     {
	/* If the border exists, delete all associated menus */
	e_int_border_menu_del(bd);
     }
   else
     {
	/* Just delete this menu */
	e_object_del(E_OBJECT(m));
     }
}

static void
_e_border_menu_cb_locks(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = data;
   if (bd->border_locks_dialog) return;
   e_int_border_locks(bd);
}

static void
_e_border_menu_cb_remember(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = data;
   if (bd->border_remember_dialog) return;
   e_int_border_remember(bd);
}

static void
_e_border_menu_cb_border(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;
   char buf[256];

   bd = data;
   if (bd->border_border_dialog) return;
   snprintf(buf, sizeof(buf), "%p", bd);
   e_configure_registry_call("internal/borders_border", bd->zone->container, buf);
}

static void
_e_border_menu_cb_close(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = data;
   if (!bd->lock_close) e_border_act_close_begin(bd);
}

static void
_e_border_menu_cb_iconify(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = data;
   if (!bd->lock_user_iconify)
     {
	if (bd->iconic)
	  e_border_uniconify(bd);
	else
	  e_border_iconify(bd);
     }
}

static void
_e_border_menu_cb_kill(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Action *a;
   E_Border *bd;

   bd = data;
   if ((bd->lock_close) || (bd->internal)) return;

   a = e_action_find("window_kill");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
}

static void
_e_border_menu_cb_maximize_pre(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Menu *subm;
   E_Menu_Item *submi;
   E_Border *bd;

   if (!(bd = data)) return;

   subm = e_menu_new();
   e_object_data_set(E_OBJECT(subm), bd);
   e_menu_item_submenu_set(mi, subm);

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("Maximize"));
   e_menu_item_radio_set(submi, 1);
   e_menu_item_radio_group_set(submi, 3);
   e_menu_item_toggle_set(submi, (bd->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_BOTH);
   e_menu_item_callback_set(submi, _e_border_menu_cb_maximize, bd);
   e_menu_item_icon_edje_set(submi,
			     e_theme_edje_file_get("base/theme/borders",
						   "e/widgets/border/default/maximize"),
			     "e/widgets/border/default/maximize");

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("Maximize vertically"));
   e_menu_item_radio_set(submi, 1);
   e_menu_item_radio_group_set(submi, 3);
   e_menu_item_toggle_set(submi, (bd->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_VERTICAL);
   e_menu_item_callback_set(submi, _e_border_menu_cb_maximize_vertically, bd);
   e_menu_item_icon_edje_set(submi,
			     e_theme_edje_file_get("base/theme/borders",
						   "e/widgets/border/default/maximize"),
			     "e/widgets/border/default/maximize");

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("Maximize horizontally"));
   e_menu_item_radio_set(submi, 1);
   e_menu_item_radio_group_set(submi, 3);
   e_menu_item_toggle_set(submi, (bd->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_HORIZONTAL);
   e_menu_item_callback_set(submi, _e_border_menu_cb_maximize_horizontally, bd);
   e_menu_item_icon_edje_set(submi,
			     e_theme_edje_file_get("base/theme/borders",
						   "e/widgets/border/default/maximize"),
			     "e/widgets/border/default/maximize");

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("Unmaximize"));
   e_menu_item_radio_set(submi, 1);
   e_menu_item_radio_group_set(submi, 3);
   e_menu_item_toggle_set(submi, (bd->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_NONE);
   e_menu_item_callback_set(submi, _e_border_menu_cb_unmaximize, bd);
   e_menu_item_icon_edje_set(submi,
			     e_theme_edje_file_get("base/theme/borders",
						   "e/widgets/border/default/maximize"),
			     "e/widgets/border/default/maximize");
}

static void
_e_border_menu_cb_maximize(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = data;
   if (!bd->lock_user_maximize)
     e_border_maximize(bd, (e_config->maximize_policy & E_MAXIMIZE_TYPE) |
		       E_MAXIMIZE_BOTH);
}

static void
_e_border_menu_cb_maximize_vertically(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = data;
   if (!bd->lock_user_maximize)
     {
	if ((bd->maximized & E_MAXIMIZE_HORIZONTAL))
	  e_border_unmaximize(bd, E_MAXIMIZE_HORIZONTAL);
	e_border_maximize(bd, (e_config->maximize_policy & E_MAXIMIZE_TYPE) |
			  E_MAXIMIZE_VERTICAL);
     }
}

static void
_e_border_menu_cb_maximize_horizontally(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = data;
   if (!bd->lock_user_maximize)
     {
	if ((bd->maximized & E_MAXIMIZE_VERTICAL))
	  e_border_unmaximize(bd, E_MAXIMIZE_VERTICAL);
	e_border_maximize(bd, (e_config->maximize_policy & E_MAXIMIZE_TYPE) |
			  E_MAXIMIZE_HORIZONTAL);
     }
}

static void
_e_border_menu_cb_unmaximize(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = data;
   e_border_unmaximize(bd, E_MAXIMIZE_BOTH);
}

static void
_e_border_menu_cb_shade(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = data;
   if (!bd->lock_user_shade)
     {
	if (bd->shaded)
	  e_border_unshade(bd, E_DIRECTION_UP);
	else
	  e_border_shade(bd, E_DIRECTION_UP);
     }
}

static void
_e_border_menu_cb_icon_edit(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = data;
   e_desktop_border_edit(m->zone->container, bd);
}

static void
_e_border_menu_cb_application_add_pre(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Menu *subm;
   E_Menu_Item *submi;
   E_Border *bd;

   if (!(bd = data)) return;

   subm = e_menu_new();
   e_object_data_set(E_OBJECT(subm), bd);
   e_menu_item_submenu_set(mi, subm);

   if (e_configure_registry_exists("keyboard_and_mouse/key_bindings"))
     {
	submi = e_menu_item_new(subm);
	e_menu_item_label_set(submi, _("Keyboard Shortcut"));
	e_menu_item_callback_set(submi, _e_border_menu_cb_kbdshrtct_add, bd);
	e_util_menu_item_edje_icon_set(submi, "enlightenment/keys");
     }

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("To Favorites Menu"));
   e_menu_item_callback_set(submi, _e_border_menu_cb_fav_add, bd);
   e_util_menu_item_edje_icon_set(submi, "enlightenment/favorites");

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("To Launcher"));
   e_menu_item_submenu_pre_callback_set(submi, 
                                        _e_border_menu_cb_ibar_add_pre, bd);
   e_util_menu_item_edje_icon_set(submi, "enlightenment/ibar_applications");
}

static void
_e_border_menu_cb_more_pre(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Menu *subm;
   E_Menu_Item *submi;
   E_Border *bd;

   if (!(bd = data)) return;

   subm = e_menu_new();
   e_object_data_set(E_OBJECT(subm), bd);
   e_menu_item_submenu_set(mi, subm);

   if (!bd->lock_border)
     {
	submi = e_menu_item_new(subm);
	e_menu_item_label_set(submi, _("Border"));
	e_menu_item_submenu_pre_callback_set(submi, _e_border_menu_cb_border_pre, bd);
	e_menu_item_icon_edje_set(submi,
				  e_theme_edje_file_get("base/theme/borders",
							"e/widgets/border/default/borderless"),
				  "e/widgets/border/default/borderless");
     }

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("Skip"));
   e_menu_item_submenu_pre_callback_set(submi, _e_border_menu_cb_skip_pre, bd);
   e_menu_item_icon_edje_set(submi,
			     e_theme_edje_file_get("base/theme/borders",
						   "e/widgets/border/default/skip"),
			     "e/widgets/border/default/skip");

   if ((!bd->lock_user_stacking) &&
       ((bd->layer == 50) || (bd->layer == 100) || (bd->layer == 150)))
     {
	submi = e_menu_item_new(subm);
	e_menu_item_label_set(submi, _("Stacking"));
	e_menu_item_submenu_pre_callback_set(submi, _e_border_menu_cb_stacking_pre, bd);
        e_menu_item_icon_edje_set(submi,
			          e_theme_edje_file_get("base/theme/borders",
						        "e/widgets/border/default/stacking"),
			          "e/widgets/border/default/stacking");
     }

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("State"));
   e_menu_item_submenu_pre_callback_set(submi, _e_border_menu_cb_state_pre, bd);
   e_menu_item_icon_edje_set(submi,
			     e_theme_edje_file_get("base/theme/borders",
						   "e/widgets/border/default/stick"),
			     "e/widgets/border/default/stick");

   submi = e_menu_item_new(subm);
   e_menu_item_separator_set(submi, 1);

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("Remember"));
   e_menu_item_callback_set(submi, _e_border_menu_cb_remember, bd);
   e_menu_item_icon_edje_set(submi,
			     e_theme_edje_file_get("base/theme/borders",
						   "e/widgets/border/default/remember"),
			     "e/widgets/border/default/remember");

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("Window Locks"));
   e_menu_item_callback_set(submi, _e_border_menu_cb_locks, bd);
   e_menu_item_icon_edje_set(submi,
			     e_theme_edje_file_get("base/theme/borders",
						   "e/widgets/border/default/locks"),
			     "e/widgets/border/default/locks");

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("Window Properties"));
   e_menu_item_callback_set(submi, _e_border_menu_cb_prop, bd);
   e_menu_item_icon_edje_set(submi,
			     e_theme_edje_file_get("base/theme/borders",
						   "e/widgets/border/default/properties"),
			     "e/widgets/border/default/properties");
}

static void
_e_border_menu_cb_prop(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = data;
   e_int_border_prop(bd);
}

static void
_e_border_menu_cb_stick(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = data;
   if (!bd->lock_user_sticky)
     {
	if (bd->sticky)
	  e_border_unstick(bd);
	else
	  e_border_stick(bd);
     }
}

static void
_e_border_menu_cb_on_top(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = data;
   if (bd->layer != 150)
     {
	e_border_layer_set(bd, 150);
	e_hints_window_stacking_set(bd, E_STACKING_ABOVE);
     }
}

static void
_e_border_menu_cb_below(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = data;
   if (bd->layer != 50)
     {
	e_border_layer_set(bd, 50);
	e_hints_window_stacking_set(bd, E_STACKING_BELOW);
     }
}

static void
_e_border_menu_cb_normal(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = data;
   if (bd->layer != 100)
     {
	e_border_layer_set(bd, 100);
	e_hints_window_stacking_set(bd, E_STACKING_NONE);
     }
}

static void
_e_border_menu_cb_fullscreen(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;
   int toggle;

   if (!(bd = data)) return;

   if (!bd->lock_user_fullscreen)
     {
	toggle = e_menu_item_toggle_get(mi);
	if (toggle)
	  e_border_fullscreen(bd, e_config->fullscreen_policy);
	else
	  e_border_unfullscreen(bd);
     }
}

static void
_e_border_menu_cb_skip_winlist(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   if (!(bd = data)) return;

   if (((bd->client.icccm.accepts_focus) || (bd->client.icccm.take_focus)) &&
       (!bd->client.netwm.state.skip_taskbar))
     bd->user_skip_winlist = e_menu_item_toggle_get(mi);
   else
     bd->user_skip_winlist = 0;
   bd->changed = 1;
   if (bd->remember) e_remember_update(bd->remember, bd);
}

static void
_e_border_menu_cb_skip_pager(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   if (!(bd = data)) return;

   if ((bd->client.icccm.accepts_focus) || (bd->client.icccm.take_focus))
     bd->client.netwm.state.skip_pager = e_menu_item_toggle_get(mi);
   else
     bd->client.netwm.state.skip_pager = 0;
   bd->changed = 1;
   if (bd->remember) e_remember_update(bd->remember, bd);
}

static void
_e_border_menu_cb_skip_taskbar(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   if (!(bd = data)) return;

   if ((bd->client.icccm.accepts_focus) || (bd->client.icccm.take_focus))
     bd->client.netwm.state.skip_taskbar = e_menu_item_toggle_get(mi);
   else
     bd->client.netwm.state.skip_taskbar = 0;
   bd->changed = 1;
   if (bd->remember) e_remember_update(bd->remember, bd);
}

static void
_e_border_menu_cb_sendto_pre(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Menu *subm;
   E_Menu_Item *submi;
   E_Border *bd;
   E_Desk *desk_cur;
   int i;

   bd = data;
   desk_cur = e_desk_current_get(bd->zone);

   subm = e_menu_new();
   e_object_data_set(E_OBJECT(subm), bd);
   e_menu_item_submenu_set(mi, subm);

   for (i = 0; i < bd->zone->desk_x_count * bd->zone->desk_y_count; i++)
     {
	E_Desk *desk;

	desk = bd->zone->desks[i];
	submi = e_menu_item_new(subm);
	e_menu_item_label_set(submi, desk->name);
        e_menu_item_radio_set(submi, 1);
        e_menu_item_radio_group_set(submi, 2);
        e_menu_item_toggle_set(submi, (desk_cur == desk ? 1 : 0));
	e_menu_item_callback_set(submi, _e_border_menu_cb_sendto, desk);
     }
}

static void
_e_border_menu_cb_sendto(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Desk *desk;
   E_Border *bd;

   desk = data;
   bd = e_object_data_get(E_OBJECT(m));
   if ((bd) && (desk)) e_border_desk_set(bd, desk);
}

static void
_e_border_menu_cb_pin(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = e_object_data_get(E_OBJECT(m));
   if (bd) e_border_pinned_set(bd, 1);
}

static void
_e_border_menu_cb_unpin(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = e_object_data_get(E_OBJECT(m));
   if (bd) e_border_pinned_set(bd, 0);
}

static void
_e_border_menu_cb_stacking_pre(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Menu *subm;
   E_Menu_Item *submi;
   E_Border *bd;

   if (!(bd = data)) return;

   subm = e_menu_new();
   e_object_data_set(E_OBJECT(subm), bd);
   e_menu_item_submenu_set(mi, subm);

   /* Only allow to change layer for windows in "normal" layers */
   e_menu_category_set(subm,"border/stacking");
   e_menu_category_data_set("border/stacking",bd);

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("Always On Top"));
   e_menu_item_radio_set(submi, 1);
   e_menu_item_radio_group_set(submi, 2);
   e_menu_item_toggle_set(submi, (bd->layer == 150 ? 1 : 0));
   e_menu_item_callback_set(submi, _e_border_menu_cb_on_top, bd);
   e_menu_item_icon_edje_set(submi,
			     e_theme_edje_file_get("base/theme/borders",
						   "e/widgets/border/default/stack_on_top"),
			     "e/widgets/border/default/stack_on_top");

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("Normal"));
   e_menu_item_radio_set(submi, 1);
   e_menu_item_radio_group_set(submi, 2);
   e_menu_item_toggle_set(submi, (bd->layer == 100 ? 1 : 0));
   e_menu_item_callback_set(submi, _e_border_menu_cb_normal, bd);
   e_menu_item_icon_edje_set(submi,
			     e_theme_edje_file_get("base/theme/borders",
						   "e/widgets/border/default/stack_normal"),
			     "e/widgets/border/default/stack_normal");

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("Always Below"));
   e_menu_item_radio_set(submi, 1);
   e_menu_item_radio_group_set(submi, 2);
   e_menu_item_toggle_set(submi, (bd->layer == 50 ? 1 : 0));
   e_menu_item_callback_set(submi, _e_border_menu_cb_below, bd);
   e_menu_item_icon_edje_set(submi,
			     e_theme_edje_file_get("base/theme/borders",
						   "e/widgets/border/default/stack_below"),
			     "e/widgets/border/default/stack_below");
}

static void
_e_border_menu_cb_raise(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = data;
   if ((!bd->lock_user_stacking) && (!bd->internal) &&
       ((bd->layer == 50) || (bd->layer == 100) || (bd->layer == 150)))
     {
	e_border_raise(bd);
     }
}

static void
_e_border_menu_cb_lower(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = data;
   if ((!bd->lock_user_stacking) && (!bd->internal) &&
       ((bd->layer == 50) || (bd->layer == 100) || (bd->layer == 150)))
     {
	e_border_lower(bd);
     }
}

static void
_e_border_menu_cb_default_icon(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;
   Evas_Object *o;
   unsigned char prev_icon_pref;

   bd = data;
   E_OBJECT_CHECK(bd);

   o = e_icon_add(m->evas);
   prev_icon_pref = bd->icon_preference;
   bd->icon_preference = E_ICON_PREF_E_DEFAULT;
   e_icon_object_set(o, e_border_icon_add(bd, m->evas));
   bd->icon_preference = prev_icon_pref;
   mi->icon_object = o;
}

static void
_e_border_menu_cb_netwm_icon(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;
   Evas_Object *o;

   bd = data;
   E_OBJECT_CHECK(bd);

   if (bd->client.netwm.icons)
     {
	o = e_icon_add(m->evas);
	e_icon_data_set(o, bd->client.netwm.icons[0].data,
			bd->client.netwm.icons[0].width,
			bd->client.netwm.icons[0].height);
	e_icon_alpha_set(o, 1);
	mi->icon_object = o;
     }
}

static void
_e_border_menu_cb_border_pre(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Menu *subm;
   E_Menu_Item *submi;
   E_Border *bd;
   Evas *evas;

   if (!(bd = data)) return;

   subm = e_menu_new();
   e_object_data_set(E_OBJECT(subm), bd);
   e_menu_item_submenu_set(mi, subm);

   if (e_configure_registry_exists("internal/borders_border"))
     {
	submi = e_menu_item_new(subm);
	e_menu_item_label_set(submi, _("Select Border Style"));
	e_menu_item_callback_set(submi, _e_border_menu_cb_border, bd);
	e_menu_item_icon_edje_set(submi,
				  e_theme_edje_file_get("base/theme/borders",
							"e/widgets/border/default/borderless"),
				  "e/widgets/border/default/borderless");

	submi = e_menu_item_new(subm);
	e_menu_item_separator_set(submi, 1);
     }

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("Use E17 Default Icon Preference"));
   e_menu_item_radio_set(submi, 1);
   e_menu_item_radio_group_set(submi, 2);
   e_menu_item_toggle_set(submi, (bd->icon_preference == E_ICON_PREF_E_DEFAULT ? 1 : 0));
   e_menu_item_realize_callback_set(submi, _e_border_menu_cb_default_icon, bd);
   e_menu_item_callback_set(submi, _e_border_menu_cb_iconpref_e, bd);

   submi = e_menu_item_new(subm);
   evas = submi->menu->evas;
   e_menu_item_label_set(submi, _("Use Application Provided Icon "));
   e_menu_item_radio_set(submi, 1);
   e_menu_item_radio_group_set(submi, 2);
   e_menu_item_toggle_set(submi, (bd->icon_preference == E_ICON_PREF_NETWM ? 1 : 0));
   e_menu_item_realize_callback_set(submi, _e_border_menu_cb_netwm_icon, bd);
   e_menu_item_callback_set(submi, _e_border_menu_cb_iconpref_netwm, bd);

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("Use User Defined Icon"));
   e_menu_item_radio_set(submi, 1);
   e_menu_item_radio_group_set(submi, 2);
   e_menu_item_toggle_set(submi, (bd->icon_preference == E_ICON_PREF_USER ? 1 : 0));
   e_util_desktop_menu_item_icon_add(bd->desktop, 16, submi);
   e_menu_item_callback_set(submi, _e_border_menu_cb_iconpref_user, bd);
}

static void
_e_border_menu_cb_iconpref_e(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   if (!(bd = data)) return;

   bd->icon_preference = E_ICON_PREF_E_DEFAULT;
   bd->changes.icon = 1;
   bd->changed = 1;
}

static void
_e_border_menu_cb_iconpref_user(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   if (!(bd = data)) return;

   bd->icon_preference = E_ICON_PREF_USER;
   bd->changes.icon = 1;
   bd->changed = 1;
}

static void
_e_border_menu_cb_iconpref_netwm(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   if (!(bd = data)) return;

   bd->icon_preference = E_ICON_PREF_NETWM;
   bd->changes.icon = 1;
   bd->changed = 1;
}

static void
_e_border_menu_cb_state_pre(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;
   E_Menu *subm;
   E_Menu_Item *submi;

   if (!(bd = data)) return;

   subm = e_menu_new();
   e_object_data_set(E_OBJECT(subm), bd);
   e_menu_item_submenu_set(mi, subm);

   if ((!bd->lock_user_shade) && (!bd->fullscreen) && (!bd->maximized) &&
       ((!bd->client.border.name) || (strcmp("borderless", bd->client.border.name))))
     {
	submi = e_menu_item_new(subm);
	e_menu_item_label_set(submi, _("Shaded"));
	e_menu_item_check_set(submi, 1);
	e_menu_item_toggle_set(submi, (bd->shaded ? 1 : 0));
	e_menu_item_callback_set(submi, _e_border_menu_cb_shade, bd);
	e_menu_item_icon_edje_set(submi,
				  e_theme_edje_file_get("base/theme/borders",
							"e/widgets/border/default/shade"),
				  "e/widgets/border/default/shade");
     }

   if (!bd->lock_user_sticky)
     {
	submi = e_menu_item_new(subm);
	e_menu_item_label_set(submi, _("Sticky"));
	e_menu_item_check_set(submi, 1);
	e_menu_item_toggle_set(submi, (bd->sticky ? 1 : 0));
	e_menu_item_callback_set(submi, _e_border_menu_cb_stick, bd);
	e_menu_item_icon_edje_set(submi,
				  e_theme_edje_file_get("base/theme/borders",
							"e/widgets/border/default/stick"),
				  "e/widgets/border/default/stick");
     }

   if ((!bd->lock_user_fullscreen) && (!bd->shaded))
     {
	submi = e_menu_item_new(subm);
	e_menu_item_label_set(submi, _("Fullscreen"));
	e_menu_item_check_set(submi, 1);
	e_menu_item_toggle_set(submi, bd->fullscreen);
	e_menu_item_callback_set(submi, _e_border_menu_cb_fullscreen, bd);
	e_menu_item_icon_edje_set(submi,
				  e_theme_edje_file_get("base/theme/borders",
							"e/widgets/border/default/fullscreen"),
				  "e/widgets/border/default/fullscreen");
     }
}

static void
_e_border_menu_cb_skip_pre(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;
   E_Menu *subm;
   E_Menu_Item *submi;

   if (!(bd = data)) return;

   subm = e_menu_new();
   e_object_data_set(E_OBJECT(subm), bd);
   e_menu_item_submenu_set(mi, subm);

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("Window List"));
   e_menu_item_check_set(submi, 1);
   e_menu_item_toggle_set(submi, bd->user_skip_winlist);
   e_menu_item_callback_set(submi, _e_border_menu_cb_skip_winlist, bd);
   e_menu_item_icon_edje_set(submi,
			     e_theme_edje_file_get("base/theme/borders",
						   "e/widgets/border/default/skip_winlist"),
			     "e/widgets/border/default/skip_winlist");

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("Pager"));
   e_menu_item_check_set(submi, 1);
   e_menu_item_toggle_set(submi, bd->client.netwm.state.skip_pager);
   e_menu_item_callback_set(submi, _e_border_menu_cb_skip_pager, bd);
   e_menu_item_icon_edje_set(submi,
			     e_theme_edje_file_get("base/theme/borders",
						   "e/widgets/border/default/skip_pager"),
			     "e/widgets/border/default/skip_pager");

   submi = e_menu_item_new(subm);
   e_menu_item_label_set(submi, _("Taskbar"));
   e_menu_item_check_set(submi, 1);
   e_menu_item_toggle_set(submi, bd->client.netwm.state.skip_taskbar);
   e_menu_item_callback_set(submi, _e_border_menu_cb_skip_taskbar, bd);
   e_menu_item_icon_edje_set(submi,
			     e_theme_edje_file_get("base/theme/borders",
						   "e/widgets/border/default/skip_taskbar"),
			     "e/widgets/border/default/skip_taskbar");
}

static void
_e_border_menu_cb_fav_add(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;
   Efreet_Menu *menu;
   char buf[4096];

   if (!(bd = data)) return;
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/menu/favorite.menu",
	    e_user_homedir_get());
   menu = efreet_menu_parse(buf);
   if (!menu) return;
   efreet_menu_desktop_insert(menu, bd->desktop, -1);
   efreet_menu_save(menu, buf);
   efreet_menu_free(menu);
}

static void
_e_border_menu_cb_kbdshrtct_add(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;
   E_Zone *zone;

   if (!(bd = data)) return;
   zone = e_util_zone_current_get(e_manager_current_get());
   if (!zone) return;
   e_configure_registry_call("keyboard_and_mouse/key_bindings",
			     zone->container, bd->desktop->exec);
}

static void
_e_border_menu_cb_ibar_add_pre(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Menu *sm;
   E_Border *bd;
   Eina_List *dirs;
   Eina_List *l;
   char buf[4096], *file;
   const char *homedir;

   if (!(bd = data)) return;
   homedir = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar", homedir);
   dirs = ecore_file_ls(buf);
   if (!dirs) return;

   sm = e_menu_new();
   EINA_LIST_FOREACH(dirs, l, file)
     {
	E_Menu_Item *smi;

	if (file[0] == '.') continue;
	snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar/%s", homedir, file);
	if (ecore_file_is_dir(buf))
	  {
	     smi = e_menu_item_new(sm);
	     e_menu_item_label_set(smi, file);
	     e_menu_item_callback_set(smi, _e_border_menu_cb_ibar_add, file);
	  }
     }
   e_object_data_set(E_OBJECT(sm), bd);
   e_menu_item_submenu_set(mi, sm);
}

static void
_e_border_menu_cb_ibar_add(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Order *od;
   E_Border *bd;
   char buf[4096];

   bd = e_object_data_get(E_OBJECT(m));
   if ((!bd) || (!bd->desktop)) return;

   snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar/%s/.order",
	    e_user_homedir_get(), (char *)data);
   od = e_order_new(buf);
   if (!od) return;
   e_order_append(od, bd->desktop);
   e_object_del(E_OBJECT(od));
}
