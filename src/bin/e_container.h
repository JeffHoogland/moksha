#ifdef E_TYPEDEFS

typedef enum _E_Container_Shape_Change
{
   E_CONTAINER_SHAPE_ADD,
   E_CONTAINER_SHAPE_DEL,
   E_CONTAINER_SHAPE_SHOW,
   E_CONTAINER_SHAPE_HIDE,
   E_CONTAINER_SHAPE_MOVE,
   E_CONTAINER_SHAPE_RESIZE,
   E_CONTAINER_SHAPE_RECTS
} E_Container_Shape_Change;

typedef struct _E_Container                E_Container;
typedef struct _E_Container_Shape          E_Container_Shape;
typedef struct _E_Container_Shape_Callback E_Container_Shape_Callback;
typedef struct _E_Event_Container_Resize   E_Event_Container_Resize;

#else
#ifndef E_CONTAINER_H
#define E_CONTAINER_H

struct _E_Container
{
   E_Object             e_obj_inherit;
   
   Ecore_X_Window       win;
   int                  x, y, w, h;
   char                 visible : 1;
   E_Manager           *manager;
   E_Gadman            *gadman;
   
   int                  num;
   char                *name;
   
   Ecore_Evas          *bg_ecore_evas;
   Evas                *bg_evas;
   Evas_Object         *bg_blank_object;
   Ecore_X_Window       bg_win;
   
   Evas_List           *shapes;
   Evas_List           *shape_change_cb;
   Evas_List           *zones;
   Evas_List           *clients;
};

struct _E_Container_Shape
{
   E_Object       e_obj_inherit;
   
   E_Container   *con;
   int            x, y, w, h;
   unsigned char  visible : 1;
   Evas_List     *shape;
};

struct _E_Container_Shape_Callback
{
   void (*func) (void *data, E_Container_Shape *es, E_Container_Shape_Change ch);
   void *data;
};

struct _E_Event_Container_Resize
{
   E_Container *container;
};

EAPI int          e_container_init(void);
EAPI int          e_container_shutdown(void);

EAPI E_Container *e_container_new(E_Manager *man);
EAPI void         e_container_show(E_Container *con);
EAPI void         e_container_hide(E_Container *con);
EAPI E_Container *e_container_current_get(E_Manager *man);
EAPI void         e_container_move(E_Container *con, int x, int y);
EAPI void         e_container_resize(E_Container *con, int w, int h);
EAPI void         e_container_move_resize(E_Container *con, int x, int y, int w, int h);
EAPI void         e_container_raise(E_Container *con);
EAPI void         e_container_lower(E_Container *con);

EAPI Evas_List   *e_container_clients_list_get(E_Container *con);

EAPI E_Container_Shape *e_container_shape_add(E_Container *con);
EAPI void               e_container_shape_show(E_Container_Shape *es);
EAPI void               e_container_shape_hide(E_Container_Shape *es);
EAPI void               e_container_shape_move(E_Container_Shape *es, int x, int y);
EAPI void               e_container_shape_resize(E_Container_Shape *es, int w, int h);
EAPI Evas_List         *e_container_shape_list_get(E_Container *con);
EAPI void               e_container_shape_geometry_get(E_Container_Shape *es, int *x, int *y, int *w, int *h);
EAPI E_Container       *e_container_shape_container_get(E_Container_Shape *es);
EAPI void               e_container_shape_change_callback_add(E_Container *con, void (*func) (void *data, E_Container_Shape *es, E_Container_Shape_Change ch), void *data);
EAPI void               e_container_shape_change_callback_del(E_Container *con, void (*func) (void *data, E_Container_Shape *es, E_Container_Shape_Change ch), void *data);
EAPI Evas_List         *e_container_shape_rects_get(E_Container_Shape *es);

extern EAPI int E_EVENT_CONTAINER_RESIZE;

#endif
#endif
