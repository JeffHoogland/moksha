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

void e_view_machine_init(void);
void e_view_machine_register_view(E_View *v);
void e_view_machine_unregister_view(E_View *v);
E_View_Model *e_view_machine_model_lookup(char *path);
#endif
