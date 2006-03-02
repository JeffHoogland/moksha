/*
 * vim:ts=8:sw=3:sts=3:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef _E_H
#define _E_H

#ifdef EAPI
#undef EAPI
#endif
#ifdef WIN32
# ifdef BUILDING_DLL
#  define EAPI __declspec(dllexport)
# else
#  define EAPI __declspec(dllimport)
# endif
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif
#endif

typedef enum E_Lib_Binding_Context
{
   E_LIB_BINDING_CONTEXT_NONE,
   E_LIB_BINDING_CONTEXT_UNKNOWN,
   E_LIB_BINDING_CONTEXT_BORDER,
   E_LIB_BINDING_CONTEXT_ZONE,
   E_LIB_BINDING_CONTEXT_CONTAINER,
   E_LIB_BINDING_CONTEXT_MANAGER,
   E_LIB_BINDING_CONTEXT_MENU,
   E_LIB_BINDING_CONTEXT_WINLIST,
   E_LIB_BINDING_CONTEXT_ANY
} E_Lib_Binding_Context;

typedef enum E_Lib_Binding_Modifier
{
   E_LIB_BINDING_MODIFIER_NONE = 0,
   E_LIB_BINDING_MODIFIER_SHIFT = (1 << 0),
   E_LIB_BINDING_MODIFIER_CTRL = (1 << 1),
   E_LIB_BINDING_MODIFIER_ALT = (1 << 2),
   E_LIB_BINDING_MODIFIER_WIN = (1 << 3)
} E_Lib_Binding_Modifier;

typedef struct _E_Response_Module_List E_Response_Module_List;
typedef struct _E_Response_Module_Data E_Response_Module_Data;
typedef struct _E_Response_Dirs_List E_Response_Dirs_List;
typedef struct _E_Response_Background_Get E_Response_Background_Get;
typedef struct _E_Response_Language_Get E_Response_Language_Get;
typedef struct _E_Response_Theme_Get E_Response_Theme_Get;

typedef struct _E_Response_Binding_Mouse_List E_Response_Binding_Mouse_List;
typedef struct _E_Response_Binding_Mouse_Data E_Response_Binding_Mouse_Data;
typedef struct _E_Response_Binding_Key_List E_Response_Binding_Key_List;
typedef struct _E_Response_Binding_Key_Data E_Response_Binding_Key_Data;
typedef struct _E_Response_Binding_Signal_List E_Response_Binding_Signal_List;
typedef struct _E_Response_Binding_Signal_Data E_Response_Binding_Signal_Data;
typedef struct _E_Response_Binding_Wheel_List E_Response_Binding_Wheel_List;
typedef struct _E_Response_Binding_Wheel_Data E_Response_Binding_Wheel_Data;

struct _E_Response_Module_List
{
    E_Response_Module_Data    **modules;
    int 			count;
};

struct _E_Response_Module_Data
{
   char   *name;
   char    enabled;
};

struct _E_Response_Dirs_List
{
   char   **dirs;
   int	    count;
};

struct _E_Response_Background_Get
{
   char   *file;
};

struct _E_Response_Theme_Get
{
   char   *file;
   char   *category;
};

struct _E_Response_Language_Get
{
   char   *lang;
};

struct _E_Response_Binding_Key_List
{
    E_Response_Binding_Key_Data    **bindings;
    int				     count;
};

struct _E_Response_Binding_Key_Data
{
   E_Lib_Binding_Context ctx;
   char *key;
   E_Lib_Binding_Modifier mod;
   unsigned char any_mod : 1;
   char *action;
   char *params;
};

struct _E_Response_Binding_Mouse_List
{
    E_Response_Binding_Mouse_Data    **bindings;
    int			   	       count;
};

struct _E_Response_Binding_Mouse_Data
{
   E_Lib_Binding_Context ctx;
   int button;
   E_Lib_Binding_Modifier mod;
   unsigned char any_mod : 1;
   char *action;
   char *params;
};

struct _E_Response_Binding_Signal_List
{
    E_Response_Binding_Signal_Data **bindings;
    int				     count;
};

struct _E_Response_Binding_Signal_Data
{
   E_Lib_Binding_Context ctx;
   char *signal;
   char *source;
   E_Lib_Binding_Modifier mod;
   unsigned char any_mod : 1;
   char *action;
   char *params;
};

struct _E_Response_Binding_Wheel_List
{
    E_Response_Binding_Wheel_Data  **bindings;
    int				     count;
};

struct _E_Response_Binding_Wheel_Data
{
   E_Lib_Binding_Context ctx;
   int direction;
   int z;
   E_Lib_Binding_Modifier mod;
   unsigned char any_mod : 1;
   char *action;
   char *params;
};


extern EAPI int E_RESPONSE_MODULE_LIST;
extern EAPI int E_RESPONSE_BACKGROUND_GET;
extern EAPI int E_RESPONSE_LANGUAGE_GET;
extern EAPI int E_RESPONSE_THEME_GET;

extern EAPI int E_RESPONSE_DATA_DIRS_LIST;
extern EAPI int E_RESPONSE_IMAGE_DIRS_LIST;
extern EAPI int E_RESPONSE_FONT_DIRS_LIST;
extern EAPI int E_RESPONSE_THEME_DIRS_LIST;
extern EAPI int E_RESPONSE_INIT_DIRS_LIST;
extern EAPI int E_RESPONSE_ICON_DIRS_LIST;
extern EAPI int E_RESPONSE_MODULE_DIRS_LIST;
extern EAPI int E_RESPONSE_BACKGROUND_DIRS_LIST;
extern EAPI int E_RESPONSE_START_DIRS_LIST;

extern EAPI int E_RESPONSE_BINDING_KEY_LIST;
extern EAPI int E_RESPONSE_BINDING_MOUSE_LIST;
extern EAPI int E_RESPONSE_BINDING_SIGNAL_LIST;
extern EAPI int E_RESPONSE_BINDING_WHEEL_LIST;

#ifdef __cplusplus
extern "C" {
#endif

   /* library startup and shutdown */
   EAPI int          e_lib_init                       (const char *display);
   EAPI int          e_lib_shutdown                   (void);

   /* E startup and shutdown */
   EAPI void         e_lib_restart                    (void);
   EAPI void         e_lib_quit                       (void);

   EAPI void         e_lib_config_panel_show          (void);

   /* E module manipulation */
   EAPI void         e_lib_module_enabled_set         (const char *module, int enable);
   EAPI void         e_lib_module_loaded_set          (const char *module, int load);
   EAPI void         e_lib_module_list                (void);

   /* E desktop manipulation */
   EAPI void         e_lib_background_set             (const char *bgfile);
   EAPI void         e_lib_background_get             (void);
   EAPI void         e_lib_desktop_background_add     (const int con, const int zone, const int desk_x, const int desk_y, const char *bgfile);
   EAPI void         e_lib_desktop_background_del     (const int con, const int zone, const int desk_x, const int desk_y);

   /* key/mouse bindings */
   EAPI void	     e_lib_bindings_key_list	      (void);
   EAPI void	     e_lib_binding_key_del	      (unsigned int context, unsigned int modifiers, const char *key, 
						       unsigned int any_mod, const char *action, const char *params);
   EAPI void	     e_lib_binding_key_add	      (unsigned int context, unsigned int modifiers, const char *key, 
						       unsigned int any_mod, const char *action, const char *params);

   EAPI void	     e_lib_bindings_mouse_list	      (void);
   EAPI void	     e_lib_binding_mouse_del	      (unsigned int context, unsigned int modifiers, unsigned int button, 
						       unsigned int any_mod, const char *action, const char *params);
   EAPI void	     e_lib_binding_mouse_add	      (unsigned int context, unsigned int modifiers, unsigned int button, 
						       unsigned int any_mod, const char *action, const char *params);

   /* E current theme manipulation */
   EAPI void         e_lib_theme_set                  (const char *category, const char *file);
   EAPI void         e_lib_theme_get                  (const char *category);

   /* languages */
   EAPI void         e_lib_language_set               (const char *lang);
   EAPI void         e_lib_language_get               (void);

   /* E path information */
   EAPI void         e_lib_data_dirs_list             (void);
   EAPI void         e_lib_image_dirs_list            (void);
   EAPI void         e_lib_font_dirs_list             (void);
   EAPI void         e_lib_theme_dirs_list            (void);
   EAPI void         e_lib_init_dirs_list             (void);
   EAPI void         e_lib_icon_dirs_list             (void);
   EAPI void         e_lib_module_dirs_list           (void);
   EAPI void         e_lib_background_dirs_list       (void);

#ifdef __cplusplus
}
#endif

#endif
