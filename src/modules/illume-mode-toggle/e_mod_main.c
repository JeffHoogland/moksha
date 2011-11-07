#include "e.h"
#include "e_mod_main.h"

/* local structures */
typedef struct _Instance Instance;
struct _Instance 
{
   E_Gadcon_Client *gcc;
   Evas_Object *o_toggle;
   Ecore_Event_Handler *hdl;
};

/* local function prototypes */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *cc);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *cc, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *cc);
static void _cb_action_mode_single(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _cb_action_mode_dual_top(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _cb_action_mode_dual_left(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);
static Eina_Bool _cb_event_client_message(void *data, int type __UNUSED__, void *event);
static void _set_icon(Instance *inst);
static void _mode_set(Instance *inst, Ecore_X_Illume_Mode mode);

/* local variables */
static Eina_List *instances = NULL;
static const char *mod_dir = NULL;

static const E_Gadcon_Client_Class _gc_class = 
{
   GADCON_CLIENT_CLASS_VERSION, "illume-mode-toggle", 
     { _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, 
          e_gadcon_site_is_not_toolbar
     }, E_GADCON_CLIENT_STYLE_PLAIN
};

/* public functions */
EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume Mode Toggle" };

EAPI void *
e_modapi_init(E_Module *m) 
{
   mod_dir = eina_stringshare_add(m->dir);
   e_gadcon_provider_register(&_gc_class);
   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m __UNUSED__) 
{
   e_gadcon_provider_unregister(&_gc_class);
   if (mod_dir) eina_stringshare_del(mod_dir);
   mod_dir = NULL;
   return 1;
}

EAPI int 
e_modapi_save(E_Module *m __UNUSED__) 
{
   return 1;
}

/* local functions */
static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style) 
{
   Instance *inst;

   inst = E_NEW(Instance, 1);
   inst->o_toggle = edje_object_add(gc->evas);
   e_theme_edje_object_set(inst->o_toggle, 
                           "base/theme/modules/illume_mode_toggle",
                           "e/modules/illume_mode_toggle/main");

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->o_toggle);
   inst->gcc->data = inst;

   edje_object_signal_callback_add(inst->o_toggle, "e,action,mode,single", "",
                                   _cb_action_mode_single, inst);
   edje_object_signal_callback_add(inst->o_toggle, "e,action,mode,dual,top", "",
                                   _cb_action_mode_dual_top, inst);
   edje_object_signal_callback_add(inst->o_toggle, "e,action,mode,dual,left", "",
                                   _cb_action_mode_dual_left, inst);

   _set_icon(inst);

   inst->hdl = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,
                                       _cb_event_client_message, inst);

   instances = eina_list_append(instances, inst);
   return inst->gcc;
}

static void 
_gc_shutdown(E_Gadcon_Client *gcc) 
{
   Instance *inst;

   if (!(inst = gcc->data)) return;
   instances = eina_list_remove(instances, inst);
   if (inst->o_toggle) evas_object_del(inst->o_toggle);
   if (inst->hdl) ecore_event_handler_del(inst->hdl);
   inst->hdl = NULL;
   E_FREE(inst);
}

static void 
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__) 
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static char *
_gc_label(E_Gadcon_Client_Class *cc __UNUSED__) 
{
   return _("Illume-Mode-Toggle");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *cc __UNUSED__, Evas *evas) 
{
   Evas_Object *o;
   char buff[PATH_MAX];

   snprintf(buff, sizeof(buff), "%s/e-module-illume-mode-toggle.edj", mod_dir);
   o = edje_object_add(evas);
   edje_object_file_set(o, buff, "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *cc __UNUSED__) 
{
   static char buff[32];

   snprintf(buff, sizeof(buff), "%s.%d", _gc_class.name, 
            eina_list_count(instances));
   return buff;
}

static void
_cb_action_mode_single(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _mode_set(data, ECORE_X_ILLUME_MODE_SINGLE);
}

static void
_cb_action_mode_dual_top(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _mode_set(data, ECORE_X_ILLUME_MODE_DUAL_TOP);
}

static void
_cb_action_mode_dual_left(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _mode_set(data, ECORE_X_ILLUME_MODE_DUAL_LEFT);
}

static Eina_Bool
_cb_event_client_message(void *data, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Client_Message *ev;
   Instance *inst;

   ev = event;
   if (ev->message_type != ECORE_X_ATOM_E_ILLUME_MODE) 
     return ECORE_CALLBACK_PASS_ON;
   if (!(inst = data)) return ECORE_CALLBACK_PASS_ON;
   _set_icon(inst);
   return ECORE_CALLBACK_PASS_ON;
}

static void
_set_icon(Instance *inst)
{
   Ecore_X_Window xwin;
   Ecore_X_Illume_Mode mode;

   xwin = inst->gcc->gadcon->zone->black_win;
   mode = ecore_x_e_illume_mode_get(xwin);

   if (mode == ECORE_X_ILLUME_MODE_DUAL_TOP)
     edje_object_signal_emit(inst->o_toggle, "e,mode,dual,top", "e");
   else if (mode == ECORE_X_ILLUME_MODE_DUAL_LEFT)
     edje_object_signal_emit(inst->o_toggle, "e,mode,dual,left", "e");
   else
     edje_object_signal_emit(inst->o_toggle, "e,mode,single", "e");
}

static void
_mode_set(Instance *inst, Ecore_X_Illume_Mode mode)
{
   Ecore_X_Window xwin;

   if (!inst) return;

   xwin = inst->gcc->gadcon->zone->black_win;
   ecore_x_e_illume_mode_set(xwin, mode);
   ecore_x_e_illume_mode_send(xwin, mode);
}
