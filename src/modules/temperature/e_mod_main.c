/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

/* TODO List:
 *
 * which options should be in main menu, and which in face menu?
 */

/* module private routines */
static Temperature *_temperature_new();
static void     _temperature_free(Temperature *e);
static void     _temperature_config_menu_new(Temperature *e);
static int      _temperature_cb_check(void *data);

static Temperature_Face *_temperature_face_new(E_Container *con);
static void     _temperature_face_free(Temperature_Face *ef);
static void     _temperature_face_enable(Temperature_Face *face);
static void     _temperature_face_disable(Temperature_Face *face);
static void     _temperature_face_menu_new(Temperature_Face *face);
static void     _temperature_face_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change);
static void     _temperature_face_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void     _temperature_face_level_set(Temperature_Face *ef, double level);
static void     _temperature_face_cb_menu_enabled(void *data, E_Menu *m, E_Menu_Item *mi);
static void     _temperature_face_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi);

static E_Config_DD *conf_edd;
static E_Config_DD *conf_face_edd;

static int temperature_count;

/* public module routines. all modules must have these */
void *
e_modapi_init(E_Module *m)
{
   Temperature *e;

   /* check module api version */
   if (m->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show(_("Module API Error"),
			    _("Error initializing Module: Temperature\n"
			      "It requires a minimum module API version of: %i.\n"
			      "The module API advertized by Enlightenment is: %i.\n"
			      "Aborting module."),
			    E_MODULE_API_VERSION,
			    m->api->version);
	return NULL;
     }
   /* actually init temperature */
   e = _temperature_new(m);
   m->config_menu = e->config_menu;
   return e;
}

int
e_modapi_shutdown(E_Module *m)
{
   Temperature *e;

   if (m->config_menu)
     m->config_menu = NULL;

   e = m->data;
   if (e)
     _temperature_free(e);
   return 1;
}

int
e_modapi_save(E_Module *m)
{
   Temperature *e;

   e = m->data;
   e_config_domain_save("module.temperature", conf_edd, e->conf);
   return 1;
}

int
e_modapi_info(E_Module *m)
{
   char buf[4096];

   m->label = strdup(_("Temperature"));
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   return 1;
}

int
e_modapi_about(E_Module *m)
{
   e_error_dialog_show(_("Enlightenment Temperature Module"),
		       _("A module to measure the ACPI Thermal sensor on Linux.\n"
			 "It is especially useful for modern Laptops with high speed\n"
			 "CPUs that generate a lot of heat."));
   return 1;
}

/* module private routines */
static Temperature *
_temperature_new()
{
   Temperature *e;
   Evas_List *managers, *l, *l2, *cl;
   E_Menu_Item *mi;

   temperature_count = 0;
   e = E_NEW(Temperature, 1);
   if (!e) return NULL;

   conf_face_edd = E_CONFIG_DD_NEW("Temperature_Config_Face", Config_Face);
#undef T
#undef D
#define T Config_Face
#define D conf_face_edd
   E_CONFIG_VAL(D, T, enabled, UCHAR);

   conf_edd = E_CONFIG_DD_NEW("Temperature_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, poll_time, DOUBLE);
   E_CONFIG_VAL(D, T, low, INT);
   E_CONFIG_VAL(D, T, high, INT);
   E_CONFIG_LIST(D, T, faces, conf_face_edd);

   e->conf = e_config_domain_load("module.temperature", conf_edd);
   if (!e->conf)
     {
	e->conf = E_NEW(Config, 1);
	e->conf->poll_time = 10.0;
	e->conf->low = 30;
	e->conf->high = 80;
     }
   E_CONFIG_LIMIT(e->conf->poll_time, 0.5, 1000.0);
   E_CONFIG_LIMIT(e->conf->low, 0, 100);
   E_CONFIG_LIMIT(e->conf->high, 0, 200);

   _temperature_config_menu_new(e);
   e->have_temp = -1;
   e->temperature_check_timer = ecore_timer_add(e->conf->poll_time, _temperature_cb_check, e);

   managers = e_manager_list();
   cl = e->conf->faces;
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;

	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     Temperature_Face *ef;

	     con = l2->data;
	     ef = _temperature_face_new(con);
	     if (ef)
	       {
		  e->faces = evas_list_append(e->faces, ef);
		  /* Config */
		  if (!cl)
		    {
		       ef->conf = E_NEW(Config_Face, 1);
		       ef->conf->enabled = 1;
		       e->conf->faces = evas_list_append(e->conf->faces, ef->conf);
		    }
		  else
		    {
		       ef->conf = cl->data;
		       cl = cl->next;
		    }

		  /* Menu */
		  /* This menu must be initialized after conf */
		  _temperature_face_menu_new(ef);

		  /* Add main menu to face menu */
		  mi = e_menu_item_new(ef->menu);
		  e_menu_item_label_set(mi, _("Check Interval"));
		  e_menu_item_submenu_set(mi, e->config_menu_poll);

		  mi = e_menu_item_new(ef->menu);
		  e_menu_item_label_set(mi, _("Low Temperature"));
		  e_menu_item_submenu_set(mi, e->config_menu_low);

		  mi = e_menu_item_new(ef->menu);
		  e_menu_item_label_set(mi, _("High Temperature"));
		  e_menu_item_submenu_set(mi, e->config_menu_high);

		  mi = e_menu_item_new(e->config_menu);
		  e_menu_item_label_set(mi, con->name);

		  e_menu_item_submenu_set(mi, ef->menu);

		  /* Setup */
		  if (!ef->conf->enabled)
		    _temperature_face_disable(ef);
	       }
	  }
     }

   _temperature_cb_check(e);

   return e;
}

static void
_temperature_free(Temperature *e)
{
   Evas_List *l;

   E_CONFIG_DD_FREE(conf_edd);
   E_CONFIG_DD_FREE(conf_face_edd);

   for (l = e->faces; l; l = l->next)
     _temperature_face_free(l->data);
   evas_list_free(e->faces);

   e_object_del(E_OBJECT(e->config_menu));
   e_object_del(E_OBJECT(e->config_menu_poll));
   e_object_del(E_OBJECT(e->config_menu_low));
   e_object_del(E_OBJECT(e->config_menu_high));

   ecore_timer_del(e->temperature_check_timer);

   for (l = e->conf->faces; l; l = l->next)
     free(l->data);
   evas_list_free(e->conf->faces);
   free(e->conf);
   free(e);
}

static void
_temperature_menu_fast(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->poll_time = 1.0;
   ecore_timer_del(e->temperature_check_timer);
   e->temperature_check_timer = ecore_timer_add(e->conf->poll_time, _temperature_cb_check, e);
   e_config_save_queue();
}

static void
_temperature_menu_medium(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->poll_time = 5.0;
   ecore_timer_del(e->temperature_check_timer);
   e->temperature_check_timer = ecore_timer_add(e->conf->poll_time, _temperature_cb_check, e);
   e_config_save_queue();
}

static void
_temperature_menu_normal(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->poll_time = 10.0;
   ecore_timer_del(e->temperature_check_timer);
   e->temperature_check_timer = ecore_timer_add(e->conf->poll_time, _temperature_cb_check, e);
   e_config_save_queue();
}

static void
_temperature_menu_slow(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->poll_time = 30.0;
   ecore_timer_del(e->temperature_check_timer);
   e->temperature_check_timer = ecore_timer_add(e->conf->poll_time, _temperature_cb_check, e);
   e_config_save_queue();
}

static void
_temperature_menu_very_slow(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->poll_time = 60.0;
   ecore_timer_del(e->temperature_check_timer);
   e->temperature_check_timer = ecore_timer_add(e->conf->poll_time, _temperature_cb_check, e);
   e_config_save_queue();
}

static void
_temperature_menu_low_10(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->low = 10;
   _temperature_cb_check(e);
   e_config_save_queue();
}

static void
_temperature_menu_low_20(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->low = 20;
   _temperature_cb_check(e);
   e_config_save_queue();
}

static void
_temperature_menu_low_30(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->low = 30;
   _temperature_cb_check(e);
   e_config_save_queue();
}

static void
_temperature_menu_low_40(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->low = 40;
   _temperature_cb_check(e);
   e_config_save_queue();
}

static void
_temperature_menu_low_50(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->low = 50;
   _temperature_cb_check(e);
   e_config_save_queue();
}

static void
_temperature_menu_high_20(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->high = 20;
   _temperature_cb_check(e);
   e_config_save_queue();
}

static void
_temperature_menu_high_30(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->high = 30;
   _temperature_cb_check(e);
   e_config_save_queue();
}

static void
_temperature_menu_high_40(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->high = 40;
   _temperature_cb_check(e);
   e_config_save_queue();
}

static void
_temperature_menu_high_50(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->high = 50;
   _temperature_cb_check(e);
   e_config_save_queue();
}

static void
_temperature_menu_high_60(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->high = 60;
   _temperature_cb_check(e);
   e_config_save_queue();
}

static void
_temperature_menu_high_70(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->high = 70;
   _temperature_cb_check(e);
   e_config_save_queue();
}

static void
_temperature_menu_high_80(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->high = 80;
   _temperature_cb_check(e);
   e_config_save_queue();
}

static void
_temperature_menu_high_90(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->high = 90;
   _temperature_cb_check(e);
   e_config_save_queue();
}

static void
_temperature_menu_high_100(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;

   e = data;
   e->conf->high = 100;
   _temperature_cb_check(e);
   e_config_save_queue();
}

static void
_temperature_config_menu_new(Temperature *e)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   /* Check interval */
   mn = e_menu_new();

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Fast (1 sec)"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time == 1.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_fast, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Medium (5 sec)"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time == 5.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_medium, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Normal (10 sec)"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time == 10.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_normal, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Slow (30 sec)"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time == 30.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_slow, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Very Slow (60 sec)"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time == 60.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_very_slow, e);

   e->config_menu_poll = mn;

   /* Low temperature */
   mn = e_menu_new();

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("10C"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->low == 10) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_low_10, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("20C"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->low == 20) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_low_20, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("30C"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->low == 30) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_low_30, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("40C"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->low == 40) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_low_40, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("50C"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->low == 50) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_low_50, e);

   e->config_menu_low = mn;

   /* High temperature */
   mn = e_menu_new();

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("20C"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->high == 20) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_high_20, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("30C"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->high == 30) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_high_30, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("40C"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->high == 40) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_high_40, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("50C"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->high == 50) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_high_50, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("60C"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->high == 60) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_high_60, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("70C"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->high == 70) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_high_70, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("80C"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->high == 80) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_high_80, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("90C"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->high == 90) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_high_90, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("100C"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->high == 100) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_high_100, e);

   e->config_menu_high = mn;

   /* Main */
   mn = e_menu_new();

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Check Interval"));
   e_menu_item_submenu_set(mi, e->config_menu_poll);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Low Temperature"));
   e_menu_item_submenu_set(mi, e->config_menu_low);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("High Temperature"));
   e_menu_item_submenu_set(mi, e->config_menu_high);

   e->config_menu = mn;
}

static Temperature_Face *
_temperature_face_new(E_Container *con)
{
   Evas_Object *o;
   Temperature_Face *ef;

   ef = E_NEW(Temperature_Face, 1);
   if (!ef) return NULL;

   ef->con = con;
   e_object_ref(E_OBJECT(con));

   evas_event_freeze(con->bg_evas);
   o = edje_object_add(con->bg_evas);
   ef->temp_object = o;

   e_theme_edje_object_set(o, "base/theme/modules/temperature",
			   "modules/temperature/main");
   evas_object_show(o);

   o = evas_object_rectangle_add(con->bg_evas);
   ef->event_object = o;
   evas_object_layer_set(o, 2);
   evas_object_repeat_events_set(o, 1);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _temperature_face_cb_mouse_down, ef);
   evas_object_show(o);

   ef->gmc = e_gadman_client_new(ef->con->gadman);
   e_gadman_client_domain_set(ef->gmc, "module.temperature", temperature_count++);
   e_gadman_client_policy_set(ef->gmc,
			      E_GADMAN_POLICY_ANYWHERE |
			      E_GADMAN_POLICY_HMOVE |
			      E_GADMAN_POLICY_VMOVE |
			      E_GADMAN_POLICY_HSIZE |
			      E_GADMAN_POLICY_VSIZE);
   e_gadman_client_min_size_set(ef->gmc, 4, 4);
   e_gadman_client_max_size_set(ef->gmc, 128, 128);
   e_gadman_client_auto_size_set(ef->gmc, 40, 40);
   e_gadman_client_align_set(ef->gmc, 1.0, 1.0);
   e_gadman_client_resize(ef->gmc, 40, 40);
   e_gadman_client_change_func_set(ef->gmc, _temperature_face_cb_gmc_change, ef);
   e_gadman_client_load(ef->gmc);

   evas_event_thaw(con->bg_evas);

   return ef;
}

static void
_temperature_face_free(Temperature_Face *ef)
{
   e_object_unref(E_OBJECT(ef->con));
   e_object_del(E_OBJECT(ef->gmc));
   evas_object_del(ef->temp_object);
   evas_object_del(ef->event_object);
   e_object_del(E_OBJECT(ef->menu));

   free(ef);
   temperature_count--;
}

static void
_temperature_face_enable(Temperature_Face *face)
{
   face->conf->enabled = 1;
   evas_object_show(face->temp_object);
   evas_object_show(face->event_object);
   e_config_save_queue();
}

static void
_temperature_face_disable(Temperature_Face *face)
{
   face->conf->enabled = 0;
   evas_object_hide(face->temp_object);
   evas_object_hide(face->event_object);
   e_config_save_queue();
}

static void
_temperature_face_menu_new(Temperature_Face *face)
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
   e_menu_item_callback_set(mi, _temperature_face_cb_menu_enabled, face);
    */
   /* Edit */
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Edit Mode"));
   e_menu_item_callback_set(mi, _temperature_face_cb_menu_edit, face);
}

static void
_temperature_face_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change)
{
   Temperature_Face *ef;
   Evas_Coord x, y, w, h;

   ef = data;
   switch (change)
     {
      case E_GADMAN_CHANGE_MOVE_RESIZE:
	 e_gadman_client_geometry_get(ef->gmc, &x, &y, &w, &h);
	 evas_object_move(ef->temp_object, x, y);
	 evas_object_move(ef->event_object, x, y);
	 evas_object_resize(ef->temp_object, w, h);
	 evas_object_resize(ef->event_object, w, h);
	 break;
      case E_GADMAN_CHANGE_RAISE:
	 evas_object_raise(ef->temp_object);
	 evas_object_raise(ef->event_object);
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
_temperature_face_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Temperature_Face *ef;

   ev = event_info;
   ef = data;
   if (ev->button == 3)
     {
	e_menu_activate_mouse(ef->menu, e_zone_current_get(ef->con),
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN);
	e_util_container_fake_mouse_up_all_later(ef->con);
     }
}

static int
_temperature_cb_check(void *data)
{
   Temperature *ef;
   Temperature_Face *face;
   int ret = 0;
   Ecore_List *therms;
   Evas_List *l;
   int temp = 0;
   char buf[4096];
#ifdef __FreeBSD__
   static int mib[5] = {-1};
   int len;
#endif
   ef = data;
#ifdef __FreeBSD__
   if (mib[0] == -1)
     {
	len = 5;
	sysctlnametomib("hw.acpi.thermal.tz0.temperature", mib, &len);
     }
   
   if (mib[0] != -1)
     {
	len = sizeof(temp);
        if (sysctl(mib, 5, &temp, &len, NULL, 0) != -1)
	  {
	     temp = (temp - 2732) / 10;
	     ret = 1;
	  }
     }
#else  
   therms = ecore_file_ls("/proc/acpi/thermal_zone");
   if (!therms || ecore_list_is_empty(therms))
     {
	FILE *f;

	f = fopen("/sys/devices/temperatures/cpu_temperature", "rb");
	if (f)
	  {
	     fgets(buf, sizeof(buf), f); buf[sizeof(buf) - 1] = 0;
	     if (sscanf(buf, "%i", &temp) == 1)
	       ret = 1;
	     fclose(f);
	  }
	else
	  {
	     therms = ecore_file_ls("/sys/bus/i2c/devices");
	     if ((therms) && (!ecore_list_is_empty(therms)))
	       {
		  char *name;
		  
		  while ((name = ecore_list_next(therms)))
		    {
		       char fname[1024];
		       
		       sprintf(fname, "/sys/bus/i2c/devices/%s/temp1_input",
			       name);
		       if (ecore_file_exists(fname))
			 {
			    FILE *f;
			    
			    f = fopen(fname,"r");
			    if (f) 
			      {
				 fgets(buf, sizeof(buf), f);
				 buf[sizeof(buf) - 1] = 0;
				 
				 /* actuallty read the temp */
				 if (sscanf(buf, "%i", &temp) == 1)
				   ret = 1;
				 /* Hack for temp */
				 temp = temp / 1000;
				 fclose(f);
			      }
			 }
		    }
	       }
	  }
     }
   else
     {
	char *name;
	while ((name = ecore_list_next(therms)))
	  {
	     char units[32];
	     FILE *f;

	     snprintf(buf, sizeof(buf), "/proc/acpi/thermal_zone/%s/temperature", name);
	     f = fopen(buf, "rb");
	     if (f)
	       {
		  fgets(buf, sizeof(buf), f); buf[sizeof(buf) - 1] = 0;
		  units[0] = 0;
		  if (sscanf(buf, "%*[^:]: %i %20s", &temp, units) == 2)
		    ret = 1;
		  fclose(f);
	       }
	     free(name);
	  }
     }
   if (therms) ecore_list_destroy(therms);
#endif   
   if (ret)
     {
	if (ef->have_temp != 1)
	  {
	     /* enable therm object */
	     for (l = ef->faces; l; l = l->next)
	       {
		  face = l->data;
		  edje_object_signal_emit(face->temp_object, "known", "");
	       }
	     ef->have_temp = 1;
	  }

	for (l = ef->faces; l; l = l->next)
	  {
	     face = l->data;
	     _temperature_face_level_set(face,
				    (double)(temp - ef->conf->low) /
				    (double)(ef->conf->high - ef->conf->low));
	     snprintf(buf, sizeof(buf), "%i°C", temp);
	     edje_object_part_text_set(face->temp_object, "reading", buf);
	  }
     }
   else
     {
	if (ef->have_temp != 0)
	  {
	     /* disable therm object */
	     for (l = ef->faces; l; l = l->next)
	       {
		  face = l->data;
		  edje_object_signal_emit(face->temp_object, "unknown", "");
		  edje_object_part_text_set(face->temp_object, "reading", "NO TEMP");
		  _temperature_face_level_set(face, 0.5);
	       }
	     ef->have_temp = 0;
	  }
     }
   return 1;
}

static void
_temperature_face_level_set(Temperature_Face *ef, double level)
{
   Edje_Message_Float msg;

   if (level < 0.0) level = 0.0;
   else if (level > 1.0) level = 1.0;
   msg.val = level;
   edje_object_message_send(ef->temp_object, EDJE_MESSAGE_FLOAT, 1, &msg);
}

static void
_temperature_face_cb_menu_enabled(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature_Face *face;
   unsigned char enabled;

   face = data;
   enabled = e_menu_item_toggle_get(mi);
   if ((face->conf->enabled) && (!enabled))
     {  
	_temperature_face_disable(face);
     }
   else if ((!face->conf->enabled) && (enabled))
     { 
	_temperature_face_enable(face);
     }
}

static void
_temperature_face_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature_Face *face;

   face = data;
   e_gadman_mode_set(face->gmc->gadman, E_GADMAN_MODE_EDIT);
}
