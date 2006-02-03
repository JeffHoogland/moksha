/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
//#include "e_int_menus.h"

static Start *_start_new(void);
static Start_Face *_start_face_new(Start *s, E_Container *con);
static void _start_free(Start *e);
static void _start_face_free(Start_Face *face);
static void _start_face_disable(Start_Face *e);
static void _start_config_menu_new(Start *e);
static void _start_face_menu_new(Start_Face *face);
static void _start_face_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi);
static void _start_face_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change);
static void _start_face_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _start_menu_cb_post_deactivate(void *data, E_Menu *m);

static void _start_face_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi);
static void _start_cb_update_policy(void *data);

static int button_count;
static E_Config_DD *conf_edd;
static E_Config_DD *conf_face_edd;

EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION,
     "Start"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   Start *e;
   
   /* Create the button */
   e = _start_new();
   m->config_menu = e->config_menu;
   return e;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   Start *e;

   if (m->config_menu) m->config_menu = NULL;
   
   e = m->data;
   if (e) _start_free(e);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   Start *e;
   
   e = m->data;
   e_config_domain_save("module.start", conf_edd, e->conf);
   
   return 1;
}

EAPI int
e_modapi_info(E_Module *m)
{
   char buf[4096];
   
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   return 1;
}

EAPI int
e_modapi_about(E_Module *m)
{
   e_module_dialog_show(_("Enlightenment Start Module"),
			_("Experimental Button module for E17"));
   return 1;
}

int
e_modapi_config(E_Module *m)
{
   Start *s;
   Start_Face *face;

   s = m->data;
   if (!s) return 0;
   if (!s->faces) return 0;
  
   face = s->faces->data;
   if (!face) return 0;
   
   _config_start_module(e_container_current_get(e_manager_current_get()), s);
   return 1;
}

static Start *
_start_new(void)
{
   Start *e;
   Evas_List *managers, *l, *l2, *cl;
   E_Menu_Item *mi;
   
   button_count = 0;
   e = E_NEW(Start, 1);
   if (!e) return NULL;
   
   conf_face_edd = E_CONFIG_DD_NEW("Start_Config_Face", Config_Face);
#undef T
#undef D
#define T Config_Face
#define D conf_face_edd
   E_CONFIG_VAL(D, T, enabled, UCHAR);

   conf_edd = E_CONFIG_DD_NEW("Start_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_LIST(D, T, faces, conf_face_edd);
   E_CONFIG_VAL(D, T, allow_overlap, INT);
   
   e->conf = e_config_domain_load("module.start", conf_edd);
   if (!e->conf)
     {
	e->conf = E_NEW(Config, 1);
	e->conf->allow_overlap = 0;
     }
   E_CONFIG_LIMIT(e->conf->allow_overlap, 0, 1);
   
   _start_config_menu_new(e);
   
   managers = e_manager_list();
   cl = e->conf->faces;
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     Start_Face *face;
	     
	     con = l2->data;
	     face = _start_face_new(e, con);
	     if (face)
	       {
		  face->start = e;
		  e->faces = evas_list_append(e->faces, face);
		  /* Config */
		  if (!cl)
		    {
		       face->conf = E_NEW(Config_Face, 1);
		       face->conf->enabled = 1;
		       e->conf->faces = evas_list_append(e->conf->faces, face->conf);
		    }
		  else
		    {
		       face->conf = cl->data;
		       cl = cl->next;
		    }
		  
		  /* Menu */
		  /* This menu must be initialized after conf */
		  _start_face_menu_new(face);

		  mi = e_menu_item_new(e->config_menu);
		  e_menu_item_label_set(mi, _("Configuration"));
		  e_menu_item_callback_set(mi, _start_face_cb_menu_configure, face);
		  
		  mi = e_menu_item_new(e->config_menu);
		  e_menu_item_label_set(mi, con->name);
		  
		  e_menu_item_submenu_set(mi, face->menu);
		  
		  /* Setup */
		  if (!face->conf->enabled) _start_face_disable(face);
	       }
	  }
     }
   return e;
}

static Start_Face *
_start_face_new(Start *s, E_Container *con)
{
   Start_Face *face;
   Evas_Object *o;
   E_Gadman_Policy policy;
   
   face = E_NEW(Start_Face, 1);
   if (!face) return NULL;
   
   face->con = con;
   e_object_ref(E_OBJECT(con));
   
   evas_event_freeze(con->bg_evas);
   o = edje_object_add(con->bg_evas);
   face->button_object = o;
   
   e_theme_edje_object_set(o, "base/theme/modules/start", "modules/start/main");
   edje_object_signal_emit(o, "passive", "");
   evas_object_show(o);
   
   o = evas_object_rectangle_add(con->bg_evas);
   face->event_object = o;
   evas_object_layer_set(o, 2);
   evas_object_repeat_events_set(o, 1);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _start_face_cb_mouse_down, face);
   evas_object_show(o);
   
   face->gmc = e_gadman_client_new(con->gadman);
   e_gadman_client_domain_set(face->gmc, "module.start", button_count++);

   policy = E_GADMAN_POLICY_ANYWHERE |
           E_GADMAN_POLICY_HMOVE |
           E_GADMAN_POLICY_VMOVE |
           E_GADMAN_POLICY_HSIZE |
           E_GADMAN_POLICY_VSIZE;

   if (s->conf->allow_overlap == 0)
     policy &= ~E_GADMAN_POLICY_ALLOW_OVERLAP;
   else
     policy |= E_GADMAN_POLICY_ALLOW_OVERLAP;

   e_gadman_client_policy_set(face->gmc, policy);

   e_gadman_client_min_size_set(face->gmc, 4, 4);
   e_gadman_client_max_size_set(face->gmc, 512, 512);
   e_gadman_client_auto_size_set(face->gmc, 40, 40);
   e_gadman_client_align_set(face->gmc, 0.0, 1.0);
   e_gadman_client_aspect_set(face->gmc, 1.0, 1.0);
   e_gadman_client_resize(face->gmc, 40, 40);
   e_gadman_client_change_func_set(face->gmc, _start_face_cb_gmc_change, face);
   e_gadman_client_load(face->gmc);
   
   evas_event_thaw(con->bg_evas);
   
   return face;
}

static void
_start_face_menu_new(Start_Face *face)
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
   
   /* Edit */
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Configuration"));
   e_menu_item_callback_set(mi, _start_face_cb_menu_configure, face);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Edit Mode"));
   e_menu_item_callback_set(mi, _start_face_cb_menu_edit, face);
   
}

static void
_start_free(Start *e)
{
   Evas_List *list;
   
   E_CONFIG_DD_FREE(conf_edd);
   E_CONFIG_DD_FREE(conf_face_edd);
   
   for (list = e->faces; list; list = list->next)
     _start_face_free(list->data);
   evas_list_free(e->faces);
   
   e_object_del(E_OBJECT(e->config_menu));
   
   evas_list_free(e->conf->faces);
   free(e->conf);
   free(e);
}

static void
_start_face_free(Start_Face *face)
{
   e_object_unref(E_OBJECT(face->con));
   e_object_del(E_OBJECT(face->gmc));
   evas_object_del(face->button_object);
   evas_object_del(face->event_object);
   e_object_del(E_OBJECT(face->menu));
   if (face->main_menu) e_object_del(E_OBJECT(face->main_menu));

   free(face->conf);
   free(face);
   button_count--;
}

static void
_start_config_menu_new(Start *e)
{
   e->config_menu = e_menu_new();
}

static void
_start_face_disable(Start_Face *e)
{
   e->conf->enabled = 0;
   evas_object_hide(e->button_object);
   evas_object_hide(e->event_object);
   e_config_save_queue();
}

static void
_start_face_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change)
{
   Start_Face *e;
   Evas_Coord x, y, w, h;
   
   e = data;
   switch (change)
     {
      case E_GADMAN_CHANGE_MOVE_RESIZE:
	e_gadman_client_geometry_get(e->gmc, &x, &y, &w, &h);
	evas_object_move(e->button_object, x, y);
	evas_object_move(e->event_object, x, y);
	evas_object_resize(e->button_object, w, h);
	evas_object_resize(e->event_object, w, h);
	break;
      case E_GADMAN_CHANGE_RAISE:
	evas_object_raise(e->button_object);
	evas_object_raise(e->event_object);
	break;
      case E_GADMAN_CHANGE_EDGE:
      case E_GADMAN_CHANGE_ZONE:
	/* FIXME: Must we do something here? */
	break;
     }
}

static void
_start_face_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Start_Face *face;
   
   face = data;
   e_gadman_mode_set(face->gmc->gadman, E_GADMAN_MODE_EDIT);
}

static void
_start_face_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Start_Face *face;
   Evas_Event_Mouse_Down *ev;
   
   face = data;
   ev = event_info;
   if (ev->button == 3)
     {
	e_menu_activate_mouse(face->menu, e_zone_current_get(face->con),
			      ev->output.x, ev->output.y, 1, 1, 
			      E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
	e_util_container_fake_mouse_up_all_later(face->con);
     }
   else if (ev->button == 1)
     {
	Evas_Coord x, y, w, h;
	
        e_gadman_client_geometry_get(face->gmc, &x, &y, &w, &h);
	if (!face->main_menu)
	  face->main_menu = e_int_menus_main_new();
	e_menu_post_deactivate_callback_set(face->main_menu, _start_menu_cb_post_deactivate, face);
	e_menu_activate_mouse(face->main_menu, e_zone_current_get(face->con),
			      x, y, w, h,
			      E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
	e_util_container_fake_mouse_up_all_later(face->con);
	edje_object_signal_emit(face->button_object, "active", "");
     }
}

static void
_start_menu_cb_post_deactivate(void *data, E_Menu *m)
{
   Start_Face *face;
   
   face = data;
   if (!face->main_menu) return;
   edje_object_signal_emit(face->button_object, "passive", "");
   e_object_del(E_OBJECT(face->main_menu));
   face->main_menu = NULL;
}

void 
_start_cb_config_updated(void *data)
{
   _start_cb_update_policy(data);
}

void
_start_face_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Start_Face *face;

   face = (Start_Face*)data;
   if (!face) return;
   _config_start_module(face->con, face->start);
}

static void 
_start_cb_update_policy(void *data)
{
   Start     *s;
   Start_Face *sf;
   Evas_List   *l;
   E_Gadman_Policy policy;

   s = data;
   for (l = s->faces; l; l = l->next)
     {
	sf = l->data;
	policy = sf->gmc->policy;

	if (s->conf->allow_overlap == 0)
	  policy &= E_GADMAN_POLICY_ALLOW_OVERLAP;
	else
	  policy |= E_GADMAN_POLICY_ALLOW_OVERLAP;
	e_gadman_client_policy_set(sf->gmc , policy);
     }
}

