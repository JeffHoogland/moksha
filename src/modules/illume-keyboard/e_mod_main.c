#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_kbd_int.h"

/* local function prototypes */
static void _il_kbd_stop(void);
static void _il_kbd_start(void);
static Eina_Bool _il_kbd_cb_exit(void *data, int type, void *event);

/* local variables */
static E_Kbd_Int *ki = NULL;
static Ecore_Exe *_kbd_exe = NULL;
static Ecore_Event_Handler *_kbd_exe_exit_handler = NULL;

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume Keyboard" };

EAPI void *
e_modapi_init(E_Module *m) 
{
   if (!il_kbd_config_init(m)) return NULL;
   _il_kbd_start();
   e_module_delayed_set(m, 1); 
   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m __UNUSED__) 
{
   _il_kbd_stop();
   il_kbd_config_shutdown();
   return 1;
}

EAPI int 
e_modapi_save(E_Module *m __UNUSED__) 
{
   return il_kbd_config_save();
}

EAPI void 
il_kbd_cfg_update(void) 
{
   _il_kbd_stop();
   _il_kbd_start();
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
             Efreet_Desktop *d;
             Eina_List *kbds, *l;

             kbds = efreet_util_desktop_category_list("Keyboard");
             if (kbds) 
               {
                  EINA_LIST_FOREACH(kbds, l, d) 
                    {
                       const char *dname;

                       dname = ecore_file_file_get(d->orig_path);
                       if (dname) 
                         {
                            if (!strcmp(dname, il_kbd_cfg->run_keyboard))
                              {
                                 desktop = d;
                                 efreet_desktop_ref(desktop);
                                 break;
                              }
                         }
                    }
                  EINA_LIST_FREE(kbds, d)
                    efreet_desktop_free(d);
               }
          }
        if (desktop) 
          {
             E_Zone *zone;

             zone = e_util_zone_current_get(e_manager_current_get());
             inst = e_exec(zone, desktop, NULL, NULL, "illume-keyboard");
             if (inst) 
               {
                  _kbd_exe = inst->exe;
                  _kbd_exe_exit_handler = 
                    ecore_event_handler_add(ECORE_EXE_EVENT_DEL, 
                                            _il_kbd_cb_exit, NULL);
               }
             efreet_desktop_free(desktop);
          }
     }
}

static Eina_Bool
_il_kbd_cb_exit(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   Ecore_Exe_Event_Del *ev;

   ev = event;
   if (ev->exe == _kbd_exe) _kbd_exe = NULL;
   return ECORE_CALLBACK_PASS_ON;
}
