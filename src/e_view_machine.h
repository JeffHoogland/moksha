#ifndef E_VIEW_MACHINE_H
#define E_VIEW_MACHINE_H
#include <Evas.h>
#include "view.h"
struct _e_view_machine
{
   Evas_List views;
   Evas_List models;
};
typedef struct _e_view_machine E_View_Machine;

void e_view_machine_init();
void e_view_machine_register_view(E_View *v);
void e_view_machine_unregister_view(E_View *v);
void e_view_machine_get_model(E_View *v, char *path, int is_desktop);
#endif
