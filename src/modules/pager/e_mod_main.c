#include "e.h"
#include "e_mod_main.h"

/* TODO List:
 *
 * should support proper resize and move handles in the edje.
 * 
 */

/* module private routines */
static Pager  *_pager_init(E_Module *m);
static void    _pager_shutdown(Pager *e);
static E_Menu *_pager_config_menu_new(Pager *e);
static void    _pager_config_menu_del(Pager *e, E_Menu *m);
static void    _pager_cb_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _pager_cb_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _pager_cb_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static int     _pager_cb_event_container_resize(void *data, int type, void *event);

static void    _pager_reconfigure(Pager *e);
static void    _pager_refresh(Pager *e);

#define         PAGER_MIN_W 10
#define         PAGER_MIN_H 7

/* public module routines. all modules must have these */
void *
init(E_Module *m)
{
   Pager *e;
   
   /* check module api version */
   if (m->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show("Module API Error",
			    "Error initializing Module: Pager\n"
			    "It requires a minimum module API version of: %i.\n"
			    "The module API advertized by Enlightenment is: %i.\n"
			    "Aborting module.",
			    E_MODULE_API_VERSION,
			    m->api->version);
	return NULL;
     }
   /* actually init pager */
   e = _pager_init(m);
   m->config_menu = _pager_config_menu_new(e);
   return e;
}

int
shutdown(E_Module *m)
{
   Pager *e;
   
   e = m->data;
   if (e)
     {
	if (m->config_menu)
	  {
	     _pager_config_menu_del(e, m->config_menu);
	     m->config_menu = NULL;
	  }
	_pager_shutdown(e);
     }
   return 1;
}

int
save(E_Module *m)
{
   Pager *e;

   e = m->data;
   e_config_domain_save("module.pager", e->conf_edd, e->conf);
   return 1;
}

int
info(E_Module *m)
{
   char buf[4096];
   
   m->label = strdup("Pager");
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   return 1;
}

int
about(E_Module *m)
{
   e_error_dialog_show("Enlightenment Pager Module",
		       "A pager module to navigate E17 desktops.");
   return 1;
}

/* module private routines */
static Pager *
_pager_init(E_Module *m)
{
   Pager       *e;
   Evas_List   *managers, *l, *l2;
   Evas_Object *o;
   
   e = calloc(1, sizeof(Pager));
   if (!e) return NULL;
   
   e->conf_edd = E_CONFIG_DD_NEW("Pager_Config", Config);
#undef T
#undef D
#define T Config
#define D e->conf_edd   
   E_CONFIG_VAL(D, T, width, INT);
   E_CONFIG_VAL(D, T, height, INT);
   E_CONFIG_VAL(D, T, x, DOUBLE);
   E_CONFIG_VAL(D, T, y, DOUBLE);

   e->conf = e_config_domain_load("module.pager", e->conf_edd);
   if (!e->conf)
     {
	e->conf = E_NEW(Config, 1);
	e->conf->width = 50;
	e->conf->height = 30;
	e->conf->x = 0.0;
	e->conf->y = 0.0;
     }
   E_CONFIG_LIMIT(e->conf->x, 0.0, 1.0);
   E_CONFIG_LIMIT(e->conf->y, 0.0, 1.0);
   E_CONFIG_LIMIT(e->conf->width, PAGER_MIN_W, 1000);
   E_CONFIG_LIMIT(e->conf->height, PAGER_MIN_H, 1000);

   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     
	     con = l2->data;
	     e->con = con;
	     e->evas = con->bg_evas;
	  }
     }

   o = evas_object_rectangle_add(e->evas);
   e->base = o;
   evas_object_color_set(o, 128, 128, 128, 0);
   evas_object_pass_events_set(o, 0);
   evas_object_repeat_events_set(o, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _pager_cb_down, e);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _pager_cb_up, e);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _pager_cb_move, e);
   evas_object_show(o);

   o = edje_object_add(e->evas);
   e->screen = o;
   edje_object_file_set(o,
                        /* FIXME: "default.eet" needs to come from conf */
                        e_path_find(path_themes, "default.eet"),
                        "modules/pager/screen");
   evas_object_show(o);
   
   e->ev_handler_container_resize =
   ecore_event_handler_add(E_EVENT_CONTAINER_RESIZE,
                           _pager_cb_event_container_resize, e);
   
   _pager_reconfigure(e);
   return e;
}

static void
_pager_shutdown(Pager *e)
{
   free(e->conf);
   E_CONFIG_DD_FREE(e->conf_edd);

   evas_object_free(e->base);
   evas_object_free(e->screen);

   while(e->desks)
     {
	evas_object_hide(e->desks->data);
        evas_object_free(e->desks->data);
        e->desks = evas_list_remove_list(e->desks, e->desks);
     }

   while(e->wins)
     {
	evas_object_hide(e->wins->data);
        evas_object_free(e->wins->data);
        e->wins = evas_list_remove_list(e->wins, e->wins);
     }

   ecore_event_handler_del(e->ev_handler_container_resize);   
   free(e);
}

static E_Menu *
_pager_config_menu_new(Pager *e)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   mn = e_menu_new();
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "(Unused)");
/*   e_menu_item_callback_set(mi, _pager_cb_scale, e);*/
   e->config_menu = mn;
   
   return mn;
}

static void
_pager_config_menu_del(Pager *e, E_Menu *m)
{
   e_object_del(E_OBJECT(m));
}

static void
_pager_reconfigure(Pager *e)
{
   Evas_Coord ww, hh;

   evas_output_viewport_get(e->evas, NULL, NULL, &ww, &hh);
   e->fx = e->conf->x * (ww - e->conf->width);
   e->fy = e->conf->y * (hh - e->conf->height);
   e->fw = e->conf->width;
   e->fh = e->conf->height;

   _pager_refresh(e);
}

static void
_pager_refresh(Pager *e)
{
   E_Zone      *zone;
   E_Desk      *desk, *current;
   E_Border     *border;
   Evas_List   *clients;

   Evas_Object *desk_obj, *win_obj;
   int          desks_x, desks_y, x, y;
   Evas_Coord   px, py, pw, ph, ww, hh;
   double       scalex, scaley;

   evas_object_resize(e->base, e->fw, e->fh);
   evas_object_move(e->base, e->fx, e->fy);

   zone = e_zone_current_get(e->con);
   e_zone_desk_count_get(zone, &desks_x, &desks_y);
   pw = e->fw / (double) desks_x;
   ph = e->fh / (double) desks_y;

   evas_output_viewport_get(e->evas, NULL, NULL, &ww, &hh);
   scalex = (double) pw / ww;
   scaley = (double) ph / hh;
   evas_object_resize(e->screen, pw, ph);

   while(e->desks)
     {
	evas_object_hide(e->desks->data);
	evas_object_free(e->desks->data);
	e->desks = evas_list_remove_list(e->desks, e->desks);
     }
   while(e->wins)
     {
	evas_object_hide(e->wins->data);
        evas_object_free(e->wins->data);
        e->wins = evas_list_remove_list(e->wins, e->wins);
     }

   current = e_desk_current_get(zone);
   for (x = 0; x < desks_x; x++)
     for (y = 0; y < desks_y; y++)
       {
	  desk = e_desk_at_xy_get(zone, x, y);
	  px = e->fx + (x * pw);
	  py = e->fy + (y * ph);
	  if (desk == current)
	    evas_object_move(e->screen, px, py);
	  desk_obj = edje_object_add(e->evas);
	  edje_object_file_set(desk_obj,
			       /* FIXME: "default.eet" needs to come from conf */
			       e_path_find(path_themes, "default.eet"),
			       "modules/pager/desk");
	  evas_object_pass_events_set(desk_obj, 1);
	  evas_object_resize(desk_obj, pw, ph);
	  evas_object_move(desk_obj, px, py);

	  evas_object_show(desk_obj);
	  e->desks = evas_list_append(e->desks, desk_obj);

	  clients = desk->clients;
	  while (clients)
	    {
	       Evas_Coord winx, winy, winw, winh;
	       border = (E_Border *) clients->data;

	       winx = (Evas_Coord) ((double) border->x) * scalex;
	       winy = (Evas_Coord) ((double) border->y) * scaley;
	       winw = (Evas_Coord) ((double) border->w) * scalex;
	       winh = (Evas_Coord) ((double) border->h) * scaley;
	       win_obj = edje_object_add(e->evas);
	       edje_object_file_set(win_obj,
				    /* FIXME: "default.eet" needs to come from conf */
				    e_path_find(path_themes, "default.eet"),
				    "modules/pager/window");
	       evas_object_pass_events_set(win_obj, 1);
	       evas_object_resize(win_obj, winw, winh);
	       evas_object_move(win_obj, px + winx, py + winy);

	       evas_object_show(win_obj);
	       e->wins = evas_list_append(e->wins, win_obj);

	       clients = clients->next;
	    }
       }
}

static void
_pager_cb_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Pager *p;
   
   ev = event_info;
   p = data;
   if (ev->button == 3)
     {
	e_menu_activate_mouse(p->config_menu, p->con,
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN);
	e_util_container_fake_mouse_up_all_later(p->con);
     }
   else if (ev->button == 2)
     {
	p->resize = 1;
     }
   else if (ev->button == 1)
     {
	p->move = 1;
     }
   evas_pointer_canvas_xy_get(p->evas, &p->xx, &p->yy);
}

static void
_pager_cb_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   Pager     *p;
   Evas_Coord ww, hh;
   double     newx, newy;
   
   ev = event_info;
   p = data;

   evas_output_viewport_get(p->evas, NULL, NULL, &ww, &hh);
   /* if we clicked, not moved - FIXME, this is a hack */
   newx = (double)p->fx / (double)(ww - p->fw);
   newy = (double)p->fy / (double)(hh - p->fy);
printf("saving %g, %g\n", newx, newy);
   if (p->move && (p->conf->x == newx) && (p->conf->y == newy))
     {
	int     x, y, w, h, xcount, ycount, cx, cy;
	E_Zone *zone;
	E_Desk *desk;

	zone = e_zone_current_get(p->con);
	e_zone_desk_count_get(zone, &xcount, &ycount);
	evas_pointer_canvas_xy_get(e, &cx, &cy);

	w = p->fw / xcount;
	h = p->fh / ycount;
	for (x = 0; x < xcount; x++)
	  for (y = 0; y < ycount; y++)
	    {
	       int left, right, top, bottom;
	       left = p->fx + x * w;
	       right = left + w;
	       top = p->fy + y * h;
	       bottom = top + h;

	       if (left <= cx && cx < right && top <= cy && cy < bottom)
		 {
		    desk = e_desk_at_xy_get(zone, x, y);
		    e_desk_show(desk);
		    evas_object_move(p->screen, left, top);
		 }
	    }
     }
   p->move = 0;
   p->resize = 0;

   p->conf->width = p->fw;
   p->conf->height = p->fh;
   p->conf->x = newx;
   p->conf->y = newy;
   e_config_save_queue();

}

static void
_pager_cb_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{ 
   Evas_Event_Mouse_Move *ev;
   Pager *p;
   Evas_Coord cx, cy, sw, sh;
   
   evas_pointer_canvas_xy_get(e, &cx, &cy);
   evas_output_viewport_get(e, NULL, NULL, &sw, &sh);

   ev = event_info;
   p = data;
   if (p->move)
     {
	p->fx += cx - p->xx;
	p->fy += cy - p->yy;
	if (p->fx < 0) p->fx = 0;
	if (p->fy < 0) p->fy = 0;
	if (p->fx + p->fw > sw) p->fx = sw - p->fw;
	if (p->fy + p->fh > sh) p->fy = sh - p->fh;
	_pager_refresh(p);
     }
   else if (p->resize)
     {
	Evas_Coord dx, dy;

	dx = cx - p->xx;
	dy = cy - p->yy;

	p->fw += dx;
	p->fh += dy;
	if (p->fw < PAGER_MIN_W) p->fw = PAGER_MIN_W;
	if (p->fh < PAGER_MIN_H) p->fh = PAGER_MIN_H;
//	if (p->fw < p->minsize) p->fw = p->minsize;
//	if (p->fw > p->maxsize) p->fw = p->maxsize;
	if (p->fx + p->fw > sw) p->fw = sw - p->fx;
	if (p->fy + p->fh > sh) p->fh = sh - p->fy;
	_pager_refresh(p);
   }
   p->xx = ev->cur.canvas.x;
   p->yy = ev->cur.canvas.y;
}  

static int
_pager_cb_event_container_resize(void *data, int type, void *event)
{
   Pager *e;
   
   e = data;
   _pager_reconfigure(e);
   return 1;
}
