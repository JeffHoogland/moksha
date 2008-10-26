/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#if ((E17_PROFILE >= LOWRES_PDA) && (E17_PROFILE <= HIRES_PDA))
#define DEF_MENUCLICK 1.25
#else
#define DEF_MENUCLICK 0.25
#endif

EAPI E_Config *e_config = NULL;

/* local subsystem functions */
static void _e_config_save_cb(void *data);
static void _e_config_free(void);
static int  _e_config_cb_timer(void *data);
static int  _e_config_eet_close_handle(Eet_File *ef, char *file);

/* local subsystem globals */
static int _e_config_save_block = 0;
static E_Powersave_Deferred_Action *_e_config_save_defer = NULL;
static char *_e_config_profile = NULL;

static E_Config_DD *_e_config_edd = NULL;
static E_Config_DD *_e_config_module_edd = NULL;
static E_Config_DD *_e_config_font_fallback_edd = NULL;
static E_Config_DD *_e_config_font_default_edd = NULL;
static E_Config_DD *_e_config_theme_edd = NULL;
static E_Config_DD *_e_config_bindings_mouse_edd = NULL;
static E_Config_DD *_e_config_bindings_key_edd = NULL;
static E_Config_DD *_e_config_bindings_signal_edd = NULL;
static E_Config_DD *_e_config_bindings_wheel_edd = NULL;
static E_Config_DD *_e_config_path_append_edd = NULL;
static E_Config_DD *_e_config_desktop_bg_edd = NULL;
static E_Config_DD *_e_config_desktop_name_edd = NULL;
static E_Config_DD *_e_config_remember_edd = NULL;
static E_Config_DD *_e_config_color_class_edd = NULL;
static E_Config_DD *_e_config_gadcon_edd = NULL;
static E_Config_DD *_e_config_gadcon_client_edd = NULL;
static E_Config_DD *_e_config_shelf_edd = NULL;
static E_Config_DD *_e_config_shelf_desk_edd = NULL;
static E_Config_DD *_e_config_mime_icon_edd = NULL;

EAPI int E_EVENT_CONFIG_ICON_THEME = 0;

/* externally accessible functions */
EAPI int
e_config_init(void)
{
   E_EVENT_CONFIG_ICON_THEME = ecore_event_type_new();

   _e_config_profile = getenv("E_CONF_PROFILE");
   if (_e_config_profile)
     /* if environment var set - use this profile name */
    _e_config_profile = strdup(_e_config_profile);
   else 
     {
	Eet_File *ef;
	char buf[4096];
	const char *homedir;

	/* try user profile config */
	homedir = e_user_homedir_get();
	snprintf(buf, sizeof(buf), "%s/.e/e/config/profile.cfg",
		 homedir);
	ef = eet_open(buf, EET_FILE_MODE_READ);
	if (!ef)
	  {
	     /* use system if no user profile config */
	     snprintf(buf, sizeof(buf), "%s/data/config/profile.cfg",
		      e_prefix_data_get());
	     ef = eet_open(buf, EET_FILE_MODE_READ);
	  }
	if (ef)
	  {
	     /* profile config exists */
	     char *data;
	     int data_len = 0;

	     data = eet_read(ef, "config", &data_len);
	     if ((data) && (data_len > 0))
	       {
		  _e_config_profile = malloc(data_len + 1);
		  if (_e_config_profile)
		    {
		       memcpy(_e_config_profile, data, data_len);
		       _e_config_profile[data_len] = 0;
		    }
		  free(data);
	       }
	     eet_close(ef);
	  }
	else
	  {
	     /* no profile config - try other means */
	     char *link = NULL;
	     
	     /* check symlink - if default is a symlink to another dir */
             snprintf(buf, sizeof(buf), "%s/data/config/default",
		      e_prefix_data_get());
	     link = ecore_file_readlink(buf);
	     /* if so use just the filename as the priofle - must be a local link */
	     if (link)
	       {
		  _e_config_profile = strdup(ecore_file_file_get(link));
		  free(link);
	       }
	     else
	       _e_config_profile = strdup("default");
	  }
	e_util_env_set("E_CONF_PROFILE", _e_config_profile);
     }

   _e_config_gadcon_client_edd = E_CONFIG_DD_NEW("E_Config_Gadcon_Client", E_Config_Gadcon_Client);
#undef T
#undef D
#define T E_Config_Gadcon_Client
#define D _e_config_gadcon_client_edd
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, id, STR);
   E_CONFIG_VAL(D, T, geom.pos, INT);
   E_CONFIG_VAL(D, T, geom.size, INT);
   E_CONFIG_VAL(D, T, geom.res, INT);
   E_CONFIG_VAL(D, T, geom.pos_x, DOUBLE);
   E_CONFIG_VAL(D, T, geom.pos_y, DOUBLE);
   E_CONFIG_VAL(D, T, geom.size_w, DOUBLE);
   E_CONFIG_VAL(D, T, geom.size_h, DOUBLE);
   E_CONFIG_VAL(D, T, state_info.seq, INT);
   E_CONFIG_VAL(D, T, state_info.flags, INT);
   E_CONFIG_VAL(D, T, style, STR);
   E_CONFIG_VAL(D, T, autoscroll, UCHAR);
   E_CONFIG_VAL(D, T, resizable, UCHAR);
   
   _e_config_gadcon_edd = E_CONFIG_DD_NEW("E_Config_Gadcon", E_Config_Gadcon);
#undef T
#undef D
#define T E_Config_Gadcon
#define D _e_config_gadcon_edd
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, id, INT);
   E_CONFIG_LIST(D, T, clients, _e_config_gadcon_client_edd);

   _e_config_shelf_desk_edd = E_CONFIG_DD_NEW("E_Config_Shelf_Desk", E_Config_Shelf_Desk);
#undef T
#undef D
#define T E_Config_Shelf_Desk
#define D _e_config_shelf_desk_edd
   E_CONFIG_VAL(D, T, x, INT);
   E_CONFIG_VAL(D, T, y, INT);
   
   _e_config_shelf_edd = E_CONFIG_DD_NEW("E_Config_Shelf", E_Config_Shelf);
#undef T
#undef D
#define T E_Config_Shelf
#define D _e_config_shelf_edd
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, id, INT);
   E_CONFIG_VAL(D, T, container, INT);
   E_CONFIG_VAL(D, T, zone, INT);
   E_CONFIG_VAL(D, T, layer, INT);
   E_CONFIG_VAL(D, T, popup, UCHAR);
   E_CONFIG_VAL(D, T, orient, INT);
   E_CONFIG_VAL(D, T, fit_along, UCHAR);
   E_CONFIG_VAL(D, T, fit_size, UCHAR);
   E_CONFIG_VAL(D, T, style, STR);
   E_CONFIG_VAL(D, T, size, INT);
   E_CONFIG_VAL(D, T, overlap, INT);
   E_CONFIG_VAL(D, T, autohide, INT);
   E_CONFIG_VAL(D, T, autohide_show_action, INT);
   E_CONFIG_VAL(D, T, hide_timeout, FLOAT);
   E_CONFIG_VAL(D, T, hide_duration, FLOAT);
   E_CONFIG_VAL(D, T, desk_show_mode, INT);
   E_CONFIG_LIST(D, T, desk_list, _e_config_shelf_desk_edd);

   _e_config_desktop_bg_edd = E_CONFIG_DD_NEW("E_Config_Desktop_Background", E_Config_Desktop_Background);
#undef T
#undef D
#define T E_Config_Desktop_Background
#define D _e_config_desktop_bg_edd
   E_CONFIG_VAL(D, T, container, INT);
   E_CONFIG_VAL(D, T, zone, INT);
   E_CONFIG_VAL(D, T, desk_x, INT);
   E_CONFIG_VAL(D, T, desk_y, INT);
   E_CONFIG_VAL(D, T, file, STR);
   
   _e_config_desktop_name_edd = E_CONFIG_DD_NEW("E_Config_Desktop_Name", E_Config_Desktop_Name);
#undef T
#undef D
#define T E_Config_Desktop_Name
#define D _e_config_desktop_name_edd
   E_CONFIG_VAL(D, T, container, INT);
   E_CONFIG_VAL(D, T, zone, INT);
   E_CONFIG_VAL(D, T, desk_x, INT);
   E_CONFIG_VAL(D, T, desk_y, INT);
   E_CONFIG_VAL(D, T, name, STR);

   _e_config_path_append_edd = E_CONFIG_DD_NEW("E_Path_Dir", E_Path_Dir);
#undef T
#undef D
#define T E_Path_Dir
#define D _e_config_path_append_edd
   E_CONFIG_VAL(D, T, dir, STR);

   _e_config_theme_edd = E_CONFIG_DD_NEW("E_Config_Theme", E_Config_Theme);
#undef T
#undef D
#define T E_Config_Theme
#define D _e_config_theme_edd
   E_CONFIG_VAL(D, T, category, STR);
   E_CONFIG_VAL(D, T, file, STR);
   
   _e_config_module_edd = E_CONFIG_DD_NEW("E_Config_Module", E_Config_Module);
#undef T
#undef D
#define T E_Config_Module
#define D _e_config_module_edd
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, enabled, UCHAR);
   E_CONFIG_VAL(D, T, delayed, UCHAR);
   E_CONFIG_VAL(D, T, priority, INT);

   _e_config_font_default_edd = E_CONFIG_DD_NEW("E_Font_Default", 
						E_Font_Default);
#undef T
#undef D
#define T E_Font_Default
#define D _e_config_font_default_edd
   E_CONFIG_VAL(D, T, text_class, STR);
   E_CONFIG_VAL(D, T, font, STR);
   E_CONFIG_VAL(D, T, size, INT);

   _e_config_font_fallback_edd = E_CONFIG_DD_NEW("E_Font_Fallback", 
						  E_Font_Fallback);
#undef T
#undef D
#define T E_Font_Fallback
#define D _e_config_font_fallback_edd
   E_CONFIG_VAL(D, T, name, STR);

   _e_config_bindings_mouse_edd = E_CONFIG_DD_NEW("E_Config_Binding_Mouse", 
						  E_Config_Binding_Mouse);
#undef T
#undef D
#define T E_Config_Binding_Mouse
#define D _e_config_bindings_mouse_edd
   E_CONFIG_VAL(D, T, context, INT);
   E_CONFIG_VAL(D, T, modifiers, INT);
   E_CONFIG_VAL(D, T, action, STR);
   E_CONFIG_VAL(D, T, params, STR);
   E_CONFIG_VAL(D, T, button, UCHAR);
   E_CONFIG_VAL(D, T, any_mod, UCHAR);

   _e_config_bindings_key_edd = E_CONFIG_DD_NEW("E_Config_Binding_Key", 
						E_Config_Binding_Key);
#undef T
#undef D
#define T E_Config_Binding_Key
#define D _e_config_bindings_key_edd
   E_CONFIG_VAL(D, T, context, INT);
   E_CONFIG_VAL(D, T, modifiers, INT);
   E_CONFIG_VAL(D, T, key, STR);
   E_CONFIG_VAL(D, T, action, STR);
   E_CONFIG_VAL(D, T, params, STR);
   E_CONFIG_VAL(D, T, any_mod, UCHAR);

   _e_config_bindings_signal_edd = E_CONFIG_DD_NEW("E_Config_Binding_Signal", 
						   E_Config_Binding_Signal);
#undef T
#undef D
#define T E_Config_Binding_Signal
#define D _e_config_bindings_signal_edd
   E_CONFIG_VAL(D, T, context, INT);
   E_CONFIG_VAL(D, T, signal, STR);
   E_CONFIG_VAL(D, T, source, STR);
   E_CONFIG_VAL(D, T, modifiers, INT);
   E_CONFIG_VAL(D, T, any_mod, UCHAR);
   E_CONFIG_VAL(D, T, action, STR);
   E_CONFIG_VAL(D, T, params, STR);

   _e_config_bindings_wheel_edd = E_CONFIG_DD_NEW("E_Config_Binding_Wheel", 
						  E_Config_Binding_Wheel);
#undef T
#undef D
#define T E_Config_Binding_Wheel
#define D _e_config_bindings_wheel_edd
   E_CONFIG_VAL(D, T, context, INT);
   E_CONFIG_VAL(D, T, direction, INT);
   E_CONFIG_VAL(D, T, z, INT);
   E_CONFIG_VAL(D, T, modifiers, INT);
   E_CONFIG_VAL(D, T, any_mod, UCHAR);
   E_CONFIG_VAL(D, T, action, STR);
   E_CONFIG_VAL(D, T, params, STR);

   _e_config_remember_edd = E_CONFIG_DD_NEW("E_Remember", E_Remember);
#undef T
#undef D
#define T E_Remember
#define D _e_config_remember_edd
   E_CONFIG_VAL(D, T, match, INT);
   E_CONFIG_VAL(D, T, apply_first_only, UCHAR);
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, class, STR);
   E_CONFIG_VAL(D, T, title, STR);
   E_CONFIG_VAL(D, T, role, STR);
   E_CONFIG_VAL(D, T, type, INT);
   E_CONFIG_VAL(D, T, transient, UCHAR);
   E_CONFIG_VAL(D, T, apply, INT);
   E_CONFIG_VAL(D, T, max_score, INT);
   E_CONFIG_VAL(D, T, prop.pos_x, INT);
   E_CONFIG_VAL(D, T, prop.pos_y, INT);
   E_CONFIG_VAL(D, T, prop.res_x, INT);
   E_CONFIG_VAL(D, T, prop.res_y, INT);
   E_CONFIG_VAL(D, T, prop.pos_w, INT);
   E_CONFIG_VAL(D, T, prop.pos_h, INT);
   E_CONFIG_VAL(D, T, prop.w, INT);
   E_CONFIG_VAL(D, T, prop.h, INT);
   E_CONFIG_VAL(D, T, prop.layer, INT);
   E_CONFIG_VAL(D, T, prop.lock_user_location, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_location, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_size, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_size, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_stacking, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_stacking, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_iconify, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_iconify, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_desk, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_desk, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_sticky, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_sticky, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_shade, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_shade, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_maximize, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_maximize, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_fullscreen, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_fullscreen, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_border, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_close, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_focus_in, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_focus_out, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_life, UCHAR);
   E_CONFIG_VAL(D, T, prop.border, STR);
   E_CONFIG_VAL(D, T, prop.sticky, UCHAR);
   E_CONFIG_VAL(D, T, prop.shaded, UCHAR);
   E_CONFIG_VAL(D, T, prop.skip_winlist, UCHAR);
   E_CONFIG_VAL(D, T, prop.skip_pager, UCHAR);
   E_CONFIG_VAL(D, T, prop.skip_taskbar, UCHAR);
   E_CONFIG_VAL(D, T, prop.desk_x, INT);
   E_CONFIG_VAL(D, T, prop.desk_y, INT);
   E_CONFIG_VAL(D, T, prop.zone, INT);
   E_CONFIG_VAL(D, T, prop.head, INT);
   E_CONFIG_VAL(D, T, prop.command, STR);
   E_CONFIG_VAL(D, T, prop.icon_preference, UCHAR);
   
   _e_config_color_class_edd = E_CONFIG_DD_NEW("E_Color_Class", E_Color_Class);
#undef T
#undef D
#define T E_Color_Class
#define D _e_config_color_class_edd
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, r, INT);
   E_CONFIG_VAL(D, T, g, INT);
   E_CONFIG_VAL(D, T, b, INT);
   E_CONFIG_VAL(D, T, a, INT);
   E_CONFIG_VAL(D, T, r2, INT);
   E_CONFIG_VAL(D, T, g2, INT);
   E_CONFIG_VAL(D, T, b2, INT);
   E_CONFIG_VAL(D, T, a2, INT);
   E_CONFIG_VAL(D, T, r3, INT);
   E_CONFIG_VAL(D, T, g3, INT);
   E_CONFIG_VAL(D, T, b3, INT);
   E_CONFIG_VAL(D, T, a3, INT);

   _e_config_mime_icon_edd = E_CONFIG_DD_NEW("E_Config_Mime_Icon", 
					     E_Config_Mime_Icon);
#undef T
#undef D
#define T E_Config_Mime_Icon
#define D _e_config_mime_icon_edd
   E_CONFIG_VAL(D, T, mime, STR);
   E_CONFIG_VAL(D, T, icon, STR);

   _e_config_edd = E_CONFIG_DD_NEW("E_Config", E_Config);
#undef T
#undef D
#define T E_Config
#define D _e_config_edd
   /**/ /* == already configurable via ipc */
   E_CONFIG_VAL(D, T, config_version, INT); /**/
   E_CONFIG_VAL(D, T, show_splash, INT); /**/
   E_CONFIG_VAL(D, T, init_default_theme, STR); /**/
   E_CONFIG_VAL(D, T, desktop_default_background, STR); /**/
   E_CONFIG_VAL(D, T, desktop_default_name, STR); /**/
   E_CONFIG_LIST(D, T, desktop_backgrounds, _e_config_desktop_bg_edd); /**/
   E_CONFIG_LIST(D, T, desktop_names, _e_config_desktop_name_edd); /**/
   E_CONFIG_VAL(D, T, menus_scroll_speed, DOUBLE); /**/
   E_CONFIG_VAL(D, T, menus_fast_mouse_move_threshhold, DOUBLE); /**/
   E_CONFIG_VAL(D, T, menus_click_drag_timeout, DOUBLE); /**/
   E_CONFIG_VAL(D, T, border_shade_animate, INT); /**/
   E_CONFIG_VAL(D, T, border_shade_transition, INT); /**/
   E_CONFIG_VAL(D, T, border_shade_speed, DOUBLE); /**/
   E_CONFIG_VAL(D, T, framerate, DOUBLE); /**/
   E_CONFIG_VAL(D, T, image_cache, INT); /**/
   E_CONFIG_VAL(D, T, font_cache, INT); /**/
   E_CONFIG_VAL(D, T, edje_cache, INT); /**/
   E_CONFIG_VAL(D, T, edje_collection_cache, INT); /**/
   E_CONFIG_VAL(D, T, zone_desks_x_count, INT); /**/
   E_CONFIG_VAL(D, T, zone_desks_y_count, INT); /**/
   E_CONFIG_VAL(D, T, use_virtual_roots, INT); /* should not make this a config option (for now) */
   E_CONFIG_VAL(D, T, show_desktop_icons, INT); /**/
   E_CONFIG_VAL(D, T, edge_flip_dragging, INT); /**/
   E_CONFIG_VAL(D, T, edge_flip_moving, INT); /**/
   E_CONFIG_VAL(D, T, edge_flip_timeout, DOUBLE); /**/
   E_CONFIG_VAL(D, T, evas_engine_default, INT); /**/
   E_CONFIG_VAL(D, T, evas_engine_container, INT); /**/
   E_CONFIG_VAL(D, T, evas_engine_init, INT); /**/
   E_CONFIG_VAL(D, T, evas_engine_menus, INT); /**/
   E_CONFIG_VAL(D, T, evas_engine_borders, INT); /**/
   E_CONFIG_VAL(D, T, evas_engine_errors, INT); /**/
   E_CONFIG_VAL(D, T, evas_engine_popups, INT); /**/
   E_CONFIG_VAL(D, T, evas_engine_drag, INT); /**/
   E_CONFIG_VAL(D, T, evas_engine_win, INT); /**/
   E_CONFIG_VAL(D, T, evas_engine_zone, INT); /**/
   E_CONFIG_VAL(D, T, use_composite, INT); /**/
   E_CONFIG_VAL(D, T, language, STR); /**/
   E_CONFIG_LIST(D, T, modules, _e_config_module_edd); /**/
   E_CONFIG_LIST(D, T, font_fallbacks, _e_config_font_fallback_edd); /**/
   E_CONFIG_LIST(D, T, font_defaults, _e_config_font_default_edd); /**/
   E_CONFIG_LIST(D, T, themes, _e_config_theme_edd); /**/
   E_CONFIG_LIST(D, T, mouse_bindings, _e_config_bindings_mouse_edd); /**/
   E_CONFIG_LIST(D, T, key_bindings, _e_config_bindings_key_edd); /**/
   E_CONFIG_LIST(D, T, signal_bindings, _e_config_bindings_signal_edd); /**/
   E_CONFIG_LIST(D, T, wheel_bindings, _e_config_bindings_wheel_edd); /**/
   E_CONFIG_LIST(D, T, path_append_data, _e_config_path_append_edd); /**/
   E_CONFIG_LIST(D, T, path_append_images, _e_config_path_append_edd); /**/
   E_CONFIG_LIST(D, T, path_append_fonts, _e_config_path_append_edd); /**/
   E_CONFIG_LIST(D, T, path_append_themes, _e_config_path_append_edd); /**/
   E_CONFIG_LIST(D, T, path_append_init, _e_config_path_append_edd); /**/
   E_CONFIG_LIST(D, T, path_append_icons, _e_config_path_append_edd); /**/
   E_CONFIG_LIST(D, T, path_append_modules, _e_config_path_append_edd); /**/
   E_CONFIG_LIST(D, T, path_append_backgrounds, _e_config_path_append_edd); /**/
   E_CONFIG_VAL(D, T, window_placement_policy, INT); /**/
   E_CONFIG_VAL(D, T, focus_policy, INT); /**/
   E_CONFIG_VAL(D, T, focus_setting, INT); /**/
   E_CONFIG_VAL(D, T, pass_click_on, INT); /**/
   E_CONFIG_VAL(D, T, always_click_to_raise, INT); /**/
   E_CONFIG_VAL(D, T, always_click_to_focus, INT); /**/
   E_CONFIG_VAL(D, T, use_auto_raise, INT); /**/
   E_CONFIG_VAL(D, T, auto_raise_delay, DOUBLE); /**/
   E_CONFIG_VAL(D, T, use_resist, INT); /**/
   E_CONFIG_VAL(D, T, drag_resist, INT); /**/
   E_CONFIG_VAL(D, T, desk_resist, INT); /**/
   E_CONFIG_VAL(D, T, window_resist, INT); /**/
   E_CONFIG_VAL(D, T, gadget_resist, INT); /**/
   E_CONFIG_VAL(D, T, winlist_warp_while_selecting, INT); /**/
   E_CONFIG_VAL(D, T, winlist_warp_at_end, INT); /**/
   E_CONFIG_VAL(D, T, winlist_warp_speed, DOUBLE); /**/
   E_CONFIG_VAL(D, T, winlist_scroll_animate, INT); /**/
   E_CONFIG_VAL(D, T, winlist_scroll_speed, DOUBLE); /**/
   E_CONFIG_VAL(D, T, winlist_list_show_iconified, INT); /**/
   E_CONFIG_VAL(D, T, winlist_list_show_other_desk_iconified, INT); /**/
   E_CONFIG_VAL(D, T, winlist_list_show_other_screen_iconified, INT); /**/
   E_CONFIG_VAL(D, T, winlist_list_show_other_desk_windows, INT); /**/
   E_CONFIG_VAL(D, T, winlist_list_show_other_screen_windows, INT); /**/
   E_CONFIG_VAL(D, T, winlist_list_uncover_while_selecting, INT); /**/
   E_CONFIG_VAL(D, T, winlist_list_jump_desk_while_selecting, INT); /**/
   E_CONFIG_VAL(D, T, winlist_list_focus_while_selecting, INT); /**/
   E_CONFIG_VAL(D, T, winlist_list_raise_while_selecting, INT); /**/
   E_CONFIG_VAL(D, T, winlist_pos_align_x, DOUBLE); /**/
   E_CONFIG_VAL(D, T, winlist_pos_align_y, DOUBLE); /**/
   E_CONFIG_VAL(D, T, winlist_pos_size_w, DOUBLE); /**/
   E_CONFIG_VAL(D, T, winlist_pos_size_h, DOUBLE); /**/
   E_CONFIG_VAL(D, T, winlist_pos_min_w, INT); /**/
   E_CONFIG_VAL(D, T, winlist_pos_min_h, INT); /**/
   E_CONFIG_VAL(D, T, winlist_pos_max_w, INT); /**/
   E_CONFIG_VAL(D, T, winlist_pos_max_h, INT); /**/
   E_CONFIG_VAL(D, T, maximize_policy, INT); /**/
   E_CONFIG_VAL(D, T, allow_manip, INT); /**/
   E_CONFIG_VAL(D, T, border_fix_on_shelf_toggle, INT); /**/
   E_CONFIG_VAL(D, T, allow_above_fullscreen, INT); /**/
   E_CONFIG_VAL(D, T, kill_if_close_not_possible, INT); /**/
   E_CONFIG_VAL(D, T, kill_process, INT); /**/
   E_CONFIG_VAL(D, T, kill_timer_wait, DOUBLE); /**/
   E_CONFIG_VAL(D, T, ping_clients, INT); /**/
   E_CONFIG_VAL(D, T, transition_start, STR); /**/
   E_CONFIG_VAL(D, T, transition_desk, STR); /**/
   E_CONFIG_VAL(D, T, transition_change, STR); /**/
   E_CONFIG_LIST(D, T, remembers, _e_config_remember_edd);
   E_CONFIG_VAL(D, T, remember_internal_windows, INT);
   E_CONFIG_VAL(D, T, move_info_follows, INT); /**/
   E_CONFIG_VAL(D, T, resize_info_follows, INT); /**/
   E_CONFIG_VAL(D, T, move_info_visible, INT); /**/
   E_CONFIG_VAL(D, T, resize_info_visible, INT); /**/
   E_CONFIG_VAL(D, T, focus_last_focused_per_desktop, INT); /**/
   E_CONFIG_VAL(D, T, focus_revert_on_hide_or_close, INT); /**/
   E_CONFIG_VAL(D, T, pointer_slide, INT); /**/
   E_CONFIG_VAL(D, T, use_e_cursor, INT); /**/
   E_CONFIG_VAL(D, T, cursor_size, INT); /**/
   E_CONFIG_VAL(D, T, menu_autoscroll_margin, INT); /**/
   E_CONFIG_VAL(D, T, menu_autoscroll_cursor_margin, INT); /**/
   E_CONFIG_VAL(D, T, transient.move, INT); /* FIXME: implement */
   E_CONFIG_VAL(D, T, transient.resize, INT); /* FIXME: implement */
   E_CONFIG_VAL(D, T, transient.raise, INT); /**/
   E_CONFIG_VAL(D, T, transient.lower, INT); /**/
   E_CONFIG_VAL(D, T, transient.layer, INT); /**/
   E_CONFIG_VAL(D, T, transient.desktop, INT); /**/
   E_CONFIG_VAL(D, T, transient.iconify, INT); /**/
   E_CONFIG_VAL(D, T, modal_windows, INT); /**/
   E_CONFIG_VAL(D, T, menu_eap_name_show, INT); /**/
   E_CONFIG_VAL(D, T, menu_eap_generic_show, INT); /**/
   E_CONFIG_VAL(D, T, menu_eap_comment_show, INT); /**/
   E_CONFIG_VAL(D, T, fullscreen_policy, INT); /**/
   E_CONFIG_VAL(D, T, input_method, STR); /**/
   E_CONFIG_LIST(D, T, path_append_messages, _e_config_path_append_edd); /**/
   E_CONFIG_VAL(D, T, exebuf_max_exe_list, INT);
   E_CONFIG_VAL(D, T, exebuf_max_eap_list, INT);
   E_CONFIG_VAL(D, T, exebuf_max_hist_list, INT);
   E_CONFIG_VAL(D, T, exebuf_scroll_animate, INT);
   E_CONFIG_VAL(D, T, exebuf_scroll_speed, DOUBLE);
   E_CONFIG_VAL(D, T, exebuf_pos_align_x, DOUBLE);
   E_CONFIG_VAL(D, T, exebuf_pos_align_y, DOUBLE);
   E_CONFIG_VAL(D, T, exebuf_pos_size_w, DOUBLE);
   E_CONFIG_VAL(D, T, exebuf_pos_size_h, DOUBLE);
   E_CONFIG_VAL(D, T, exebuf_pos_min_w, INT);
   E_CONFIG_VAL(D, T, exebuf_pos_min_h, INT);
   E_CONFIG_VAL(D, T, exebuf_pos_max_w, INT);
   E_CONFIG_VAL(D, T, exebuf_pos_max_h, INT);
   E_CONFIG_VAL(D, T, exebuf_term_cmd, STR);
   E_CONFIG_LIST(D, T, color_classes, _e_config_color_class_edd);
   E_CONFIG_VAL(D, T, use_app_icon, INT);
   E_CONFIG_VAL(D, T, cnfmdlg_disabled, INT); /**/
   E_CONFIG_VAL(D, T, cfgdlg_auto_apply, INT); /**/
   E_CONFIG_VAL(D, T, cfgdlg_default_mode, INT); /**/
   E_CONFIG_LIST(D, T, gadcons, _e_config_gadcon_edd);
   E_CONFIG_LIST(D, T, shelves, _e_config_shelf_edd);
   E_CONFIG_VAL(D, T, font_hinting, INT); /**/
   E_CONFIG_VAL(D, T, desklock_personal_passwd, STR);
   E_CONFIG_VAL(D, T, desklock_background, STR);
   E_CONFIG_VAL(D, T, desklock_auth_method, INT);
   E_CONFIG_VAL(D, T, desklock_login_box_zone, INT);
   E_CONFIG_VAL(D, T, desklock_autolock_screensaver, INT);
   E_CONFIG_VAL(D, T, desklock_autolock_idle, INT);
   E_CONFIG_VAL(D, T, desklock_autolock_idle_timeout, DOUBLE);
   E_CONFIG_VAL(D, T, desklock_use_custom_desklock, INT);
   E_CONFIG_VAL(D, T, desklock_custom_desklock_cmd, STR);
   E_CONFIG_VAL(D, T, display_res_restore, INT);
   E_CONFIG_VAL(D, T, display_res_width, INT);
   E_CONFIG_VAL(D, T, display_res_height, INT);
   E_CONFIG_VAL(D, T, display_res_hz, INT);
   E_CONFIG_VAL(D, T, display_res_rotation, INT);
   
   E_CONFIG_VAL(D, T, screensaver_enable, INT);
   E_CONFIG_VAL(D, T, screensaver_timeout, INT);
   E_CONFIG_VAL(D, T, screensaver_interval, INT);
   E_CONFIG_VAL(D, T, screensaver_blanking, INT);
   E_CONFIG_VAL(D, T, screensaver_expose, INT);
   
   E_CONFIG_VAL(D, T, dpms_enable, INT);
   E_CONFIG_VAL(D, T, dpms_standby_enable, INT);
   E_CONFIG_VAL(D, T, dpms_suspend_enable, INT);
   E_CONFIG_VAL(D, T, dpms_off_enable, INT);
   E_CONFIG_VAL(D, T, dpms_standby_timeout, INT);
   E_CONFIG_VAL(D, T, dpms_suspend_timeout, INT);
   E_CONFIG_VAL(D, T, dpms_off_timeout, INT);
   
   E_CONFIG_VAL(D, T, clientlist_group_by, INT);
   E_CONFIG_VAL(D, T, clientlist_include_all_zones, INT);
   E_CONFIG_VAL(D, T, clientlist_separate_with, INT);
   E_CONFIG_VAL(D, T, clientlist_sort_by, INT);
   E_CONFIG_VAL(D, T, clientlist_separate_iconified_apps, INT);
   E_CONFIG_VAL(D, T, clientlist_warp_to_iconified_desktop, INT);
   E_CONFIG_VAL(D, T, clientlist_limit_caption_len, INT);
   E_CONFIG_VAL(D, T, clientlist_max_caption_len, INT);
   
   E_CONFIG_VAL(D, T, mouse_hand, INT);
   E_CONFIG_VAL(D, T, mouse_accel_numerator, INT);
   E_CONFIG_VAL(D, T, mouse_accel_denominator, INT);
   E_CONFIG_VAL(D, T, mouse_accel_threshold, INT);

   E_CONFIG_VAL(D, T, border_raise_on_mouse_action, INT);
   E_CONFIG_VAL(D, T, border_raise_on_focus, INT);
   E_CONFIG_VAL(D, T, desk_flip_wrap, INT);

   E_CONFIG_VAL(D, T, icon_theme, STR);
   
   E_CONFIG_VAL(D, T, desk_flip_animate_mode, INT);
   E_CONFIG_VAL(D, T, desk_flip_animate_interpolation, INT);
   E_CONFIG_VAL(D, T, desk_flip_animate_time, DOUBLE);
   
   E_CONFIG_VAL(D, T, wallpaper_import_last_dev, STR);
   E_CONFIG_VAL(D, T, wallpaper_import_last_path, STR);

   E_CONFIG_VAL(D, T, wallpaper_grad_c1_r, INT);
   E_CONFIG_VAL(D, T, wallpaper_grad_c1_g, INT);
   E_CONFIG_VAL(D, T, wallpaper_grad_c1_b, INT);
   E_CONFIG_VAL(D, T, wallpaper_grad_c2_r, INT);
   E_CONFIG_VAL(D, T, wallpaper_grad_c2_g, INT);
   E_CONFIG_VAL(D, T, wallpaper_grad_c2_b, INT);

   E_CONFIG_VAL(D, T, theme_default_border_style, STR);
   
   E_CONFIG_LIST(D, T, mime_icons, _e_config_mime_icon_edd); /**/

   E_CONFIG_VAL(D, T, desk_auto_switch, INT);

   E_CONFIG_VAL(D, T, thumb_nice, INT);

   E_CONFIG_VAL(D, T, menu_favorites_show, INT);
   E_CONFIG_VAL(D, T, menu_apps_show, INT);
   
   E_CONFIG_VAL(D, T, ping_clients_interval, INT);
   E_CONFIG_VAL(D, T, cache_flush_poll_interval, INT);

   E_CONFIG_VAL(D, T, thumbscroll_enable, INT);
   E_CONFIG_VAL(D, T, thumbscroll_threshhold, INT);
   E_CONFIG_VAL(D, T, thumbscroll_momentum_threshhold, DOUBLE);
   E_CONFIG_VAL(D, T, thumbscroll_friction, DOUBLE);
   
   E_CONFIG_VAL(D, T, hal_desktop, INT);

   E_CONFIG_VAL(D, T, border_keyboard.timeout, DOUBLE);
   E_CONFIG_VAL(D, T, border_keyboard.move.dx, UCHAR);
   E_CONFIG_VAL(D, T, border_keyboard.move.dy, UCHAR);
   E_CONFIG_VAL(D, T, border_keyboard.resize.dx, UCHAR);
   E_CONFIG_VAL(D, T, border_keyboard.resize.dy, UCHAR);

   E_CONFIG_VAL(D, T, hal_desktop, INT);

   E_CONFIG_VAL(D, T, scale.min, DOUBLE);
   E_CONFIG_VAL(D, T, scale.max, DOUBLE);
   E_CONFIG_VAL(D, T, scale.factor, DOUBLE);
   E_CONFIG_VAL(D, T, scale.base_dpi, INT);
   E_CONFIG_VAL(D, T, scale.use_dpi, UCHAR);
   E_CONFIG_VAL(D, T, scale.use_custom, UCHAR);

   E_CONFIG_VAL(D, T, show_cursor, UCHAR); /**/
   E_CONFIG_VAL(D, T, idle_cursor, UCHAR); /**/

   E_CONFIG_VAL(D, T, default_system_menu, STR);

   e_config_load();
   
   e_config_save_queue();
   return 1;
}

EAPI int
e_config_shutdown(void)
{
   E_FREE(_e_config_profile);
   E_CONFIG_DD_FREE(_e_config_edd);
   E_CONFIG_DD_FREE(_e_config_module_edd);
   E_CONFIG_DD_FREE(_e_config_font_default_edd);
   E_CONFIG_DD_FREE(_e_config_font_fallback_edd);
   E_CONFIG_DD_FREE(_e_config_theme_edd);
   E_CONFIG_DD_FREE(_e_config_bindings_mouse_edd);
   E_CONFIG_DD_FREE(_e_config_bindings_key_edd);
   E_CONFIG_DD_FREE(_e_config_bindings_signal_edd);
   E_CONFIG_DD_FREE(_e_config_bindings_wheel_edd);
   E_CONFIG_DD_FREE(_e_config_path_append_edd);
   E_CONFIG_DD_FREE(_e_config_desktop_bg_edd);
   E_CONFIG_DD_FREE(_e_config_desktop_name_edd);
   E_CONFIG_DD_FREE(_e_config_remember_edd);
   E_CONFIG_DD_FREE(_e_config_gadcon_edd);
   E_CONFIG_DD_FREE(_e_config_gadcon_client_edd);
   E_CONFIG_DD_FREE(_e_config_shelf_edd);
   E_CONFIG_DD_FREE(_e_config_shelf_desk_edd);
   return 1;
}

EAPI void
e_config_load(void)
{
   e_config = e_config_domain_load("e", _e_config_edd);
   if (e_config)
     {
	if ((e_config->config_version >> 16) < E_CONFIG_FILE_EPOCH)
	  {
	     /* your config is too old - need new defaults */
	     _e_config_free();
	     ecore_timer_add(1.0, _e_config_cb_timer,
			     _("Configuration data needed upgrading. Your old configuration<br>"
			       "has been wiped and a new set of defaults initialized. This<br>"
			       "will happen regularly during development, so don't report a<br>"
			       "bug. This simply means Enlightenment needs new configuration<br>"
			       "data by default for usable functionality that your old<br>"
			       "configuration simply lacks. This new set of defaults will fix<br>"
			       "that by adding it in. You can re-configure things now to your<br>"
			       "liking. Sorry for the hiccup in your configuration.<br>"));
	  }
	else if (e_config->config_version > E_CONFIG_FILE_VERSION)
	  {
	     /* your config is too new - what the fuck??? */
	     _e_config_free();
	     ecore_timer_add(1.0, _e_config_cb_timer,
			     _("Your configuration is NEWER than Enlightenment. This is very<br>"
			       "strange. This should not happen unless you downgraded<br>"
			       "Enlightenment or copied the configuration from a place where<br>"
			       "a newer version of Enlightenment was running. This is bad and<br>"
			       "as a precaution your configuration has been now restored to<br>"
			       "defaults. Sorry for the inconvenience.<br>"));
	  }
     }
   
   if (!e_config)
     {
	/* DEFAULT CONFIG */
	e_config = E_NEW(E_Config, 1);
	e_config->config_version = (E_CONFIG_FILE_EPOCH << 16);
     }
#define IFCFG(v) \
   if ((e_config->config_version & 0xffff) < (v)) {
#define IFCFGEND }
   IFCFG(0x008d);
   e_config->show_splash = 1;
   e_config->init_default_theme = eina_stringshare_add("default.edj");
   e_config->desktop_default_background = NULL;
   e_config->desktop_default_name = eina_stringshare_add(_("Desktop %i, %i"));
   e_config->menus_scroll_speed = 1000.0;
   e_config->menus_fast_mouse_move_threshhold = 300.0;
   e_config->menus_click_drag_timeout = DEF_MENUCLICK;
   e_config->border_shade_animate = 1;
   e_config->border_shade_transition = E_TRANSITION_DECELERATE;
   e_config->border_shade_speed = 3000.0;
   e_config->framerate = 30.0;
   e_config->image_cache = 4096;
   e_config->font_cache = 512;
   e_config->edje_cache = 32;
   e_config->edje_collection_cache = 64;
   e_config->zone_desks_x_count = 1;
   e_config->zone_desks_y_count = 1;
   e_config->use_virtual_roots = 0;
   e_config->edge_flip_dragging = 0;
   e_config->edge_flip_moving = 0;
   e_config->edge_flip_timeout = 0.25;
   e_config->evas_engine_default = E_EVAS_ENGINE_SOFTWARE_X11;
   e_config->evas_engine_container = E_EVAS_ENGINE_DEFAULT;
   e_config->evas_engine_init = E_EVAS_ENGINE_DEFAULT;
   e_config->evas_engine_menus = E_EVAS_ENGINE_DEFAULT;
   e_config->evas_engine_borders = E_EVAS_ENGINE_DEFAULT;
   e_config->evas_engine_errors = E_EVAS_ENGINE_DEFAULT;
   e_config->evas_engine_popups = E_EVAS_ENGINE_DEFAULT;
   e_config->evas_engine_drag = E_EVAS_ENGINE_DEFAULT;
   e_config->evas_engine_win = E_EVAS_ENGINE_DEFAULT;
   e_config->evas_engine_zone = E_EVAS_ENGINE_DEFAULT;
   e_config->use_composite = 0;
   e_config->language = NULL;
   e_config->window_placement_policy = E_WINDOW_PLACEMENT_SMART;
   e_config->focus_policy = E_FOCUS_SLOPPY;
   e_config->focus_setting = E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED;
   e_config->pass_click_on = 1;
   e_config->always_click_to_raise = 0;
   e_config->always_click_to_focus = 0;
   e_config->use_auto_raise = 0;
   e_config->auto_raise_delay = 0.5;
   e_config->use_resist = 1;
   e_config->drag_resist = 16;
   e_config->desk_resist = 32;
   e_config->window_resist = 12;
   e_config->gadget_resist = 32;
   e_config->winlist_warp_while_selecting = 1;
   e_config->winlist_warp_at_end = 1;
   e_config->winlist_warp_speed = 0.1;
   e_config->winlist_scroll_animate = 1;
   e_config->winlist_scroll_speed = 0.1;
   e_config->winlist_list_show_iconified = 1;
   e_config->winlist_list_show_other_desk_windows = 0;
   e_config->winlist_list_show_other_screen_windows = 0;
   e_config->winlist_list_uncover_while_selecting = 0;
   e_config->winlist_list_jump_desk_while_selecting = 0;
   e_config->winlist_list_focus_while_selecting = 1;
   e_config->winlist_list_raise_while_selecting = 1;
   e_config->winlist_pos_align_x = 0.5;
   e_config->winlist_pos_align_y = 0.5;
   e_config->winlist_pos_size_w = 0.5;
   e_config->winlist_pos_size_h = 0.5;
   e_config->winlist_pos_min_w = 0;
   e_config->winlist_pos_min_h = 0;
   e_config->winlist_pos_max_w = 320;
   e_config->winlist_pos_max_h = 320;
   e_config->maximize_policy = E_MAXIMIZE_SMART | E_MAXIMIZE_BOTH;
   e_config->allow_manip = 0;
   e_config->kill_if_close_not_possible = 1;
   e_config->kill_process = 1;
   e_config->kill_timer_wait = 10.0;
   e_config->ping_clients = 1;
   e_config->transition_start = NULL;
   e_config->transition_desk = eina_stringshare_add("vswipe");
   e_config->transition_change = eina_stringshare_add("crossfade");
   e_config->move_info_follows = 1;
   e_config->resize_info_follows = 1;
   e_config->move_info_visible = 1;
   e_config->resize_info_visible = 1;
   e_config->focus_last_focused_per_desktop = 1;
   e_config->focus_revert_on_hide_or_close = 1;
   e_config->pointer_slide = 1;
   e_config->show_cursor = 1;
   e_config->use_e_cursor = 1;
   e_config->cursor_size = 32;
   e_config->menu_autoscroll_margin = 0;
   e_config->menu_autoscroll_cursor_margin = 1;
   e_config->transient.move = 1;
   e_config->transient.resize = 0;
   e_config->transient.raise = 1;
   e_config->transient.lower = 1;
   e_config->transient.layer = 1;
   e_config->transient.desktop = 1;
   e_config->transient.iconify = 1;
   e_config->modal_windows = 1;
   e_config->menu_eap_name_show = 1;
   e_config->menu_eap_generic_show = 1;
   e_config->menu_eap_comment_show = 0;
   e_config->fullscreen_policy = E_FULLSCREEN_RESIZE;
   e_config->input_method = NULL;	
   e_config->exebuf_max_exe_list = 20;
   e_config->exebuf_max_eap_list = 20;
   e_config->exebuf_max_hist_list = 20;
   e_config->exebuf_scroll_animate = 1;
   e_config->exebuf_scroll_speed = 0.1;
   e_config->exebuf_pos_align_x = 0.5;
   e_config->exebuf_pos_align_y = 0.5;
   e_config->exebuf_pos_size_w = 0.75;
   e_config->exebuf_pos_size_h = 0.25;
   e_config->exebuf_pos_min_w = 200;
   e_config->exebuf_pos_min_h = 160;
   e_config->exebuf_pos_max_w = 400;
   e_config->exebuf_pos_max_h = 320;
   e_config->exebuf_term_cmd = eina_stringshare_add("xterm -hold -e");
   e_config->color_classes = NULL;
   e_config->use_app_icon = 0;
   e_config->cnfmdlg_disabled = 0;
   e_config->cfgdlg_auto_apply = 0;
   e_config->cfgdlg_default_mode = 0;
   e_config->gadcons = NULL;
   e_config->font_hinting = 0;
   
   e_config->desklock_personal_passwd = NULL;
   e_config->desklock_background = NULL;
   e_config->desklock_auth_method = 0;
   e_config->desklock_login_box_zone = -1;
   e_config->desklock_autolock_screensaver = 0;
   e_config->desklock_autolock_idle = 0;
   e_config->desklock_autolock_idle_timeout = 300.0;
   
   e_config->display_res_restore = 0;
   e_config->display_res_width = 0;
   e_config->display_res_height = 0;
   e_config->display_res_hz = 0;
   e_config->display_res_rotation = 0;

   e_config->hal_desktop = 1;

   e_config->border_keyboard.timeout = 5.0;
   e_config->border_keyboard.move.dx = 5;
   e_config->border_keyboard.move.dy = 5;
   e_config->border_keyboard.resize.dx = 5;
   e_config->border_keyboard.resize.dy = 5;

     {
	E_Config_Module *em;

#define CFG_MODULE(_name, _enabled, _delayed, _priority) \
   em = E_NEW(E_Config_Module, 1); \
   em->name = eina_stringshare_add(_name); \
   em->enabled = _enabled; \
   em->delayed = _delayed; \
   em->priority = _priority; \
   e_config->modules = eina_list_append(e_config->modules, em)

	CFG_MODULE("wizard", 1, 0, 0);
     }
     {
	E_Config_Binding_Key *eb;

#define CFG_KEYBIND(_context, _key, _modifiers, _anymod, _action, _params) \
   eb = E_NEW(E_Config_Binding_Key, 1); \
   eb->context = _context; \
   eb->key = eina_stringshare_add(_key); \
   eb->modifiers = _modifiers; \
   eb->any_mod = _anymod; \
   eb->action = _action == NULL ? NULL : eina_stringshare_add(_action); \
   eb->params = _params == NULL ? NULL : eina_stringshare_add(_params); \
   e_config->key_bindings = eina_list_append(e_config->key_bindings, eb)

	/*
	 * FIXME:
	 * If new key binding are added/changed/modified, then do not
	 * forget to reflect those changes in e_int_config_keybinding.c in 
	 * _restore_key_binding_defaults_cb function
	 */

	CFG_KEYBIND(E_BINDING_CONTEXT_ANY, "m",
		    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
		    "menu_show", "main");
	CFG_KEYBIND(E_BINDING_CONTEXT_ANY, "End",
		    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
		    "restart", NULL);
	CFG_KEYBIND(E_BINDING_CONTEXT_ANY, "Delete",
		    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
		    "logout", NULL);
     }
     {
	E_Config_Binding_Signal *eb;

#define CFG_SIGNALBIND(_context, _signal, _source, _modifiers, _anymod, _action, _params) \
   eb = E_NEW(E_Config_Binding_Signal, 1); \
   eb->context = _context; \
   eb->signal = _signal == NULL ? NULL : eina_stringshare_add(_signal); \
   eb->source = _source == NULL ? NULL : eina_stringshare_add(_source); \
   eb->modifiers = _modifiers; \
   eb->any_mod = _anymod; \
   eb->action = _action == NULL ? NULL : eina_stringshare_add(_action); \
   eb->params = _params == NULL ? NULL : eina_stringshare_add(_params); \
   e_config->signal_bindings = eina_list_append(e_config->signal_bindings, eb)
   
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,down,1,double", 
		       "e.event.titlebar",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_shaded_toggle", "up");
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,down,2", 
		       "e.event.titlebar",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_shaded_toggle", "up");		       
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,wheel,?,1", 
		       "e.event.titlebar",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_shaded", "0 up");
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,wheel,?,-1", 
		       "e.event.titlebar",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_shaded", "1 up");
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,clicked,3", 
		       "e.event.titlebar",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_menu", NULL);
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,clicked,?", 
		       "e.event.icon",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_menu", NULL);
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,clicked,[12]", 
		       "e.event.close",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_close", NULL);
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,clicked,3", 
		       "e.event.close",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_kill", NULL);
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,clicked,1", 
		       "e.event.maximize",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_maximized_toggle", NULL);
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,clicked,2", 
		       "e.event.maximize",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_maximized_toggle", "smart");
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,clicked,3", 
		       "e.event.maximize",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_maximized_toggle", "expand");
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,clicked,?", 
		       "e.event.minimize",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_iconic_toggle", NULL);
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,clicked,?", 
		       "e.event.shade",
		       E_BINDING_MODIFIER_NONE, 1,
		       "window_shaded_toggle", "up");
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,clicked,?", 
		       "e.event.lower",
		       E_BINDING_MODIFIER_NONE, 1,
		       "window_lower", NULL);	       
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,down,1", 
		       "e.event.icon",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_drag_icon", NULL);
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,down,1", 
		       "e.event.titlebar",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_move", NULL);
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,up,1", 
		       "e.event.titlebar",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_move", "end");
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,down,1", 
		       "e.event.resize.tl",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_resize", "tl");
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,down,1", 
		       "e.event.resize.t",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_resize", "t");
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,down,1", 
		       "e.event.resize.tr",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_resize", "tr");
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,down,1", 
		       "e.event.resize.r",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_resize", "r");
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,down,1", 
		       "e.event.resize.br",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_resize", "br");
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,down,1", 
		       "e.event.resize.b",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_resize", "b");
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,down,1", 
		       "e.event.resize.bl",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_resize", "bl");
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,down,1", 
		       "e.event.resize.l",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_resize", "l");
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,up,1", 
		       "e.event.resize.*",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_resize", "end");
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,down,3", 
		       "e.event.resize.*",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_move", NULL);
	CFG_SIGNALBIND(E_BINDING_CONTEXT_BORDER, "mouse,up,3", 
		       "e.event.resize.*",
		       E_BINDING_MODIFIER_NONE, 1, 
		       "window_move", "end");
     }
   IFCFGEND;
   
   IFCFG(0x0093); /* the version # where this value(s) was introduced */
   /* FIXME: wipe previous shelves and gadcons - remove this eventually */
   e_config->shelves = NULL;
   e_config->gadcons = NULL;
   IFCFGEND;
   
   IFCFG(0x0094); /* the version # where this value(s) was introduced */
   e_config->border_raise_on_mouse_action = 1;
   e_config->border_raise_on_focus = 1;
   e_config->desk_flip_wrap = 0;
   e_config->wallpaper_import_last_dev = NULL;
   e_config->wallpaper_import_last_path = NULL;
   IFCFGEND;
   
   IFCFG(0x0096); /* the version # where this value(s) was introduced */
   e_config->wallpaper_import_last_dev = eina_stringshare_add("~/");
   e_config->wallpaper_import_last_path = eina_stringshare_add("/");
   IFCFGEND;

   IFCFG(0x0098);
   e_config->wallpaper_grad_c1_r = 0;
   e_config->wallpaper_grad_c1_g = 0;
   e_config->wallpaper_grad_c1_b = 0;
   e_config->wallpaper_grad_c2_r = 255;
   e_config->wallpaper_grad_c2_g = 255;
   e_config->wallpaper_grad_c2_b = 255;
   IFCFGEND;

   IFCFG(0x0101);
   e_config->desk_flip_animate_mode = 1;
   e_config->desk_flip_animate_interpolation = 0;
   e_config->desk_flip_animate_time = 0.2;
   IFCFGEND;
   
   IFCFG(0x0102);
   e_config->mime_icons = NULL;
   IFCFGEND;

   IFCFG(0x00103);
   e_config->theme_default_border_style = eina_stringshare_add("default");
   IFCFGEND;

   IFCFG(0x00104);
   e_config->winlist_list_show_other_desk_iconified = 1;
   e_config->winlist_list_show_other_screen_iconified = 0;
   IFCFGEND;
   
   IFCFG(0x00105);
   e_config->remember_internal_windows = 0;
   IFCFGEND;
     
   IFCFG(0x00106);
   e_config->desklock_use_custom_desklock = 0;
   e_config->desklock_custom_desklock_cmd = NULL;     
   IFCFGEND;

   IFCFG(0x0121);
   e_config->gadcons = NULL;
   IFCFGEND;

   IFCFG(0x0108);
   e_config->desk_auto_switch = 0;
   IFCFGEND;

   IFCFG(0x0109);
   e_config->dpms_enable = 0;
   e_config->dpms_standby_enable = 0;
   e_config->dpms_suspend_enable = 0;
   e_config->dpms_off_enable = 0;
   e_config->dpms_standby_timeout = 1;
   e_config->dpms_suspend_timeout = 1;
   e_config->dpms_off_timeout = 1;
   e_config->screensaver_enable = 0;
   e_config->screensaver_timeout = 0;
   e_config->screensaver_interval = 5;
   e_config->screensaver_blanking = 2;
   e_config->screensaver_expose = 2;
   IFCFGEND;
    
   IFCFG(0x0110);
   e_config->clientlist_group_by = 0;
   e_config->clientlist_separate_with = 0;
   e_config->clientlist_sort_by = 0;
   e_config->clientlist_separate_iconified_apps = 0;
   e_config->clientlist_warp_to_iconified_desktop = 0;
   IFCFGEND;

   IFCFG(0x0111);
   e_config->clientlist_include_all_zones = 0;
   IFCFGEND;

   IFCFG(0x0112);
   e_config->mouse_accel_numerator = 2;
   e_config->mouse_accel_denominator = 1;
   e_config->mouse_accel_threshold = 4;
   IFCFGEND;

   if (!e_config->icon_theme) e_config->icon_theme = eina_stringshare_add("hicolor");

   IFCFG(0x113)
   e_config->clientlist_max_caption_len = 0;
   IFCFGEND;

   IFCFG(0x114)
   e_config->thumb_nice = 0;
   IFCFGEND;

   IFCFG(0x0115);
   e_config->clientlist_limit_caption_len = 0;
   IFCFGEND;

   IFCFG(0x0116);
   e_config->border_fix_on_shelf_toggle = 0;
   IFCFGEND;
   
   IFCFG(0x0117);
   e_config->cnfmdlg_disabled = 0;
   IFCFGEND;

   IFCFG(0x0118);
   e_config->menu_favorites_show = 1;
   e_config->menu_apps_show = 1;
   IFCFGEND;

   IFCFG(0x0119);
   e_config->allow_above_fullscreen = 0;
   IFCFGEND;

   IFCFG(0x0120);
   e_config->show_desktop_icons = 1;
   IFCFGEND;

   IFCFG(0x0123);
   e_config->ping_clients_interval = 128;
   e_config->cache_flush_poll_interval = 512;
   IFCFGEND;
   
   IFCFG(0x0124);
   e_config->thumbscroll_enable = 1;
   e_config->thumbscroll_threshhold = 24;
   e_config->thumbscroll_momentum_threshhold = 100.0;
   e_config->thumbscroll_friction = 1.0;
   IFCFGEND;
   
   IFCFG(0x0125);
   e_config->mouse_hand = E_MOUSE_HAND_RIGHT;
   IFCFGEND;

   IFCFG(0x0126);
   e_config->border_keyboard.timeout = 5.0;
   e_config->border_keyboard.move.dx = 5;
   e_config->border_keyboard.move.dy = 5;
   e_config->border_keyboard.resize.dx = 5;
   e_config->border_keyboard.resize.dy = 5;
   IFCFGEND;
   
   IFCFG(0x0127);
   e_config->scale.min = 1.0;
   e_config->scale.max = 3.0;
   e_config->scale.factor = 1.0;
   e_config->scale.base_dpi = 142;
   e_config->scale.use_dpi = 0;
   e_config->scale.use_custom = 1;
   IFCFGEND;
   
   IFCFG(0x0128);
   e_config->show_cursor = 1;
   e_config->idle_cursor = 0;
   IFCFGEND;
   
   IFCFG(0x0129);
   e_config->default_system_menu = NULL;
   IFCFGEND;
   
   e_config->config_version = E_CONFIG_FILE_VERSION;   
     
#if 0 /* example of new config */
   IFCFG(0x0090); /* the version # where this value(s) was introduced */
   e_config->new_value = 10; /* set the value(s) */
   IFCFGEND;
#endif
   
   E_CONFIG_LIMIT(e_config->menus_scroll_speed, 1.0, 20000.0);
   E_CONFIG_LIMIT(e_config->show_splash, 0, 1);
   E_CONFIG_LIMIT(e_config->menus_fast_mouse_move_threshhold, 1.0, 2000.0);
   E_CONFIG_LIMIT(e_config->menus_click_drag_timeout, 0.0, 10.0);
   E_CONFIG_LIMIT(e_config->border_shade_animate, 0, 1);
   E_CONFIG_LIMIT(e_config->border_shade_transition, 0, 3);
   E_CONFIG_LIMIT(e_config->border_shade_speed, 1.0, 20000.0);
   E_CONFIG_LIMIT(e_config->framerate, 1.0, 200.0);
   E_CONFIG_LIMIT(e_config->image_cache, 0, 256 * 1024);
   E_CONFIG_LIMIT(e_config->font_cache, 0, 32 * 1024);
   E_CONFIG_LIMIT(e_config->edje_cache, 0, 256);
   E_CONFIG_LIMIT(e_config->edje_collection_cache, 0, 512);
   E_CONFIG_LIMIT(e_config->cache_flush_poll_interval, 8, 32768);
   E_CONFIG_LIMIT(e_config->zone_desks_x_count, 1, 64);
   E_CONFIG_LIMIT(e_config->zone_desks_y_count, 1, 64);
   E_CONFIG_LIMIT(e_config->show_desktop_icons, 0, 1);
   E_CONFIG_LIMIT(e_config->edge_flip_dragging, 0, 1);
   E_CONFIG_LIMIT(e_config->edge_flip_moving, 0, 1);
   E_CONFIG_LIMIT(e_config->edge_flip_timeout, 0.0, 2.0);
   E_CONFIG_LIMIT(e_config->window_placement_policy, E_WINDOW_PLACEMENT_SMART, E_WINDOW_PLACEMENT_MANUAL);
   E_CONFIG_LIMIT(e_config->focus_policy, 0, 2);
   E_CONFIG_LIMIT(e_config->focus_setting, 0, 3);
   E_CONFIG_LIMIT(e_config->pass_click_on, 0, 1);
   E_CONFIG_LIMIT(e_config->always_click_to_raise, 0, 1);
   E_CONFIG_LIMIT(e_config->always_click_to_focus, 0, 1);
   E_CONFIG_LIMIT(e_config->use_auto_raise, 0, 1);
   E_CONFIG_LIMIT(e_config->auto_raise_delay, 0.0, 5.0);
   E_CONFIG_LIMIT(e_config->use_resist, 0, 1);
   E_CONFIG_LIMIT(e_config->drag_resist, 0, 100);
   E_CONFIG_LIMIT(e_config->desk_resist, 0, 100);
   E_CONFIG_LIMIT(e_config->window_resist, 0, 100);
   E_CONFIG_LIMIT(e_config->gadget_resist, 0, 100);
   E_CONFIG_LIMIT(e_config->winlist_warp_while_selecting, 0, 1);
   E_CONFIG_LIMIT(e_config->winlist_warp_at_end, 0, 1);
   E_CONFIG_LIMIT(e_config->winlist_warp_speed, 0.0, 1.0);
   E_CONFIG_LIMIT(e_config->winlist_scroll_animate, 0, 1);
   E_CONFIG_LIMIT(e_config->winlist_scroll_speed, 0.0, 1.0);
   E_CONFIG_LIMIT(e_config->winlist_list_show_iconified, 0, 1);
   E_CONFIG_LIMIT(e_config->winlist_list_show_other_desk_iconified, 0, 1);
   E_CONFIG_LIMIT(e_config->winlist_list_show_other_screen_iconified, 0, 1);
   E_CONFIG_LIMIT(e_config->winlist_list_show_other_desk_windows, 0, 1);
   E_CONFIG_LIMIT(e_config->winlist_list_show_other_screen_windows, 0, 1);
   E_CONFIG_LIMIT(e_config->winlist_list_uncover_while_selecting, 0, 1);
   E_CONFIG_LIMIT(e_config->winlist_list_jump_desk_while_selecting, 0, 1);
   E_CONFIG_LIMIT(e_config->winlist_pos_align_x, 0.0, 1.0);
   E_CONFIG_LIMIT(e_config->winlist_pos_align_y, 0.0, 1.0);
   E_CONFIG_LIMIT(e_config->winlist_pos_size_w, 0.0, 1.0);
   E_CONFIG_LIMIT(e_config->winlist_pos_size_h, 0.0, 1.0);
   E_CONFIG_LIMIT(e_config->winlist_pos_min_w, 0, 4000);
   E_CONFIG_LIMIT(e_config->winlist_pos_min_h, 0, 4000);
   E_CONFIG_LIMIT(e_config->winlist_pos_max_w, 8, 4000);
   E_CONFIG_LIMIT(e_config->winlist_pos_max_h, 8, 4000);
   E_CONFIG_LIMIT(e_config->maximize_policy, E_MAXIMIZE_FULLSCREEN, E_MAXIMIZE_DIRECTION);
   E_CONFIG_LIMIT(e_config->allow_manip, 0, 1);
   E_CONFIG_LIMIT(e_config->border_fix_on_shelf_toggle, 0, 1);
   E_CONFIG_LIMIT(e_config->allow_above_fullscreen, 0, 1);
   E_CONFIG_LIMIT(e_config->kill_if_close_not_possible, 0, 1);
   E_CONFIG_LIMIT(e_config->kill_process, 0, 1);
   E_CONFIG_LIMIT(e_config->kill_timer_wait, 0.0, 120.0);
   E_CONFIG_LIMIT(e_config->ping_clients, 0, 1);
   E_CONFIG_LIMIT(e_config->move_info_follows, 0, 1);
   E_CONFIG_LIMIT(e_config->resize_info_follows, 0, 1);
   E_CONFIG_LIMIT(e_config->move_info_visible, 0, 1);
   E_CONFIG_LIMIT(e_config->resize_info_visible, 0, 1);
   E_CONFIG_LIMIT(e_config->focus_last_focused_per_desktop, 0, 1);
   E_CONFIG_LIMIT(e_config->focus_revert_on_hide_or_close, 0, 1);
   E_CONFIG_LIMIT(e_config->pointer_slide, 0, 1);
   E_CONFIG_LIMIT(e_config->show_cursor, 0, 1);
   E_CONFIG_LIMIT(e_config->use_e_cursor, 0, 1);
   E_CONFIG_LIMIT(e_config->cursor_size, 0, 1024);
   E_CONFIG_LIMIT(e_config->menu_autoscroll_margin, 0, 50);
   E_CONFIG_LIMIT(e_config->menu_autoscroll_cursor_margin, 0, 50);
   E_CONFIG_LIMIT(e_config->menu_eap_name_show, 0, 1);
   E_CONFIG_LIMIT(e_config->menu_eap_generic_show, 0, 1);
   E_CONFIG_LIMIT(e_config->menu_eap_comment_show, 0, 1);
   E_CONFIG_LIMIT(e_config->use_app_icon, 0, 1);
   E_CONFIG_LIMIT(e_config->cnfmdlg_disabled, 0, 1);
   E_CONFIG_LIMIT(e_config->cfgdlg_auto_apply, 0, 1);
   E_CONFIG_LIMIT(e_config->cfgdlg_default_mode, 0, 1);
   E_CONFIG_LIMIT(e_config->font_hinting, 0, 2);
   E_CONFIG_LIMIT(e_config->desklock_login_box_zone, -2, 1000);
   E_CONFIG_LIMIT(e_config->desklock_autolock_screensaver, 0, 1);
   E_CONFIG_LIMIT(e_config->desklock_autolock_idle, 0, 1);
   E_CONFIG_LIMIT(e_config->desklock_autolock_idle_timeout, 1.0, 5400.0);
   E_CONFIG_LIMIT(e_config->desklock_use_custom_desklock, 0, 1);
   E_CONFIG_LIMIT(e_config->display_res_restore, 0, 1);
   E_CONFIG_LIMIT(e_config->display_res_width, 1, 8192);
   E_CONFIG_LIMIT(e_config->display_res_height, 1, 8192);
   E_CONFIG_LIMIT(e_config->display_res_hz, 0, 250);
   E_CONFIG_LIMIT(e_config->display_res_rotation, 0, 0xff);
   E_CONFIG_LIMIT(e_config->border_raise_on_mouse_action, 0, 1);
   E_CONFIG_LIMIT(e_config->border_raise_on_focus, 0, 1);
   E_CONFIG_LIMIT(e_config->desk_flip_wrap, 0, 1);
   E_CONFIG_LIMIT(e_config->remember_internal_windows, 0, 1);
   E_CONFIG_LIMIT(e_config->desk_auto_switch, 0, 1);

   E_CONFIG_LIMIT(e_config->dpms_enable, 0, 1);
   E_CONFIG_LIMIT(e_config->dpms_standby_enable, 0, 1);
   E_CONFIG_LIMIT(e_config->dpms_suspend_enable, 0, 1);
   E_CONFIG_LIMIT(e_config->dpms_off_enable, 0, 1);
   E_CONFIG_LIMIT(e_config->dpms_standby_timeout, 0, 5400);
   E_CONFIG_LIMIT(e_config->dpms_suspend_timeout, 0, 5400);
   E_CONFIG_LIMIT(e_config->dpms_off_timeout, 0, 5400);

   E_CONFIG_LIMIT(e_config->screensaver_timeout, 0, 5400);
   E_CONFIG_LIMIT(e_config->screensaver_interval, 0, 5400);
   E_CONFIG_LIMIT(e_config->screensaver_blanking, 0, 2);
   E_CONFIG_LIMIT(e_config->screensaver_expose, 0, 2);
   
   E_CONFIG_LIMIT(e_config->clientlist_group_by, 0, 2);
   E_CONFIG_LIMIT(e_config->clientlist_include_all_zones, 0, 1);
   E_CONFIG_LIMIT(e_config->clientlist_separate_with, 0, 2);
   E_CONFIG_LIMIT(e_config->clientlist_sort_by, 0, 3);
   E_CONFIG_LIMIT(e_config->clientlist_separate_iconified_apps, 0, 2);
   E_CONFIG_LIMIT(e_config->clientlist_warp_to_iconified_desktop, 0, 1);
   E_CONFIG_LIMIT(e_config->mouse_hand, 0, 1);
   E_CONFIG_LIMIT(e_config->clientlist_limit_caption_len, 0, 1);
   E_CONFIG_LIMIT(e_config->clientlist_max_caption_len, 2, E_CLIENTLIST_MAX_CAPTION_LEN);
   
   E_CONFIG_LIMIT(e_config->mouse_accel_numerator, 1, 10);
   E_CONFIG_LIMIT(e_config->mouse_accel_denominator, 1, 10);
   E_CONFIG_LIMIT(e_config->mouse_accel_threshold, 1, 10);

   E_CONFIG_LIMIT(e_config->menu_favorites_show, 0, 1);
   E_CONFIG_LIMIT(e_config->menu_apps_show, 0, 1);

   E_CONFIG_LIMIT(e_config->ping_clients_interval, 16, 1024);
   
   /* FIXME: disabled auto apply because it causes problems */
   e_config->cfgdlg_auto_apply = 0;
   /* FIXME: desklock personalized password id disabled for security reasons */
   e_config->desklock_auth_method = 0;
   if (e_config->desklock_personal_passwd)
     eina_stringshare_del(e_config->desklock_personal_passwd);
   e_config->desklock_personal_passwd = NULL;
}

EAPI int
e_config_save(void)
{
   if (_e_config_save_defer)
     {
	e_powersave_deferred_action_del(_e_config_save_defer);
	_e_config_save_defer = NULL;
     }
   _e_config_save_cb(NULL);
   return e_config_domain_save("e", _e_config_edd, e_config);
}

EAPI void
e_config_save_flush(void)
{
   if (_e_config_save_defer)
     {
	e_powersave_deferred_action_del(_e_config_save_defer);
	_e_config_save_defer = NULL;
	_e_config_save_cb(NULL);
     }
}

EAPI void
e_config_save_queue(void)
{
   if (_e_config_save_defer)
     e_powersave_deferred_action_del(_e_config_save_defer);
   _e_config_save_defer = e_powersave_deferred_action_add(_e_config_save_cb,
							  NULL);
}

EAPI const char *
e_config_profile_get(void)
{
   return _e_config_profile;
}

EAPI void
e_config_profile_set(const char *prof)
{
   E_FREE(_e_config_profile);
   _e_config_profile = strdup(prof);
   e_util_env_set("E_CONF_PROFILE", _e_config_profile);
}

EAPI char *
e_config_profile_dir_get(const char *prof)
{
   char buf[PATH_MAX];
   const char *homedir;
   const char *dir;

   homedir = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/config/%s", homedir, prof);
   if (ecore_file_is_dir(buf)) return strdup(buf);
   dir = e_prefix_data_get();
   snprintf(buf, sizeof(buf), "%s/data/config/%s", dir, prof);
   if (ecore_file_is_dir(buf)) return strdup(buf);
   return NULL;
}

static int _cb_sort_files(char *f1, char *f2)
{
   return strcmp(f1, f2);
}

EAPI Eina_List *
e_config_profile_list(void)
{
   Ecore_List *files;
   char buf[PATH_MAX];
   const char *homedir;
   const char *dir;
   Eina_List *flist = NULL;
   
   homedir = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/config/", homedir);
   files = ecore_file_ls(buf);
   if (files)
     {
	char *file;
	
	ecore_list_sort(files, ECORE_COMPARE_CB(_cb_sort_files), ECORE_SORT_MIN);
	ecore_list_first_goto(files);
	while ((file = ecore_list_current(files)))
	  {
	     snprintf(buf, sizeof(buf), "%s/.e/e/config/%s", homedir, file);
	     if (ecore_file_is_dir(buf))
	       flist = eina_list_append(flist, strdup(file));
	     ecore_list_next(files);
	  }
        ecore_list_destroy(files);
     }
   dir = e_prefix_data_get();
   snprintf(buf, sizeof(buf), "%s/data/config", dir);
   files = ecore_file_ls(buf);
   if (files)
     {
	char *file;
	
	ecore_list_sort(files, ECORE_COMPARE_CB(_cb_sort_files), ECORE_SORT_MIN);
	ecore_list_first_goto(files);
	while ((file = ecore_list_current(files)))
	  {
	     snprintf(buf, sizeof(buf), "%s/data/config/%s", dir, file);
	     if (ecore_file_is_dir(buf))
	       {
		  Eina_List *l;
		  
		  for (l = flist; l; l = l->next)
		    {
		       if (!strcmp(file, l->data)) break;
		    }
		  if (!l) flist = eina_list_append(flist, strdup(file));
	       }
	     ecore_list_next(files);
	  }
	ecore_list_destroy(files);
     }
   return flist;
}

EAPI void
e_config_profile_add(const char *prof)
{
   char buf[4096];
   const char *homedir;
   
   homedir = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/config/%s", homedir, prof);
   ecore_file_mkdir(buf);
}

EAPI void
e_config_profile_del(const char *prof)
{
   Ecore_List *files;
   char buf[4096];
   const char *homedir;
   
   homedir = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/config/%s", homedir, prof);
   files = ecore_file_ls(buf);
   if (files)
     {
	char *file;
	
	ecore_list_first_goto(files);
	while ((file = ecore_list_current(files)))
	  {
	     snprintf(buf, sizeof(buf), "%s/.e/e/config/%s/%s",
		      homedir, prof, file);
	     ecore_file_unlink(buf);
	     ecore_list_next(files);
	  }
        ecore_list_destroy(files);
     }
   snprintf(buf, sizeof(buf), "%s/.e/e/config/%s", homedir, prof);
   ecore_file_rmdir(buf);
}

EAPI Eina_List *
e_config_engine_list(void)
{
   Eina_List *l = NULL;
   l = eina_list_append(l, strdup("SOFTWARE"));
   /*
    * DISABLE GL as an accessible engine - it does have problems, ESPECIALLY with
    * shaped windows (it can't do them), and multiple gl windows and shared
    * contexts, so for now just disable it. xrender is much more complete in
    * this regard.
    */
#if 0
   l = eina_list_append(l, strdup("GL"));
#endif
   if (ecore_evas_engine_type_supported_get(ECORE_EVAS_ENGINE_XRENDER_X11))
     l = eina_list_append(l, strdup("XRENDER"));
   if (ecore_evas_engine_type_supported_get(ECORE_EVAS_ENGINE_SOFTWARE_16_X11))
     l = eina_list_append(l, strdup("SOFTWARE_16"));
   return l;
}

EAPI void
e_config_save_block_set(int block)
{
  _e_config_save_block = block;
}

EAPI int
e_config_save_block_get(void)
{
   return _e_config_save_block;
}

EAPI void *
e_config_domain_load(const char *domain, E_Config_DD *edd)
{
   Eet_File *ef;
   char buf[4096];
   const char *homedir;
   void *data = NULL;

   homedir = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/config/%s/%s.cfg",
	    homedir, _e_config_profile, domain);
   ef = eet_open(buf, EET_FILE_MODE_READ);
   if (ef)
     {
	data = eet_data_read(ef, edd, "config");
	eet_close(ef);
        return data;
     }

   /* fallback to a system directory
    * FIXME proper $PATH like handling might be wanted
    */ 
   snprintf(buf, sizeof(buf), "%s/data/config/%s/%s.cfg",
	    e_prefix_data_get(), _e_config_profile, domain);
   ef = eet_open(buf, EET_FILE_MODE_READ);
   if (ef)
     {
	data = eet_data_read(ef, edd, "config");
	eet_close(ef);
        return data;
     }

   return data;
}

EAPI int
e_config_profile_save(void)
{
   Eet_File *ef;
   char buf[4096], buf2[4096];
   const char *homedir;
   int ok = 0;

   /* FIXME: check for other sessions fo E running */
   homedir = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/config/profile.cfg", homedir);
   snprintf(buf2, sizeof(buf2), "%s.tmp", buf);

   ef = eet_open(buf2, EET_FILE_MODE_WRITE);
   if (ef)
     {
	ok = eet_write(ef, "config", _e_config_profile, 
		       strlen(_e_config_profile), 0);
	if (_e_config_eet_close_handle(ef, buf2))
	  {
	     int ret;

	     ret = ecore_file_mv(buf2, buf);
	     if (!ret)
	       {
		  printf("*** Error saving profile. ***");
	       }
	  }
	ecore_file_unlink(buf2);
     }
   return ok;
}

EAPI int
e_config_domain_save(const char *domain, E_Config_DD *edd, const void *data)
{
   Eet_File *ef;
   char buf[4096], buf2[4096];
   const char *homedir;
   int ok = 0, ret;

   if (_e_config_save_block) return 0;
   /* FIXME: check for other sessions fo E running */
   homedir = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/config/%s", 
	    homedir, _e_config_profile);
   ecore_file_mkdir(buf);
   snprintf(buf, sizeof(buf), "%s/.e/e/config/%s/%s.cfg", 
	    homedir, _e_config_profile, domain);
   snprintf(buf2, sizeof(buf2), "%s.tmp", buf);
   ef = eet_open(buf2, EET_FILE_MODE_WRITE);
   if (ef)
     {
	ok = eet_data_write(ef, edd, "config", data, 1);
	if (_e_config_eet_close_handle(ef, buf2))
	  {
	     ret = ecore_file_mv(buf2, buf);
	     if (!ret)
	       {
		  printf("*** Error saving profile. ***");
	       }
	  }
	ecore_file_unlink(buf2);
     }
   return ok;
}

EAPI E_Config_Binding_Mouse *
e_config_binding_mouse_match(E_Config_Binding_Mouse *eb_in)
{
   Eina_List *l;
   
   for (l = e_config->mouse_bindings; l; l = l->next)
     {
	E_Config_Binding_Mouse *eb;
	
	eb = l->data;
	if ((eb->context == eb_in->context) &&
	    (eb->button == eb_in->button) &&
	    (eb->modifiers == eb_in->modifiers) &&
	    (eb->any_mod == eb_in->any_mod) &&
	    (((eb->action) && (eb_in->action) && (!strcmp(eb->action, eb_in->action))) ||
	     ((!eb->action) && (!eb_in->action))) &&
	    (((eb->params) && (eb_in->params) && (!strcmp(eb->params, eb_in->params))) ||
	     ((!eb->params) && (!eb_in->params))))
	  return eb;
     }
   return NULL;
}

EAPI E_Config_Binding_Key *
e_config_binding_key_match(E_Config_Binding_Key *eb_in)
{
   Eina_List *l;
   
   for (l = e_config->key_bindings; l; l = l->next)
     {
	E_Config_Binding_Key *eb;
	
	eb = l->data;
	if ((eb->context == eb_in->context) &&
	    (eb->modifiers == eb_in->modifiers) &&
	    (eb->any_mod == eb_in->any_mod) &&
	    (((eb->key) && (eb_in->key) && (!strcmp(eb->key, eb_in->key))) ||
	     ((!eb->key) && (!eb_in->key))) &&
	    (((eb->action) && (eb_in->action) && (!strcmp(eb->action, eb_in->action))) ||
	     ((!eb->action) && (!eb_in->action))) &&
	    (((eb->params) && (eb_in->params) && (!strcmp(eb->params, eb_in->params))) ||
	     ((!eb->params) && (!eb_in->params))))
	  return eb;
     }
   return NULL;
}

EAPI E_Config_Binding_Signal *
e_config_binding_signal_match(E_Config_Binding_Signal *eb_in)
{
   Eina_List *l;
   
   for (l = e_config->signal_bindings; l; l = l->next)
     {
	E_Config_Binding_Signal *eb;
	
	eb = l->data;
	if ((eb->context == eb_in->context) &&
	    (eb->modifiers == eb_in->modifiers) &&
	    (eb->any_mod == eb_in->any_mod) &&
	    (((eb->signal) && (eb_in->signal) && (!strcmp(eb->signal, eb_in->signal))) ||
	     ((!eb->signal) && (!eb_in->signal))) &&
	    (((eb->source) && (eb_in->source) && (!strcmp(eb->source, eb_in->source))) ||
	     ((!eb->source) && (!eb_in->source))) &&
	    (((eb->action) && (eb_in->action) && (!strcmp(eb->action, eb_in->action))) ||
	     ((!eb->action) && (!eb_in->action))) &&
	    (((eb->params) && (eb_in->params) && (!strcmp(eb->params, eb_in->params))) ||
	     ((!eb->params) && (!eb_in->params))))
	  return eb;
     }
   return NULL;
}

EAPI E_Config_Binding_Wheel *
e_config_binding_wheel_match(E_Config_Binding_Wheel *eb_in)
{
   Eina_List *l;
   
   for (l = e_config->wheel_bindings; l; l = l->next)
     {
	E_Config_Binding_Wheel *eb;
	
	eb = l->data;
	if ((eb->context == eb_in->context) &&
	    (eb->direction == eb_in->direction) &&
	    (eb->z == eb_in->z) &&
	    (eb->modifiers == eb_in->modifiers) &&
	    (eb->any_mod == eb_in->any_mod) &&
	    (((eb->action) && (eb_in->action) && (!strcmp(eb->action, eb_in->action))) ||
	     ((!eb->action) && (!eb_in->action))) &&
	    (((eb->params) && (eb_in->params) && (!strcmp(eb->params, eb_in->params))) ||
	     ((!eb->params) && (!eb_in->params))))
	  return eb;
     }
   return NULL;
}

/* local subsystem functions */
static void
_e_config_save_cb(void *data)
{
   e_config_profile_save();
   e_module_save_all();
   e_config_domain_save("e", _e_config_edd, e_config);
   _e_config_save_defer = NULL;
}

static void
_e_config_free(void)
{
   if (e_config)
     {
	while (e_config->modules)
	  {
	     E_Config_Module *em;

	     em = e_config->modules->data;
	     e_config->modules = eina_list_remove_list(e_config->modules, e_config->modules);
	     if (em->name) eina_stringshare_del(em->name);
	     E_FREE(em);
	  }
	while (e_config->font_fallbacks)
	  {
	     E_Font_Fallback *eff;
	     
	     eff = e_config->font_fallbacks->data;
	     e_config->font_fallbacks = eina_list_remove_list(e_config->font_fallbacks, e_config->font_fallbacks);
	     if (eff->name) eina_stringshare_del(eff->name);
	     E_FREE(eff);
	  }
	while (e_config->font_defaults)
	  {
	     E_Font_Default *efd;
	     
	     efd = e_config->font_defaults->data;
	     e_config->font_defaults = eina_list_remove_list(e_config->font_defaults, e_config->font_defaults);
	     if (efd->text_class) eina_stringshare_del(efd->text_class);
	     if (efd->font) eina_stringshare_del(efd->font);
	     E_FREE(efd);
	  }
	while (e_config->themes)
	  {
	     E_Config_Theme *et;
	     
	     et = e_config->themes->data;
	     e_config->themes = eina_list_remove_list(e_config->themes, e_config->themes);
	     if (et->category) eina_stringshare_del(et->category);
	     if (et->file) eina_stringshare_del(et->file);
	     E_FREE(et);
	  }
	while (e_config->mouse_bindings)
	  {
	     E_Config_Binding_Mouse *eb;
	     
	     eb = e_config->mouse_bindings->data;
	     e_config->mouse_bindings  = eina_list_remove_list(e_config->mouse_bindings, e_config->mouse_bindings);
	     if (eb->action) eina_stringshare_del(eb->action);
	     if (eb->params) eina_stringshare_del(eb->params);
	     E_FREE(eb);
	  }
	while (e_config->key_bindings)
	  {
	     E_Config_Binding_Key *eb;
	     
	     eb = e_config->key_bindings->data;
	     e_config->key_bindings  = eina_list_remove_list(e_config->key_bindings, e_config->key_bindings);
	     if (eb->key) eina_stringshare_del(eb->key);
	     if (eb->action) eina_stringshare_del(eb->action);
	     if (eb->params) eina_stringshare_del(eb->params);
	     E_FREE(eb);
	  }
	while (e_config->signal_bindings)
	  {
	     E_Config_Binding_Signal *eb;
	     
	     eb = e_config->signal_bindings->data;
	     e_config->signal_bindings  = eina_list_remove_list(e_config->signal_bindings, e_config->signal_bindings);
	     if (eb->signal) eina_stringshare_del(eb->signal);
	     if (eb->source) eina_stringshare_del(eb->source);
	     if (eb->action) eina_stringshare_del(eb->action);
	     if (eb->params) eina_stringshare_del(eb->params);
	     E_FREE(eb);
	  }
	while (e_config->wheel_bindings)
	  {
	     E_Config_Binding_Wheel *eb;
	     
	     eb = e_config->wheel_bindings->data;
	     e_config->wheel_bindings  = eina_list_remove_list(e_config->wheel_bindings, e_config->wheel_bindings);
	     if (eb->action) eina_stringshare_del(eb->action);
	     if (eb->params) eina_stringshare_del(eb->params);
	     E_FREE(eb);
	  }
	while (e_config->path_append_data)
	  {
	     E_Path_Dir *epd;
	     epd = e_config->path_append_data->data;
	     e_config->path_append_data = eina_list_remove_list(e_config->path_append_data, e_config->path_append_data);
	     if (epd->dir) eina_stringshare_del(epd->dir);
	     E_FREE(epd);
	  }
	while (e_config->path_append_images)
	  {
	     E_Path_Dir *epd;
	     epd = e_config->path_append_images->data;
	     e_config->path_append_images = eina_list_remove_list(e_config->path_append_images, e_config->path_append_images);
	     if (epd->dir) eina_stringshare_del(epd->dir);
	     E_FREE(epd);
	  }
	while (e_config->path_append_fonts)
	  {
	     E_Path_Dir *epd;
	     epd = e_config->path_append_fonts->data;
	     e_config->path_append_fonts = eina_list_remove_list(e_config->path_append_fonts, e_config->path_append_fonts);
	     if (epd->dir) eina_stringshare_del(epd->dir);
	     E_FREE(epd);
	  }
	while (e_config->path_append_themes)
	  {
	     E_Path_Dir *epd;
	     epd = e_config->path_append_themes->data;
	     e_config->path_append_themes = eina_list_remove_list(e_config->path_append_themes, e_config->path_append_themes);
	     if (epd->dir) eina_stringshare_del(epd->dir);
	     E_FREE(epd);
	  }
	while (e_config->path_append_init)
	  {
	     E_Path_Dir *epd;
	     epd = e_config->path_append_init->data;
	     e_config->path_append_init = eina_list_remove_list(e_config->path_append_init, e_config->path_append_init);
	     if (epd->dir) eina_stringshare_del(epd->dir);
	     E_FREE(epd);
	  }
	while (e_config->path_append_icons)
	  {
	     E_Path_Dir *epd;
	     epd = e_config->path_append_icons->data;
	     e_config->path_append_icons = eina_list_remove_list(e_config->path_append_icons, e_config->path_append_icons);
	     if (epd->dir) eina_stringshare_del(epd->dir);
	     E_FREE(epd);
	  }
	while (e_config->path_append_modules)
	  {
	     E_Path_Dir *epd;
	     epd = e_config->path_append_modules->data;
	     e_config->path_append_modules = eina_list_remove_list(e_config->path_append_modules, e_config->path_append_modules);
	     if (epd->dir) eina_stringshare_del(epd->dir);
	     E_FREE(epd);
	  }
	while (e_config->path_append_backgrounds)
	  {
	     E_Path_Dir *epd;
	     epd = e_config->path_append_backgrounds->data;
	     e_config->path_append_backgrounds = eina_list_remove_list(e_config->path_append_backgrounds, e_config->path_append_backgrounds);
	     if (epd->dir) eina_stringshare_del(epd->dir);
	     E_FREE(epd);
	  }
	while (e_config->path_append_messages)
	  {
	     E_Path_Dir *epd;
	     epd = e_config->path_append_messages->data;
	     e_config->path_append_messages = eina_list_remove_list(e_config->path_append_messages, e_config->path_append_messages);
	     if (epd->dir) eina_stringshare_del(epd->dir);
	     E_FREE(epd);
	  }
	while (e_config->remembers)
	  {
	     E_Remember *rem;
	     rem = e_config->remembers->data;
	     e_config->remembers = eina_list_remove_list(e_config->remembers, e_config->remembers);
	     
	     if (rem->name) eina_stringshare_del(rem->name);
	     if (rem->class) eina_stringshare_del(rem->class);
	     if (rem->title) eina_stringshare_del(rem->title);		   
	     if (rem->role) eina_stringshare_del(rem->role);
	     if (rem->prop.border) eina_stringshare_del(rem->prop.border);
	     if (rem->prop.command) eina_stringshare_del(rem->prop.command);
	     
	     E_FREE(rem);
	  }
	while (e_config->color_classes)
	  {
	     E_Color_Class *cc;
	     cc = e_config->color_classes->data;
	     e_config->color_classes = eina_list_remove_list(e_config->color_classes, e_config->color_classes);

	     if (cc->name) eina_stringshare_del(cc->name);
	     E_FREE(cc);
	  }
	if (e_config->init_default_theme) eina_stringshare_del(e_config->init_default_theme);
	if (e_config->desktop_default_background) eina_stringshare_del(e_config->desktop_default_background);
	if (e_config->desktop_default_name) eina_stringshare_del(e_config->desktop_default_name);
	if (e_config->language) eina_stringshare_del(e_config->language);
	if (e_config->transition_start) eina_stringshare_del(e_config->transition_start);
	if (e_config->transition_desk) eina_stringshare_del(e_config->transition_desk);
	if (e_config->transition_change) eina_stringshare_del(e_config->transition_change);
	if (e_config->input_method) eina_stringshare_del(e_config->input_method);
	if (e_config->exebuf_term_cmd) eina_stringshare_del(e_config->exebuf_term_cmd);
	if (e_config->desklock_personal_passwd) eina_stringshare_del(e_config->desklock_personal_passwd);
	if (e_config->desklock_background) eina_stringshare_del(e_config->desklock_background);
	if (e_config->icon_theme) eina_stringshare_del(e_config->icon_theme);
	if (e_config->wallpaper_import_last_dev) eina_stringshare_del(e_config->wallpaper_import_last_dev);
	if (e_config->wallpaper_import_last_path) eina_stringshare_del(e_config->wallpaper_import_last_path);
	if (e_config->theme_default_border_style) eina_stringshare_del(e_config->theme_default_border_style);
	if (e_config->desklock_custom_desklock_cmd) eina_stringshare_del(e_config->desklock_custom_desklock_cmd);
	E_FREE(e_config);
     }
}

static int
_e_config_cb_timer(void *data)
{
   e_util_dialog_show(_("Configuration Upgraded"), data);
   return 0;
}

static E_Dialog *_e_config_error_dialog = NULL;

static void
_e_config_error_dialog_cb_delete(void *dia)
{
   if (dia == _e_config_error_dialog)
     {
	_e_config_error_dialog = NULL;
     }
}

static int
_e_config_eet_close_handle(Eet_File *ef, char *file)
{
   Eet_Error err;
   char *erstr = NULL;
   
   err = eet_close(ef);
   switch (err)
     {
      case EET_ERROR_WRITE_ERROR:
	erstr = _("An error occured while saving Enlightenment's<br>"
		  "configuration to disk. The error could not be<br>"
		  "deterimined.<br>"
		  "<br>"
		  "The file where the error occured was:<br>"
		  "%s<br>"
		  "<br>"
		  "This file has been deleted to avoid corrupt data.<br>"
		  );
	break;
      case EET_ERROR_WRITE_ERROR_FILE_TOO_BIG:
	erstr = _("Enlightenment's configuration files are too big<br>"
		  "for the file system they are being saved to.<br>"
		  "This error is very strange as the files should<br>"
		  "be extremely small. Please check the settings<br>"
		  "for your home directory.<br>"
		  "<br>"
		  "The file where the error occured was:<br>"
		  "%s<br>"
		  "<br>"
		  "This file has been deleted to avoid corrupt data.<br>"
		  );
	break;
      case EET_ERROR_WRITE_ERROR_IO_ERROR:
	erstr = _("An output error occured when writing the configuration<br>"
		  "files for Enlightenment. Your disk is having troubles<br>"
		  "and possibly needs replacement.<br>"
		  "<br>"
		  "The file where the error occured was:<br>"
		  "%s<br>"
		  "<br>"
		  "This file has been deleted to avoid corrupt data.<br>"
		  );
	break;
      case EET_ERROR_WRITE_ERROR_OUT_OF_SPACE:
	erstr = _("Enlightenment cannot write its configuration file<br>"
		  "because it ran out of space to write the file.<br>"
		  "You have either run out of disk space or have<br>"
		  "gone over your quota limit.<br>"
		  "<br>"
		  "The file where the error occured was:<br>"
		  "%s<br>"
		  "<br>"
		  "This file has been deleted to avoid corrupt data.<br>"
		  );
	break;
      case EET_ERROR_WRITE_ERROR_FILE_CLOSED:
	erstr = _("Enlightenment unexpectedly had the configuration file<br>"
		  "it was writing closed on it. This is very unusual.<br>"
		  "<br>"
		  "The file where the error occured was:<br>"
		  "%s<br>"
		  "<br>"
		  "This file has been deleted to avoid corrupt data.<br>"
		  );
	break;
      default:
	break;
     }
   if (erstr)
     {
	/* delete any partially-written file */
	ecore_file_unlink(file);
	if (!_e_config_error_dialog)
	  {
             E_Dialog *dia;
	     
	     dia = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_sys_error_logout_slow");
	     if (dia)
	       {
		  char buf[8192];
		  
		  e_dialog_title_set(dia, _("Enlightenment Configuration Write Problems"));
		  e_dialog_icon_set(dia, "enlightenment/error", 64);
		  snprintf(buf, sizeof(buf), erstr, file);
		  e_dialog_text_set(dia, buf);
		  e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
		  e_dialog_button_focus_num(dia, 0);
		  e_win_centered_set(dia->win, 1);
		  e_object_del_attach_func_set(E_OBJECT(dia), _e_config_error_dialog_cb_delete);
		  e_dialog_show(dia);
		  _e_config_error_dialog = dia;
	       }
	  }
	return 0;
     }
   return 1;
}
