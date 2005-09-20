#ifdef E_TYPEDEFS

typedef struct _E_Configure E_Configure;

#ifndef E_CONFIGURE_H
#define E_CONFIGURE_H

struct _E_Configure
{
   E_Container *con;
   E_Win       *win;
   Evas        *evas;
   Evas_Object *edje;
   Evas_Object *box;
   E_App       *apps;
   Evas_List   *icons;
   Evas_List   *app_ref;
};

E_Configure *e_configure_show(E_Container *con);

#endif
#endif
