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
# ifdef GCC_HASCLASSVISIBILITY
#  define EAPI __attribute__ ((visibility("default")))
# else
#  define EAPI
# endif
#endif

/* FIXME: this is all borken right now :( */

typedef struct _E_Response_Module_List    E_Response_Module_List;
typedef struct _E_Response_Module_Data		E_Response_Module_Data;
typedef struct _E_Response_Dirs_List	E_Response_Dirs_List;
typedef struct _E_Response_Background_Get E_Response_Background_Get;

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

extern EAPI int E_RESPONSE_MODULE_LIST;
extern EAPI int E_RESPONSE_BACKGROUND_GET;

extern EAPI int E_RESPONSE_DATA_DIRS_LIST;
extern EAPI int E_RESPONSE_IMAGE_DIRS_LIST;
extern EAPI int E_RESPONSE_FONT_DIRS_LIST;
extern EAPI int E_RESPONSE_THEME_DIRS_LIST;
extern EAPI int E_RESPONSE_INIT_DIRS_LIST;
extern EAPI int E_RESPONSE_ICON_DIRS_LIST;
extern EAPI int E_RESPONSE_MODULE_DIRS_LIST;
extern EAPI int E_RESPONSE_BACKGROUND_DIRS_LIST;

#ifdef __cplusplus
extern "C" {
#endif

   /* library startup and shutdown */
   EAPI int          e_init                       (const char *display);
   EAPI int          e_shutdown                   (void);

   /* E startup and shutdown */
   EAPI void         e_restart                    (void);
   EAPI void         e_quit                       (void);

   /* E module manipulation */
   EAPI void         e_module_enabled_set         (const char *module, int enable);
   EAPI void         e_module_loaded_set          (const char *module, int load);
   EAPI void         e_module_list                (void);

   /* E desktop manipulation */
   EAPI void         e_background_set             (const char *bgfile);
   EAPI void         e_background_get             (void);

   /* E path information */
   EAPI void         e_data_dirs_list             (void);
   EAPI void         e_image_dirs_list            (void);
   EAPI void         e_font_dirs_list             (void);
   EAPI void         e_theme_dirs_list            (void);
   EAPI void         e_init_dirs_list             (void);
   EAPI void         e_icon_dirs_list             (void);
   EAPI void         e_module_dirs_list           (void);
   EAPI void         e_background_dirs_list       (void);

#ifdef __cplusplus
}
#endif

#endif
