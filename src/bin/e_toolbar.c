#include "e.h"

/* local function protos */
static void _e_toolbar_free(E_Toolbar *tbar);
static void _e_toolbar_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_toolbar_menu_cb_post(void *data, E_Menu *mn);
static void _e_toolbar_menu_cb_pre(void *data, E_Menu *mn);
static void _e_toolbar_menu_append(E_Toolbar *tbar, E_Menu *mn);
static void _e_toolbar_menu_cb_edit(void *data, E_Menu *mn, E_Menu_Item *mi);
static void _e_toolbar_menu_cb_config(void *data, E_Menu *mn, E_Menu_Item *mi);
static void _e_toolbar_menu_cb_contents(void *data, E_Menu *mn, E_Menu_Item *mi);
static void _e_toolbar_gadcon_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h);
static const char *_e_toolbar_orient_string_get(E_Toolbar *tbar);
static void _e_toolbar_fm2_changed(void *data, Evas_Object *obj, void *event_info);
static void _e_toolbar_fm2_dir_changed(void *data, Evas_Object *obj, void *event_info);
static void _e_toolbar_fm2_dir_deleted(void *data, Evas_Object *obj, void *event_info);
static void _e_toolbar_fm2_files_deleted(void *data, Evas_Object *obj, void *event_info);
static void _e_toolbar_fm2_selected(void *data, Evas_Object *obj, void *event_info);
static void _e_toolbar_fm2_selection_changed(void *data, Evas_Object *obj, void *event_info);
static void _e_toolbar_menu_items_append(void *data, E_Gadcon_Client *gcc, E_Menu *mn);

/* local vars */
static Eina_List *toolbars = NULL;

EAPI int 
e_toolbar_init(void) 
{
   return 1;
}

EAPI int 
e_toolbar_shutdown(void) 
{
   while (toolbars) 
     {
	E_Toolbar *tbar;

	tbar = toolbars->data;
	e_object_del(E_OBJECT(tbar));
     }
   return 1;
}

EAPI E_Toolbar *
e_toolbar_new(Evas *evas, const char *name, E_Win *fwin, Evas_Object *fm2) 
{
   E_Toolbar *tbar = NULL;

   if (!name) return NULL;
   if ((!fwin) || (!fm2)) return NULL;

   tbar = E_OBJECT_ALLOC(E_Toolbar, E_TOOLBAR_TYPE, _e_toolbar_free);
   if (!tbar) return NULL;
   tbar->id = 1;
//   tbar->id = eina_list_count(toolbars) + 1;
   tbar->x = tbar->y = 0;
   tbar->h = 32;
   tbar->w = fwin->w;
   tbar->evas = evas;
   tbar->name = eina_stringshare_add(name);
   tbar->fwin = fwin;
   tbar->fm2 = fm2;

   evas_object_smart_callback_add(tbar->fm2, "changed", 
				  _e_toolbar_fm2_changed, tbar);
   evas_object_smart_callback_add(tbar->fm2, "dir_changed", 
				  _e_toolbar_fm2_dir_changed, tbar);
   evas_object_smart_callback_add(tbar->fm2, "dir_deleted", 
				  _e_toolbar_fm2_dir_deleted, tbar);
   evas_object_smart_callback_add(tbar->fm2, "files_deleted", 
				  _e_toolbar_fm2_files_deleted, tbar);
   evas_object_smart_callback_add(tbar->fm2, "selected", 
				  _e_toolbar_fm2_selected, tbar);
   evas_object_smart_callback_add(tbar->fm2, "selection_change", 
				  _e_toolbar_fm2_selection_changed, tbar);

   tbar->o_event = evas_object_rectangle_add(evas);
   evas_object_color_set(tbar->o_event, 0, 0, 0, 0);
   evas_object_resize(tbar->o_event, tbar->w, tbar->h);
   evas_object_event_callback_add(tbar->o_event, EVAS_CALLBACK_MOUSE_DOWN, 
				  _e_toolbar_cb_mouse_down, tbar);
   evas_object_layer_set(tbar->o_event, 0);
   evas_object_show(tbar->o_event);

   tbar->o_base = edje_object_add(evas);
   evas_object_resize(tbar->o_base, tbar->w, tbar->h);
   e_theme_edje_object_set(tbar->o_base, "base/theme/toolbar", 
			   "e/toolbar/default/base");

   e_toolbar_move_resize(tbar, tbar->x, tbar->y, tbar->w, tbar->h);

   tbar->gadcon = e_gadcon_swallowed_new(tbar->name, tbar->id, tbar->o_base, 
					 "e.swallow.content");
   e_gadcon_size_request_callback_set(tbar->gadcon, 
				      _e_toolbar_gadcon_size_request, tbar);
   /* FIXME: We want to implement "styles" here ? */

   e_toolbar_orient(tbar, E_GADCON_ORIENT_TOP);

   e_gadcon_toolbar_set(tbar->gadcon, tbar);
   e_gadcon_util_menu_attach_func_set(tbar->gadcon, 
				      _e_toolbar_menu_items_append, tbar);
   e_gadcon_populate(tbar->gadcon);

   toolbars = eina_list_append(toolbars, tbar);
   return tbar;
}

EAPI void 
e_toolbar_fwin_set(E_Toolbar *tbar, E_Win *fwin) 
{
   E_OBJECT_CHECK(tbar);
   E_OBJECT_TYPE_CHECK(tbar, E_TOOLBAR_TYPE);
   tbar->fwin = fwin;
}

EAPI E_Win *
e_toolbar_fwin_get(E_Toolbar *tbar) 
{
   E_OBJECT_CHECK_RETURN(tbar, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(tbar, E_TOOLBAR_TYPE, NULL);
   return tbar->fwin;
}

EAPI void 
e_toolbar_fm2_set(E_Toolbar *tbar, Evas_Object *fm2) 
{
   E_OBJECT_CHECK(tbar);
   E_OBJECT_TYPE_CHECK(tbar, E_TOOLBAR_TYPE);
   tbar->fm2 = fm2;
}

EAPI Evas_Object *
e_toolbar_fm2_get(E_Toolbar *tbar) 
{
   E_OBJECT_CHECK_RETURN(tbar, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(tbar, E_TOOLBAR_TYPE, NULL);
   return tbar->fm2;
}

EAPI void 
e_toolbar_show(E_Toolbar *tbar) 
{
   E_OBJECT_CHECK(tbar);
   E_OBJECT_TYPE_CHECK(tbar, E_TOOLBAR_TYPE);
   evas_object_show(tbar->o_event);
   evas_object_show(tbar->o_base);
}

EAPI void 
e_toolbar_move(E_Toolbar *tbar, int x, int y) 
{
   E_OBJECT_CHECK(tbar);
   E_OBJECT_TYPE_CHECK(tbar, E_TOOLBAR_TYPE);
   tbar->x = x;
   tbar->y = y;
   evas_object_move(tbar->o_event, tbar->x, tbar->y);
   evas_object_move(tbar->o_base, tbar->x, tbar->y);
}

EAPI void 
e_toolbar_resize(E_Toolbar *tbar, int w, int h) 
{
   E_OBJECT_CHECK(tbar);
   E_OBJECT_TYPE_CHECK(tbar, E_TOOLBAR_TYPE);
   tbar->w = w;
   tbar->h = h;
   evas_object_resize(tbar->o_event, tbar->w, tbar->h);
   evas_object_resize(tbar->o_base, tbar->w, tbar->h);
}

EAPI void 
e_toolbar_move_resize(E_Toolbar *tbar, int x, int y, int w, int h) 
{
   E_OBJECT_CHECK(tbar);
   E_OBJECT_TYPE_CHECK(tbar, E_TOOLBAR_TYPE);
   tbar->x = x;
   tbar->y = y;
   tbar->w = w;
   tbar->h = h;
   evas_object_move(tbar->o_event, x, y);
   evas_object_move(tbar->o_base, x, y);
   evas_object_resize(tbar->o_event, w, h);
   evas_object_resize(tbar->o_base, w, h);
}

EAPI void 
e_toolbar_orient(E_Toolbar *tbar, E_Gadcon_Orient orient) 
{
   char buf[4096];

   E_OBJECT_CHECK(tbar);
   E_OBJECT_TYPE_CHECK(tbar, E_TOOLBAR_TYPE);
   e_gadcon_orient(tbar->gadcon, orient);
   snprintf(buf, sizeof(buf), "e,state,orientation,%s", 
	    _e_toolbar_orient_string_get(tbar));
   edje_object_signal_emit(tbar->o_base, buf, "e");
   edje_object_message_signal_process(tbar->o_base);
}

EAPI void 
e_toolbar_position_calc(E_Toolbar *tbar) 
{
   E_Gadcon_Orient orient = E_GADCON_ORIENT_TOP;

   E_OBJECT_CHECK(tbar);
   E_OBJECT_TYPE_CHECK(tbar, E_TOOLBAR_TYPE);
   if (!tbar->fwin) return;
   orient = tbar->gadcon->orient;
   switch (orient) 
     {
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_TOP:
	tbar->x = 0;
	tbar->y = 0;
	tbar->h = 32;
	tbar->w = tbar->fwin->w;
	break;
      case E_GADCON_ORIENT_BOTTOM:
	tbar->x = 0;
	tbar->h = 32;
	tbar->w = tbar->fwin->w;
	tbar->y = (tbar->fwin->h - tbar->h);
	break;
      case E_GADCON_ORIENT_VERT:
      case E_GADCON_ORIENT_LEFT:
	tbar->x = 0;
	tbar->w = 32;
	tbar->h = tbar->fwin->h;
	tbar->y = (tbar->fwin->h - tbar->h);
	break;
      case E_GADCON_ORIENT_RIGHT:
	tbar->y = 0;
	tbar->w = 32;
	tbar->x = (tbar->fwin->w - tbar->w);
	tbar->h = tbar->fwin->h;
	break;
      default:
	break;
     }
   e_toolbar_move_resize(tbar, tbar->x, tbar->y, tbar->w, tbar->h);
}

EAPI void 
e_toolbar_populate(E_Toolbar *tbar) 
{
   E_OBJECT_CHECK(tbar);
   E_OBJECT_TYPE_CHECK(tbar, E_TOOLBAR_TYPE);
   e_gadcon_populate(tbar->gadcon);
}

/* local functions */
static void 
_e_toolbar_free(E_Toolbar *tbar) 
{
   toolbars = eina_list_remove(toolbars, tbar);

   if (tbar->menu) 
     {
	e_menu_post_deactivate_callback_set(tbar->menu, NULL, NULL);
	e_object_del(E_OBJECT(tbar->menu));
	tbar->menu = NULL;
     }
   if (tbar->cfg_dlg) e_object_del(E_OBJECT(tbar->cfg_dlg));
   e_object_del(E_OBJECT(tbar->gadcon));
   if (tbar->name) eina_stringshare_del(tbar->name);
   evas_object_del(tbar->o_event);
   evas_object_del(tbar->o_base);
   E_FREE(tbar);
}

static void 
_e_toolbar_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info) 
{
   Evas_Event_Mouse_Down *ev;
   E_Toolbar *tbar;
   E_Menu *mn;
   E_Zone *zone;
   int x, y;

   ev = event_info;
   tbar = data;
   if (ev->button != 3) return;
   mn = e_menu_new();
   e_menu_post_deactivate_callback_set(mn, _e_toolbar_menu_cb_post, tbar);
   tbar->menu = mn;
   _e_toolbar_menu_append(tbar, mn);
   zone = e_util_zone_current_get(e_manager_current_get());
   ecore_x_pointer_xy_get(zone->container->win, &x, &y);
   e_menu_activate_mouse(mn, zone, x, y, 1, 1, 
			 E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
}

static void 
_e_toolbar_menu_cb_post(void *data, E_Menu *mn) 
{
   E_Toolbar *tbar;

   tbar = data;
   if (!tbar->menu) return;
   e_object_del(E_OBJECT(tbar->menu));
   tbar->menu = NULL;
}

static void 
_e_toolbar_menu_cb_pre(void *data, E_Menu *mn) 
{
   E_Toolbar *tbar;
   E_Menu_Item *mi;

   tbar = data;
   e_menu_pre_activate_callback_set(mn, NULL, NULL);

   mi = e_menu_item_new(mn);
   if (tbar->gadcon->editing)
     e_menu_item_label_set(mi, _("Stop Moving/Resizing Items"));
   else
     e_menu_item_label_set(mi, _("Begin Moving/Resizing Items"));
   e_util_menu_item_edje_icon_set(mi, "widget/resize");
   e_menu_item_callback_set(mi, _e_toolbar_menu_cb_edit, tbar);

   mi = e_menu_item_new(mn);
   e_menu_item_separator_set(mi, 1);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Toolbar Settings"));
   e_util_menu_item_edje_icon_set(mi, "widget/config");
   e_menu_item_callback_set(mi, _e_toolbar_menu_cb_config, tbar);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Configure Toolbar Contents"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/toolbar");
   e_menu_item_callback_set(mi, _e_toolbar_menu_cb_contents, tbar);
}

static void 
_e_toolbar_menu_items_append(void *data, E_Gadcon_Client *gcc, E_Menu *mn) 
{
   E_Toolbar *tbar;
   
   tbar = data;
   _e_toolbar_menu_append(tbar, mn);
}

static void 
_e_toolbar_menu_append(E_Toolbar *tbar, E_Menu *mn) 
{
   E_Menu_Item *mi;
   E_Menu *subm;

   subm = e_menu_new();
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, tbar->name);
   e_util_menu_item_edje_icon_set(mi, "enlightenment/toolbar");
   e_menu_pre_activate_callback_set(subm, _e_toolbar_menu_cb_pre, tbar);
   e_menu_item_submenu_set(mi, subm);
}

static void 
_e_toolbar_menu_cb_edit(void *data, E_Menu *mn, E_Menu_Item *mi) 
{
   E_Toolbar *tbar;

   tbar = data;
   if (tbar->gadcon->editing) 
     e_gadcon_edit_end(tbar->gadcon);
   else 
     e_gadcon_edit_begin(tbar->gadcon);
}

static void 
_e_toolbar_menu_cb_config(void *data, E_Menu *mn, E_Menu_Item *mi) 
{
   E_Toolbar *tbar;

   tbar = data;
   if (!tbar->cfg_dlg) e_int_toolbar_config(tbar);
}

static void 
_e_toolbar_menu_cb_contents(void *data, E_Menu *mn, E_Menu_Item *mi) 
{
   E_Toolbar *tbar;

   tbar = data;
   if (!tbar->gadcon->config_dialog) e_int_gadcon_config_toolbar(tbar->gadcon);
}

static void 
_e_toolbar_gadcon_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h) 
{
   E_Toolbar *tbar;
   Evas_Coord nx, ny, nw, nh, ww, hh;

   tbar = data;
   nx = tbar->x;
   ny = tbar->y;
   nw = tbar->w;
   nh = tbar->h;
   ww = hh = 0;
   evas_object_geometry_get(gc->o_container, NULL, NULL, &ww, &hh);
   switch (gc->orient) 
     {
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_BOTTOM:
	w = ww;
	h = 32;
	break;
      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_RIGHT:
	w = 32;
	h = hh;
	break;
      default:
	break;
     }
   e_gadcon_swallowed_min_size_set(gc, w, h);
   edje_object_size_min_calc(tbar->o_base, &nw, &nh);
   switch (gc->orient) 
     {
      case E_GADCON_ORIENT_TOP:
	nx = ny = 0;
	nw = tbar->w;
	nh = tbar->h;
	if (nw > tbar->fwin->w) nw = tbar->fwin->w;
	if (nh > tbar->fwin->h) nh = 32;
	break;
      case E_GADCON_ORIENT_BOTTOM:
	nx = 0;
	nw = tbar->w;
	nh = tbar->h;
	if (nw > tbar->fwin->w) nw = tbar->fwin->w;
	if (nh > tbar->fwin->h) nh = 32;
	ny = (tbar->fwin->h - nh);
	break;
      case E_GADCON_ORIENT_LEFT:
	nx = ny = 0;
	nw = tbar->w;
	nh = tbar->h;
	if (nh > tbar->fwin->h) nh = tbar->fwin->h;
	if (nw > tbar->fwin->w) nw = 32;
	break;
      case E_GADCON_ORIENT_RIGHT:
	ny = 0;
	nh = tbar->h;
	nw = tbar->w;
	if (nw > tbar->fwin->w) nw = 32;
	if (nh > tbar->fwin->h) nh = tbar->fwin->h;
	nx = (tbar->fwin->w - tbar->w);
	break;
      default:
	break;
     }
   e_toolbar_move_resize(tbar, nx, ny, nw, nh);
}

static const char *
_e_toolbar_orient_string_get(E_Toolbar *tbar) 
{
   const char *sig = "";

   switch (tbar->gadcon->orient)
     {
      case E_GADCON_ORIENT_HORIZ:
	sig = "horizontal";
	break;
      case E_GADCON_ORIENT_VERT:
	sig = "vertical";
	break;
      case E_GADCON_ORIENT_LEFT:
	sig = "left";
	break;
      case E_GADCON_ORIENT_RIGHT:
	sig = "right";
	break;
      case E_GADCON_ORIENT_TOP:
	sig = "top";
	break;
      case E_GADCON_ORIENT_BOTTOM:
	sig = "bottom";
	break;
      default:
	break;
     }
   return sig;
}

static void 
_e_toolbar_fm2_changed(void *data, Evas_Object *obj, void *event_info) 
{
   E_Toolbar *tbar;
   Eina_List *l = NULL;

   tbar = data;
   if (!tbar) return;
   for (l = tbar->gadcon->clients; l; l = l->next) 
     {
	E_Gadcon_Client *gcc = NULL;

	gcc = l->data;
	if (!gcc) continue;
	evas_object_smart_callback_call(gcc->o_base, "changed", tbar);
     }
}

static void 
_e_toolbar_fm2_dir_changed(void *data, Evas_Object *obj, void *event_info) 
{
   E_Toolbar *tbar;
   Eina_List *l = NULL;

   tbar = data;
   if (!tbar) return;
   for (l = tbar->gadcon->clients; l; l = l->next) 
     {
	E_Gadcon_Client *gcc = NULL;

	gcc = l->data;
	if (!gcc) continue;
	evas_object_smart_callback_call(gcc->o_base, "dir_changed", tbar);
     }
}

static void 
_e_toolbar_fm2_dir_deleted(void *data, Evas_Object *obj, void *event_info) 
{
   E_Toolbar *tbar;
   Eina_List *l = NULL;

   tbar = data;
   if (!tbar) return;
   for (l = tbar->gadcon->clients; l; l = l->next) 
     {
	E_Gadcon_Client *gcc = NULL;

	gcc = l->data;
	if (!gcc) continue;
	evas_object_smart_callback_call(gcc->o_base, "dir_deleted", tbar);
     }
}

static void 
_e_toolbar_fm2_files_deleted(void *data, Evas_Object *obj, void *event_info) 
{
   E_Toolbar *tbar;
   Eina_List *l = NULL;

   tbar = data;
   if (!tbar) return;
   for (l = tbar->gadcon->clients; l; l = l->next) 
     {
	E_Gadcon_Client *gcc = NULL;

	gcc = l->data;
	if (!gcc) continue;
	evas_object_smart_callback_call(gcc->o_base, "files_deleted", tbar);
     }
}

static void 
_e_toolbar_fm2_selected(void *data, Evas_Object *obj, void *event_info) 
{
   E_Toolbar *tbar;
   Eina_List *l = NULL;

   tbar = data;
   if (!tbar) return;
   for (l = tbar->gadcon->clients; l; l = l->next) 
     {
	E_Gadcon_Client *gcc = NULL;

	gcc = l->data;
	if (!gcc) continue;
	evas_object_smart_callback_call(gcc->o_base, "selected", tbar);
     }
}

static void 
_e_toolbar_fm2_selection_changed(void *data, Evas_Object *obj, void *event_info) 
{
   E_Toolbar *tbar;
   Eina_List *l = NULL;

   tbar = data;
   if (!tbar) return;
   for (l = tbar->gadcon->clients; l; l = l->next) 
     {
	E_Gadcon_Client *gcc = NULL;

	gcc = l->data;
	if (!gcc) continue;
	evas_object_smart_callback_call(gcc->o_base, "selection_changed", tbar);
     }
}
