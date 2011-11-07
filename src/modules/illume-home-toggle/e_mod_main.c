#include "e.h"
#include "e_mod_main.h"

typedef struct _Instance Instance;
struct _Instance 
{
   E_Gadcon_Client *gcc;
   Evas_Object *o_toggle;
};

/* local function prototypes */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *cc);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *cc, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *cc);
static void _cb_action_home(void *data, Evas_Object *obj, const char *emission, const char *source);

/* local variables */
static Eina_List *instances = NULL;
static const char *mod_dir = NULL;

static const E_Gadcon_Client_Class _gc_class = 
{
   GADCON_CLIENT_CLASS_VERSION, "illume-home-toggle", 
     { _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, 
          e_gadcon_site_is_not_toolbar
     }, E_GADCON_CLIENT_STYLE_PLAIN
};

/* public functions */
EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume Home Toggle" };

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
                           "base/theme/modules/illume_home_toggle",
                           "e/modules/illume_home_toggle/main");

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->o_toggle);
   inst->gcc->data = inst;

   edje_object_signal_callback_add(inst->o_toggle, "e,action,home", "",
                                   _cb_action_home, inst);

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
   return _("Illume-Home-Toggle");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *cc __UNUSED__, Evas *evas) 
{
   Evas_Object *o;
   char buff[PATH_MAX];

   snprintf(buff, sizeof(buff), "%s/e-module-illume-home-toggle.edj", mod_dir);
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
_cb_action_home(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Instance *inst;
   E_Zone *zone;

   if (!(inst = data)) return;
   zone = inst->gcc->gadcon->zone;
   ecore_x_e_illume_focus_home_send(zone->black_win);
}
