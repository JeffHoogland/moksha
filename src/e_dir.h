#ifndef E_VIEW_MODEL_H
#define E_VIEW_MODEL_H

#include "fs.h"
#include "iconbar.h"
#include <Evas.h>

#ifndef E_VIEW_TYPEDEF
#define E_VIEW_TYPEDEF
typedef struct _E_View E_View;
#endif

#ifndef E_ICON_TYPEDEF
#define E_ICON_TYPEDEF
typedef struct _E_Icon E_Icon;
#endif

#ifndef E_VIEW_MODEL_TYPEDEF
#define E_VIEW_MODEL_TYPEDEF
typedef struct _E_Dir E_Dir;
#endif

struct _E_Dir
{
   E_Object            o;

   /* The realpath of the view's directory */
   char               *dir;

   Evas_List           files;

   Evas_Object         obj_bg;

   char               *bg_file;

   E_FS_Restarter     *restarter;

   int                 monitor_id;

   /* A list of all the views for which an E_Dir is sharing data */
   Evas_List           views;

   int                 is_desktop;
};

/**
 * e_dir_set_dir - Assigns a directory to a view E_Dir
 * @ed:   The view E_Dir to set the dir for
 * @dir: The directory
 *
 * Set the directory for a dir and starts monitoring it via efsd.
 */
void                e_dir_set_dir(E_Dir * ed, char *dir);

E_Dir       *e_dir_new(void);
void                e_dir_init(void);
void                e_dir_register_view(E_Dir * d, E_View * v);
void                e_dir_unregister_view(E_View * v);

E_Dir       *e_dir_find_by_monitor_id(int id);

/* Deal with incoming file events */
void                e_dir_file_added(int id, char *file);
void                e_dir_file_deleted(int id, char *file);
void                e_dir_file_changed(int id, char *file);
void                e_dir_file_moved(int id, char *file);
#endif
