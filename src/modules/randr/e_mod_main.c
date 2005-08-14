/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

static Randr *_randr_new(void);
static Randr_Face *_randr_face_new(E_Container *con);
static void _randr_free(Randr *e);
static void _randr_face_free(Randr_Face *face);
static void _randr_face_enable(Randr_Face *e);
static void _randr_face_disable(Randr_Face *e);
static void _randr_config_menu_new(Randr *e);
static void _randr_face_menu_new(Randr_Face *face);
static void _randr_face_menu_resolution_new(Randr_Face *face);
static void _randr_face_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi);
static void _randr_face_post_cb_menu_resolution_change(void *data, E_Menu *m);
static void _randr_face_cb_menu_resolution_change(void *data, E_Menu *m, E_Menu_Item *mi);
static void _randr_face_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change);
static void _randr_face_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static int  _randr_face_cb_screen_change(void *data, int type, void *event);

static int button_count;
static E_Config_DD *conf_edd;
static E_Config_DD *conf_face_edd;

void *
e_modapi_init(E_Module *m)
{
   Randr *e;
   
   if (m->api->version < E_MODULE_API_VERSION) {
      e_error_dialog_show(_("Module API Error"),
			  _("Error initializing Module: randr\n"
			    "It requires a minimum module API version of: %i.\n"
			    "The module API advertized by Enlightenment is: %i.\n"
			    "Aborting module."),
			  E_MODULE_API_VERSION,
			  m->api->version);
      return NULL;
   }
   
   /* Create the button */
   e = _randr_new();
   m->config_menu = e->config_menu;
   return e;
}

int
e_modapi_shutdown(E_Module *m)
{
   Randr *e;

   if (m->config_menu) m->config_menu = NULL;
   
   e = m->data;
   if (e) _randr_free(e);
   return 1;
}

int
e_modapi_save(E_Module *m)
{
   Randr *e;
   
   e = m->data;
   e_config_domain_save("module.randr", conf_edd, e->conf);
   
   return 1;
}

int
e_modapi_info(E_Module *m)
{
   char buf[4096];
   
   m->label = strdup(_("Randr"));
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   return 1;
}

int
e_modapi_about(E_Module *m)
{
   e_error_dialog_show(_("Enlightenment Randr Module"),
		       _("Module to change screen resolution for E17"));
   return 1;
}

static Randr *
_randr_new(void)
{
   Randr *e;
   Evas_List *managers, *l, *l2, *cl;
   E_Menu_Item *mi;
   
   button_count = 0;
   e = E_NEW(Randr, 1);
   if (!e) return NULL;
   
   conf_face_edd = E_CONFIG_DD_NEW("Randr_Config_Face", Config_Face);
#undef T
#undef D
#define T Config_Face
#define D conf_face_edd
   E_CONFIG_VAL(D, T, enabled, UCHAR);

   conf_edd = E_CONFIG_DD_NEW("Randr_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_LIST(D, T, faces, conf_face_edd);
   
   e->conf = e_config_domain_load("module.randr", conf_edd);
   if (!e->conf) e->conf = E_NEW(Config, 1);
   
   _randr_config_menu_new(e);
   
   managers = e_manager_list();
   cl = e->conf->faces;
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     Randr_Face *face;
	     
	     con = l2->data;
	     face = _randr_face_new(con);
	     if (face)
	       {
		  ecore_x_randr_events_select(con->bg_win, 1);
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
		  _randr_face_menu_new(face);
		  
		  mi = e_menu_item_new(e->config_menu);
		  e_menu_item_label_set(mi, con->name);
		  
		  e_menu_item_submenu_set(mi, face->config_menu);
		  
		  /* Setup */
		  if (!face->conf->enabled) _randr_face_disable(face);
	       }
	  }
     }
   return e;
}

static Randr_Face *
_randr_face_new(E_Container *con)
{
   Randr_Face *face;
   Evas_Object *o;
   
   face = E_NEW(Randr_Face, 1);
   if (!face) return NULL;
   
   face->con = con;
   e_object_ref(E_OBJECT(con));
   
   evas_event_freeze(con->bg_evas);
   o = edje_object_add(con->bg_evas);
   face->button_object = o;
   
   e_theme_edje_object_set(o, "base/theme/modules/randr", "modules/randr/main");
   edje_object_signal_emit(o, "passive", "");
   evas_object_show(o);
   
   o = evas_object_rectangle_add(con->bg_evas);
   face->event_object = o;
   evas_object_layer_set(o, 2);
   evas_object_repeat_events_set(o, 1);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _randr_face_cb_mouse_down, face);
   evas_object_show(o);
   
   face->gmc = e_gadman_client_new(con->gadman);
   e_gadman_client_domain_set(face->gmc, "module.randr", button_count++);
   e_gadman_client_policy_set(face->gmc,
			      E_GADMAN_POLICY_ANYWHERE |
			      E_GADMAN_POLICY_HMOVE |
			      E_GADMAN_POLICY_VMOVE |
			      E_GADMAN_POLICY_HSIZE |
			      E_GADMAN_POLICY_VSIZE);
   e_gadman_client_min_size_set(face->gmc, 4, 4);
   e_gadman_client_max_size_set(face->gmc, 512, 512);
   e_gadman_client_auto_size_set(face->gmc, 40, 40);
   e_gadman_client_align_set(face->gmc, 0.0, 1.0);
   e_gadman_client_aspect_set(face->gmc, 1.0, 1.0);
   e_gadman_client_resize(face->gmc, 40, 40);
   e_gadman_client_change_func_set(face->gmc, _randr_face_cb_gmc_change, face);
   e_gadman_client_load(face->gmc);
   
   evas_event_thaw(con->bg_evas);
   
   face->randr_handler = ecore_event_handler_add(ECORE_X_EVENT_SCREEN_CHANGE,
						 _randr_face_cb_screen_change,
						 face);
   return face;
}

static void
_randr_face_menu_new(Randr_Face *face)
{
   E_Menu *mn;
   E_Menu_Item *mi;
 
   mn = e_menu_new();
   face->config_menu = mn;
   
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
   e_menu_item_label_set(mi, _("Edit Mode"));
   e_menu_item_callback_set(mi, _randr_face_cb_menu_edit, face);
}

static void
_randr_face_menu_resolution_new(Randr_Face *face)
{
   E_Menu *mn;
   E_Menu_Item *mi;
   Ecore_X_Screen_Size *sizes;
   Ecore_X_Screen_Size size;
   int i, n;

   mn = e_menu_new();
   face->resolution_menu = mn;

   sizes = ecore_x_randr_screen_sizes_get(face->con->manager->root, &n);
   size = ecore_x_randr_current_screen_size_get(face->con->manager->root);
   if (sizes)
     {
	char buf[16];

	for (i = 0; i < n; i++)
	  {
	     snprintf(buf, sizeof(buf), "%dx%d", sizes[i].width, sizes[i].height);
	     mi = e_menu_item_new(mn);
	     e_menu_item_radio_set(mi, 1);
	     e_menu_item_radio_group_set(mi, 1);
	     if ((sizes[i].width == size.width) && (sizes[i].height == size.height))
	       e_menu_item_toggle_set(mi, 1);
	     e_menu_item_label_set(mi, buf);
	     e_menu_item_callback_set(mi, _randr_face_cb_menu_resolution_change, face);
	  }
	free(sizes);
     }
}

static void
_randr_free(Randr *e)
{
   Evas_List *list;
   
   E_CONFIG_DD_FREE(conf_edd);
   E_CONFIG_DD_FREE(conf_face_edd);
   
   for (list = e->faces; list; list = list->next)
     _randr_face_free(list->data);
   evas_list_free(e->faces);
   
   e_object_del(E_OBJECT(e->config_menu));
   
   evas_list_free(e->conf->faces);
   free(e->conf);
   free(e);
}

static void
_randr_face_free(Randr_Face *face)
{
   e_object_unref(E_OBJECT(face->con));
   e_object_del(E_OBJECT(face->gmc));
   evas_object_del(face->button_object);
   evas_object_del(face->event_object);
   e_object_del(E_OBJECT(face->config_menu));
   e_object_del(E_OBJECT(face->resolution_menu));
   ecore_event_handler_del(face->randr_handler);

   free(face->conf);
   free(face);
   button_count--;
}

static void
_randr_config_menu_new(Randr *e)
{
   e->config_menu = e_menu_new();
}

static void
_randr_face_disable(Randr_Face *e)
{
   e->conf->enabled = 0;
   evas_object_hide(e->button_object);
   evas_object_hide(e->event_object);
   e_config_save_queue();
}

static void
_randr_face_enable(Randr_Face *e)
{
   e->conf->enabled = 1;
   evas_object_show(e->button_object);
   evas_object_show(e->event_object);
   e_config_save_queue();
}

static void
_randr_face_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change)
{
   Randr_Face *e;
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
_randr_face_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Randr_Face *face;
   
   face = data;
   e_gadman_mode_set(face->gmc->gadman, E_GADMAN_MODE_EDIT);
}

static void
_randr_face_post_cb_menu_resolution_change(void *data, E_Menu *m)
{
   Randr_Face *face;
   
   face = data;
   e_object_del(E_OBJECT(face->resolution_menu));
   face->resolution_menu = NULL;
   edje_object_signal_emit(face->button_object, "passive", "");
}

static void
_randr_face_cb_menu_resolution_change(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Randr_Face *face;
   Ecore_X_Screen_Size size;
   
   face = data;
   if (sscanf(mi->label, "%dx%d", &size.width, &size.height) != 2) return;
   ecore_x_randr_screen_size_set(face->con->manager->root, size);
}

static void
_randr_face_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Randr_Face *face;
   Evas_Event_Mouse_Down *ev;
   
   face = data;
   ev = event_info;
   if (ev->button == 3)
     {
	e_menu_activate_mouse(face->config_menu, e_zone_current_get(face->con),
			      ev->output.x, ev->output.y, 1, 1, 
			      E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
	e_util_container_fake_mouse_up_all_later(face->con);
     }
   else if (ev->button == 1)
     {
	Evas_Coord x, y, w, h;

	_randr_face_menu_resolution_new(face);
	e_menu_post_deactivate_callback_set(face->resolution_menu,
					    _randr_face_post_cb_menu_resolution_change,
					    face);
        e_gadman_client_geometry_get(face->gmc, &x, &y, &w, &h);
	e_menu_activate_mouse(face->resolution_menu, e_zone_current_get(face->con),
			      x, y, w, h,
			      E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
	e_util_container_fake_mouse_up_all_later(face->con);
	edje_object_signal_emit(face->button_object, "active", "");
     }
}

static int
_randr_face_cb_screen_change(void *data, int type, void *event)
{
   Randr_Face *face;
   Ecore_X_Event_Screen_Change *ev;

   face = data;
   ev = event;
   if (ev->win != face->con->bg_win) return 1;
   printf("res: %dx%d\n", ev->width, ev->height);
   return 1;
}
