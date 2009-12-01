#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"

/* local structures */
typedef struct _Instance Instance;
struct _Instance 
{
   E_Gadcon_Client *gcc;
   Evas_Object *o_btn;
};

/* local function prototypes */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *cc);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *cc, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *cc);
static void _btn_cb_click(void *data, void *data2);

/* local variables */
static Eina_List *instances = NULL;

static const E_Gadcon_Client_Class _gc_class = 
{
   GADCON_CLIENT_CLASS_VERSION, "illume2", 
     { _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, 
          e_gadcon_site_is_not_toolbar 
     }, E_GADCON_CLIENT_STYLE_PLAIN
};

/* local functions */
static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style) 
{
   Instance *inst;
   Evas_Object *icon;
   char buff[PATH_MAX];

   snprintf(buff, sizeof(buff), "%s/e-module-illume2.edj", il_cfg->mod_dir);

   inst = E_NEW(Instance, 1);
   inst->o_btn = e_widget_button_add(gc->evas, NULL, NULL, 
                                     _btn_cb_click, inst, NULL);
   icon = e_icon_add(evas_object_evas_get(inst->o_btn));
   e_icon_file_edje_set(icon, buff, "btn_icon");
   e_widget_button_icon_set(inst->o_btn, icon);

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->o_btn);
   inst->gcc->data = inst;

   instances = eina_list_append(instances, inst);
   return inst->gcc;
}

static void 
_gc_shutdown(E_Gadcon_Client *gcc) 
{
   Instance *inst;

   if (!(inst = gcc->data)) return;
   instances = eina_list_remove(instances, inst);
   if (inst->o_btn) evas_object_del(inst->o_btn);
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
   return _("Illume2");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *cc, Evas *evas) 
{
   Evas_Object *o;
   char buff[PATH_MAX];

   snprintf(buff, sizeof(buff), "%s/e-module-illume2.edj", il_cfg->mod_dir);
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
_btn_cb_click(void *data, void *data2) 
{
   if (il_cfg->policy.mode.dual)
     il_cfg->policy.mode.dual = 0;
   else 
     il_cfg->policy.mode.dual = 1;
   e_config_save_queue();
}

/* public functions */
int 
e_mod_gadcon_init(void) 
{
   e_gadcon_provider_register(&_gc_class);
   return 1;
}

int 
e_mod_gadcon_shutdown(void) 
{
   e_gadcon_provider_unregister(&_gc_class);
   return 1;
}
