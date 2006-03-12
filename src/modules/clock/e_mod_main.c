/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"

/* TODO List:
 *
 */

/* module private routines */
static Clock  *_clock_new();
static void    _clock_shutdown(Clock *clock);
static void    _clock_config_menu_new(Clock *clock);

static Clock_Face *_clock_face_new(Clock *clock, E_Container *con);
static void    _clock_face_free(Clock_Face *face);
/* static void    _clock_face_enable(Clock_Face *face); */
static void    _clock_face_disable(Clock_Face *face);
static void    _clock_face_menu_new(Clock_Face *face);
static void    _clock_face_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change);
static void    _clock_face_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _clock_face_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi);
static void    _clock_face_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi);

static int _clock_count;

static E_Config_DD *conf_edd;
static E_Config_DD *conf_face_edd;

const int
  DIGITAL_STYLE_NONE = 0,
  DIGITAL_STYLE_12HOUR = 1,
  DIGITAL_STYLE_24HOUR = 2
  ;

/* public module routines. all modules must have these */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Clock"
};

EAPI void *
e_modapi_init(E_Module *module)
{
   Clock *clock;

   /* actually init clock */
   clock = _clock_new();
   module->config_menu = clock->config_menu;
   return clock;
}

EAPI int
e_modapi_shutdown(E_Module *module)
{
   Clock *clock;

   if (module->config_menu)
     module->config_menu = NULL;

   clock = module->data;
   if (clock) 
     {
	_clock_shutdown(clock);
     }
   return 1;
}

EAPI int
e_modapi_save(E_Module *module)
{
   Clock *clock;

   clock = module->data;
   e_config_domain_save("module.clock", conf_edd, clock->conf);
   return 1;
}

EAPI int
e_modapi_info(E_Module *module)
{
   char buf[4096];

   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(module));
   module->icon_file = strdup(buf);
   return 1;
}

EAPI int
e_modapi_about(E_Module *module)
{
   e_module_dialog_show(_("Enlightenment Clock Module"),
			_("A simple module to give E17 a clock."));
   return 1;
}

EAPI int
e_modapi_config(E_Module *m)
{
   Clock *e;
   Clock_Face *face;
   
   e = m->data;
   if (!e) return 0;
   if (!e->faces) return 0;
   face = e->faces->data;
   if (!face) return 0;
   _config_clock_module(face->con, face);
   return 1;
}

/* module private routines */
static Clock *
_clock_new()
{
   Clock *clock;
   Evas_List *managers, *l, *l2, *cl;
   E_Menu_Item *mi;
   
   _clock_count = 0;
   clock = E_NEW(Clock, 1);
   if (!clock) return NULL;

   conf_face_edd = E_CONFIG_DD_NEW("Clock_Config_Face", Config_Face);
#undef T
#undef D
#define T Config_Face
#define D conf_face_edd
   E_CONFIG_VAL(D, T, enabled, UCHAR);
   E_CONFIG_VAL(D, T, digitalStyle, INT);

   conf_edd = E_CONFIG_DD_NEW("Clock_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_LIST(D, T, faces, conf_face_edd);

   clock->conf = e_config_domain_load("module.clock", conf_edd);
   if (!clock->conf)
     {
	clock->conf = E_NEW(Config, 1);
     }

   _clock_config_menu_new(clock);

   managers = e_manager_list();
   cl = clock->conf->faces;
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;

	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     Clock_Face *face;

	     con = l2->data;
	     face = _clock_face_new(clock, con);
	     if (face)
	       {
		  face->clock = clock;
		  clock->faces = evas_list_append(clock->faces, face);
		  /* Config */
		  if (!cl)
		    {
		       face->conf = E_NEW(Config_Face, 1);
		       face->conf->enabled = 1;
                       face->conf->digitalStyle = DIGITAL_STYLE_NONE;
		       clock->conf->faces = evas_list_append(clock->conf->faces, face->conf);
		    }
		  else
		    {
		       face->conf = cl->data;
		       cl = cl->next;
		    }

			_clock_face_cb_config_updated(face);

		  /* Menu */
		  /* This menu must be initialized after conf */
		  _clock_face_menu_new(face);

		  mi = e_menu_item_new(clock->config_menu);
		  e_menu_item_label_set(mi, _("Configuration"));
		  e_util_menu_item_edje_icon_set(mi, "enlightenment/configuration");		  
		  e_menu_item_callback_set(mi, _clock_face_cb_menu_configure, face);

		  mi = e_menu_item_new(clock->config_menu);
		  e_menu_item_label_set(mi, con->name);

		  e_menu_item_submenu_set(mi, face->menu);

		  /* Setup */
		  if (!face->conf->enabled)
		    _clock_face_disable(face);
	       }
	  }
     }
   return clock;
}

static void
_clock_shutdown(Clock *clock)
{
   Evas_List *list;

   E_CONFIG_DD_FREE(conf_edd);
   E_CONFIG_DD_FREE(conf_face_edd);

   for (list = clock->faces; list; list = list->next)
     _clock_face_free(list->data);
   evas_list_free(clock->faces);

   e_object_del(E_OBJECT(clock->config_menu));

   evas_list_free(clock->conf->faces);
   free(clock->conf);
   free(clock);
}

static void
_clock_config_menu_new(Clock *clock)
{
   clock->config_menu = e_menu_new();
}

static Clock_Face *
_clock_face_new(Clock *clock, E_Container *con)
{
   Clock_Face *face;
   Evas_Object *o;
   Evas_Coord x, y, w, h;
   E_Gadman_Policy  policy;

   face = E_NEW(Clock_Face, 1);
   if (!face) return NULL;

   face->con = con;
   e_object_ref(E_OBJECT(con));

   evas_event_freeze(con->bg_evas);
   o = edje_object_add(con->bg_evas);
   face->clock_object = o;

   e_theme_edje_object_set(o, "base/theme/modules/clock",
			   "modules/clock/main");
   evas_object_show(o);
   
   o = evas_object_rectangle_add(con->bg_evas);
   face->event_object = o;
   evas_object_layer_set(o, 2);
   evas_object_repeat_events_set(o, 1);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _clock_face_cb_mouse_down, face);
   evas_object_show(o);

   evas_object_resize(face->clock_object, 200, 200);
   edje_object_calc_force(face->clock_object);
   edje_object_part_geometry_get(face->clock_object, "main", &x, &y, &w, &h);
   face->inset.l = x;
   face->inset.r = 200 - (x + w);
   face->inset.t = y;
   face->inset.b = 200 - (y + h);

   face->gmc = e_gadman_client_new(con->gadman);
   e_gadman_client_domain_set(face->gmc, "module.clock", _clock_count++);

   policy = E_GADMAN_POLICY_ANYWHERE |
	    E_GADMAN_POLICY_HMOVE |
	    E_GADMAN_POLICY_VMOVE |
	    E_GADMAN_POLICY_HSIZE |
	    E_GADMAN_POLICY_VSIZE;

   e_gadman_client_policy_set(face->gmc, policy);
   e_gadman_client_min_size_set(face->gmc, 4, 4);
   e_gadman_client_max_size_set(face->gmc, 512, 512);
   e_gadman_client_auto_size_set(face->gmc,
				 40 + (face->inset.l + face->inset.r),
				 40 + (face->inset.t + face->inset.b));
   e_gadman_client_align_set(face->gmc, 1.0, 1.0);
   e_gadman_client_aspect_set(face->gmc, 1.0, 1.0);
   e_gadman_client_padding_set(face->gmc,
			       face->inset.l, face->inset.r,
			       face->inset.t, face->inset.b);
   e_gadman_client_resize(face->gmc,
			  40 + (face->inset.l + face->inset.r),
			  40 + (face->inset.t + face->inset.b));
   e_gadman_client_change_func_set(face->gmc, _clock_face_cb_gmc_change, face);
   e_gadman_client_load(face->gmc);

   evas_event_thaw(con->bg_evas);

   return face;
}

static void
_clock_face_free(Clock_Face *face)
{
   if (face->config_dialog) 
     {
	e_object_del(E_OBJECT(face->config_dialog));
	face->config_dialog = NULL;
     }
   e_object_unref(E_OBJECT(face->con));
   e_object_del(E_OBJECT(face->gmc));
   evas_object_del(face->clock_object);
   evas_object_del(face->event_object);
   e_object_del(E_OBJECT(face->menu));

   free(face->conf);
   free(face);
   _clock_count--;
}

/*
static void
_clock_face_enable(Clock_Face *face)
{
   face->conf->enabled = 1;
   evas_object_show(face->clock_object);
   evas_object_show(face->event_object);
   e_config_save_queue();
}
*/

static void
_clock_face_disable(Clock_Face *face)
{
   face->conf->enabled = 0;
   evas_object_hide(face->clock_object);
   evas_object_hide(face->event_object);
   e_config_save_queue();
}

static void
_clock_face_menu_new(Clock_Face *face)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   mn = e_menu_new();
   face->menu = mn;

   /* Enabled */
   /*
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Enabled"));
   e_menu_item_check_set(mi, 1);
   if (face->conf->enabled) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _clock_face_cb_menu_enabled, face);
    */

   /* Config */
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Configuration"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/configuration");   
   e_menu_item_callback_set(mi, _clock_face_cb_menu_configure, face);

   /* Edit */
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Edit Mode"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/gadgets");
   e_menu_item_callback_set(mi, _clock_face_cb_menu_edit, face);
}

static void
_clock_face_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change)
{
   Clock_Face *face;
   Evas_Coord x, y, w, h;

   face = data;
   switch (change)
     {
      case E_GADMAN_CHANGE_MOVE_RESIZE:
	e_gadman_client_geometry_get(face->gmc, &x, &y, &w, &h);
	evas_object_move(face->clock_object, x, y);
	evas_object_move(face->event_object, x, y);
	evas_object_resize(face->clock_object, w, h);
	evas_object_resize(face->event_object, w, h);
	break;
      case E_GADMAN_CHANGE_RAISE:
	evas_object_raise(face->clock_object);
	evas_object_raise(face->event_object);
	break;
      case E_GADMAN_CHANGE_EDGE:
      case E_GADMAN_CHANGE_ZONE:
	 /* FIXME
	  * Must we do something here?
	  */
	break;
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
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
	e_util_container_fake_mouse_up_all_later(face->con);
     }
}

static void
_clock_face_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Clock_Face *face;

   face = data;
   e_gadman_mode_set(face->gmc->gadman, E_GADMAN_MODE_EDIT);
}

static void
_clock_face_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Clock_Face *face;

   face = data;
   if (!face) return;
   _config_clock_module(face->con, face);
}

void
_clock_face_cb_config_updated(void *data)
{
   Clock_Face *face;
   char buf[2];

   face = data;

   memset(buf, 0, sizeof(buf));
   snprintf(buf, sizeof(buf), "%i", face->conf->digitalStyle);
   edje_object_part_text_set(face->clock_object, "digitalStyle", buf);

}

