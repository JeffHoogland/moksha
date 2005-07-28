/*
 *  * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"
#include "e_int_menus.h"

static DesktopName *_dn_new(void);
static E_Menu      *_dn_config_menu_new(DesktopName *dn);
static void        _dn_free(DesktopName *dn);
static int         _dn_cb_event_desk_show(void *data, int type, void *event);
static int         _dn_popup_timeout_cb(void *data);

static void _dn_config_speed_set(DesktopName *dn, double speed);
static void _dn_menu_cb_speed_very_slow(void *data, E_Menu *m, E_Menu_Item *mi);
static void _dn_menu_cb_speed_slow(void *data, E_Menu *m, E_Menu_Item *mi);
static void _dn_menu_cb_speed_normal(void *data, E_Menu *m, E_Menu_Item *mi);
static void _dn_menu_cb_speed_fast(void *data, E_Menu *m, E_Menu_Item *mi);
static void _dn_menu_cb_speed_very_fast(void *data, E_Menu *m, E_Menu_Item *mi);

static Ecore_Timer *timeout_timer = NULL;

 void *
e_modapi_init(E_Module *m)
{
   DesktopName *dn;
   
   if (m->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show(_("Module API Error"),
			    _("Error initializing Module: desktop\n"
			      "It requires a minimum module API version of: %i.\n"
			      "The module API advertized by Enlightenment is: %i.\n"
			      "Aborting module."),
			    E_MODULE_API_VERSION,
			    m->api->version);
	return NULL;
     }
   
   dn = _dn_new();
   m->config_menu = _dn_config_menu_new(dn);
   return dn;
}

int
e_modapi_shutdown(E_Module *m)
{
   DesktopName *dn;
   dn = m->data;
   if (dn)
     {
	if (m->config_menu)
	  {
	     e_menu_deactivate(m->config_menu);
	     e_object_del(E_OBJECT(m->config_menu));
	     m->config_menu = NULL;
	  }
	_dn_free(dn);
     }
   return 1;
}

int
e_modapi_save(E_Module *m)
{
   DesktopName *dn;
   dn = m->data;
   e_config_domain_save("module.desktopname", dn->conf_edd, dn->conf);
   return 1;
}

int
e_modapi_info(E_Module *m)
{
   char buf[4096];
   
   m->label = strdup(_("DesktopName"));
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   return 1;
}

int
e_modapi_about(E_Module *m)
{
   e_error_dialog_show(_("Enlightenment DesktopName Module"),
		       _("Experimental module for E17: display desktop name on screen."));
   return 1;
}

static DesktopName *
_dn_new(void)
{
   DesktopName *dn;

   Evas_List *managers, *l, *l2;
   E_Manager *man;
   E_Container *con;

   dn = E_NEW(DesktopName, 1);
   if (!dn) return NULL;

   dn->conf_edd = E_CONFIG_DD_NEW("DesktopName_Config", Config);
#undef T
#undef D
#define T Config
#define D dn->conf_edd
   E_CONFIG_VAL(D, T, speed, DOUBLE);

   dn->conf = e_config_domain_load("module.desktopname", dn->conf_edd);
   if (!dn->conf) 
     {
	dn->conf = E_NEW(Config, 1);
	dn->conf->speed = 1.2;
     }
   E_CONFIG_LIMIT(dn->conf->speed, 0.2, 10.0);
   
   dn->ev_handler_desk_show = ecore_event_handler_add(E_EVENT_DESK_SHOW,
						      _dn_cb_event_desk_show,
						      dn);
   return dn;
}

static E_Menu *
_dn_config_menu_new(DesktopName *dn)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   mn = e_menu_new();
   if(!mn) return NULL;


   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Very Slow"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (dn->conf->speed == 6.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _dn_menu_cb_speed_very_slow, dn);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Slow"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (dn->conf->speed == 4.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _dn_menu_cb_speed_slow, dn);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Normal"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (dn->conf->speed == 1.2) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _dn_menu_cb_speed_normal, dn);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Fast"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (dn->conf->speed == 0.7) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _dn_menu_cb_speed_fast, dn);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Very Fast"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (dn->conf->speed == 0.3) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _dn_menu_cb_speed_very_fast, dn);
   
   return mn;
}

static void
_dn_free(DesktopName *dn)
{
   free(dn->conf);
   E_CONFIG_DD_FREE(dn->conf_edd);
   ecore_event_handler_del(dn->ev_handler_desk_show);
   if (dn->obj)
     {
	evas_object_del(dn->obj);
	dn->obj = NULL;
     }
   if (dn->popup)
     {
	e_object_del(E_OBJECT(dn->popup));
	dn->popup = NULL;
     }
   free(dn);
}
   
static void
_dn_menu_cb_speed_very_slow(void *data, E_Menu *m, E_Menu_Item *mi)
{
   DesktopName	*dn;
   dn = data;
   _dn_config_speed_set(dn, 6.0);
}

static void
_dn_menu_cb_speed_slow(void *data, E_Menu *m, E_Menu_Item *mi)
{
   DesktopName	*dn;
   dn = data;
   _dn_config_speed_set(dn, 4.0);
}

static void
_dn_menu_cb_speed_normal(void *data, E_Menu *m, E_Menu_Item *mi)
{
   DesktopName	*dn;
   dn = data;
   _dn_config_speed_set(dn, 1.2);
}

static void
_dn_menu_cb_speed_fast(void *data, E_Menu *m, E_Menu_Item *mi)
{
   DesktopName	*dn;
   
   dn = data;
   _dn_config_speed_set(dn, 0.7);
}

static void
_dn_menu_cb_speed_very_fast(void *data, E_Menu *m, E_Menu_Item *mi)
{
   DesktopName	*dn;
   
   dn = data;
   _dn_config_speed_set(dn, 0.3);
}

static void
_dn_config_speed_set(DesktopName *dn, double speed)
{
   if (speed < 0.0) speed = 1.2;
   dn->conf->speed = speed;
}

 static int
_dn_cb_event_desk_show(void *data, int type, void *event)
{
   DesktopName	*dn;
   E_Event_Desk_Show *ev;
   E_Zone *zone;

   Evas_Coord ew, eh;
   char buf[128];
   
   dn = data;
   ev = event;
   zone = ev->desk->zone;

   if (timeout_timer) ecore_timer_del(timeout_timer);	   
  
   if (dn->popup) e_object_del(E_OBJECT(dn->popup));

   dn->popup = e_popup_new(zone, 0, 0, 1, 1);
   if (!dn->popup) return 1;
   e_popup_layer_set(dn->popup, 255);
   dn->obj = edje_object_add(dn->popup->evas);
   e_theme_edje_object_set(dn->obj, "base/theme/desktop",
			   "widgets/desktop/main");
   snprintf(buf, sizeof(buf), ev->desk->name);
   edje_object_part_text_set(dn->obj, "text", buf);
   

   edje_object_size_min_calc(dn->obj, &ew, &eh);
   
   evas_object_move(dn->obj, 0, 0);
   evas_object_resize(dn->obj, ew, eh);
   evas_object_show(dn->obj);
   edje_object_signal_emit(dn->obj, "active", "");
   e_popup_edje_bg_object_set(dn->popup, dn->obj);
   e_popup_move_resize(dn->popup,
		       (zone->x - dn->popup->zone->x) +
		       ((zone->w - ew) / 2),       
		       (zone->y - dn->popup->zone->y) +                      
		       ((zone->h - eh) / 2),
		       ew, eh);
   e_popup_show(dn->popup);
   
   timeout_timer = ecore_timer_add(dn->conf->speed, _dn_popup_timeout_cb, dn);
   return 1;
}

static int
_dn_popup_timeout_cb(void *data)
{
   DesktopName	*dn;

   dn = data;
   if (dn->obj)
     {
	evas_object_del(dn->obj);
	dn->obj = NULL;
     }
   if (dn->popup)
     {
	e_object_del(E_OBJECT(dn->popup));
	dn->popup = NULL;
     }
   timeout_timer = NULL;
   return 0;
} 
