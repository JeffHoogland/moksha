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
static void    _clock_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change);
static void    _clock_cb_face_down(void *data, Evas *e, Evas_Object *obj, void *event_info);

/* public module routines. all modules must have these */
void *
init(E_Module *m)
{
   Clock *e;
   
   /* check module api version */
   if (m->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show("Module API Error",
			    "Error initializing Module: Clock\n"
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
/*   e_config_domain_save("module.clock", e->conf_edd, e->conf);*/
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

   /*
    e->conf_edd = E_CONFIG_DD_NEW("Clock_Config", Config);
    * 
    e->conf = e_config_domain_load("module.clock", e->conf_edd);
    if (!e->conf)
    {
    e->conf = E_NEW(Config, 1);
    }
    */
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
/*   E_CONFIG_DD_FREE(e->conf_edd);*/
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
   e_menu_item_label_set(mi, "(Unused)");
/*   e_menu_item_callback_set(mi, _clock_cb_time_set, e);*/
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
   Evas_Coord ww, hh, bw, bh;
   Evas_Object *o;
   
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
   evas_object_show(o);
   
   ef->gmc = e_gadman_client_new(ef->con->gadman);
   e_gadman_client_domain_set(ef->gmc, "module.clock", 0);
   e_gadman_client_policy_set(ef->gmc,
			      E_GADMAN_POLICY_ANYWHERE |
			      E_GADMAN_POLICY_HMOVE |
			      E_GADMAN_POLICY_VMOVE |
			      E_GADMAN_POLICY_HSIZE |
			      E_GADMAN_POLICY_VSIZE);
   e_gadman_client_min_size_set(ef->gmc, 4, 4);
   e_gadman_client_max_size_set(ef->gmc, 512, 512);
   e_gadman_client_auto_size_set(ef->gmc, 64, 64);
   e_gadman_client_align_set(ef->gmc, 0.0, 1.0);
   e_gadman_client_aspect_set(ef->gmc, 1.0, 1.0);
   e_gadman_client_resize(ef->gmc, 64, 64);
   e_gadman_client_change_func_set(ef->gmc, _clock_cb_gmc_change, ef);
   e_gadman_client_load(ef->gmc);
   evas_event_thaw(ef->evas);
}

static void
_clock_face_free(Clock_Face *ef)
{
   e_object_del(E_OBJECT(ef->gmc));
   evas_object_del(ef->clock_object);
   evas_object_del(ef->event_object);
   free(ef);
}

static void
_clock_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change)
{
   Clock_Face *ef;
   Evas_Coord x, y, w, h;

   ef = data;
   if (change == E_GADMAN_CHANGE_MOVE_RESIZE)
     {
	e_gadman_client_geometry_get(ef->gmc, &x, &y, &w, &h);
	evas_object_move(ef->clock_object, x, y);
	evas_object_move(ef->event_object, x, y);
	evas_object_resize(ef->clock_object, w, h);
	evas_object_resize(ef->event_object, w, h);
     }
   else if (change == E_GADMAN_CHANGE_RAISE)
     {
	evas_object_raise(ef->clock_object);
	evas_object_raise(ef->event_object);
     }
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
	e_menu_activate_mouse(ef->clock->config_menu, e_zone_current_get(ef->con),
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN);
	e_util_container_fake_mouse_up_all_later(ef->con);
     }
}
