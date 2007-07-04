#ifdef E_TYPEDEFS

typedef struct _E_Configure E_Configure;

#else
#ifndef E_CONFIGURE_H
#define E_CONFIGURE_H

#define E_CONFIGURE_TYPE 0xE0b01014

struct _E_Configure 
{
   E_Object e_obj_inherit;
   
   E_Container *con;
   E_Win *win;
   Evas *evas;
   Evas_Object *edje;
   
   Evas_Object *o_list;
   Evas_Object *cat_list;
   Evas_Object *item_list;
   Evas_Object *close;
   
   Evas_List *cats;
};

EAPI void e_configure_registry_item_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon, E_Config_Dialog *(*func) (E_Container *con, const char *params));
EAPI void e_configure_registry_item_del(const char *path);
EAPI void e_configure_registry_category_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon);
EAPI void e_configure_registry_category_del(const char *path);
EAPI void e_configure_registry_call(const char *path, E_Container *con, const char *params);
EAPI int  e_configure_registry_exists(const char *path);

EAPI E_Configure *e_configure_show(E_Container *con);
EAPI void e_configure_init(void);
    
#endif
#endif
