/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#ifdef E_TYPEDEFS

typedef struct _E_Drop_Handler E_Drop_Handler;
typedef struct _E_Drop_Event   E_Drop_Event;
#if 0
typedef struct _E_Drag_Event   E_Drag_Event;
#endif

#else
#ifndef E_DND_H
#define E_DND_H

struct _E_Drop_Handler
{
   void *data;
   void (*func)(void *data, const char *type, void *drop);
   char *type;
   int x, y, w, h;
};

struct _E_Drop_Event
{
   void *data;
   int x, y;
};

#if 0
struct _E_Drag_Event
{
   int drag;
   int x, y;
};
#endif

EAPI int  e_dnd_init(void);
EAPI int  e_dnd_shutdown(void);

EAPI int  e_dnd_active(void);

EAPI void e_drag_start(E_Zone *zone, const char *type, void *data,
		       const char *icon_path, const char *icon);
EAPI void e_drag_update(int x, int y);
EAPI void e_drag_end(int x, int y);
#if 0
EAPI void e_drag_callback_set(void *data, void (*func)(void *data, void *event));
#endif

EAPI E_Drop_Handler *e_drop_handler_add(void *data,
					void (*func)(void *data, const char *type, void *event_info),
				       	const char *type, int x, int y, int w, int h);
EAPI void e_drop_handler_del(E_Drop_Handler *handler);

#endif
#endif
