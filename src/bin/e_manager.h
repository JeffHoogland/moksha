#ifndef E_MANAGER_H
#define E_MANAGER_H

typedef struct _E_Manager E_Manager;

struct _E_Manager
{
   E_Object             e_obj_inherit;
   
   Ecore_X_Window       win;
   int                  x, y, w, h;
   char                 visible : 1;
   Ecore_X_Window       root;
   Evas_List           *handlers;
   Evas_List           *containers;
};

int        e_manager_init(void);
int        e_manager_shutdown(void);
Evas_List *e_manager_list(void);
    
E_Manager *e_manager_new(Ecore_X_Window root);
void       e_manager_show(E_Manager *man);
void       e_manager_hide(E_Manager *man);
void       e_manager_move(E_Manager *man, int x, int y);
void       e_manager_resize(E_Manager *man, int w, int h);
void       e_manager_move_resize(E_Manager *man, int x, int y, int w, int h);
void       e_manager_raise(E_Manager *man);
void       e_manager_lower(E_Manager *man);

#endif
