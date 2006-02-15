/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#define E_CONFIG_DD_NEW(str, typ) \
   e_config_descriptor_new(str, sizeof(typ))
#define E_CONFIG_DD_FREE(eed) if (eed) { eet_data_descriptor_free((eed)); (eed) = NULL; }
#define E_CONFIG_VAL(edd, type, member, dtype) EET_DATA_DESCRIPTOR_ADD_BASIC(edd, type, #member, member, dtype)
#define E_CONFIG_SUB(edd, type, member, eddtype) EET_DATA_DESCRIPTOR_ADD_SUB(edd, type, #member, member, eddtype)
#define E_CONFIG_LIST(edd, type, member, eddtype) EET_DATA_DESCRIPTOR_ADD_LIST(edd, type, #member, member, eddtype)

#define CHAR   EET_T_CHAR
#define SHORT  EET_T_SHORT
#define INT    EET_T_INT
#define LL     EET_T_LONG_LONG
#define FLOAT  EET_T_FLOAT
#define DOUBLE EET_T_DOUBLE
#define UCHAR  EET_T_UCHAR
#define USHORT EET_T_USHORT
#define UINT   EET_T_UINT
#define ULL    EET_T_ULONG_LONG
#define STR    EET_T_STRING

#define E_CONFIG_LIMIT(v, min, max) {if (v > max) v = max; else if (v < min) v = min;}

typedef struct _E_Config                    E_Config;
typedef struct _E_Config_Module             E_Config_Module;
typedef struct _E_Config_Theme              E_Config_Theme;
typedef struct _E_Config_Binding_Mouse      E_Config_Binding_Mouse;
typedef struct _E_Config_Binding_Key        E_Config_Binding_Key;
typedef struct _E_Config_Binding_Signal     E_Config_Binding_Signal;
typedef struct _E_Config_Binding_Wheel      E_Config_Binding_Wheel;
typedef struct _E_Config_Desktop_Background E_Config_Desktop_Background;
typedef struct _E_Config_Desktop_Name       E_Config_Desktop_Name;
typedef struct _E_Config_Gadcon             E_Config_Gadcon;
typedef struct _E_Config_Gadcon_Client      E_Config_Gadcon_Client;

typedef Eet_Data_Descriptor                 E_Config_DD;

#else
#ifndef E_CONFIG_H
#define E_CONFIG_H

/* increment this whenever we change config enough that you need new 
 * defaults for e to work - started at 100 when we introduced this config
 * versioning feature. the value of this is really irrelevant - just as
 * long as it increases every time we change something
 */
#define E_CONFIG_FILE_VERSION 137

#define E_EVAS_ENGINE_DEFAULT      0
#define E_EVAS_ENGINE_SOFTWARE_X11 1
#define E_EVAS_ENGINE_GL_X11       2
#define E_EVAS_ENGINE_XRENDER_X11  3

typedef enum _E_Engine_Context
{
   E_ENGINE_CONTEXT_INIT,
   E_ENGINE_CONTEXT_CONTAINER,
   E_ENGINE_CONTEXT_ZONE,
   E_ENGINE_CONTEXT_BORDER,
   E_ENGINE_CONTEXT_MENU,
   E_ENGINE_CONTEXT_ERROR,
   E_ENGINE_CONTEXT_WIN,
   E_ENGINE_CONTEXT_POPUP,
   E_ENGINE_CONTEXT_DRAG
} E_Engine_Context;

struct _E_Config
{
   int         config_version;
   int         show_splash; //GUI
   char       *desktop_default_background;
   Evas_List  *desktop_backgrounds;
   char       *desktop_default_name;
   Evas_List  *desktop_names;
   double      menus_scroll_speed; // GUI
   double      menus_fast_mouse_move_threshhold; // GUI
   double      menus_click_drag_timeout; // GUI
   int         border_shade_animate; // GUI
   int         border_shade_transition; // GUI
   double      border_shade_speed; // GUI
   double      framerate; //GUI
   int         image_cache; //GUI
   int         font_cache; //GUI
   int         edje_cache; //GUI
   int         edje_collection_cache; //GUI
   double      cache_flush_interval; //GUI
   int         zone_desks_x_count;
   int         zone_desks_y_count;
   int         use_virtual_roots;
   int         use_edge_flip;
   double      edge_flip_timeout;
   int         evas_engine_default;
   int         evas_engine_container;
   int         evas_engine_init;
   int         evas_engine_menus;
   int         evas_engine_borders;
   int         evas_engine_errors;
   int         evas_engine_popups;
   int         evas_engine_drag;
   int         evas_engine_win;
   int         evas_engine_zone;
   char       *language;
   Evas_List  *modules;
   Evas_List  *font_fallbacks;
   Evas_List  *font_defaults;
   Evas_List  *themes;
   Evas_List  *mouse_bindings;
   Evas_List  *key_bindings;
   Evas_List  *signal_bindings;
   Evas_List  *wheel_bindings;
   Evas_List  *path_append_data;
   Evas_List  *path_append_images;
   Evas_List  *path_append_fonts;
   Evas_List  *path_append_themes;
   Evas_List  *path_append_init;
   Evas_List  *path_append_icons;
   Evas_List  *path_append_modules;
   Evas_List  *path_append_backgrounds;
   Evas_List  *path_append_input_methods;
   Evas_List  *path_append_messages;
   int         window_placement_policy; // GUI
   int         focus_policy; // GUI
   int         focus_setting; // GUI
   int         pass_click_on; // GUI
   int         always_click_to_raise; // GUI
   int         always_click_to_focus; // GUI
   int         use_auto_raise; // GUI
   double      auto_raise_delay; // GUI
   int         use_resist; // GUI
   int         drag_resist;
   int         desk_resist; // GUI
   int         window_resist; // GUI
   int         gadget_resist; // GUI
   int         winlist_warp_while_selecting; // GUI
   int         winlist_warp_at_end; // GUI
   double      winlist_warp_speed; // GUI
   int         winlist_scroll_animate; // GUI
   double      winlist_scroll_speed; // GUI
   int         winlist_list_show_iconified; // GUI
   int         winlist_list_show_other_desk_windows; // GUI
   int         winlist_list_show_other_screen_windows; // GUI
   int         winlist_list_uncover_while_selecting; // GUI
   int         winlist_list_jump_desk_while_selecting; // GUI
   int         winlist_list_focus_while_selecting; // GUI
   int         winlist_list_raise_while_selecting; // GUI
   double      winlist_pos_align_x; // GUI
   double      winlist_pos_align_y; // GUI
   double      winlist_pos_size_w; // GUI
   double      winlist_pos_size_h; // GUI
   int         winlist_pos_min_w; // GUI
   int         winlist_pos_min_h; // GUI
   int         winlist_pos_max_w; // GUI
   int         winlist_pos_max_h; // GUI
   int         maximize_policy; // GUI
   int         allow_shading; // GUI
   int         kill_if_close_not_possible;
   int         kill_process;
   double      kill_timer_wait;
   int         ping_clients;
   double      ping_clients_wait;
   char       *transition_start;
   char       *transition_desk;
   char       *transition_change;
   Evas_List  *remembers;
   int         move_info_follows; // GUI
   int         resize_info_follows; // GUI
   int         move_info_visible; // GUI
   int         resize_info_visible; // GUI
   int         focus_last_focused_per_desktop;
   int         focus_revert_on_hide_or_close;
   int         use_e_cursor; // GUI
   int         cursor_size; //GUI
   int         menu_autoscroll_margin; // GUI
   int         menu_autoscroll_cursor_margin; // GUI
   char	      *input_method;
   struct {
	int    move;
	int    resize;
	int    raise;
	int    lower;
	int    layer;
	int    desktop;
	int    iconify;
   } transient;
   int         modal_windows;
   int         menu_eap_name_show; // GUI
   int         menu_eap_generic_show; // GUI
   int         menu_eap_comment_show; // GUI
   int         fullscreen_policy;
   int         exebuf_max_exe_list; // GUI
   int         exebuf_max_eap_list; // GUI
   int         exebuf_scroll_animate; // GUI
   double      exebuf_scroll_speed; // GUI
   double      exebuf_pos_align_x; // GUI
   double      exebuf_pos_align_y; // GUI
   double      exebuf_pos_size_w; // GUI
   double      exebuf_pos_size_h; // GUI
   int         exebuf_pos_min_w; // GUI
   int         exebuf_pos_min_h; // GUI
   int         exebuf_pos_max_w; // GUI
   int         exebuf_pos_max_h; // GUI
   Evas_List  *color_classes;
   int         use_app_icon; // GUI
   int         cfgdlg_auto_apply; // GUI
   int         cfgdlg_default_mode; // GUI   
   Evas_List  *gadcons;
};

struct _E_Config_Module
{
   char          *name;
   unsigned char  enabled;
};

struct _E_Config_Theme
{
   char          *category;
   char          *file;
};

struct _E_Config_Binding_Mouse
{
   int            context;
   int            modifiers;
   char          *action;
   char          *params;
   unsigned char  button;
   unsigned char  any_mod;
};

struct _E_Config_Binding_Key
{
   int            context;
   int            modifiers;
   char          *key;
   char          *action;
   char          *params;
   unsigned char  any_mod;
};

struct _E_Config_Binding_Signal
{
   int            context;
   char          *signal;
   char          *source;
   int            modifiers;
   unsigned char  any_mod;
   char          *action;
   char          *params;
};

struct _E_Config_Binding_Wheel
{
   int            context;
   int            direction;
   int            z;
   int            modifiers;
   unsigned char  any_mod;
   char          *action;
   char          *params;
};

struct _E_Config_Desktop_Background
{
   int            container;
   int            zone;
   int            desk_x;
   int            desk_y;
   char          *file;
};

struct _E_Config_Desktop_Name
{
   int            container;
   int            zone;
   int            desk_x;
   int            desk_y;
   char          *name;
};

struct _E_Config_Gadcon
{
   char *name, *id;
   Evas_List *clients;
};

struct _E_Config_Gadcon_Client
{
   char *name, *id;
   struct {
      int pos, size, res;
   } geom;
};

EAPI int        e_config_init(void);
EAPI int        e_config_shutdown(void);

EAPI E_Config_DD *e_config_descriptor_new(const char *name, int size);

EAPI int        e_config_save(void);
EAPI void       e_config_save_flush(void);
EAPI void       e_config_save_queue(void);

EAPI char      *e_config_profile_get(void);
EAPI void       e_config_profile_set(char *prof);
EAPI Evas_List *e_config_profile_list(void);
EAPI void       e_config_profile_add(char *prof);
EAPI void       e_config_profile_del(char *prof);

EAPI Evas_List *e_config_engine_list(void);

EAPI void       e_config_save_block_set(int block);
EAPI int        e_config_save_block_get(void);
    
EAPI void      *e_config_domain_load(char *domain, E_Config_DD *edd);
EAPI int        e_config_profile_save(void);
EAPI int        e_config_domain_save(char *domain, E_Config_DD *edd, void *data);

EAPI E_Config_Binding_Mouse  *e_config_binding_mouse_match(E_Config_Binding_Mouse *eb_in);
EAPI E_Config_Binding_Key    *e_config_binding_key_match(E_Config_Binding_Key *eb_in);
EAPI E_Config_Binding_Signal *e_config_binding_signal_match(E_Config_Binding_Signal *eb_in);
EAPI E_Config_Binding_Wheel  *e_config_binding_wheel_match(E_Config_Binding_Wheel *eb_in);
    
extern EAPI E_Config *e_config;

#endif
#endif
