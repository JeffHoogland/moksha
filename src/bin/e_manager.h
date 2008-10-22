/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Manager E_Manager;

#else
#ifndef E_MANAGER_H
#define E_MANAGER_H

#define E_MANAGER_TYPE 0xE0b01008

struct _E_Manager
{
   E_Object             e_obj_inherit;
   
   Ecore_X_Window       win;
   int                  num;
   int                  x, y, w, h;
   char                 visible : 1;
   Ecore_X_Window       root;
   Eina_List           *handlers;
   Eina_List           *containers;

   E_Pointer           *pointer;
   Ecore_X_Window       initwin;
};

EAPI int        e_manager_init(void);
EAPI int        e_manager_shutdown(void);
EAPI Eina_List *e_manager_list(void);
    
EAPI E_Manager      *e_manager_new(Ecore_X_Window root, int num);
EAPI void            e_manager_manage_windows(E_Manager *man);
EAPI void            e_manager_show(E_Manager *man);
EAPI void            e_manager_hide(E_Manager *man);
EAPI void            e_manager_move(E_Manager *man, int x, int y);
EAPI void            e_manager_resize(E_Manager *man, int w, int h);
EAPI void            e_manager_move_resize(E_Manager *man, int x, int y, int w, int h);
EAPI void            e_manager_raise(E_Manager *man);
EAPI void            e_manager_lower(E_Manager *man);
EAPI E_Manager      *e_manager_current_get(void);
EAPI E_Manager      *e_manager_number_get(int num);

EAPI void            e_managers_keys_grab(void);
EAPI void            e_managers_keys_ungrab(void);
    
#endif
#endif
