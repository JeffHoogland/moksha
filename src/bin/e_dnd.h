/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#ifdef E_TYPEDEFS

typedef struct _E_Drag             E_Drag;
typedef struct _E_Drop_Handler     E_Drop_Handler;
typedef struct _E_Event_Dnd_Enter  E_Event_Dnd_Enter;
typedef struct _E_Event_Dnd_Move   E_Event_Dnd_Move;
typedef struct _E_Event_Dnd_Leave  E_Event_Dnd_Leave;
typedef struct _E_Event_Dnd_Drop   E_Event_Dnd_Drop;

#else
#ifndef E_DND_H
#define E_DND_H

#define E_DRAG_TYPE 0xE0b0100f

struct _E_Drag
{
   E_Object             e_obj_inherit;

   char *type;
   void *data;
   struct {
	void (*finished)(E_Drag *drag, int dropped);
   } cb;
   E_Container       *container;
   Ecore_Evas        *ecore_evas;
   Evas              *evas;
   Ecore_X_Window     evas_win;
   E_Container_Shape *shape;
   Evas_Object       *object;

   int x, y, w, h;
   int dx, dy;

   int shape_rects_num;
   Ecore_X_Rectangle *shape_rects;
   
   unsigned int   layer;
   unsigned char  visible : 1;
   unsigned char  need_shape_export : 1;
};

struct _E_Drop_Handler
{
   struct {
	void (*enter)(void *data, const char *type, void *event);
	void (*move)(void *data, const char *type, void *event);
	void (*leave)(void *data, const char *type, void *event);
	void (*drop)(void *data, const char *type, void *event);
	void *data;
   } cb;

   char         **types;
   unsigned int   num_types;

   int x, y, w, h;
   unsigned char active : 1;
   unsigned char entered : 1;
};

struct _E_Event_Dnd_Enter
{
   int x, y;
};

struct _E_Event_Dnd_Move
{
   int x, y;
};

struct _E_Event_Dnd_Leave
{
   int x, y;
};

struct _E_Event_Dnd_Drop
{
   void *data;
   int x, y;
};

EAPI int  e_dnd_init(void);
EAPI int  e_dnd_shutdown(void);

EAPI int  e_dnd_active(void);

/* x and y are the top left coords of the object that is to be dragged */
EAPI E_Drag* e_drag_new(E_Container *container, int x, int y, const char *type, void *data,
			void (*finished_cb)(E_Drag *drag, int dropped));
EAPI Evas   *e_drag_evas_get(E_Drag *drag);
EAPI void    e_drag_object_set(E_Drag *drag, Evas_Object *object);
EAPI void    e_drag_show(E_Drag *drag);
EAPI void    e_drag_hide(E_Drag *drag);
EAPI void    e_drag_move(E_Drag *drag, int x, int y);
EAPI void    e_drag_resize(E_Drag *drag, int w, int h);
EAPI void    e_drag_idler_before(void);

/* x and y are the coords where the mouse is when dragging starts */
EAPI void e_drag_start(E_Drag *drag, int x, int y);
EAPI void e_drag_update(int x, int y);
EAPI void e_drag_end(int x, int y);

EAPI E_Drop_Handler *e_drop_handler_add(void *data,
					void (*enter_cb)(void *data, const char *type, void *event),
					void (*move_cb)(void *data, const char *type, void *event),
					void (*leave_cb)(void *data, const char *type, void *event),
					void (*drop_cb)(void *data, const char *type, void *event),
				       	const char **types, unsigned int num_types,
					int x, int y, int w, int h);
EAPI void e_drop_handler_geometry_set(E_Drop_Handler *handler, int x, int y, int w, int h);
EAPI void e_drop_handler_del(E_Drop_Handler *handler);

#endif
#endif
