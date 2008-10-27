/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"

/* actual module specifics */
static void  _e_mod_action_fileman_cb(E_Object *obj, const char *params);
static void  _e_mod_fileman_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void  _e_mod_menu_add(void *data, E_Menu *m);
static void  _e_mod_fileman_config_load(void);
static void  _e_mod_fileman_config_free(void);
static int   _e_mod_cb_config_timer(void *data);
static int   _e_mod_zone_add(void *data, int type, void *event);

static E_Module *conf_module = NULL;
static E_Action *act = NULL;
static E_Int_Menu_Augmentation *maug = NULL;
static Ecore_Event_Handler *zone_add_handler = NULL;

static E_Config_DD *conf_edd = NULL;
Config *fileman_config = NULL;

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Fileman"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   Eina_List *l, *ll, *lll;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;
   
   conf_module = m;

   eina_stringshare_init();

   /* Setup Entry in Config Panel */
   e_configure_registry_category_add("fileman", 100, _("File Manager"), 
				     NULL, "enlightenment/fileman");
   e_configure_registry_item_add("fileman/fileman", 10, _("File Manager"), 
				 NULL, "enlightenment/fileman", 
				 e_int_config_fileman);
   
   /* Setup Config edd */
   _e_mod_fileman_config_load();
   
   /* add module supplied action */
   act = e_action_add("fileman");
   if (act)
     {
	act->func.go = _e_mod_action_fileman_cb;
	e_action_predef_name_set(_("Launch"), _("File Manager"), 
				 "fileman", NULL, "syntax: /path/to/dir or ~/path/to/dir or favorites or desktop, examples: /boot/grub, ~/downloads", 1);
     }
   maug = e_int_menus_menu_augmentation_add("main/1", _e_mod_menu_add, 
					    NULL, NULL, NULL);
   e_module_delayed_set(m, 1);

   /* Hook into zones */
   for (l = e_manager_list(); l; l = l->next)
     {
	man = l->data;
	for (ll = man->containers; ll; ll = ll->next)
	  {
	     con = ll->data;
	     for (lll = con->zones; lll; lll = lll->next)
	       {
		  zone = lll->data;
                  if (e_fwin_zone_find(zone)) continue;
		  if ((zone->container->num == 0) && (zone->num == 0) && 
		      (fileman_config->view.show_desktop_icons))
		    e_fwin_zone_new(zone, "desktop", "/");
		  else 
		    {
		       char buf[256];
		       
		       if (fileman_config->view.show_desktop_icons) 
			 {
			    snprintf(buf, sizeof(buf), "%i", 
				(zone->container->num + zone->num));
			    e_fwin_zone_new(zone, "desktop", buf);
			 }
		    }
	       }
	  }
     }
   zone_add_handler = ecore_event_handler_add(E_EVENT_ZONE_ADD,
					      _e_mod_zone_add, NULL);
   
   /* FIXME: add system event for new zone creation, and on creation, add an fwin to the zone */
   
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   Eina_List *l, *ll, *lll;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;

   ecore_event_handler_del(zone_add_handler);
   zone_add_handler = NULL;
   
   /* Unhook zone fm */
   for (l = e_manager_list(); l; l = l->next)
     {
	man = l->data;
	for (ll = man->containers; ll; ll = ll->next)
	  {
	     con = ll->data;
	     for (lll = con->zones; lll; lll = lll->next)
	       {
		  zone = lll->data;
		  if (!zone) continue;
		  e_fwin_zone_shutdown(zone);
	       }
	  }
     }   
   
   /* remove module-supplied menu additions */
   if (maug)
     {
	e_int_menus_menu_augmentation_del("main/1", maug);
	maug = NULL;
     }
   /* remove module-supplied action */
   if (act)
     {
	e_action_predef_name_del(_("Launch"), _("File Manager"));
	e_action_del("fileman");
	act = NULL;
     }
   /* remove fileman category from config panel */
   e_configure_registry_item_del("fileman/fileman");
   e_configure_registry_category_del("fileman");
   
   _e_mod_fileman_config_free();
   E_CONFIG_DD_FREE(conf_edd);

   eina_stringshare_shutdown();

   conf_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   e_config_domain_save("module.fileman", conf_edd, fileman_config);
   return 1;
}

/* action callback */
static void
_e_mod_action_fileman_cb(E_Object *obj, const char *params)
{
   E_Zone *zone = NULL;
   
   if (obj)
     {
	if (obj->type == E_MANAGER_TYPE)
	  zone = e_util_zone_current_get((E_Manager *)obj);
	else if (obj->type == E_CONTAINER_TYPE)
	  zone = e_util_zone_current_get(((E_Container *)obj)->manager);
	else if (obj->type == E_ZONE_TYPE)
	  zone = e_util_zone_current_get(((E_Zone *)obj)->container->manager);
	else
	  zone = e_util_zone_current_get(e_manager_current_get());
     }
   if (!zone) zone = e_util_zone_current_get(e_manager_current_get());
   if (zone)
     {
	if (params && params[0] == '/')
	  e_fwin_new(zone->container, "/", params);
	else if (params && params[0] == '~')
	  e_fwin_new(zone->container, "~/", params + 1);
	else if (params)
	  e_fwin_new(zone->container, params, "/");
	else
	  e_fwin_new(zone->container, "favorites", "/");
     }
}

/* menu item callback(s) */
static int
_e_mod_fileman_defer_cb(void *data) 
{
   E_Zone *zone;
   
   zone = data;
   if (zone) e_fwin_new(zone->container, "favorites", "/");
   return 0;
}

static void 
_e_mod_fileman_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   ecore_idle_enterer_add(_e_mod_fileman_defer_cb, m->zone);
}

/* menu item add hook */
static void
_e_mod_menu_add(void *data, E_Menu *m)
{
   E_Menu_Item *mi;
   
#ifdef ENABLE_FILES
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Files"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/fileman");
   e_menu_item_callback_set(mi, _e_mod_fileman_cb, NULL);
#endif
}

/* Abstract fileman config load/create to one function for maintainability */
static void 
_e_mod_fileman_config_load(void) 
{
   conf_edd = E_CONFIG_DD_NEW("Fileman_Config", Config);
   #undef T
   #undef D
   #define T Config
   #define D conf_edd
   E_CONFIG_VAL(D, T, config_version, INT);
   E_CONFIG_VAL(D, T, view.mode, INT);
   E_CONFIG_VAL(D, T, view.open_dirs_in_place, UCHAR);
   E_CONFIG_VAL(D, T, view.selector, UCHAR);
   E_CONFIG_VAL(D, T, view.single_click, UCHAR);
   E_CONFIG_VAL(D, T, view.no_subdir_jump, UCHAR);
   E_CONFIG_VAL(D, T, view.no_subdir_drop, UCHAR);
   E_CONFIG_VAL(D, T, view.always_order, UCHAR);
   E_CONFIG_VAL(D, T, view.link_drop, UCHAR);
   E_CONFIG_VAL(D, T, view.fit_custom_pos, UCHAR);
   E_CONFIG_VAL(D, T, view.show_full_path, UCHAR);
   E_CONFIG_VAL(D, T, view.show_desktop_icons, UCHAR);
   E_CONFIG_VAL(D, T, view.show_toolbar, UCHAR);
   E_CONFIG_VAL(D, T, icon.icon.w, INT);
   E_CONFIG_VAL(D, T, icon.icon.h, INT);
   E_CONFIG_VAL(D, T, icon.list.w, INT);
   E_CONFIG_VAL(D, T, icon.list.h, INT);
   E_CONFIG_VAL(D, T, icon.fixed.w, UCHAR);
   E_CONFIG_VAL(D, T, icon.fixed.h, UCHAR);
   E_CONFIG_VAL(D, T, icon.extension.show, UCHAR);
   E_CONFIG_VAL(D, T, list.sort.no_case, UCHAR);
   E_CONFIG_VAL(D, T, list.sort.dirs.first, UCHAR);
   E_CONFIG_VAL(D, T, list.sort.dirs.last, UCHAR);
   E_CONFIG_VAL(D, T, selection.single, UCHAR);
   E_CONFIG_VAL(D, T, selection.windows_modifiers, UCHAR);
   E_CONFIG_VAL(D, T, theme.background, STR);
   E_CONFIG_VAL(D, T, theme.frame, STR);
   E_CONFIG_VAL(D, T, theme.icons, STR);
   E_CONFIG_VAL(D, T, theme.fixed, UCHAR);
   
   fileman_config = e_config_domain_load("module.fileman", conf_edd);
   if (fileman_config) 
     {
	if ((fileman_config->config_version >> 16) < MOD_CONFIG_FILE_EPOCH) 
	  {
	     _e_mod_fileman_config_free();
	     ecore_timer_add(1.0, _e_mod_cb_config_timer,
			     _("Fileman Module Configuration data needed upgrading. Your old configuration<br>"
			       "has been wiped and a new set of defaults initialized. This<br>"
			       "will happen regularly during development, so don't report a<br>"
			       "bug. This simply means Fileman module needs new configuration<br>"
			       "data by default for usable functionality that your old<br>"
			       "configuration simply lacks. This new set of defaults will fix<br>"
			       "that by adding it in. You can re-configure things now to your<br>"
			       "liking. Sorry for the hiccup in your configuration.<br>"));
	  }
	else if (fileman_config->config_version > MOD_CONFIG_FILE_VERSION) 
	  {
	     _e_mod_fileman_config_free();
	     ecore_timer_add(1.0, _e_mod_cb_config_timer, 
			     _("Your Fileman Module configuration is NEWER than Fileman Module version. This is very<br>"
			       "strange. This should not happen unless you downgraded<br>"
			       "the Fileman Module or copied the configuration from a place where<br>"
			       "a newer version of the Fileman Module was running. This is bad and<br>"
			       "as a precaution your configuration has been now restored to<br>"
			       "defaults. Sorry for the inconvenience.<br>"));
	  }
     }
   
   if (!fileman_config) 
     {
	fileman_config = E_NEW(Config, 1);
	fileman_config->config_version = (MOD_CONFIG_FILE_EPOCH << 16);
     }
#define IFMODCFG(v) \
   if ((fileman_config->config_version & 0xffff) < (v)) {
#define IFMODCFGEND }

   IFMODCFG(0x008d);
   fileman_config->view.mode = E_FM2_VIEW_MODE_GRID_ICONS;
   fileman_config->view.open_dirs_in_place = 0;
   fileman_config->view.selector = 0;
   fileman_config->view.single_click = 0;
   fileman_config->view.no_subdir_jump = 0;
   fileman_config->view.show_full_path = 0;
   fileman_config->view.show_desktop_icons = 1;
   fileman_config->icon.icon.w = 48;
   fileman_config->icon.icon.h = 48;
   fileman_config->icon.fixed.w = 0;
   fileman_config->icon.fixed.h = 0;
   fileman_config->icon.extension.show = 1;
   fileman_config->list.sort.no_case = 1;
   fileman_config->list.sort.dirs.first = 1;
   fileman_config->list.sort.dirs.last = 0;
   fileman_config->selection.single = 0;
   fileman_config->selection.windows_modifiers = 0;
   IFMODCFGEND;

   IFMODCFG(0x0101);
   fileman_config->view.show_toolbar = 0;
   IFMODCFGEND;

   fileman_config->config_version = MOD_CONFIG_FILE_VERSION;
   
   /* UCHAR's give nasty compile warnings about comparisons so not gonna limit those */
   E_CONFIG_LIMIT(fileman_config->view.mode, E_FM2_VIEW_MODE_ICONS, E_FM2_VIEW_MODE_LIST);
   E_CONFIG_LIMIT(fileman_config->icon.icon.w, 16, 256);
   E_CONFIG_LIMIT(fileman_config->icon.icon.h, 16, 256);
   E_CONFIG_LIMIT(fileman_config->icon.list.w, 16, 256);
   E_CONFIG_LIMIT(fileman_config->icon.list.h, 16, 256);
   
   e_config_save_queue();
}

static void
_e_mod_fileman_config_free(void) 
{
   if (fileman_config->theme.background)
     eina_stringshare_del(fileman_config->theme.background);
   if (fileman_config->theme.frame)
     eina_stringshare_del(fileman_config->theme.frame);
   if (fileman_config->theme.icons)
     eina_stringshare_del(fileman_config->theme.icons);
   E_FREE(fileman_config);   
}

static int 
_e_mod_cb_config_timer(void *data) 
{
   e_util_dialog_show(_("Fileman Configuration Updated"), "%s", (char *)data);
   return 0;
}

static int
_e_mod_zone_add(void *data, int type, void *event)
{
   E_Event_Zone_Add *ev;
   E_Zone *zone;
   
   if (type != E_EVENT_ZONE_ADD) return 1;
   ev = event;
   zone = ev->zone;
   if (e_fwin_zone_find(zone)) return 1;
   if ((zone->container->num == 0) && (zone->num == 0) && 
       (fileman_config->view.show_desktop_icons))
     e_fwin_zone_new(zone, "desktop", "/");
   else 
     {
	char buf[256];
	
	if (fileman_config->view.show_desktop_icons) 
	  {
	     snprintf(buf, sizeof(buf), "%i", 
		      (zone->container->num + zone->num));
	     e_fwin_zone_new(zone, "desktop", buf);
	  }
     }
   return 1;
}
