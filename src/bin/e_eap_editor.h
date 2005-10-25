#ifdef E_TYPEDEFS

typedef struct _E_App_Edit E_App_Edit;

#else
#ifndef E_EAP_EDIT_H
#define E_EAP_EDIT_H

#define E_EAP_EDIT_TYPE 0xE0b01019

struct _E_App_Edit
{
   E_Object             e_obj_inherit;
   
   E_Container *con;
   E_Win       *win;
   Evas        *evas;
   E_Dialog    *dia;
   
   E_App       *eap;
   
   struct {
      char *name;
      char *generic;
      char *comments;
      char *exe;
      char *wname;
      char *wclass;
      char *path;
      int   startup_notify;
      int   wait_exit;
   } data;
};

E_App_Edit *e_eap_edit_show(E_Container *con, E_App *a);

#endif
#endif
