/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef enum _E_App_Change
{
   E_APP_ADD,
   E_APP_DEL,
   E_APP_CHANGE,
   E_APP_ORDER,
   E_APP_EXEC,
   E_APP_READY,
   E_APP_EXIT,
   E_APP_READY_EXPIRE
} E_App_Change;

typedef struct _E_App          E_App;
typedef struct _E_App_Instance E_App_Instance;
typedef struct _E_App_Autopsy  E_App_Autopsy;

#else
#ifndef E_APPS_H
#define E_APPS_H

#define E_APP_TYPE 0xE0b01001

struct _E_App
{
   E_Object            e_obj_inherit;
   
   E_App              *parent; /* the parent e_app node */
   E_App              *orig; /* if this is a copy, point to the original */
   
   const char         *name; /* app name */
   const char         *generic; /* generic app name */
   const char         *comment; /* a longer description */
   const char         *exe; /* command to execute, NULL if directory */
   
   const char         *path; /* path to .eet containing icons etc. etc. */
//   const char         *icon_path; /* path to icon file, in case it is different from path. */

   const char         *win_name; /* window name */
   const char         *win_class; /* window class */
   const char         *win_title; /* window title */
   const char         *win_role; /* window role */

   const char         *icon_class; /* icon_class */
   
   Evas_List          *subapps; /* if this a directory, a list of more E_App's */
   
   Evas_List          *instances; /* a list of all the exe handles for executions */

   Evas_List          *references; /* If this app is in a main repository, this would
				      be a list to other eapp pointing to this */
   Ecore_File_Monitor *monitor; /* Check for changes and files */
   
   unsigned char       startup_notify : 1; /* disable while starting etc. */
   unsigned char       wait_exit : 1; /* wait for app to exit before execing next */
   unsigned char       starting : 1; /* this app is starting */

   unsigned char       scanned : 1; /* have we scanned a subdir app yet */

   unsigned char       deleted : 1; /* this app's file is deleted from disk */
   
   /* used for eap edit */
   const char         *image; /* used when we're saving a image into the eap */
   int                 width; /* used for saving eap's image width in edje */
   int                 height; /* used for saving eap's image height in edje */
   unsigned char       tmpfile : 1;
};

struct _E_App_Instance
{
   E_App       *app;
   Ecore_Exe   *exe;
   int          launch_id;
   double       launch_time;
   Ecore_Timer *expire_timer;
};

struct _E_App_Autopsy
{
   E_App       *app;

   Ecore_Exe_Event_Del del;
   E_Config_Dialog *error_dialog;   
   Ecore_Exe_Event_Data *error, *read;
};

EAPI int         e_app_init                              (void);
EAPI int         e_app_shutdown                          (void);

EAPI void        e_app_unmonitor_all                     (void);
    
EAPI E_App      *e_app_new                               (const char *path, int scan_subdirs);
EAPI E_App      *e_app_empty_new                         (const char *path);
EAPI void        e_app_image_size_set                    (E_App *a, int w, int h);
EAPI int         e_app_is_parent                         (E_App *parent, E_App *app);
EAPI int         e_app_equals                            (E_App *app1, E_App *app2);
EAPI void        e_app_subdir_scan                       (E_App *a, int scan_subdirs);
EAPI int         e_app_exec                              (E_App *a, int launch_id);
EAPI int         e_app_starting_get                      (E_App *a);
EAPI int         e_app_running_get                       (E_App *a);
EAPI void        e_app_list_prepend_relative             (E_App *add, E_App *before);
EAPI void        e_app_list_append                       (E_App *add, E_App *parent);
EAPI void        e_app_files_list_prepend_relative       (Evas_List *files, E_App *before);
EAPI void        e_app_files_list_append                 (Evas_List *files, E_App *parent);
EAPI void        e_app_prepend_relative                  (E_App *add, E_App *before);
EAPI void        e_app_append                            (E_App *add, E_App *parent);
EAPI void        e_app_files_prepend_relative            (Evas_List *files, E_App *before);
EAPI void        e_app_files_append                      (Evas_List *files, E_App *parent);
EAPI void        e_app_remove                            (E_App *a);
    
EAPI void        e_app_change_callback_add               (void (*func) (void *data, E_App *a, E_App_Change ch), void *data);
EAPI void        e_app_change_callback_del               (void (*func) (void *data, E_App *a, E_App_Change ch), void *data);

EAPI E_App      *e_app_launch_id_pid_find                (int launch_id, pid_t pid);
EAPI E_App      *e_app_window_name_class_title_role_find (const char *name, const char *class, const char *title, const char *role);
EAPI E_App      *e_app_file_find                         (const char *file);
EAPI E_App      *e_app_name_find                         (const char *name);
EAPI E_App      *e_app_generic_find                      (const char *generic);
EAPI E_App      *e_app_exe_find                          (const char *exe);

EAPI Evas_List  *e_app_name_glob_list                    (const char *name);
EAPI Evas_List  *e_app_generic_glob_list                 (const char *generic);
EAPI Evas_List  *e_app_exe_glob_list                     (const char *exe);
EAPI Evas_List  *e_app_comment_glob_list                 (const char *comment);
    
EAPI void         e_app_fields_fill                       (E_App *a, const char *path);
EAPI void         e_app_fields_save                       (E_App *a);
EAPI E_App       *e_app_raw_new                           (void);
EAPI Ecore_List  *e_app_dir_file_list_get                 (E_App *a);
EAPI void         e_app_fields_empty                      (E_App *a);
EAPI int          e_app_valid_exe_get                     (E_App *a);
EAPI Evas_Object *e_app_icon_add                          (Evas *evas, E_App *a);

#endif
#endif
