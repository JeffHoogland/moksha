#include "e.h"
#include "e_mod_main.h"

/* local structures */
typedef struct _Instance Instance;
struct _Instance 
{
   E_Gadcon_Client *gcc;
   Evas_Object *o_btn;
   Eina_List *handlers;
};

/* local function prototypes */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *cc);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *cc, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *cc);
static void _cb_btn_click(void *data, void *data2);
static int _cb_border_focus_in(void *data, int type __UNUSED__, void *event);
static int _cb_border_remove(void *data, int type __UNUSED__, void *event);
static int _cb_border_property(void *data, int type __UNUSED__, void *event);
static void _set_btn_icon(Evas_Object *obj, Ecore_X_Virtual_Keyboard_State state);

/* local variables */
static Eina_List *instances = NULL;
static const char *mod_dir = NULL;

static const E_Gadcon_Client_Class _gc_class = 
{
   GADCON_CLIENT_CLASS_VERSION, "illume-kbd-toggle", 
     { _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, 
          e_gadcon_site_is_not_toolbar
     }, E_GADCON_CLIENT_STYLE_PLAIN
};

/* public functions */
EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume Keyboard Toggle" };

EAPI void *
e_modapi_init(E_Module *m) 
{
   mod_dir = eina_stringshare_add(m->dir);
   e_gadcon_provider_register(&_gc_class);
   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m) 
{
   e_gadcon_provider_unregister(&_gc_class);
   if (mod_dir) eina_stringshare_del(mod_dir);
   mod_dir = NULL;
   return 1;
}

EAPI int 
e_modapi_save(E_Module *m) 
{
   return 1;
}

/* local function prototypes */
static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style) 
{
   Instance *inst;

   inst = E_NEW(Instance, 1);

   inst->o_btn = e_widget_button_add(gc->evas, NULL, NULL, 
                                     _cb_btn_click, inst, NULL);
   _set_btn_icon(inst->o_btn, ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF);

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->o_btn);
   inst->gcc->data = inst;

   inst->handlers = 
     eina_list_append(inst->handlers, 
                      ecore_event_handler_add(E_EVENT_BORDER_FOCUS_IN, 
                                              _cb_border_focus_in, inst));
   inst->handlers = 
     eina_list_append(inst->handlers, 
                      ecore_event_handler_add(E_EVENT_BORDER_REMOVE, 
                                              _cb_border_remove, inst));
   inst->handlers = 
     eina_list_append(inst->handlers, 
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, 
                                              _cb_border_property, inst));

   instances = eina_list_append(instances, inst);
   return inst->gcc;
}

static void 
_gc_shutdown(E_Gadcon_Client *gcc) 
{
   Instance *inst;
   Ecore_Event_Handler *handler;

   if (!(inst = gcc->data)) return;
   instances = eina_list_remove(instances, inst);
   if (inst->o_btn) evas_object_del(inst->o_btn);
   EINA_LIST_FREE(inst->handlers, handler)
     ecore_event_handler_del(handler);
   E_FREE(inst);
}

static void 
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient) 
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static char *
_gc_label(E_Gadcon_Client_Class *cc) 
{
   return _("Illume-Keyboard-Toggle");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *cc, Evas *evas) 
{
   Evas_Object *o;
   char buff[PATH_MAX];

   snprintf(buff, sizeof(buff), "%s/e-module-illume-kbd-toggle.edj", mod_dir);
   o = edje_object_add(evas);
   edje_object_file_set(o, buff, "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *cc) 
{
   static char buff[32];

   snprintf(buff, sizeof(buff), "%s.%d", _gc_class.name, 
            eina_list_count(instances));
   return buff;
}

static void 
_cb_btn_click(void *data, void *data2) 
{
   Instance *inst;
   E_Border *bd;

   if (!(inst = data)) return;
   if (!(bd = e_border_focused_get())) return;
   if (bd->zone != inst->gcc->gadcon->zone) return;

   if (bd->client.vkbd.state <= ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF) 
     ecore_x_e_virtual_keyboard_state_set(bd->client.win, 
                                          ECORE_X_VIRTUAL_KEYBOARD_STATE_ON);
   else 
     ecore_x_e_virtual_keyboard_state_set(bd->client.win, 
                                          ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF);
}

static int 
_cb_border_focus_in(void *data, int type __UNUSED__, void *event) 
{
   Instance *inst;
   E_Event_Border_Focus_In *ev;
   E_Border *bd;

   if (!(inst = data)) return 1;
   ev = event;
   if (ev->border->stolen) return 1;
   if (!(bd = ev->border)) return 1;
   if (bd->zone != inst->gcc->gadcon->zone) return 1;
   _set_btn_icon(inst->o_btn, bd->client.vkbd.state);
   return 1;
}

static int 
_cb_border_remove(void *data, int type __UNUSED__, void *event) 
{
   Instance *inst;

   if (!(inst = data)) return 1;
   _set_btn_icon(inst->o_btn, ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF);
   return 1;
}

static int 
_cb_border_property(void *data, int type __UNUSED__, void *event) 
{
   Instance *inst;
   Ecore_X_Event_Window_Property *ev;
   E_Border *bd;

   ev = event;
   if (ev->atom != ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE) return 1;
   if (!(bd = e_border_find_by_client_window(ev->win))) return 1;
   if (!bd->focused) return 1;
   if (!(inst = data)) return 1;
   if (bd->zone != inst->gcc->gadcon->zone) return 1;
   _set_btn_icon(inst->o_btn, bd->client.vkbd.state);
   return 1;
}

static void 
_set_btn_icon(Evas_Object *obj, Ecore_X_Virtual_Keyboard_State state) 
{
   Evas_Object *icon;
   char buff[PATH_MAX];

   snprintf(buff, sizeof(buff), "%s/e-module-illume-kbd-toggle.edj", mod_dir);

   icon = e_icon_add(evas_object_evas_get(obj));
   if (state <= ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF)
     e_icon_file_edje_set(icon, buff, "icon");
   else 
     e_icon_file_edje_set(icon, buff, "btn_icon");
   e_widget_button_icon_set(obj, icon);
}
