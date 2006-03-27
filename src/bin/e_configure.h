#ifdef E_TYPEDEFS

typedef struct _E_Configure E_Configure;

#else
#ifndef E_CONFIGURE_H
#define E_CONFIGURE_H

#define E_CONFIGURE_TYPE 0xE0b01014

struct _E_Configure
{
   E_Object             e_obj_inherit;
      
   E_Container *con;
   E_Win       *win;
   Evas        *evas;
   Evas_Object *edje;
   Evas_Object *ilist;
   Evas_Object *close;
   
   Evas_List *cblist;
};

EAPI E_Configure *e_configure_show(E_Container *con);
EAPI void         e_configure_standard_item_add(E_Configure *eco, char *icon, char *label, E_Config_Dialog *(*func) (E_Container *con));
EAPI void         e_configure_header_item_add(E_Configure *eco, char *icon, char *label);
    
#endif
#endif
