#include "e.h"
#include "e_mod_main.h"
#include "e_mod_dnd.h"

/* local function prototypes */
static void _cb_dnd_enter(void *data, const char *type, void *event);
static void _cb_dnd_move(void *data, const char *type, void *event);
static void _cb_dnd_leave(void *data, const char *type, void *event);
static void _cb_dnd_drop(void *data, const char *type, void *event);

/* local variables */
static E_Drop_Handler *drop_handler = NULL;
static const char *drop_types[] = { "illume/indicator" };

/* public functions */
EAPI int 
e_mod_dnd_init(void) 
{
   E_Container *con;

   con = e_container_current_get(e_manager_current_get());
   drop_handler = 
     e_drop_handler_add(E_OBJECT(con), NULL, 
                        _cb_dnd_enter, _cb_dnd_move, 
                        _cb_dnd_leave, _cb_dnd_drop, 
                        drop_types, 1, con->x, con->y, con->w, con->h);
   if (!drop_handler) return 0;

   return 1;
}

EAPI int 
e_mod_dnd_shutdown(void) 
{
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
   if (strcmp(type, drop_types[0])) return;
}

static void 
_cb_dnd_move(void *data, const char *type, void *event) 
{
   E_Event_Dnd_Move *ev;

   ev = event;
   if (strcmp(type, drop_types[0])) return;
}

static void 
_cb_dnd_leave(void *data, const char *type, void *event) 
{
   E_Event_Dnd_Leave *ev;

   ev = event;
   if (strcmp(type, drop_types[0])) return;
}

static void 
_cb_dnd_drop(void *data, const char *type, void *event) 
{
   E_Event_Dnd_Drop *ev;

   ev = event;
   if (strcmp(type, drop_types[0])) return;
}
