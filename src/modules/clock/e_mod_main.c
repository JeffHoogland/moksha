#include "e.h"
#include "e_mod_main.h"

/* TODO List:
 *
 * should support proepr resize and move handles in the edje.
 * 
 */

/* module private routines */
static Clock *_clock_init(E_Module *m);
static void    _clock_shutdown(Clock *e);
static E_Menu *_clock_config_menu_new(Clock *e);
static void    _clock_config_menu_del(Clock *e, E_Menu *m);
static void    _clock_face_init(Clock_Face *ef);
static void    _clock_face_free(Clock_Face *ef);
static void    _clock_face_reconfigure(Clock_Face *ef);
static void    _clock_cb_face_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _clock_cb_face_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _clock_cb_face_move(void *data, Evas *e, Evas_Object *obj, void *event_info);

/* public module routines. all modules must have these */
void *
init(E_Module *m)
{
   Clock *e;
   
   /* check module api version */
   if (m->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show("Module API Error",
			    "Error initializing Module: IBar\n"
			    "It requires a minimum module API version of: %i.\n"
			    "The module API advertized by Enlightenment is: %i.\n"
			    "Aborting module.",
			    E_MODULE_API_VERSION,
			    m->api->version);
	return NULL;
     }
   /* actually init clock */
   e = _clock_init(m);
   m->config_menu = _clock_config_menu_new(e);
   return e;
}

int
shutdown(E_Module *m)
{
   Clock *e;
   
   e = m->data;
   if (e)
     {
	if (m->config_menu)
	  {
	     _clock_config_menu_del(e, m->config_menu);
	     m->config_menu = NULL;
	  }
	_clock_shutdown(e);
     }
   return 1;
}

int
save(E_Module *m)
{
   Clock *e;

   e = m->data;
   e_config_domain_save("module.clock", e->conf_edd, e->conf);
   return 1;
}

int
info(E_Module *m)
{
   char buf[4096];
   
   m->label = strdup("Clock");
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   return 1;
}

int
about(E_Module *m)
{
   e_error_dialog_show("Enlightenment Clock Module",
		       "A simple module to give E17 a clock.");
   return 1;
}

/* module private routines */
static Clock *
_clock_init(E_Module *m)
{
   Clock *e;
   Evas_List *managers, *l, *l2;
   
   e = calloc(1, sizeof(Clock));
   if (!e) return NULL;
   
   e->conf_edd = E_CONFIG_DD_NEW("Clock_Config", Config);
#undef T
#undef D
#define T Config
#define D e->conf_edd   
   E_CONFIG_VAL(D, T, width, INT);
   E_CONFIG_VAL(D, T, x, DOUBLE);
   E_CONFIG_VAL(D, T, y, DOUBLE);

   e->conf = e_config_domain_load("module.clock", e->conf_edd);
   if (!e->conf)
     {
	e->conf = E_NEW(Config, 1);
	e->conf->width = 64;
	e->conf->x = 0.0;
	e->conf->y = 1.0;
     }
   E_CONFIG_LIMIT(e->conf->width, 2, 256);
   E_CONFIG_LIMIT(e->conf->x, 0.0, 1.0);
   E_CONFIG_LIMIT(e->conf->y, 0.0, 1.0);
   
   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     Clock_Face *ef;
	     
	     con = l2->data;
	     ef = calloc(1, sizeof(Clock_Face));
	     if (ef)
	       {
		  ef->clock = e;
		  ef->con = con;
		  ef->evas = con->bg_evas;
		  _clock_face_init(ef);
		  e->face = ef;
	       }
	  }
     }
   return e;
}

static void
_clock_shutdown(Clock *e)
{
   free(e->conf);
   E_CONFIG_DD_FREE(e->conf_edd);
   
   _clock_face_free(e->face);
   free(e);
}

static E_Menu *
_clock_config_menu_new(Clock *e)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   /* FIXME: hook callbacks to each menu item */
   mn = e_menu_new();
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Set Time");
//   e_menu_item_callback_set(mi, _clock_cb_time_set, e);

/*   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "More Options...");
*/
   e->config_menu = mn;
   
   return mn;
}

static void
_clock_config_menu_del(Clock *e, E_Menu *m)
{
   e_object_del(E_OBJECT(m));
}

static void
_clock_face_init(Clock_Face *ef)
{
   Evas_List *l;
   Evas_Coord ww, hh, bw, bh;
   Evas_Object *o;
   
   evas_output_viewport_get(ef->evas, NULL, NULL, &ww, &hh);
   ef->fx = ef->clock->conf->x * (ww - ef->clock->conf->width);
   ef->fy = ef->clock->conf->y * (hh - ef->clock->conf->width);
      
   evas_event_freeze(ef->evas);
   o = edje_object_add(ef->evas);
   ef->clock_object = o;

   edje_object_file_set(o,
			/* FIXME: "default.eet" needs to come from conf */
			e_path_find(path_themes, "default.eet"),
			"modules/clock/main");
   evas_object_show(o);
   
   o = evas_object_rectangle_add(ef->evas);
   ef->event_object = o;
   evas_object_layer_set(o, 2);
   evas_object_repeat_events_set(o, 1);
   evas_object_color_set(o, 0, 0, 0, 0);

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _clock_cb_face_down, ef);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _clock_cb_face_up, ef);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _clock_cb_face_move, ef);
   evas_object_show(o);
   
   edje_object_size_min_calc(ef->clock_object, &bw, &bh);
   ef->minsize = bh;
   ef->minsize = bw;

   _clock_face_reconfigure(ef);
   
   evas_event_thaw(ef->evas);
}

static void
_clock_face_free(Clock_Face *ef)
{
   evas_object_del(ef->clock_object);
   evas_object_del(ef->event_object);
   free(ef);
}

static void
_clock_face_reconfigure(Clock_Face *ef)
{
   Evas_Coord minw, minh, maxw, maxh, ww, hh;

   edje_object_size_min_calc(ef->clock_object, &minw, &maxh);
   edje_object_size_max_get(ef->clock_object, &maxw, &minh);
   evas_output_viewport_get(ef->evas, NULL, NULL, &ww, &hh);
   ef->fx = ef->clock->conf->x * (ww - ef->clock->conf->width);
   ef->fy = ef->clock->conf->y * (hh - ef->clock->conf->width);
   ef->fw = ef->clock->conf->width;
   ef->minsize = minw;
   ef->maxsize = maxw;

   evas_object_move(ef->clock_object, ef->fx, ef->fy);
   evas_object_resize(ef->clock_object, ef->clock->conf->width, ef->clock->conf->width);
   evas_object_move(ef->event_object, ef->fx, ef->fy);
   evas_object_resize(ef->event_object, ef->clock->conf->width, ef->clock->conf->width);
}

static void
_clock_cb_face_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Clock_Face *ef;
   
   ev = event_info;
   ef = data;
   if (ev->button == 3)
     {
	e_menu_activate_mouse(ef->clock->config_menu, ef->con,
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN);
	e_util_container_fake_mouse_up_all_later(ef->con);
     }
   else if (ev->button == 2)
     {
	ef->resize = 1;
     }
   else if (ev->button == 1)
     {
	ef->move = 1;
     }
   evas_pointer_canvas_xy_get(e, &ef->xx, &ef->yy);
}

static void
_clock_cb_face_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   Clock_Face *ef;
   Evas_Coord ww, hh;
   
   ev = event_info;
   ef = data;
   ef->move = 0;
   ef->resize = 0;
   evas_output_viewport_get(ef->evas, NULL, NULL, &ww, &hh);
   ef->clock->conf->x = (double)ef->fx / (double)(ww - ef->clock->conf->width);
   ef->clock->conf->y = (double)ef->fy / (double)(hh - ef->clock->conf->width);
   e_config_save_queue();
}

static void
_clock_cb_face_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{ 
   Evas_Event_Mouse_Move *ev;
   Clock_Face *ef;
   Evas_Coord x, y, w, h, cx, cy, sw, sh;
   
   evas_pointer_canvas_xy_get(e, &cx, &cy);
   evas_output_viewport_get(e, NULL, NULL, &sw, &sh);

   ev = event_info;
   ef = data;
   if (ef->move)
     {
	ef->fx += cx - ef->xx;
	ef->fy += cy - ef->yy;
	if (ef->fx < 0) ef->fx = 0;
	if (ef->fy < 0) ef->fy = 0;
	if (ef->fx + ef->fw > sw) ef->fx = sw - ef->fw;
	if (ef->fy + ef->fw > sh) ef->fy = sh - ef->fw;
	evas_object_move(ef->clock_object, ef->fx, ef->fy);
	evas_object_move(ef->event_object, ef->fx, ef->fy);
     }
   else if (ef->resize)
   {
	Evas_Coord d;

	d = cx - ef->xx;
	ef->fw += d;
	if (ef->fw < ef->minsize) ef->fw = ef->minsize;
	if (ef->fw > ef->maxsize) ef->fw = ef->maxsize;
	if (ef->fx + ef->fw > sw) ef->fw = sw - ef->fx;
	if (ef->fy + ef->fw > sh) ef->fw = sh - ef->fy;
	evas_object_resize(ef->clock_object, ef->fw, ef->fw);
	evas_object_resize(ef->event_object, ef->fw, ef->fw);
   }
   ef->xx = ev->cur.canvas.x;
   ef->yy = ev->cur.canvas.y;
}  

