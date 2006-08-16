/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_App_Cache E_App_Cache;

#else
#ifndef E_APPS_CACHE_H
#define E_APPS_CACHE_H

struct _E_App_Cache
{
   const char         *name; /* app name */
   const char         *generic; /* generic app name */
   const char         *comment; /* a longer description */
   const char         *exe; /* command to execute, NULL if directory */
  
   const char         *file; /* the .eap filename */
   unsigned long long  file_mod_time; /* the last modified time of the file */
   
   const char         *win_name; /* window name */
   const char         *win_class; /* window class */
   const char         *win_title; /* window title */
   const char         *win_role; /* window role */
   
   const char         *icon_class; /* icon_class */
   const char         *icon_path;  /* FDO icon path */
   
   Evas_List          *subapps; /* if this a directory, a list of more E_App's */
   
   unsigned char       startup_notify; /* disable while starting etc. */
   unsigned char       wait_exit; /* wait for app to exit before execing next */

   unsigned char       is_link; /* cached .order logic info */
   unsigned char       is_dir; /* cached stat info */
   
   /* these are generated post-load */
   Evas_Hash          *subapps_hash; /* a quick lookup hash */
};

EAPI int          e_app_cache_init(void);
EAPI int          e_app_cache_shutdown(void);

EAPI E_App_Cache *e_app_cache_load(const char *path);
EAPI E_App_Cache *e_app_cache_generate(E_App *a);
EAPI void         e_app_cache_free(E_App_Cache *ac);
EAPI int          e_app_cache_save(E_App_Cache *ac, const char *path);

#endif
#endif
