#ifndef E_VIEW_MACHINE_H
#define E_VIEW_MACHINE_H
#include <Evas.h>
#include "view.h"
struct _e_view_machine
{
   Evas_List *           views;
   Evas_List *           dirs;
   Evas_List *           looks;
   
};
typedef struct _e_view_machine E_View_Machine;

void                e_view_machine_init(void);
void                e_view_machine_register_view(E_View * v);
void                e_view_machine_unregister_view(E_View * v);
void                e_view_machine_register_dir(E_Dir * d);
void                e_view_machine_unregister_dir(E_Dir * d);
void                e_view_machine_register_look(E_View_Look * l);
void                e_view_machine_unregister_look(E_View_Look * l);

void                e_view_machine_close_all_views(void);
E_Dir              *e_view_machine_dir_lookup(char *path);
E_View_Look        *e_view_machine_look_lookup(char *path);
E_View             *e_view_machine_get_view_by_main_window(Window win);
E_View             *e_view_machine_get_view_by_base_window(Window win);

#endif
