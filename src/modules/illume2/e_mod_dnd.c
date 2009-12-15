#include "e.h"
#include "e_mod_main.h"
#include "e_mod_dnd.h"

/* local function prototypes */
static void _cb_dnd_enter(void *data, const char *type, void *event);
static void _cb_dnd_move(void *data, const char *type, void *event);
static void _cb_dnd_leave(void *data, const char *type, void *event);
static void _cb_dnd_drop(void *data, const char *type, void *event);
static int _cb_zone_move_resize(void *data, int type, void *event);

/* local variables */
static E_Drop_Handler *drop_handler = NULL;
static const char *drop_types[] = { "illume/indicator" };
static Ecore_Event_Handler *handler = NULL;

/* public functions */
EAPI int 
e_mod_dnd_init(void) 
{
   E_Zone *z;

   z = e_util_container_zone_number_get(0, 0);
   e_drop_xdnd_register_set(z->container->bg_win, 1);

   handler = 
     ecore_event_handler_add(E_EVENT_ZONE_MOVE_RESIZE, 
                             _cb_zone_move_resize, z);

   drop_handler = 
     e_drop_handler_add(E_OBJECT(z), z, 
                        _cb_dnd_enter, _cb_dnd_move, 
                        _cb_dnd_leave, _cb_dnd_drop, 
                        drop_types, 1, z->x, z->y, z->w, z->h);
   e_drop_handler_responsive_set(drop_handler);

   return 1;
}

EAPI int 
e_mod_dnd_shutdown(void) 
{
   if (handler) ecore_event_handler_del(handler);
   handler = NULL;
   if (drop_handler) e_drop_handler_del(drop_handler);
   drop_handler = NULL;
   return 1;
}

/* local functions */
static void 
_cb_dnd_enter(void *data, const char *type, void *event) 
{
   E_Event_Dnd_Enter *ev;

   ev = event;
   printf("Dnd Enter\n");
   if (strcmp(type, drop_types[0])) return;
   e_drop_handler_action_set(ev->action);
}

static void 
_cb_dnd_move(void *data, const char *type, void *event) 
{
   E_Event_Dnd_Move *ev;

   ev = event;
   printf("Dnd Move\n");
   if (strcmp(type, drop_types[0])) return;
   e_drop_handler_action_set(ev->action);
}

static void 
_cb_dnd_leave(void *data, const char *type, void *event) 
{
   E_Event_Dnd_Leave *ev;

   ev = event;
   printf("Dnd Leave\n");
   if (strcmp(type, drop_types[0])) return;
}

static void 
_cb_dnd_drop(void *data, const char *type, void *event) 
{
   E_Event_Dnd_Drop *ev;

   ev = event;
   printf("Dnd Drop\n");
   if (strcmp(type, drop_types[0])) return;
}

static int 
_cb_zone_move_resize(void *data, int type, void *event) 
{
   E_Event_Zone_Move_Resize *ev;
   E_Zone *z;

   if (!(z = data)) return 1;
   ev = event;
   if ((ev->zone != z)) return 1;
   e_drop_handler_geometry_set(drop_handler, z->x, z->y, z->w, z->h);
   return 1;
}
