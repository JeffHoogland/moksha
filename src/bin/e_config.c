#include "e.h"

/* TODO List
 * 
 * * setting up a new config value and a listener callback is too long winded - need to have helper funcs and macros do this so it's more like 1 line per new config value or 2
 */

#if ((E17_PROFILE >= LOWRES_PDA) && (E17_PROFILE <= HIRES_PDA))
#define DEF_MENUCLICK 1.25
#else
#define DEF_MENUCLICK 0.25
#endif

char   *e_config_val_desktop_default_background = NULL;
double  e_config_val_menus_scroll_speed = 1000.0;
double  e_config_val_menus_fast_mouse_move_thresthold = 500.0;
double  e_config_val_menus_click_drag_timeout = DEF_MENUCLICK;
double  e_config_val_framerate = 30.0;
int     e_config_val_image_cache = 2048;
int     e_config_val_font_cache = 512;

/* local subsystem functions */
static void _e_config_save_cb(void *data);

static int _e_config_listener_desktop_default_background(const char *key, const Ecore_Config_Type type, const int tag, void *data);
static int _e_config_listener_menus_scroll_speed(const char *key, const Ecore_Config_Type type, const int tag, void *data);
static int _e_config_listener_menus_fast_mouse_move_threshold(const char *key, const Ecore_Config_Type type, const int tag, void *data);
static int _e_config_listener_menus_click_drag_timeout(const char *key, const Ecore_Config_Type type, const int tag, void *data);
static int _e_config_listener_framerate(const char *key, const Ecore_Config_Type type, const int tag, void *data);
static int _e_config_listener_image_cache(const char *key, const Ecore_Config_Type type, const int tag, void *data);
static int _e_config_listener_font_cache(const char *key, const Ecore_Config_Type type, const int tag, void *data);

/* local subsystem globals */
static Ecore_Job *_e_config_save_job = NULL;

/* externally accessible functions */
int
e_config_init(void)
{
   int ret;
   
   ecore_config_init("e");
   
   ecore_config_string_create
     ("e.desktop.default.background",
      PACKAGE_DATA_DIR"/data/themes/default.eet",
      'b', "default-background",
      "The default background for desktops without a custom background");
   ecore_config_float_create_bound
     ("e.menus.scroll_speed",
      1000.0, 1.0, 20000.0, 10.0,
      0, "menus-scroll-speed",
      "Pixels per second menus scroll around the screen");
   ecore_config_float_create_bound
     ("e.menus.fast_mouse_move_threshold",
      300.0, 1.0, 2000.0, 1.0,
      0, "menus-scroll-speed",
      "Pixels per second menus scroll around the screen");
   ecore_config_float_create_bound
     ("e.menus.click_drag_timeout",
      DEF_MENUCLICK, 0.0, 10.0, 0.01,
      0, "menus-click-drag-timeout",
      "Seconds after a mouse press when a release will not hide the menu");
   ecore_config_float_create_bound
     ("e.framerate",
      30.0, 1.0, 200.0, 0.1,
      0, "framerate",
      "A hint at the framerate (in frames per second) Enlightenment should try and animate at");
   ecore_config_int_create_bound
     ("e.image-cache",
      2048, 0, 32768, 1,
      0, "image-cache",
      "The mount of memory (in Kb) to use as a sepculative image cache");
   ecore_config_int_create_bound
     ("e.font-cache",
      512, 0, 4096, 1,
      0, "font-cache",
      "The mount of memory (in Kb) to use as a sepculative font cache");
   
   ecore_config_load();
   ret = ecore_config_args_parse();
   
   e_config_val_desktop_default_background = 
     ecore_config_string_get("e.desktop.default.background");
   ecore_config_listen("e.desktop.default.background",
		       "e.desktop.default.background",
		       _e_config_listener_desktop_default_background,
		       0, NULL);
   e_config_val_menus_scroll_speed =
     ecore_config_float_get("e.menus.scroll_speed");
   ecore_config_listen("e.menus.scroll_speed",
		       "e.menus.scroll_speed",
		       _e_config_listener_menus_scroll_speed,
		       0, NULL);
   e_config_val_menus_fast_mouse_move_thresthold =
     ecore_config_float_get("e.menus.fast_mouse_move_threshold");
   ecore_config_listen("e.menus.fast_mouse_move_threshold",
		       "e.menus.fast_mouse_move_threshold",
		       _e_config_listener_menus_fast_mouse_move_threshold,
		       0, NULL);
   e_config_val_menus_click_drag_timeout =
     ecore_config_float_get("e.menus.click_drag_timeout");
   ecore_config_listen("e.menus.click_drag_timeout",
		       "e.menus.click_drag_timeout",
		       _e_config_listener_menus_click_drag_timeout,
		       0, NULL);
   e_config_val_framerate =
     ecore_config_float_get("e.framerate");
   if (e_config_val_framerate <= 0.0) e_config_val_framerate = 30.0;
   ecore_config_listen("e.framerate",
		       "e.framerate",
		       _e_config_listener_framerate,
		       0, NULL);
   e_config_val_image_cache =
     ecore_config_int_get("e.image-cache");
   ecore_config_listen("e.image-cache",
		       "e.image-cache",
		       _e_config_listener_image_cache,
		       0, NULL);
   e_config_val_font_cache =
     ecore_config_int_get("e.font-cache");
   ecore_config_listen("e.font-cache",
		       "e.font-cache",
		       _e_config_listener_font_cache,
		       0, NULL);
   return 1;
}

int
e_config_shutdown(void)
{
   /* FIXME: unset listeners */
   return 1;
}

int
e_config_save(void)
{
   if (_e_config_save_job)
     {
	ecore_job_del(_e_config_save_job);
	_e_config_save_job = NULL;
     }
   return ecore_config_save();
}

void
e_config_save_queue(void)
{
   if (_e_config_save_job) ecore_job_del(_e_config_save_job);
   _e_config_save_job = ecore_job_add(_e_config_save_cb, NULL);
}

/* local subsystem functions */
static void
_e_config_save_cb(void *data)
{
   _e_config_save_job = NULL;
   e_module_save_all();
   e_config_save();
}

static int
_e_config_listener_desktop_default_background(const char *key, const Ecore_Config_Type type, const int tag, void *data)
{
   Evas_List *managers, *l;
   
   if (e_config_val_desktop_default_background)
     free(e_config_val_desktop_default_background);
   e_config_val_desktop_default_background = 
     ecore_config_string_get("e.desktop.default.background");
   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
	Evas_List *ll;
	E_Manager *man;
	
	man = l->data;
	for (ll = man->containers; ll; ll = ll->next)
	  {
	     E_Container *con;
	     
	     con = ll->data;
	     e_container_bg_reconfigure(con);
	  }
     }
   return 1;
}

static int
_e_config_listener_menus_scroll_speed(const char *key, const Ecore_Config_Type type, const int tag, void *data)
{
   e_config_val_menus_scroll_speed =
     ecore_config_float_get("e.menus.scroll_speed");
   return 1;
}

static int
_e_config_listener_menus_fast_mouse_move_threshold(const char *key, const Ecore_Config_Type type, const int tag, void *data)
{
   e_config_val_menus_fast_mouse_move_thresthold =
     ecore_config_float_get("e.menus.fast_mouse_move_threshold");
   return 1;
}

static int
_e_config_listener_menus_click_drag_timeout(const char *key, const Ecore_Config_Type type, const int tag, void *data)
{
   e_config_val_menus_click_drag_timeout =
     ecore_config_float_get("e.menus.click_drag_timeout");
   return 1;
}

static int
_e_config_listener_framerate(const char *key, const Ecore_Config_Type type, const int tag, void *data)
{
   e_config_val_framerate =
     ecore_config_float_get("e.framerate");
   if (e_config_val_framerate <= 0.0) e_config_val_framerate = 30.0;
   edje_frametime_set(1.0 / e_config_val_framerate);
   return 1;
}

static int
_e_config_listener_image_cache(const char *key, const Ecore_Config_Type type, const int tag, void *data)
{
   e_config_val_image_cache =
     ecore_config_int_get("e.image-cache");
   ecore_config_listen("e.image-cache",
		       "e.image-cache",
		       _e_config_listener_image_cache,
		       0, NULL);
   e_canvas_recache();
   return 1;
}

static int
_e_config_listener_font_cache(const char *key, const Ecore_Config_Type type, const int tag, void *data)
{
   e_config_val_font_cache =
     ecore_config_int_get("e.font-cache");
   ecore_config_listen("e.font-cache",
		       "e.font-cache",
		       _e_config_listener_font_cache,
		       0, NULL);
   e_canvas_recache();
   return 1;
}
