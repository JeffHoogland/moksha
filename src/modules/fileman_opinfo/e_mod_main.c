/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

typedef struct _Instance Instance;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_btn;
   Ecore_Event_Handler *fm_op_entry_add_handler;
   Ecore_Event_Handler *fm_op_entry_del_handler;
};

/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *client_class);
static const E_Gadcon_Client_Class _gadcon_class = {
   GADCON_CLIENT_CLASS_VERSION, "efm_info", {
        _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL
     }, E_GADCON_CLIENT_STYLE_PLAIN
};

/********************   PROTOS   *******************************************/
void _opinfo_button_cb(void *data, void *data2);
static void _opinfo_update_gadget(Instance *inst);

/********************   GLOBALS   ******************************************/
static E_Module *opinfo_module = NULL;

/********************   OP_REGISTRY   *************************************/
static int
_opinfo_op_registry_entry_cb(void *data, int type, void *event)
{
   _opinfo_update_gadget(data);
   return ECORE_CALLBACK_RENEW;
}

static void
_opinfo_update_gadget(Instance *inst)
{
   char buf[1024];
   int count;

   count = e_fm2_op_registry_count();
   if (count)
     snprintf(buf, sizeof(buf), _("%d operations"), count);
   else
     snprintf(buf, sizeof(buf), _("idle"));
   e_widget_button_label_set(inst->o_btn, buf);
   e_widget_disabled_set(inst->o_btn, count ? 0 : 1);
}

void
_opinfo_button_cb(void *data, void *data2)
{
   Ecore_X_Window win;
   Eina_Iterator *itr;
   E_Fm2_Op_Registry_Entry *ere;

   itr = e_fm2_op_registry_iterator_new();
   EINA_ITERATOR_FOREACH(itr, ere)
     {
	win = e_fm2_op_registry_entry_xwin_get(ere);
	e_util_dialog_show("TODO","What to show here ?");
	//ecore_x_window_show(win);
     }
   eina_iterator_free(itr);
}
/********************   GADCON   *******************************************/
static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;

   inst = E_NEW(Instance, 1);

   o = e_widget_button_add(gc->evas, "", NULL, _opinfo_button_cb, NULL, NULL);
   inst->o_btn = o;
   _opinfo_update_gadget(inst);

   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;
   inst->gcc = gcc;

   e_gadcon_client_util_menu_attach(gcc);

   inst->fm_op_entry_add_handler =
      ecore_event_handler_add(E_EVENT_FM_OP_REGISTRY_ADD,
			       _opinfo_op_registry_entry_cb, inst);
   inst->fm_op_entry_del_handler =
      ecore_event_handler_add(E_EVENT_FM_OP_REGISTRY_DEL,
			       _opinfo_op_registry_entry_cb, inst);

   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

   inst = gcc->data;

   if (inst->fm_op_entry_add_handler) 
     ecore_event_handler_del(inst->fm_op_entry_add_handler);
   if (inst->fm_op_entry_del_handler) 
     ecore_event_handler_del(inst->fm_op_entry_del_handler);

   evas_object_del(inst->o_btn);
   free(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   Instance *inst;
   Evas_Coord mw, mh;

   inst = gcc->data;

   mw = 120, mh = 40;
   evas_object_size_hint_min_set(inst->o_btn, mw, mh);
   e_gadcon_client_aspect_set(gcc, mw, mh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class)
{
   return _("EFM Operation Info");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas)
{
   Evas_Object *o;
   char buf[4096];

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-fileman_opinfo.edj",
	    e_module_dir_get(opinfo_module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class)
{
   return _gadcon_class.name;
}

/********************   E MODULE   ****************************************/
EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION,
     "EFM Info"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   opinfo_module = m;

   e_gadcon_provider_register(&_gadcon_class);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   opinfo_module = NULL;

   e_gadcon_provider_unregister(&_gadcon_class);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}

/***************************************************************************/
