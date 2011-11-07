#include "e.h"
#include "e_mod_main.h"

typedef struct _Instance Instance;
struct _Instance 
{
   E_Gadcon_Client *gcc;
   Evas_Object *obj;
   Ecore_Poller *poller;
   int on;
};

/* local function prototypes */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *cc);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *cc, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *cc);
static Eina_Bool _cb_poll(void *data);
static int _get_interface_class(int iclass);

/* local variables */
static Eina_List *instances = NULL;
static const char *_bt_mod_dir = NULL;

static const E_Gadcon_Client_Class _gc_class = 
{
   GADCON_CLIENT_CLASS_VERSION, "illume-bluetooth", 
     { _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, 
          NULL, e_gadcon_site_is_not_toolbar }, 
   E_GADCON_CLIENT_STYLE_PLAIN
};

/* local function prototypes */
static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style) 
{
   Instance *inst;
   char buff[PATH_MAX];

   inst = E_NEW(Instance, 1);

   inst->obj = edje_object_add(gc->evas);
   if (!e_theme_edje_object_set(inst->obj, "base/theme/modules/illume-bluetooth", 
                                "modules/illume-bluetooth/main")) 
     {
        snprintf(buff, sizeof(buff), "%s/e-module-illume-bluetooth.edj", 
                 _bt_mod_dir);
        edje_object_file_set(inst->obj, buff, "modules/illume-bluetooth/main");
     }
   evas_object_show(inst->obj);

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->obj);
   inst->gcc->data = inst;

   inst->on = -1;
   inst->poller = ecore_poller_add(ECORE_POLLER_CORE, 16, _cb_poll, inst);
   return inst->gcc;
}

static void 
_gc_shutdown(E_Gadcon_Client *gcc) 
{
   Instance *inst;

   if (!(inst = gcc->data)) return;
   instances = eina_list_remove(instances, inst);
   if (inst->poller) ecore_poller_del(inst->poller);
   if (inst->obj) evas_object_del(inst->obj);
   E_FREE(inst);
}

static void 
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__) 
{
   Instance *inst;
   int mw, mh, xw, xh;

   inst = gcc->data;
   edje_object_size_min_get(inst->obj, &mw, &mh);
   edje_object_size_max_get(inst->obj, &xw, &xh);
   if ((mw < 1) || (mh < 1))
     edje_object_size_min_calc(inst->obj, &mw, &mh);
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   if ((xw > 0) && (xh > 0))
     e_gadcon_client_aspect_set(gcc, xw, xh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static char *
_gc_label(E_Gadcon_Client_Class *cc __UNUSED__) 
{
   return _("Illume Bluetooth");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *cc __UNUSED__, Evas *evas) 
{
   Evas_Object *o;
   char buff[PATH_MAX];

   snprintf(buff, sizeof(buff), "%s/e-module-illume-bluetooth.edj", 
            _bt_mod_dir);
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

static Eina_Bool
_cb_poll(void *data) 
{
   Instance *inst;
   int pon;

   inst = data;
   pon = inst->on;
   inst->on = _get_interface_class(0xe0);
   if (inst->on != pon) 
     {
        if (inst->on)
          edje_object_signal_emit(inst->obj, "e,state,active", "e");
        else
          edje_object_signal_emit(inst->obj, "e,state,passive", "e");
     }
   return ECORE_CALLBACK_RENEW;
}

static int 
_get_interface_class(int iclass) 
{
   Eina_List *devs;
   char *name;

   devs = ecore_file_ls("/sys/bus/usb/devices");
   EINA_LIST_FREE(devs, name)
     {
        char buf[PATH_MAX];
        FILE *f;

        snprintf(buf, sizeof(buf), "%s/%s/%s",
                 "/sys/bus/usb/devices", name, "bInterfaceClass");
        f = fopen(buf, "r");
        if (f)
          {
             if (fgets(buf, sizeof(buf), f))
               {
                  int id = -1;

                  sscanf(buf, "%x", &id);
                  if (iclass == id)
                    {
                       EINA_LIST_FREE(devs, name)
                         free(name);
                       fclose(f);
                       return 1;
                    }
               }
             fclose(f);
          }
        free(name);
     }
   return 0;
}

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume Bluetooth" };

EAPI void *
e_modapi_init(E_Module *m) 
{
   _bt_mod_dir = eina_stringshare_add(m->dir);
   e_gadcon_provider_register(&_gc_class);
   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m __UNUSED__) 
{
   e_gadcon_provider_unregister(&_gc_class);
   if (_bt_mod_dir) eina_stringshare_del(_bt_mod_dir);
   _bt_mod_dir = NULL;
   return 1;
}

EAPI int 
e_modapi_save(E_Module *m __UNUSED__) 
{
   return 1;
}
