/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/* TODO List:
 *
 * save the visible flag for each face
 * 
 */

/* module private routines */
static Clock  *_clock_init(E_Module *module);
static void    _clock_shutdown(Clock *clock);
static E_Menu *_clock_config_menu_new();
static void    _clock_config_menu_del(E_Menu *menu);
static void    _clock_face_init(Clock_Face *face);
static void    _clock_face_free(Clock_Face *face);
static void    _clock_face_enable(Clock_Face *face);
static void    _clock_face_disable(Clock_Face *face);
static void    _clock_face_menu_new(Clock_Face *face);
static void    _clock_face_menu_del(E_Menu *menu);
static void    _clock_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change);
static void    _clock_face_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _clock_face_cb_menu_enabled(void *data, E_Menu *m, E_Menu_Item *mi);

static int _clock_count;
/* public module routines. all modules must have these */
void *
init(E_Module *module)
{
   Clock *clock;
   
   /* check module api version */
   if (module->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show("Module API Error",
			    "Error initializing Module: Clock\n"
			    "It requires a minimum module API version of: %i.\n"
			    "The module API advertized by Enlightenment is: %i.\n"
			    "Aborting module.",
			    E_MODULE_API_VERSION,
			    module->api->version);
	return NULL;
     }
   /* actually init clock */
   module->config_menu = _clock_config_menu_new();
   clock = _clock_init(module);
   return clock;
}

int
shutdown(E_Module *module)
{
   Clock *clock;
   
   clock = module->data;
   if (clock)
     {
	if (module->config_menu)
	  {
	     _clock_config_menu_del(module->config_menu);
	     module->config_menu = NULL;
	  }
	_clock_shutdown(clock);
     }
   return 1;
}

int
save(E_Module *module)
{
   Clock *clock;

   clock = module->data;
/*   e_config_domain_save("module.clock", e->conf_edd, e->conf);*/
   return 1;
}

int
info(E_Module *module)
{
   char buf[4096];
   
   module->label = strdup("Clock");
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(module));
   module->icon_file = strdup(buf);
   return 1;
}

int
about(E_Module *module)
{
   e_error_dialog_show("Enlightenment Clock Module",
		       "A simple module to give E17 a clock.");
   return 1;
}

/* module private routines */
static Clock *
_clock_init(E_Module *module)
{
   Clock *clock;
   Evas_List *managers, *l, *l2;
   E_Menu_Item *mi;
  
   _clock_count = 0;
   clock = E_NEW(Clock, 1);
   if (!clock) return NULL;

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
	     Clock_Face *face;
	     
	     con = l2->data;
	     face = E_NEW(Clock_Face, 1);
	     if (face)
	       {
		  /* FIXME : config */
		  face->enabled = 1;
		  face->clock = clock;
		  face->con = con;
		  e_object_ref(E_OBJECT(con));
		  face->evas = con->bg_evas;
		  _clock_face_init(face);
		  clock->faces = evas_list_append(clock->faces, face);

		  mi = e_menu_item_new(module->config_menu);
		  e_menu_item_label_set(mi, con->name);

		  e_menu_item_submenu_set(mi, face->menu);
	       }
	  }
     }
   return clock;
}

static void
_clock_shutdown(Clock *clock)
{
   Evas_List *list;

   free(clock->conf);
/*   E_CONFIG_DD_FREE(e->conf_edd);*/

   for (list = clock->faces; list; list = list->next)
     _clock_face_free(list->data);
   evas_list_free(clock->faces);
   free(clock);
}

static E_Menu *
_clock_config_menu_new()
{
   E_Menu *mn;

   mn = e_menu_new();

   return mn;
}

static void
_clock_config_menu_del(E_Menu *menu)
{
   e_object_del(E_OBJECT(menu));
}

static void
_clock_face_init(Clock_Face *face)
{
   Evas_Object *o;
   
   evas_event_freeze(face->evas);
   o = edje_object_add(face->evas);
   face->clock_object = o;

   edje_object_file_set(o,
			/* FIXME: "default.eet" needs to come from conf */
			e_path_find(path_themes, "default.eet"),
			"modules/clock/main");
   evas_object_show(o);
   
   o = evas_object_rectangle_add(face->evas);
   face->event_object = o;
   evas_object_layer_set(o, 2);
   evas_object_repeat_events_set(o, 1);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _clock_face_cb_mouse_down, face);
   evas_object_show(o);

   face->gmc = e_gadman_client_new(face->con->gadman);
   e_gadman_client_domain_set(face->gmc, "module.clock", _clock_count++);
   e_gadman_client_policy_set(face->gmc,
			      E_GADMAN_POLICY_ANYWHERE |
			      E_GADMAN_POLICY_HMOVE |
			      E_GADMAN_POLICY_VMOVE |
			      E_GADMAN_POLICY_HSIZE |
			      E_GADMAN_POLICY_VSIZE);
   e_gadman_client_min_size_set(face->gmc, 4, 4);
   e_gadman_client_max_size_set(face->gmc, 512, 512);
   e_gadman_client_auto_size_set(face->gmc, 64, 64);
   e_gadman_client_align_set(face->gmc, 0.0, 1.0);
   e_gadman_client_aspect_set(face->gmc, 1.0, 1.0);
   e_gadman_client_resize(face->gmc, 64, 64);
   e_gadman_client_change_func_set(face->gmc, _clock_cb_gmc_change, face);
   e_gadman_client_load(face->gmc);

   _clock_face_menu_new(face);
   
   evas_event_thaw(face->evas);
}

static void
_clock_face_free(Clock_Face *face)
{
   e_object_del(E_OBJECT(face->gmc));
   evas_object_del(face->clock_object);
   evas_object_del(face->event_object);
   _clock_face_menu_del(face->menu);
   e_object_unref(E_OBJECT(face->con));
   free(face);
   _clock_count--;
}

static void
_clock_face_enable(Clock_Face *face)
{
   face->enabled = 1;
   evas_object_show(face->clock_object);
   evas_object_show(face->event_object);
}

static void
_clock_face_disable(Clock_Face *face)
{
   face->enabled = 0;
   evas_object_hide(face->clock_object);
   evas_object_hide(face->event_object);
}

static void
_clock_face_menu_new(Clock_Face *face)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   /* FIXME: hook callbacks to each menu item */
   mn = e_menu_new();

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Enabled");
   e_menu_item_check_set(mi, 1);
   if (face->enabled) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _clock_face_cb_menu_enabled, face);

   face->menu = mn;
}

static void
_clock_face_menu_del(E_Menu *menu)
{
   e_object_del(E_OBJECT(menu));
}

static void
_clock_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change)
{
   Clock_Face *face;
   Evas_Coord x, y, w, h;

   face = data;
   if (change == E_GADMAN_CHANGE_MOVE_RESIZE)
     {
	e_gadman_client_geometry_get(face->gmc, &x, &y, &w, &h);
	evas_object_move(face->clock_object, x, y);
	evas_object_move(face->event_object, x, y);
	evas_object_resize(face->clock_object, w, h);
	evas_object_resize(face->event_object, w, h);
     }
   else if (change == E_GADMAN_CHANGE_RAISE)
     {
	evas_object_raise(face->clock_object);
	evas_object_raise(face->event_object);
     }
}

static void
_clock_face_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Clock_Face *face;
   Evas_Event_Mouse_Down *ev;
   
   face = data;
   ev = event_info;
   if (ev->button == 3)
     {
	e_menu_activate_mouse(face->menu, e_zone_current_get(face->con),
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN);
	e_util_container_fake_mouse_up_all_later(face->con);
     }
}

static void
_clock_face_cb_menu_enabled(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Clock_Face *face;
   unsigned char enabled;

   face = data;
   enabled = e_menu_item_toggle_get(mi);
   if ((face->enabled) && (!enabled))
     {  
	_clock_face_disable(face);
     }
   else if ((!face->enabled) && (enabled))
     { 
	_clock_face_enable(face);
     }
   e_menu_item_toggle_set(mi, face->enabled);
}
