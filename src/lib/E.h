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

extern EAPI int E_RESPONSE_MODULE_LIST;
extern EAPI int E_RESPONSE_BACKGROUND_GET;

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


#ifdef __cplusplus
extern "C" {
#endif

   /* edje_main.c */
   EAPI int          e_init                       (const char *display);
   EAPI int          e_shutdown                   (void);

   EAPI void         e_module_enabled_set         (const char *module,
                                                   int enable);
   EAPI void         e_module_loaded_set          (const char *module,
                                                   int load);
   EAPI void         e_module_list                (void);
   EAPI void         e_background_set             (const char *bgfile);
   EAPI void         e_background_get             (void);
     
   
#ifdef __cplusplus
}
#endif

#endif
