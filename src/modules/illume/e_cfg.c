#include "config.h"
#include "e.h"
#include "e_cfg.h"
#include "e_slipshelf.h"
#include "e_mod_win.h"

/* internal calls */
static void _e_cfg_dbus_if_init(void);
static void _e_cfg_dbus_if_shutdown(void);

static Evas_Object *_e_cfg_win_new(const char *title, const char *name, const char *themedir, void (*delfunc) (const void *data), const void *data);
static void _e_cfg_win_complete(Evas_Object *ol);

static void _cb_signal_ok(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _cb_delete(E_Win *win);
static void _cb_resize(E_Win *win);

static Evas_Object *_theme_obj_new(Evas *e, const char *custom_dir, const char *group);

/* state */
EAPI Illume_Cfg *illume_cfg = NULL;

static E_Config_DD *conf_edd = NULL;
static E_Module *mod = NULL;

/* called from the module core */
EAPI int
e_cfg_init(E_Module *m)
{
   mod = m;
   conf_edd = E_CONFIG_DD_NEW("Illume_Cfg", Illume_Cfg);

   E_CONFIG_VAL(conf_edd, Illume_Cfg, config_version, INT);
   
   E_CONFIG_VAL(conf_edd, Illume_Cfg, launcher.mode, INT);
   E_CONFIG_VAL(conf_edd, Illume_Cfg, launcher.icon_size, INT);
   E_CONFIG_VAL(conf_edd, Illume_Cfg, launcher.single_click, INT);
   
   E_CONFIG_VAL(conf_edd, Illume_Cfg, power.auto_suspend, INT);
   E_CONFIG_VAL(conf_edd, Illume_Cfg, power.auto_suspend_delay, INT);
   
   E_CONFIG_VAL(conf_edd, Illume_Cfg, performance.cache_level, INT);
   E_CONFIG_VAL(conf_edd, Illume_Cfg, performance.fps, INT);
   
   E_CONFIG_VAL(conf_edd, Illume_Cfg, slipshelf.main_gadget_size, INT);
   E_CONFIG_VAL(conf_edd, Illume_Cfg, slipshelf.extra_gagdet_size, INT);
   E_CONFIG_VAL(conf_edd, Illume_Cfg, slipshelf.style, INT);
   
   E_CONFIG_VAL(conf_edd, Illume_Cfg, sliding.slipshelf.duration, INT);
   E_CONFIG_VAL(conf_edd, Illume_Cfg, sliding.kbd.duration, INT);
   E_CONFIG_VAL(conf_edd, Illume_Cfg, sliding.busywin.duration, INT);
   E_CONFIG_VAL(conf_edd, Illume_Cfg, sliding.layout.duration, INT);

   E_CONFIG_VAL(conf_edd, Illume_Cfg, kbd.use_internal, INT);
   E_CONFIG_VAL(conf_edd, Illume_Cfg, kbd.run_keyboard, STR);
   
   illume_cfg = e_config_domain_load("module.illume", conf_edd);
   if ((illume_cfg) && 
       ((illume_cfg->config_version >> 16) < ILLUME_CONFIG_MAJ))
     {
	// free config - start from scratch if major version changes
	free(illume_cfg);
	illume_cfg = NULL;
     }
   
   if (!illume_cfg)
     {
	illume_cfg = E_NEW(Illume_Cfg, 1);
	
	/* CONFIG DEFAULTS */
	illume_cfg->config_version = 0;
	
	illume_cfg->launcher.mode = 0;
	illume_cfg->launcher.icon_size = 120;
	illume_cfg->launcher.single_click = 1;
	
	illume_cfg->power.auto_suspend = 1;
	illume_cfg->power.auto_suspend_delay = 1;
	
	illume_cfg->performance.cache_level = 3;
	illume_cfg->performance.fps = 30;
	
	illume_cfg->slipshelf.main_gadget_size = 42;
	illume_cfg->slipshelf.extra_gagdet_size = 32;
	
	illume_cfg->sliding.slipshelf.duration = 1000;
	illume_cfg->sliding.kbd.duration = 1000;
	illume_cfg->sliding.busywin.duration = 1000;
	illume_cfg->sliding.layout.duration = 1000;
     }
   if (illume_cfg)
     {
	if ((illume_cfg->config_version & 0xffff) < 1) // new in minor ver 1
	  {
	     illume_cfg->kbd.use_internal = 1;
	     illume_cfg->kbd.run_keyboard = NULL;
	  }
	if ((illume_cfg->config_version & 0xffff) < 2) // new in minor ver 2
	  {
	     illume_cfg->kbd.dict = evas_stringshare_add("English_(US).dic");
	  }
	if ((illume_cfg->config_version & 0xffff) < 3) // new in minor ver 3
	  {
	     illume_cfg->slipshelf.style = 1;
	  }
	illume_cfg->config_version = (ILLUME_CONFIG_MAJ << 16) | ILLUME_CONFIG_MIN;
     }
   // duplicate system settings
   illume_cfg->performance.fps = e_config->framerate;
   
   e_configure_registry_category_add("display", 0, "Display", NULL, "enlightenment/display");
   e_configure_registry_generic_item_add("display/launcher", 0, "Launcher", NULL, "enlightenment/launcher", e_cfg_launcher);
   e_configure_registry_generic_item_add("display/power", 0, "Power", NULL, "enlightenment/power", e_cfg_power);
   e_configure_registry_generic_item_add("display/keyboard", 0, "Keyboard", NULL, "enlightenment/keyboard", e_cfg_keyboard);
   e_configure_registry_generic_item_add("display/animation", 0, "Animation", NULL, "enlightenment/animation", e_cfg_animation);
   e_configure_registry_generic_item_add("display/slipshelf", 0, "Top Shelf", NULL, "enlightenment/slipshelf", e_cfg_slipshelf);
   e_configure_registry_generic_item_add("display/thumbscroll", 0, "Finger Scrolling", NULL, "enlightenment/thumbscroll", e_cfg_thumbscroll);
   e_configure_registry_generic_item_add("display/gadgets", 0, "Shelf Gadgets", NULL, "enlightenment/gadgets", e_cfg_gadgets);
   e_configure_registry_generic_item_add("display/fps", 0, "Framerate", NULL, "enlightenment/fps", e_cfg_fps);
   _e_cfg_dbus_if_init();
   return 1;
}

EAPI int
e_cfg_shutdown(void)
{
   _e_cfg_dbus_if_shutdown();
   e_configure_registry_item_del("display/fps");
   e_configure_registry_item_del("display/gadgets");
   e_configure_registry_item_del("display/thumbscroll");
   e_configure_registry_item_del("display/slipshelf");
   e_configure_registry_item_del("display/animation");
   e_configure_registry_item_del("display/keyboard");
   e_configure_registry_item_del("display/power");
   e_configure_registry_item_del("display/launcher");
   e_configure_registry_category_del("display");
   if (illume_cfg->kbd.run_keyboard) evas_stringshare_del(illume_cfg->kbd.run_keyboard);
   if (illume_cfg->kbd.dict) evas_stringshare_del(illume_cfg->kbd.dict);
   free(illume_cfg);
   E_CONFIG_DD_FREE(conf_edd);
   illume_cfg = NULL;
   mod = NULL;
   return 1;
}

EAPI int
e_cfg_save(void)
{
   e_config_domain_save("module.illume", conf_edd, illume_cfg);
   return 1;
}

///////////////////////////////////////////////////////////////////////////////
Ecore_Timer *_e_cfg_launcher_change_timer = NULL;
static int _e_cfg_launcher_change_timeout(void *data) {
   _e_mod_win_cfg_update();
   e_config_save_queue();
   _e_cfg_launcher_change_timer = NULL; return 0;
}
static void
_e_cfg_launcher_change(void *data, Evas_Object *obj, void *event_info) {
   if (_e_cfg_launcher_change_timer) ecore_timer_del(_e_cfg_launcher_change_timer);
   _e_cfg_launcher_change_timer = ecore_timer_add(0.5, _e_cfg_launcher_change_timeout, data);
}

static void *
_e_cfg_launcher_create(E_Config_Dialog *cfd)
{ // alloc cfd->cfdata
   return NULL;
}

static void 
_e_cfg_launcher_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{ // free cfd->cfdata
}

static Evas_Object *
_e_cfg_launcher_ui(E_Config_Dialog *cfd, Evas *e, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *list, *o, *frame;
   E_Radio_Group *rg;

   list = e_widget_list_add(e, 0, 0);
   
   frame = e_widget_framelist_add(e, "Display Type", 0);
   rg = e_widget_radio_group_new(&(illume_cfg->launcher.mode));
   o = e_widget_radio_add(e, "Slider", 0, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_launcher_change, NULL);
   o = e_widget_radio_add(e, "Icons", 1, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_launcher_change, NULL);
   e_widget_list_object_append(list, frame, 1, 0, 0.0); // fill, expand, align
   
   frame = e_widget_framelist_add(e, "Icon Size", 0);
   rg = e_widget_radio_group_new(&(illume_cfg->launcher.icon_size));
   o = e_widget_radio_add(e, "Small", 60, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_launcher_change, NULL);
   o = e_widget_radio_add(e, "Medium", 80, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_launcher_change, NULL);
   o = e_widget_radio_add(e, "Large", 120, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_launcher_change, NULL);
   o = e_widget_radio_add(e, "Very large", 160, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_launcher_change, NULL);
   o = e_widget_radio_add(e, "Massive", 240, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_launcher_change, NULL);
   e_widget_list_object_append(list, frame, 1, 0, 0.0); // fill, expand, align
   
   frame = e_widget_framelist_add(e, "Launch Action", 0);
   o = e_widget_check_add(e, "Single press", &(illume_cfg->launcher.single_click));
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_launcher_change, NULL);
   e_widget_list_object_append(list, frame, 1, 0, 0.0); // fill, expand, align

   return list;
}

EAPI void
e_cfg_launcher(E_Container *con, const char *params)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v = NULL;
   
   if (e_config_dialog_find("E", "_config_illume_launcher_settings")) return;
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata        = _e_cfg_launcher_create;
   v->free_cfdata          = _e_cfg_launcher_free;
   v->basic.create_widgets = _e_cfg_launcher_ui;
   v->basic_only           = 1;
   v->normal_win           = 1;
   v->scroll               = 1;
   cfd = e_config_dialog_new(con, "Launcher Settings",
			     "E", "_config_illume_launcher_settings",
			     "enlightenment/launcher_settings", 0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
}

///////////////////////////////////////////////////////////////////////////////
Ecore_Timer *_e_cfg_power_change_timer = NULL;
static int _e_cfg_power_change_timeout(void *data) {
   if (e_config->screensaver_timeout > 0)
     {
	e_config->screensaver_enable = 1;
     }
   else
     {
	e_config->screensaver_enable = 0;
	e_config->screensaver_timeout = 0;
     }
   if (illume_cfg->power.auto_suspend_delay > 0)
     {
	illume_cfg->power.auto_suspend = 1;
     }
   else
     {
	illume_cfg->power.auto_suspend = 0;
	illume_cfg->power.auto_suspend_delay = 0;
     }
   e_pwr_cfg_update();
   e_config_save_queue();
   _e_cfg_power_change_timer = NULL; return 0;
}
static void
_e_cfg_power_change(void *data, Evas_Object *obj, void *event_info) {
   if (_e_cfg_power_change_timer) ecore_timer_del(_e_cfg_power_change_timer);
   _e_cfg_power_change_timer = ecore_timer_add(0.5, _e_cfg_power_change_timeout, data);
}

static void *
_e_cfg_power_create(E_Config_Dialog *cfd)
{ // alloc cfd->cfdata
   return NULL;
}

static void 
_e_cfg_power_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{ // free cfd->cfdata
}

static Evas_Object *
_e_cfg_power_ui(E_Config_Dialog *cfd, Evas *e, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *list, *o, *frame;
   E_Radio_Group *rg;

   list = e_widget_list_add(e, 0, 0);
   
   frame = e_widget_framelist_add(e, "Blank Time", 0);
   rg = e_widget_radio_group_new(&(e_config->screensaver_timeout));
   o = e_widget_radio_add(e, "5 Seconds", 5, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_power_change, NULL);
   o = e_widget_radio_add(e, "10 Seconds", 10, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_power_change, NULL);
   o = e_widget_radio_add(e, "15 Seconds", 15, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_power_change, NULL);
   o = e_widget_radio_add(e, "30 Seconds", 30, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_power_change, NULL);
   o = e_widget_radio_add(e, "60 Seconds", 60, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_power_change, NULL);
   o = e_widget_radio_add(e, "2 Minutes", 120, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_power_change, NULL);
   o = e_widget_radio_add(e, "5 Minutes", 300, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_power_change, NULL);
   o = e_widget_radio_add(e, "Off", 0, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_power_change, NULL);
   e_widget_list_object_append(list, frame, 1, 0, 0.0); // fill, expand, align

   frame = e_widget_framelist_add(e, "Suspend After Blank", 0);
   rg = e_widget_radio_group_new(&(illume_cfg->power.auto_suspend_delay));
   o = e_widget_radio_add(e, "1 Second", 1, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_power_change, NULL);
   o = e_widget_radio_add(e, "5 Seconds", 5, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_power_change, NULL);
   o = e_widget_radio_add(e, "10 Seconds", 10, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_power_change, NULL);
   o = e_widget_radio_add(e, "30 Seconds", 30, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_power_change, NULL);
   o = e_widget_radio_add(e, "60 Seconds", 60, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_power_change, NULL);
   o = e_widget_radio_add(e, "Off", 0, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_power_change, NULL);
   e_widget_list_object_append(list, frame, 1, 0, 0.0); // fill, expand, align

   return list;
}

EAPI void
e_cfg_power(E_Container *con, const char *params)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v = NULL;
   
   if (e_config_dialog_find("E", "_config_illume_power_settings")) return;
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata        = _e_cfg_power_create;
   v->free_cfdata          = _e_cfg_power_free;
   v->basic.create_widgets = _e_cfg_power_ui;
   v->basic_only           = 1;
   v->normal_win           = 1;
   v->scroll               = 1;
   cfd = e_config_dialog_new(con, "Power Settings",
			     "E", "_config_illume_power_settings",
			     "enlightenment/power_settings", 0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
}

///////////////////////////////////////////////////////////////////////////////
Ecore_Timer *_e_cfg_animation_change_timer = NULL;
static int _e_cfg_animation_change_timeout(void *data) {
   e_config_save_queue();
   _e_cfg_animation_change_timer = NULL; return 0;
}
static void
_e_cfg_animation_change(void *data, Evas_Object *obj, void *event_info) {
   if (_e_cfg_animation_change_timer) ecore_timer_del(_e_cfg_animation_change_timer);
   _e_cfg_animation_change_timer = ecore_timer_add(0.5, _e_cfg_animation_change_timeout, data);
}

static void *
_e_cfg_animation_create(E_Config_Dialog *cfd)
{ // alloc cfd->cfdata
   return NULL;
}

static void 
_e_cfg_animation_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{ // free cfd->cfdata
}

static Evas_Object *
_e_cfg_animation_ui(E_Config_Dialog *cfd, Evas *e, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *list, *o, *frame;
   E_Radio_Group *rg;

   list = e_widget_list_add(e, 0, 0);
   
   frame = e_widget_framelist_add(e, "Applications", 0);
   rg = e_widget_radio_group_new(&(illume_cfg->sliding.layout.duration));
   o = e_widget_radio_add(e, "Slow", 2000, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   o = e_widget_radio_add(e, "Medium", 1000, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   o = e_widget_radio_add(e, "Fast", 500, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   o = e_widget_radio_add(e, "Very Fast", 250, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   o = e_widget_radio_add(e, "Off", 0, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   e_widget_list_object_append(list, frame, 1, 0, 0.0); // fill, expand, align
     
   frame = e_widget_framelist_add(e, "Top Shelf", 0);
   rg = e_widget_radio_group_new(&(illume_cfg->sliding.slipshelf.duration));
   o = e_widget_radio_add(e, "Slow", 2000, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   o = e_widget_radio_add(e, "Medium", 1000, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   o = e_widget_radio_add(e, "Fast", 500, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   o = e_widget_radio_add(e, "Very Fast", 250, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   o = e_widget_radio_add(e, "Off", 0, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   e_widget_list_object_append(list, frame, 1, 0, 0.0); // fill, expand, align
     
   frame = e_widget_framelist_add(e, "Keyboard", 0);
   rg = e_widget_radio_group_new(&(illume_cfg->sliding.kbd.duration));
   o = e_widget_radio_add(e, "Slow", 2000, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   o = e_widget_radio_add(e, "Medium", 1000, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   o = e_widget_radio_add(e, "Fast", 500, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   o = e_widget_radio_add(e, "Very Fast", 250, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   o = e_widget_radio_add(e, "Off", 0, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   e_widget_list_object_append(list, frame, 1, 0, 0.0); // fill, expand, align
     
   frame = e_widget_framelist_add(e, "Status", 0);
   rg = e_widget_radio_group_new(&(illume_cfg->sliding.busywin.duration));
   o = e_widget_radio_add(e, "Slow", 2000, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   o = e_widget_radio_add(e, "Medium", 1000, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   o = e_widget_radio_add(e, "Fast", 500, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   o = e_widget_radio_add(e, "Very Fast", 250, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   o = e_widget_radio_add(e, "Off", 0, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_animation_change, NULL);
   e_widget_list_object_append(list, frame, 1, 0, 0.0); // fill, expand, align

   return list;
}

EAPI void
e_cfg_animation(E_Container *con, const char *params)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v = NULL;
   
   if (e_config_dialog_find("E", "_config_illume_animation_settings")) return;
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata        = _e_cfg_animation_create;
   v->free_cfdata          = _e_cfg_animation_free;
   v->basic.create_widgets = _e_cfg_animation_ui;
   v->basic_only           = 1;
   v->normal_win           = 1;
   v->scroll               = 1;
   cfd = e_config_dialog_new(con, "Animation Settings",
			     "E", "_config_illume_animation_settings",
			     "enlightenment/animation_settings", 0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
}

///////////////////////////////////////////////////////////////////////////////
Ecore_Timer *_e_cfg_slipshelf_change_timer = NULL;
static int _e_cfg_slipshelf_change_timeout(void *data) {
   _e_mod_win_slipshelf_cfg_update();
   e_config_save_queue();
   _e_cfg_slipshelf_change_timer = NULL; return 0;
}
static void
_e_cfg_slipshelf_change(void *data, Evas_Object *obj, void *event_info) {
   if (_e_cfg_slipshelf_change_timer) ecore_timer_del(_e_cfg_slipshelf_change_timer);
   _e_cfg_slipshelf_change_timer = ecore_timer_add(0.5, _e_cfg_slipshelf_change_timeout, data);
}

static void *
_e_cfg_slipshelf_create(E_Config_Dialog *cfd)
{ // alloc cfd->cfdata
   return NULL;
}

static void 
_e_cfg_slipshelf_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{ // free cfd->cfdata
}

static Evas_Object *
_e_cfg_slipshelf_ui(E_Config_Dialog *cfd, Evas *e, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *list, *o, *frame;
   E_Radio_Group *rg;

   list = e_widget_list_add(e, 0, 0);

   frame = e_widget_framelist_add(e, "Visible Gadgets", 0);
   rg = e_widget_radio_group_new(&(illume_cfg->slipshelf.main_gadget_size));
   o = e_widget_radio_add(e, "(24) Tiny", 24, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_slipshelf_change, NULL);
   o = e_widget_radio_add(e, "(32)", 32, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_slipshelf_change, NULL);
   o = e_widget_radio_add(e, "(36)", 36, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_slipshelf_change, NULL);
   o = e_widget_radio_add(e, "Small (40)", 40, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_slipshelf_change, NULL);
   o = e_widget_radio_add(e, "(42)", 42, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_slipshelf_change, NULL);
   o = e_widget_radio_add(e, "(44)", 44, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_slipshelf_change, NULL);
   o = e_widget_radio_add(e, "(46)", 46, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_slipshelf_change, NULL);
   o = e_widget_radio_add(e, "(48)", 48, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_slipshelf_change, NULL);
   o = e_widget_radio_add(e, "(50)", 50, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_slipshelf_change, NULL);
   o = e_widget_radio_add(e, "(52)", 52, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_slipshelf_change, NULL);
   o = e_widget_radio_add(e, "(56) Medium", 56, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_slipshelf_change, NULL);
   o = e_widget_radio_add(e, "(70) Large", 70, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_slipshelf_change, NULL);
   o = e_widget_radio_add(e, "(86) Very Large", 86, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_slipshelf_change, NULL);
   e_widget_list_object_append(list, frame, 1, 1, 0.0); // fill, expand, align
   
   frame = e_widget_framelist_add(e, "Hidden Gadgets", 0);
   rg = e_widget_radio_group_new(&(illume_cfg->slipshelf.extra_gagdet_size));
   o = e_widget_radio_add(e, "Tiny", 16, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_slipshelf_change, NULL);
   o = e_widget_radio_add(e, "Small", 24, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_slipshelf_change, NULL);
   o = e_widget_radio_add(e, "Medium", 32, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_slipshelf_change, NULL);
   o = e_widget_radio_add(e, "Large", 40, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_slipshelf_change, NULL);
   o = e_widget_radio_add(e, "Very Large", 48, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_slipshelf_change, NULL);
   e_widget_list_object_append(list, frame, 1, 0, 0.0); // fill, expand, align
   
   return list;
}

EAPI void
e_cfg_slipshelf(E_Container *con, const char *params)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v = NULL;
   
   if (e_config_dialog_find("E", "_config_illume_slipshelf_settings")) return;
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata        = _e_cfg_slipshelf_create;
   v->free_cfdata          = _e_cfg_slipshelf_free;
   v->basic.create_widgets = _e_cfg_slipshelf_ui;
   v->basic_only           = 1;
   v->normal_win           = 1;
   v->scroll               = 1;
   cfd = e_config_dialog_new(con, "Top Shelf Settings",
			     "E", "_config_illume_slipshelf_settings",
			     "enlightenment/slipshelf_settings", 0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
}

///////////////////////////////////////////////////////////////////////////////
Ecore_Timer *_e_cfg_thumbscroll_change_timer = NULL;
static int _e_cfg_thumbscroll_change_timeout(void *data) {
   if (e_config->thumbscroll_threshhold == 0)
     {
	e_config->thumbscroll_enable = 0;
     }
   else
     {
	e_config->thumbscroll_enable = 1;
	e_config->thumbscroll_momentum_threshhold = 100.0;
	e_config->thumbscroll_friction = 1.0;
     }
   e_config_save_queue();
   _e_cfg_thumbscroll_change_timer = NULL; return 0;
}
static void
_e_cfg_thumbscroll_change(void *data, Evas_Object *obj, void *event_info) {
   if (_e_cfg_thumbscroll_change_timer) ecore_timer_del(_e_cfg_thumbscroll_change_timer);
   _e_cfg_thumbscroll_change_timer = ecore_timer_add(0.5, _e_cfg_thumbscroll_change_timeout, data);
}

static void *
_e_cfg_thumbscroll_create(E_Config_Dialog *cfd)
{ // alloc cfd->cfdata
   return NULL;
}

static void 
_e_cfg_thumbscroll_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{ // free cfd->cfdata
}

static Evas_Object *
_e_cfg_thumbscroll_ui(E_Config_Dialog *cfd, Evas *e, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *list, *o, *frame;
   E_Radio_Group *rg;

   list = e_widget_list_add(e, 0, 0);
   
   frame = e_widget_framelist_add(e, "Drag Sensitivity", 0);
   rg = e_widget_radio_group_new(&(e_config->thumbscroll_threshhold));
   o = e_widget_radio_add(e, "Very High", 6, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_thumbscroll_change, NULL);
   o = e_widget_radio_add(e, "High", 8, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_thumbscroll_change, NULL);
   o = e_widget_radio_add(e, "Medium", 12, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_thumbscroll_change, NULL);
   o = e_widget_radio_add(e, "Low", 18, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_thumbscroll_change, NULL);
   o = e_widget_radio_add(e, "Very Low", 24, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_thumbscroll_change, NULL);
   o = e_widget_radio_add(e, "Extremely Low", 36, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_thumbscroll_change, NULL);
   o = e_widget_radio_add(e, "Lowest", 48, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_thumbscroll_change, NULL);
   e_widget_list_object_append(list, frame, 1, 0, 0.0); // fill, expand, align
   
   return list;
}

EAPI void
e_cfg_thumbscroll(E_Container *con, const char *params)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v = NULL;
   
   if (e_config_dialog_find("E", "_config_illume_thumbscroll_settings")) return;
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata        = _e_cfg_thumbscroll_create;
   v->free_cfdata          = _e_cfg_thumbscroll_free;
   v->basic.create_widgets = _e_cfg_thumbscroll_ui;
   v->basic_only           = 1;
   v->normal_win           = 1;
   v->scroll               = 0;
   cfd = e_config_dialog_new(con, "Finger Scrolling",
			     "E", "_config_illume_thumbscroll_settings",
			     "enlightenment/thumbscroll_settings", 0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
}

///////////////////////////////////////////////////////////////////////////////
Ecore_Timer *_e_cfg_fps_change_timer = NULL;
static int _e_cfg_fps_change_timeout(void *data) {
   e_config->framerate = illume_cfg->performance.fps;
   edje_frametime_set(1.0 / e_config->framerate);
   e_config_save_queue();
   _e_cfg_fps_change_timer = NULL; return 0;
}
static void
_e_cfg_fps_change(void *data, Evas_Object *obj, void *event_info) {
   if (_e_cfg_fps_change_timer) ecore_timer_del(_e_cfg_fps_change_timer);
   _e_cfg_fps_change_timer = ecore_timer_add(0.5, _e_cfg_fps_change_timeout, data);
}

static void *
_e_cfg_fps_create(E_Config_Dialog *cfd)
{ // alloc cfd->cfdata
   return NULL;
}

static void 
_e_cfg_fps_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{ // free cfd->cfdata
}

static Evas_Object *
_e_cfg_fps_ui(E_Config_Dialog *cfd, Evas *e, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *list, *o, *frame;
   E_Radio_Group *rg;

   list = e_widget_list_add(e, 0, 0);
   
   frame = e_widget_framelist_add(e, "Frames Per Second", 0);
   rg = e_widget_radio_group_new(&(illume_cfg->performance.fps));
   o = e_widget_radio_add(e, "5", 5, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_fps_change, NULL);
   o = e_widget_radio_add(e, "10", 10, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_fps_change, NULL);
   o = e_widget_radio_add(e, "15", 15, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_fps_change, NULL);
   o = e_widget_radio_add(e, "25", 25, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_fps_change, NULL);
   o = e_widget_radio_add(e, "30", 30, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_fps_change, NULL);
   o = e_widget_radio_add(e, "40", 40, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_fps_change, NULL);
   o = e_widget_radio_add(e, "50", 50, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_fps_change, NULL);
   o = e_widget_radio_add(e, "60", 60, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_fps_change, NULL);
   e_widget_list_object_append(list, frame, 1, 0, 0.0); // fill, expand, align
   
   return list;
}

EAPI void
e_cfg_fps(E_Container *con, const char *params)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v = NULL;
   
   if (e_config_dialog_find("E", "_config_illume_fps_settings")) return;
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata        = _e_cfg_fps_create;
   v->free_cfdata          = _e_cfg_fps_free;
   v->basic.create_widgets = _e_cfg_fps_ui;
   v->basic_only           = 1;
   v->normal_win           = 1;
   v->scroll               = 0;
   cfd = e_config_dialog_new(con, "Framerate",
			     "E", "_config_illume_fps_settings",
			     "enlightenment/fps_settings", 0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
}

///////////////////////////////////////////////////////////////////////////////
typedef struct _Gadit Gadit;
struct _Gadit
{
   E_Gadcon *gc;
   const char *name;
   int was_enabled, enabled;
};

E_Slipshelf *local_slipshelf = NULL;
Eina_List *gadits = NULL;
Ecore_Timer *_e_cfg_gadgets_change_timer = NULL;
static int _e_cfg_gadgets_change_timeout(void *data) {
   Eina_List *l, *l2, *l3;
   int update;
   E_Gadcon *up_gc1;
   
   update = 0;
   for (l = gadits; l; l = l->next)
     {
	Gadit *gi;
	
	gi = l->data;
	if (gi->enabled != gi->was_enabled)
	  {
	     if (gi->enabled)
	       {
		  e_gadcon_client_config_new(gi->gc, gi->name);
	       }
	     else
	       {
		  for (l2 = gi->gc->cf->clients; l2; l2 = l2->next)
		    {
		       E_Config_Gadcon_Client *gccc;
		       
		       gccc = l2->data;
		       if (strcmp(gi->name, gccc->name)) continue;
		       e_gadcon_client_config_del(gi->gc->cf, gccc);
		    }
	       }
	     update = 1;
	     gi->was_enabled = gi->enabled;
	  }
     }
   if (update)
     {
        e_gadcon_unpopulate(local_slipshelf->gadcon);
        e_gadcon_populate(local_slipshelf->gadcon);
        e_gadcon_unpopulate(local_slipshelf->gadcon_extra);
        e_gadcon_populate(local_slipshelf->gadcon_extra);
     }
   e_config_save_queue();
   _e_cfg_gadgets_change_timer = NULL; return 0;
}
static void
_e_cfg_gadgets_change(void *data, Evas_Object *obj, void *event_info) {
   if (_e_cfg_gadgets_change_timer) ecore_timer_del(_e_cfg_gadgets_change_timer);
   _e_cfg_gadgets_change_timer = ecore_timer_add(0.5, _e_cfg_gadgets_change_timeout, data);
}

static void *
_e_cfg_gadgets_create(E_Config_Dialog *cfd)
{ // alloc cfd->cfdata
   local_slipshelf = slipshelf;
   e_object_ref(E_OBJECT(local_slipshelf));
   return NULL;
}

static void 
_e_cfg_gadgets_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{ // free cfd->cfdata
   while (gadits)
     {
	Gadit *gi;
	
	gi = gadits->data;
	evas_stringshare_del(gi->name);
	free(gi);
	gadits = eina_list_remove_list(gadits, gadits);
     }
   e_object_unref(E_OBJECT(local_slipshelf));
   local_slipshelf = NULL;
}

static Evas_Object *
_e_cfg_gadgets_ui(E_Config_Dialog *cfd, Evas *e, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *list, *o, *frame;
   E_Radio_Group *rg;
   Eina_List *l, *l2, *l3;

   list = e_widget_list_add(e, 0, 0);
   
   frame = e_widget_framelist_add(e, "Visible Gadgets", 0);
   for (l = e_gadcon_provider_list(); l; l = l->next)
     {
	E_Gadcon_Client_Class *cc;
	const char *lbl = NULL;
	int on;
	Gadit *gi;
	
	if (!(cc = l->data)) continue;
	if (cc->func.label) lbl = cc->func.label();
	if (!lbl) lbl = cc->name;
	on = 0;
	for (l3 = local_slipshelf->gadcon->cf->clients; l3; l3 = l3->next)
	  {
	     E_Config_Gadcon_Client *gccc;
	     
	     gccc = l3->data;
	     if (!strcmp(cc->name, gccc->name))
	       {
		  on = 1;
		  break;
	       }
	  }
	
	gi = E_NEW(Gadit, 1);
	gi->gc = local_slipshelf->gadcon;
	gi->name = evas_stringshare_add(cc->name);
	gi->was_enabled = on;
	gi->enabled = on;
	gadits = eina_list_append(gadits, gi);
	o = e_widget_check_add(e, lbl, &(gi->enabled));
	e_widget_framelist_object_append(frame, o);
	evas_object_smart_callback_add(o, "changed", _e_cfg_gadgets_change, NULL);
     }
   e_widget_list_object_append(list, frame, 1, 0, 0.0); // fill, expand, align
   
   frame = e_widget_framelist_add(e, "Hidden Gadgets", 0);
   for (l = e_gadcon_provider_list(); l; l = l->next)
     {
	E_Gadcon_Client_Class *cc;
	const char *lbl = NULL;
	int on;
	Gadit *gi;
	
	if (!(cc = l->data)) continue;
	if (cc->func.label) lbl = cc->func.label();
	if (!lbl) lbl = cc->name;
	on = 0;
	for (l3 = local_slipshelf->gadcon_extra->cf->clients; l3; l3 = l3->next)
	  {
	     E_Config_Gadcon_Client *gccc;
	     
	     gccc = l3->data;
	     if (!strcmp(cc->name, gccc->name))
	       {
		  on = 1;
		  break;
	       }
	  }
	
	gi = E_NEW(Gadit, 1);
	gi->gc = local_slipshelf->gadcon_extra;
	gi->name = evas_stringshare_add(cc->name);
	gi->was_enabled = on;
	gi->enabled = on;
	gadits = eina_list_append(gadits, gi);
	o = e_widget_check_add(e, lbl, &(gi->enabled));
	e_widget_framelist_object_append(frame, o);
	evas_object_smart_callback_add(o, "changed", _e_cfg_gadgets_change, NULL);
     }
   e_widget_list_object_append(list, frame, 1, 0, 0.0); // fill, expand, align
  
   return list;
}

EAPI void
e_cfg_gadgets(E_Container *con, const char *params)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v = NULL;
   
   if (e_config_dialog_find("E", "_config_illume_gadgets_settings")) return;
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata        = _e_cfg_gadgets_create;
   v->free_cfdata          = _e_cfg_gadgets_free;
   v->basic.create_widgets = _e_cfg_gadgets_ui;
   v->basic_only           = 1;
   v->normal_win           = 1;
   v->scroll               = 1;
   cfd = e_config_dialog_new(con, "Top Shelf Gadgets",
			     "E", "_config_illume_gadgets_settings",
			     "enlightenment/gadgets_settings", 0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
}

///////////////////////////////////////////////////////////////////////////////
int external_keyboard = 0;
Ecore_Timer *_e_cfg_keyboard_change_timer = NULL;
static int _e_cfg_keyboard_change_timeout(void *data) {
   illume_cfg->kbd.use_internal = 0;
   if (illume_cfg->kbd.run_keyboard)
     {
	evas_stringshare_del(illume_cfg->kbd.run_keyboard);
	illume_cfg->kbd.run_keyboard = NULL;
     }
   if (external_keyboard == 0)
     {
	illume_cfg->kbd.use_internal = 0;
     }
   else if (external_keyboard == 1)
     {
	illume_cfg->kbd.use_internal = 1;
     }
   else
     {
	Ecore_List *kbds;
	Efreet_Desktop *desktop;
	int nn;
	
	kbds = efreet_util_desktop_category_list("Keyboard");
	if (kbds)
	  {
	     ecore_list_first_goto(kbds);
	     nn = 2;
	     while ((desktop = ecore_list_next(kbds)))
	       {
                  const char *dname;
		  
		  dname = ecore_file_file_get(desktop->orig_path);
		  if (nn == external_keyboard)
		    {
		       if (dname)
			 illume_cfg->kbd.run_keyboard = evas_stringshare_add(dname);
		       break;
		    }
		  nn++;
	       }
	  }
     }
   e_mod_win_cfg_kbd_update();
   e_config_save_queue();
   _e_cfg_keyboard_change_timer = NULL; return 0;
}
static void
_e_cfg_keyboard_change(void *data, Evas_Object *obj, void *event_info) {
   if (_e_cfg_keyboard_change_timer) ecore_timer_del(_e_cfg_keyboard_change_timer);
   _e_cfg_keyboard_change_timer = ecore_timer_add(0.5, _e_cfg_keyboard_change_timeout, data);
}

static void *
_e_cfg_keyboard_create(E_Config_Dialog *cfd)
{ // alloc cfd->cfdata
   local_slipshelf = slipshelf;
   e_object_ref(E_OBJECT(local_slipshelf));
   return NULL;
}

static void 
_e_cfg_keyboard_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{ // free cfd->cfdata
}

static Evas_Object *
_e_cfg_keyboard_ui(E_Config_Dialog *cfd, Evas *e, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *list, *o, *frame;
   E_Radio_Group *rg;
   Eina_List *l, *l2, *l3;

   list = e_widget_list_add(e, 0, 0);
   
   if (!illume_cfg->kbd.run_keyboard)
     {
	if (illume_cfg->kbd.use_internal)
	  external_keyboard = 1;
	else
	  external_keyboard = 0;
     }
   else
     {
	Ecore_List *kbds;
	Efreet_Desktop *desktop;
	int nn;
	
        external_keyboard = 0;
	kbds = efreet_util_desktop_category_list("Keyboard");
	if (kbds)
	  {
	     ecore_list_first_goto(kbds);
	     nn = 2;
	     while ((desktop = ecore_list_next(kbds)))
	       {
                  const char *dname;
		  
		  dname = ecore_file_file_get(desktop->orig_path);
		  if (dname)
		    {
		       if (!strcmp(illume_cfg->kbd.run_keyboard, dname))
			 {
			    external_keyboard = nn;
			    break;
			 }
		    }
		  nn++;
	       }
	  }
     }
   
   frame = e_widget_framelist_add(e, "Keyboards", 0);
   rg = e_widget_radio_group_new(&(external_keyboard));
   o = e_widget_radio_add(e, "None", 0, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_keyboard_change, NULL);
   o = e_widget_radio_add(e, "Default", 1, rg);
   e_widget_framelist_object_append(frame, o);
   evas_object_smart_callback_add(o, "changed", _e_cfg_keyboard_change, NULL);
     {
	Ecore_List *kbds;
	Efreet_Desktop *desktop;
	int nn;
	
	kbds = efreet_util_desktop_category_list("Keyboard");
	if (kbds)
	  {
	     ecore_list_first_goto(kbds);
	     nn = 2;
	     while ((desktop = ecore_list_next(kbds)))
	       {
                  const char *dname;
		  
		  dname = ecore_file_file_get(desktop->orig_path);
		  o = e_widget_radio_add(e, desktop->name, nn, rg);
		  e_widget_framelist_object_append(frame, o);
		  evas_object_smart_callback_add(o, "changed", _e_cfg_keyboard_change, NULL);
		  nn++;
	       }
	  }
     }
   e_widget_list_object_append(list, frame, 1, 0, 0.0); // fill, expand, align
   
   return list;
}

EAPI void
e_cfg_keyboard(E_Container *con, const char *params)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v = NULL;
   
   if (e_config_dialog_find("E", "_config_illume_keyboard_settings")) return;
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata        = _e_cfg_keyboard_create;
   v->free_cfdata          = _e_cfg_keyboard_free;
   v->basic.create_widgets = _e_cfg_keyboard_ui;
   v->basic_only           = 1;
   v->normal_win           = 1;
   v->scroll               = 1;
   cfd = e_config_dialog_new(con, "Keyboard Settings",
			     "E", "_config_illume_keyboard_settings",
			     "enlightenment/keyboard_settings", 0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
}

///////////////////////////////////////////////////////////////////////////////

/* internal calls */
typedef struct _Data Data;
struct _Data
{
   E_Win *win;
   Evas_Object *o, *o_sf, *o_l;
   void (*delfunc) (const void *data);
   const void *data;
};

static Evas_Object *
_e_cfg_win_new(const char *title, const char *name, const char *themedir, void (*delfunc) (const void *data), const void *data)
{
   E_Zone *zone;
   E_Win *win;
   Evas_Object *o, *sf, *ol;
   Data *d;
   
   zone = e_util_zone_current_get(e_manager_current_get());
   if (!zone) return NULL;
   win = e_win_new(zone->container);
   e_win_delete_callback_set(win, _cb_delete);
   e_win_resize_callback_set(win, _cb_resize);
   e_win_name_class_set(win, "E", name);
   e_win_title_set(win, title);
   d = E_NEW(Data, 1);
   d->win = win;
   d->delfunc = delfunc;
   d->data = data;
   win->data = d;
   o = _theme_obj_new(e_win_evas_get(win), themedir,
		      "e/modules/illume/config/dialog");
   edje_object_part_text_set(o, "e.text.label", "OK");
   edje_object_signal_callback_add(o, "e,action,do,ok", "", _cb_signal_ok, win);
   evas_object_show(o);
   d->o = o;
   
   ol = e_widget_list_add(e_win_evas_get(win), 0, 0);
   d->o_l = ol;
   
   evas_object_data_set(ol, "win", win);
   
   return ol;
}

static void
_e_cfg_win_complete(Evas_Object *ol)
{
   Evas_Object *sf;
   E_Win *win;
   Data *d;
   Evas_Coord mw, mh;

   win = evas_object_data_get(ol, "win");
   d = win->data;

   e_widget_min_size_get(ol, &mw, &mh);
   evas_object_resize(ol, mw, mh);

   sf = e_widget_scrollframe_simple_add(evas_object_evas_get(ol), ol);
   edje_object_part_swallow(d->o, "e.swallow.content", sf);
   d->o_sf = sf;
   
   e_win_show(win);

   e_widget_focus_set(ol, 1);
   evas_object_focus_set(ol, 1);

}

static void
_cb_signal_ok(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Win *win;
   Data *d;
   
   win = data;
   d = win->data;
   if (d->delfunc) d->delfunc(d->data);
   free(d);
   e_object_del(E_OBJECT(win));
}

static void
_cb_delete(E_Win *win)
{
   Data *d;
   
   d = win->data;
   if (d->delfunc) d->delfunc(d->data);
   free(d);
   e_object_del(E_OBJECT(win));
}
   
static void
_cb_resize(E_Win *win)
{
   Data *d;
   
   d = win->data;
   evas_object_resize(d->o, win->w, win->h);
}

static Evas_Object *
_theme_obj_new(Evas *e, const char *custom_dir, const char *group)
{
   Evas_Object *o;
   
   o = edje_object_add(e);
   if (!e_theme_edje_object_set(o, "base/theme/modules/illume", group))
     {
	if (custom_dir)
	  {
	     char buf[PATH_MAX];
	     
	     snprintf(buf, sizeof(buf), "%s/illume.edj", custom_dir);
	     if (edje_object_file_set(o, buf, group))
	       {
		  printf("OK FALLBACK %s\n", buf);
	       }
	  }
     }
   return o;
}







///////////////////////////////////////////////////////////////////////////////

typedef struct _DB_Method DB_Method;
struct _DB_Method
{
   const char *name;
   const char *params;
   const char *retrn;
   E_DBus_Method_Cb func;
};

//---

// illume_cfg->launcher.mode 0/1
static DBusMessage *
_dbcb_launcher_type_get(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessage *reply;
   DBusMessageIter iter;

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &(illume_cfg->launcher.mode));
   return reply;
}

static DBusMessage *
_dbcb_launcher_type_set(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   int val;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(val));
   if ((val >= 0) && (val <= 1))
     {
	illume_cfg->launcher.mode = val;
	_e_cfg_launcher_change(NULL, NULL, NULL);
	return dbus_message_new_method_return(msg);
     }
   reply = dbus_message_new_error(msg,
				  "org.enlightenment.DBus.InvalidArgument",
				  "Parameter must be 0 or 1");
   return reply;
}

// illume_cfg->launcher.icon_size 1-640
static DBusMessage *
_dbcb_launcher_icon_size_get(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessage *reply;
   DBusMessageIter iter;

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &(illume_cfg->launcher.icon_size));
   return reply;
}

static DBusMessage *
_dbcb_launcher_icon_size_set(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   int val;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(val));
   if ((val >= 1) && (val <= 640))
     {
	illume_cfg->launcher.icon_size = val;
	_e_cfg_launcher_change(NULL, NULL, NULL);
	return dbus_message_new_method_return(msg);
     }
   reply = dbus_message_new_error(msg,
				  "org.enlightenment.DBus.InvalidArgument",
				  "Parameter must be from 1 to 640");
   return reply;
}

// illume_cfg->launcher.single_click 0/1
static DBusMessage *
_dbcb_launcher_single_click_get(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessage *reply;
   DBusMessageIter iter;

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &(illume_cfg->launcher.single_click));
   return reply;
}

static DBusMessage *
_dbcb_launcher_single_click_set(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   int val;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(val));
   if ((val >= 0) && (val <= 1))
     {
	illume_cfg->launcher.single_click = val;
	_e_cfg_launcher_change(NULL, NULL, NULL);
	return dbus_message_new_method_return(msg);
     }
   reply = dbus_message_new_error(msg,
				  "org.enlightenment.DBus.InvalidArgument",
				  "Parameter must be 0 or 1");
   return reply;
}

// e_config->screensaver_timeout 0(off)-3600
static DBusMessage *
_dbcb_screensaver_timeout_get(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessage *reply;
   DBusMessageIter iter;
   int val;
     
   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   val = e_config->screensaver_timeout;
   if (!e_config->screensaver_enable) val = 0;
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &(val));
   return reply;
}

static DBusMessage *
_dbcb_screensaver_timeout_set(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   int val;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(val));
   if ((val >= 0) && (val <= 3600))
     {
	e_config->screensaver_timeout = val;
	_e_cfg_power_change(NULL, NULL, NULL);
	return dbus_message_new_method_return(msg);
     }
   reply = dbus_message_new_error(msg,
				  "org.enlightenment.DBus.InvalidArgument",
				  "Parameter must be from 0 to 3600");
   return reply;
}

// illume_cfg->power.auto_suspend_delay 0(off)-600
static DBusMessage *
_dbcb_autosuspend_timeout_get(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessage *reply;
   DBusMessageIter iter;
   int val;
     
   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   val = illume_cfg->power.auto_suspend_delay;
   if (!illume_cfg->power.auto_suspend) val = 0;
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &(val));
   return reply;
}

static DBusMessage *
_dbcb_autosuspend_timeout_set(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   int val;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(val));
   if ((val >= 0) && (val <= 600))
     {
	illume_cfg->power.auto_suspend_delay = val;
	_e_cfg_power_change(NULL, NULL, NULL);
	return dbus_message_new_method_return(msg);
     }
   reply = dbus_message_new_error(msg,
				  "org.enlightenment.DBus.InvalidArgument",
				  "Parameter must be from 0 to 600");
   return reply;
}

// illume_cfg->sliding.layout.duration
static DBusMessage *
_dbcb_slide_window_duration_get(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessage *reply;
   DBusMessageIter iter;
     
   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &(illume_cfg->sliding.layout.duration));
   return reply;
}

static DBusMessage *
_dbcb_slide_window_duration_set(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   int val;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(val));
   if ((val >= 0) && (val <= 4000))
     {
	illume_cfg->sliding.layout.duration = val;
	_e_cfg_animation_change(NULL, NULL, NULL);
	return dbus_message_new_method_return(msg);
     }
   reply = dbus_message_new_error(msg,
				  "org.enlightenment.DBus.InvalidArgument",
				  "Parameter must be from 0 to 4000");
   return reply;
}

// illume_cfg->sliding.slipshelf.duration
static DBusMessage *
_dbcb_slide_slipshelf_duration_get(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessage *reply;
   DBusMessageIter iter;
     
   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &(illume_cfg->sliding.slipshelf.duration));
   return reply;
}

static DBusMessage *
_dbcb_slide_slipshelf_duration_set(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   int val;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(val));
   if ((val >= 0) && (val <= 4000))
     {
	illume_cfg->sliding.slipshelf.duration = val;
	_e_cfg_animation_change(NULL, NULL, NULL);
	return dbus_message_new_method_return(msg);
     }
   reply = dbus_message_new_error(msg,
				  "org.enlightenment.DBus.InvalidArgument",
				  "Parameter must be from 0 to 4000");
   return reply;
}

// illume_cfg->sliding.kbd.duration
static DBusMessage *
_dbcb_slide_kbd_duration_get(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessage *reply;
   DBusMessageIter iter;
     
   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &(illume_cfg->sliding.kbd.duration));
   return reply;
}

static DBusMessage *
_dbcb_slide_kbd_duration_set(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   int val;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(val));
   if ((val >= 0) && (val <= 4000))
     {
	illume_cfg->sliding.kbd.duration = val;
	_e_cfg_animation_change(NULL, NULL, NULL);
	return dbus_message_new_method_return(msg);
     }
   reply = dbus_message_new_error(msg,
				  "org.enlightenment.DBus.InvalidArgument",
				  "Parameter must be from 0 to 4000");
   return reply;
}

// illume_cfg->sliding.busywin.duration
static DBusMessage *
_dbcb_slide_busywin_duration_get(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessage *reply;
   DBusMessageIter iter;
     
   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &(illume_cfg->sliding.busywin.duration));
   return reply;
}

static DBusMessage *
_dbcb_slide_busywin_duration_set(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   int val;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(val));
   if ((val >= 0) && (val <= 4000))
     {
	illume_cfg->sliding.busywin.duration = val;
	_e_cfg_animation_change(NULL, NULL, NULL);
	return dbus_message_new_method_return(msg);
     }
   reply = dbus_message_new_error(msg,
				  "org.enlightenment.DBus.InvalidArgument",
				  "Parameter must be from 0 to 4000");
   return reply;
}

// illume_cfg->slipshelf.main_gadget_size
static DBusMessage *
_dbcb_slipshelf_main_gadget_size_get(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessage *reply;
   DBusMessageIter iter;
     
   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &(illume_cfg->slipshelf.main_gadget_size));
   return reply;
}

static DBusMessage *
_dbcb_slipshelf_main_gadget_size_set(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   int val;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(val));
   if ((val >= 1) && (val <= 480))
     {
	illume_cfg->slipshelf.main_gadget_size = val;
	_e_cfg_slipshelf_change(NULL, NULL, NULL);
	return dbus_message_new_method_return(msg);
     }
   reply = dbus_message_new_error(msg,
				  "org.enlightenment.DBus.InvalidArgument",
				  "Parameter must be from 1 to 480");
   return reply;
}

// illume_cfg->slipshelf.extra_gagdet_size
static DBusMessage *
_dbcb_slipshelf_extra_gadget_size_get(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessage *reply;
   DBusMessageIter iter;
     
   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &(illume_cfg->slipshelf.extra_gagdet_size));
   return reply;
}

static DBusMessage *
_dbcb_slipshelf_extra_gadget_size_set(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   int val;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(val));
   if ((val >= 1) && (val <= 480))
     {
	illume_cfg->slipshelf.extra_gagdet_size = val;
	_e_cfg_slipshelf_change(NULL, NULL, NULL);
	return dbus_message_new_method_return(msg);
     }
   reply = dbus_message_new_error(msg,
				  "org.enlightenment.DBus.InvalidArgument",
				  "Parameter must be from 1 to 480");
   return reply;
}

// e_config->thumbscroll_threshhold
static DBusMessage *
_dbcb_thumbscroll_threshhold_get(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessage *reply;
   DBusMessageIter iter;
   int val;
   
   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   val = e_config->thumbscroll_threshhold;
   if (!e_config->thumbscroll_enable) val = 0;
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &(val));
   return reply;
}

static DBusMessage *
_dbcb_thumbscroll_threshhold_set(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   int val;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(val));
   if ((val >= 0) && (val <= 100))
     {
	e_config->thumbscroll_threshhold = val;
	_e_cfg_thumbscroll_change(NULL, NULL, NULL);
	return dbus_message_new_method_return(msg);
     }
   reply = dbus_message_new_error(msg,
				  "org.enlightenment.DBus.InvalidArgument",
				  "Parameter must be from 0 to 100");
   return reply;
}

// illume_cfg->performance.fps
static DBusMessage *
_dbcb_animation_fps_get(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessage *reply;
   DBusMessageIter iter;
     
   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &(illume_cfg->performance.fps));
   return reply;
}

static DBusMessage *
_dbcb_animation_fps_set(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   int val;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(val));
   if ((val >= 1) && (val <= 120))
     {
	illume_cfg->performance.fps = val;
	_e_cfg_fps_change(NULL, NULL, NULL);
	return dbus_message_new_method_return(msg);
     }
   reply = dbus_message_new_error(msg,
				  "org.enlightenment.DBus.InvalidArgument",
				  "Parameter must be from 1 to 120");
   return reply;
}

// list available gadgets
static DBusMessage *
_dbcb_gadget_list_get(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessage *reply;
   DBusMessageIter iter, arr;
   Eina_List *l;
     
   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &arr);
   for (l = e_gadcon_provider_list(); l; l = l->next)
     {
	E_Gadcon_Client_Class *cc;
	
	if (!(cc = l->data)) continue;
        dbus_message_iter_append_basic(&arr, DBUS_TYPE_STRING, &(cc->name));
     }
   dbus_message_iter_close_container(&iter, &arr);
   return reply;
}

// list gadgets for main shelf
static DBusMessage *
_dbcb_slipshelf_main_gadget_list_get(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessage *reply;
   DBusMessageIter iter, arr;
   Eina_List *l;
     
   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &arr);
   for (l = slipshelf->gadcon->cf->clients; l; l = l->next)
     {
	E_Config_Gadcon_Client *gccc;

	if (!(gccc = l->data)) continue;
        dbus_message_iter_append_basic(&arr, DBUS_TYPE_STRING, &(gccc->name));
     }
   dbus_message_iter_close_container(&iter, &arr);
   return reply;
}

// + gadgets for main shelf
static DBusMessage *
_dbcb_slipshelf_main_gadget_add(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   char *s;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(s));
   if (s)
     {
	if (!e_gadcon_client_config_new(slipshelf->gadcon, s))
	  goto invalid;
	e_gadcon_unpopulate(slipshelf->gadcon);
	e_gadcon_populate(slipshelf->gadcon);
	e_config_save_queue();
	return dbus_message_new_method_return(msg);
     }
   invalid:
   reply = dbus_message_new_error(msg,
				  "org.enlightenment.DBus.InvalidArgument",
				  "Parameter not valid");
   return reply;
}

// - gadgets for main shelf
static DBusMessage *
_dbcb_slipshelf_main_gadget_del(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   char *s;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(s));
   if (s)
     {
	Eina_List *l;
	
	for (l = slipshelf->gadcon->clients; l; l = l->next)
	  {
	     E_Config_Gadcon_Client *cgc;
	     
	     if (!(cgc = l->data)) continue;
	     if (strcmp(s, cgc->name)) continue;
	     e_gadcon_client_config_del(slipshelf->gadcon->cf, cgc);
	     break;
	  }
	e_gadcon_unpopulate(slipshelf->gadcon);
	e_gadcon_populate(slipshelf->gadcon);
	e_config_save_queue();
	return dbus_message_new_method_return(msg);
     }
   reply = dbus_message_new_error(msg,
				  "org.enlightenment.DBus.InvalidArgument",
				  "Parameter not valid");
   return reply;
}

// list gadgets for extra shelf
static DBusMessage *
_dbcb_slipshelf_extra_gadget_list_get(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessage *reply;
   DBusMessageIter iter, arr;
   Eina_List *l;
     
   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &arr);
   for (l = slipshelf->gadcon_extra->cf->clients; l; l = l->next)
     {
	E_Config_Gadcon_Client *gccc;

	if (!(gccc = l->data)) continue;
        dbus_message_iter_append_basic(&arr, DBUS_TYPE_STRING, &(gccc->name));
     }
   dbus_message_iter_close_container(&iter, &arr);
   return reply;
}

// + gadgets for extra shelf
static DBusMessage *
_dbcb_slipshelf_extra_gadget_add(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   char *s;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(s));
   if (s)
     {
	if (!e_gadcon_client_config_new(slipshelf->gadcon_extra, s))
	  goto invalid;
	e_gadcon_unpopulate(slipshelf->gadcon_extra);
	e_gadcon_populate(slipshelf->gadcon_extra);
	e_config_save_queue();
	return dbus_message_new_method_return(msg);
     }
   invalid:
   reply = dbus_message_new_error(msg,
				  "org.enlightenment.DBus.InvalidArgument",
				  "Parameter not valid");
   return reply;
}

// - gadgets for extra shelf
static DBusMessage *
_dbcb_slipshelf_extra_gadget_del(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   char *s;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(s));
   if (s)
     {
	Eina_List *l;
	
	for (l = slipshelf->gadcon_extra->clients; l; l = l->next)
	  {
	     E_Config_Gadcon_Client *cgc;
	     
	     if (!(cgc = l->data)) continue;
	     if (strcmp(s, cgc->name)) continue;
	     e_gadcon_client_config_del(slipshelf->gadcon_extra->cf, cgc);
	     break;
	  }
	e_gadcon_unpopulate(slipshelf->gadcon_extra);
	e_gadcon_populate(slipshelf->gadcon_extra);
	e_config_save_queue();
	return dbus_message_new_method_return(msg);
     }
   reply = dbus_message_new_error(msg,
				  "org.enlightenment.DBus.InvalidArgument",
				  "Parameter not valid");
   return reply;
}

// get keyboard - "none", "internal", ["a.desktop", "b.desktop" ...]
static DBusMessage *
_dbcb_keyboard_get(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   char *s;
   
   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   s = "invalid";
   if ((!illume_cfg->kbd.use_internal) && (!illume_cfg->kbd.run_keyboard))
     s = "none";
   else if ((illume_cfg->kbd.use_internal) && (!illume_cfg->kbd.run_keyboard))
     s = "internal";
   else if (illume_cfg->kbd.run_keyboard)
     s = illume_cfg->kbd.run_keyboard;
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &(s));
   return reply;
}

// set kebyoard - "none", "internal", ["a.desktop", "b.desktop" ...]
static DBusMessage *
_dbcb_keyboard_set(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   char *s;
   
   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &(s));
   if (s)
     {
	illume_cfg->kbd.use_internal = 0;
	if (illume_cfg->kbd.run_keyboard)
	  {
	     evas_stringshare_del(illume_cfg->kbd.run_keyboard);
	     illume_cfg->kbd.run_keyboard = NULL;
	  }
	if (!strcmp(s, "none"))
	  illume_cfg->kbd.use_internal = 0;
	else if (!strcmp(s, "internal"))
	  illume_cfg->kbd.use_internal = 1;
	else
	  illume_cfg->kbd.run_keyboard = evas_stringshare_add(s);
	e_mod_win_cfg_kbd_update();
	e_config_save_queue();
	return dbus_message_new_method_return(msg);
     }
   reply = dbus_message_new_error(msg,
				  "org.enlightenment.DBus.InvalidArgument",
				  "Parameter not valid. must be a valid .desktop file name, or 'none' or 'internal'");
   return reply;
}

static const DB_Method methods[] =
{
   {"LauncherTypeGet", "", "i", _dbcb_launcher_type_get},
   {"LauncherTypeSet", "i", "", _dbcb_launcher_type_set},
   {"LauncherIconSizeGet", "", "i", _dbcb_launcher_icon_size_get},
   {"LauncherIconSizeSet", "i", "", _dbcb_launcher_icon_size_set},
   {"LauncherSingleClickGet", "", "i", _dbcb_launcher_single_click_get},
   {"LauncherSingleClickSet", "i", "", _dbcb_launcher_single_click_set},
   {"ScreensaverTimeoutGet", "", "i", _dbcb_screensaver_timeout_get},
   {"ScreensaverTimeoutSet", "i", "", _dbcb_screensaver_timeout_set},
   {"AutosuspendTimeoutGet", "", "i", _dbcb_autosuspend_timeout_get},
   {"AutosuspendTimeoutSet", "i", "", _dbcb_autosuspend_timeout_set},
   {"SlideWindowDurationGet", "", "i", _dbcb_slide_window_duration_get},
   {"SlideWindowDurationSet", "i", "", _dbcb_slide_window_duration_set},
   {"SlideSlipshelfDurationGet", "", "i", _dbcb_slide_slipshelf_duration_get},
   {"SlideSlipshelfDurationSet", "i", "", _dbcb_slide_slipshelf_duration_set},
   {"SlideKeyboardDurationGet", "", "i", _dbcb_slide_kbd_duration_get},
   {"SlideKeyboardDurationSet", "i", "", _dbcb_slide_kbd_duration_set},
   {"SlideBusywinDurationGet", "", "i", _dbcb_slide_busywin_duration_get},
   {"SlideBusywinDurationSet", "i", "", _dbcb_slide_busywin_duration_set},
   {"SlipshelfMainGadgetSizeGet", "", "i", _dbcb_slipshelf_main_gadget_size_get},
   {"SlipshelfMainGadgetSizeSet", "i", "", _dbcb_slipshelf_main_gadget_size_set},
   {"SlipshelfExtraGadgetSizeGet", "", "i", _dbcb_slipshelf_extra_gadget_size_get},
   {"SlipshelfExtraGadgetSizeSet", "i", "", _dbcb_slipshelf_extra_gadget_size_set},
   {"ThumbscrollThreshholdGet", "", "i", _dbcb_thumbscroll_threshhold_get},
   {"ThumbscrollThreshholdSet", "i", "", _dbcb_thumbscroll_threshhold_set},
   {"AnimationFramerateGet", "", "i", _dbcb_animation_fps_get},
   {"AnimationFramerateSet", "i", "", _dbcb_animation_fps_set},
   {"GadgetListGet", "", "as", _dbcb_gadget_list_get},
   {"SlipshelfMainGadgetListGet", "", "as", _dbcb_slipshelf_main_gadget_list_get},
   {"SlipshelfMainGadgetAdd", "s", "", _dbcb_slipshelf_main_gadget_add},
   {"SlipshelfMainGadgetDel", "s", "", _dbcb_slipshelf_main_gadget_del},
   {"SlipshelfExtraGadgetListGet", "", "as", _dbcb_slipshelf_extra_gadget_list_get},
   {"SlipshelfExtraGadgetAdd", "s", "", _dbcb_slipshelf_extra_gadget_add},
   {"SlipshelfExtraGadgetDel", "s", "", _dbcb_slipshelf_extra_gadget_del},
   {"KeyboardGet", "", "s", _dbcb_keyboard_get},
   {"KeyboardSet", "s", "", _dbcb_keyboard_set}
};

//---

static E_DBus_Interface *dbus_if = NULL;

static void
_e_cfg_dbus_if_init(void)
{
   dbus_if = e_dbus_interface_new("org.enlightenment.wm.IllumeConfiguration");
   if (dbus_if)
     {
	int i;

	for (i = 0; i < sizeof(methods) / sizeof(DB_Method); i++)
	  e_dbus_interface_method_add(dbus_if, methods[i].name,
				      methods[i].params, methods[i].retrn,
				      methods[i].func);
	e_msgbus_interface_attach(dbus_if);
     }
}

static void
_e_cfg_dbus_if_shutdown(void)
{
   if (dbus_if)
     {
	e_msgbus_interface_detach(dbus_if);
	e_dbus_interface_unref(dbus_if);
	dbus_if = NULL;
     }
}
