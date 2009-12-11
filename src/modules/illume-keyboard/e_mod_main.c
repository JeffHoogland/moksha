#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_kbd_int.h"

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

static void _il_kbd_stop(void);
static void _il_kbd_start(void);
static int _il_kbd_cb_exit(void *data, int type, void *event);
static void _il_kbd_btn_cb_click(void *data, void *data2);
static int _il_kbd_cb_border_focus(void *data, int type, void *event);

/* local variables */
static E_Kbd_Int *ki = NULL;
static Ecore_Exe *_kbd_exe = NULL;
static Ecore_Event_Handler *_kbd_exe_exit_handler = NULL;
static Eina_List *instances = NULL;

static const E_Gadcon_Client_Class _gc_class = 
{
   GADCON_CLIENT_CLASS_VERSION, "illume-keyboard", 
     { _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, 
          e_gadcon_site_is_not_toolbar 
     }, E_GADCON_CLIENT_STYLE_PLAIN
};

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume Keyboard" };

EAPI void *
e_modapi_init(E_Module *m) 
{
   if (!il_kbd_config_init(m)) return NULL;
   _il_kbd_start();
   e_gadcon_provider_register(&_gc_class);
   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m) 
{
   e_gadcon_provider_unregister(&_gc_class);
   _il_kbd_stop();
   il_kbd_config_shutdown();
   return 1;
}

EAPI int 
e_modapi_save(E_Module *m) 
{
   return il_kbd_config_save();
}

EAPI void 
il_kbd_cfg_update(void) 
{
   _il_kbd_stop();
   _il_kbd_start();
}

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style) 
{
   Instance *inst;
   Evas_Object *icon;
   char buff[PATH_MAX];

   snprintf(buff, sizeof(buff), "%s/e-module-illume-keyboard.edj", 
            il_kbd_cfg->mod_dir);
   inst = E_NEW(Instance, 1);
   inst->o_btn = e_widget_button_add(gc->evas, NULL, NULL, 
                                     _il_kbd_btn_cb_click, inst, NULL);
   icon = e_icon_add(evas_object_evas_get(inst->o_btn));
   e_icon_file_edje_set(icon, buff, "icon");
   e_widget_button_icon_set(inst->o_btn, icon);

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->o_btn);
   inst->gcc->data = inst;

   inst->handlers = 
     eina_list_append(inst->handlers, 
                      ecore_event_handler_add(E_EVENT_BORDER_FOCUS_IN, 
                                              _il_kbd_cb_border_focus, inst));

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
   return _("Illume-Keyboard");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *cc, Evas *evas) 
{
   Evas_Object *o;
   char buff[PATH_MAX];

   snprintf(buff, sizeof(buff), "%s/e-module-illume-keyboard.edj", 
            il_kbd_cfg->mod_dir);
   o = edje_object_add(evas);
   edje_object_file_set(o, buff, "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *cc) 
{
   char buff[PATH_MAX];

   snprintf(buff, sizeof(buff), "%s.%d", _gc_class.name, 
            eina_list_count(instances));
   return strdup(buff);
}

static void 
_il_kbd_stop(void) 
{
   if (ki) e_kbd_int_free(ki);
   ki = NULL;
   if (_kbd_exe) ecore_exe_interrupt(_kbd_exe);
   _kbd_exe = NULL;
   if (_kbd_exe_exit_handler) ecore_event_handler_del(_kbd_exe_exit_handler);
   _kbd_exe_exit_handler = NULL;
}

static void 
_il_kbd_start(void) 
{
   if (il_kbd_cfg->use_internal) 
     {
        ki = e_kbd_int_new(il_kbd_cfg->mod_dir, 
                           il_kbd_cfg->mod_dir, il_kbd_cfg->mod_dir);
     }
   else if (il_kbd_cfg->run_keyboard) 
     {
        E_Exec_Instance *inst;
        Efreet_Desktop *desktop;

        desktop = efreet_util_desktop_file_id_find(il_kbd_cfg->run_keyboard);
        if (!desktop) 
          {
             Eina_List *kbds, *l;

             kbds = efreet_util_desktop_category_list("Keyboard");
             if (kbds) 
               {
                  EINA_LIST_FOREACH(kbds, l, desktop) 
                    {
                       const char *dname;

                       dname = ecore_file_file_get(desktop->orig_path);
                       if (dname) 
                         {
                            if (!strcmp(dname, il_kbd_cfg->run_keyboard))
                              break;
                         }
                    }
               }
          }
        if (desktop) 
          {
             E_Zone *zone;

             zone = e_util_container_zone_number_get(0, 0);
             inst = e_exec(zone, desktop, NULL, NULL, "illume-keyboard");
             if (inst) 
               {
                  _kbd_exe = inst->exe;
                  _kbd_exe_exit_handler = 
                    ecore_event_handler_add(ECORE_EXE_EVENT_DEL, 
                                            _il_kbd_cb_exit, NULL);
               }
          }
     }
}

static int 
_il_kbd_cb_exit(void *data, int type, void *event) 
{
   Ecore_Exe_Event_Del *ev;

   ev = event;
   if (ev->exe == _kbd_exe) _kbd_exe = NULL;
   return 1;
}

static void 
_il_kbd_btn_cb_click(void *data, void *data2) 
{
   Ecore_X_Virtual_Keyboard_State state;
   Instance *inst;
   E_Border *bd;
   Evas_Object *icon;
   char buff[PATH_MAX];

   if (!(inst = data)) return;
   if (!(bd = e_border_focused_get())) return;

   snprintf(buff, sizeof(buff), "%s/e-module-illume-keyboard.edj", 
            il_kbd_cfg->mod_dir);

   icon = e_icon_add(evas_object_evas_get(inst->o_btn));
   state = ecore_x_e_virtual_keyboard_state_get(bd->client.win);
   if ((state == ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF) ||
       (state == ECORE_X_VIRTUAL_KEYBOARD_STATE_UNKNOWN)) 
     {
        ecore_x_e_virtual_keyboard_state_set(bd->client.win, 
                                             ECORE_X_VIRTUAL_KEYBOARD_STATE_ON);
        e_icon_file_edje_set(icon, buff, "btn_icon");
     }
   else 
     {
        ecore_x_e_virtual_keyboard_state_set(bd->client.win, 
                                             ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF);
        e_icon_file_edje_set(icon, buff, "icon");
     }
   e_widget_button_icon_set(inst->o_btn, icon);
}

static int 
_il_kbd_cb_border_focus(void *data, int type, void *event) 
{
   Instance *inst;
   E_Event_Border_Focus_In *ev;
   E_Border *bd;
   Evas_Object *icon;
   Ecore_X_Virtual_Keyboard_State state;
   char buff[PATH_MAX];

   if (!(inst = data)) return 1;
   ev = event;
   if (ev->border->stolen) return 1;
   if (!(bd = ev->border)) return 1;

   snprintf(buff, sizeof(buff), "%s/e-module-illume-keyboard.edj", 
            il_kbd_cfg->mod_dir);

   icon = e_icon_add(evas_object_evas_get(inst->o_btn));
   state = ecore_x_e_virtual_keyboard_state_get(bd->client.win);
   if ((state == ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF) ||
       (state == ECORE_X_VIRTUAL_KEYBOARD_STATE_UNKNOWN)) 
     e_icon_file_edje_set(icon, buff, "icon");
   else 
     e_icon_file_edje_set(icon, buff, "btn_icon");
   e_widget_button_icon_set(inst->o_btn, icon);

   return 1;
}
