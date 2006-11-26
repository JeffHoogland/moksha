#ifdef E_TYPEDEFS

typedef struct _E_Configure E_Configure;
typedef struct _E_Configure_CB E_Configure_CB;

typedef struct _E_Configure_Category E_Configure_Category;
typedef struct _E_Configure_Item E_Configure_Item;

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

struct _E_Configure_CB 
{
   E_Configure *eco;
   E_Config_Dialog *(*func) (E_Container *con);
};

struct _E_Configure_Category 
{
   E_Configure *eco;
   const char *label;
   
   Evas_List *items;
};

struct _E_Configure_Item 
{
   E_Configure_CB *cb;
   
   const char *label;
   const char *icon;
};

EAPI E_Configure *e_configure_show(E_Container *con);

#endif
#endif
