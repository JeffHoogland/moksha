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
   EAPI void         e_background_set             (const char *bgfile);
     
   
#ifdef __cplusplus
}
#endif

#endif
