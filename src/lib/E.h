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

typedef struct _E_Response_Module_List    E_Response_Module_List;
typedef struct _E_Response_Background_Get E_Response_Background_Get;

struct _E_Response_Module_List
{
   char   *name;
   char    enabled;
};

struct _E_Response_Background_Get
{
   char   *data;
};

extern EAPI int E_RESPONSE_MODULE_LIST;
extern EAPI int E_RESPONSE_BACKGROUND_GET;

#ifdef __cplusplus
extern "C" {
#endif

   /* library startup and shutdown */
   EAPI int          e_init                       (const char *display);
   EAPI int          e_shutdown                   (void);

   /* E startup and shutdown */
   EAPI void         e_restart                    (void);

   /* E module manipulation */
   EAPI void         e_module_enabled_set         (const char *module, int enable);
   EAPI void         e_module_loaded_set          (const char *module, int load);
   EAPI void         e_module_list                (void);

   /* E desktop manipulation */
   EAPI void         e_background_set             (const char *bgfile);
   EAPI void         e_background_get             (void);
   
#ifdef __cplusplus
}
#endif

#endif
