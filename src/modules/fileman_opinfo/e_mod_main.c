/**
 * @addtogroup Optional_Fileman
 * @{
 *
 * @defgroup Module_Fileman_Opinfo File Manager Operation Information
 *
 * Shows information about file manager operations such as copy, move
 * and delete.
 *
 * @}
 */

#include "e.h"

typedef struct _Instance
{
   char            *theme_file;
   E_Gadcon_Client *gcc;

   Evas_Object     *o_box, *o_status;

   Ecore_Event_Handler *fm_op_entry_add_handler, *fm_op_entry_del_handler;
} Instance;

/* gadcon requirements */

static E_Gadcon_Client *_gc_init    (E_Gadcon *gc, const char *name, const char *id, const char *style);
static void             _gc_shutdown(E_Gadcon_Client *gcc);
static void             _gc_orient  (E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char      *_gc_label   (const E_Gadcon_Client_Class *client_class);
static Evas_Object     *_gc_icon    (const E_Gadcon_Client_Class *client_class, Evas *evas);
static const char      *_gc_id_new  (const E_Gadcon_Client_Class *client_class);

static const E_Gadcon_Client_Class _gadcon_class = {
   GADCON_CLIENT_CLASS_VERSION, "efm_info",
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL,
      e_gadcon_site_is_desktop
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

/********************   PROTOS   *******************************************/

static Eina_Bool  _opinfo_op_registry_entry_add_cb     (void *data, int type, void *event);
static Eina_Bool  _opinfo_op_registry_entry_del_cb     (void *data, int type, void *event);
static void _opinfo_op_registry_update_all       (Instance *inst);
static void _opinfo_op_registry_listener         (void *data, const E_Fm2_Op_Registry_Entry *ere);
static void _opinfo_op_registry_free_data        (void *data);
static Eina_Bool  _opinfo_op_registry_free_data_delayed(void *data);
static void _opinfo_op_registry_abort_cb         (void *data, Evas_Object *obj, const char *emission, const char *source);
static void _opinfo_op_registry_window_jump_cb   (void *data, Evas_Object *obj, const char *emission, const char *source);
static void _opinfo_op_registry_update_status    (Instance *inst);

/********************   GLOBALS   ******************************************/

static E_Module *opinfo_module = NULL;

/********************   OP_REGISTRY   *************************************/

static void
_opinfo_op_registry_listener(void *data, const E_Fm2_Op_Registry_Entry *ere)
{
   Evas_Object *o = data;
   char *total, buf[4096];
   Edje_Message_Float msg;
   int mw, mh;

   if (!o || !ere) return;

   // Don't show if the operation keep less than 1 second
   if (!ere->needs_attention && (ere->start_time + 1.0 > ecore_loop_time_get()))
     return;
   
   // Update icon
   switch (ere->op)
   {
      case E_FM_OP_COPY:
        edje_object_signal_emit(o, "e,action,icon,copy", "e");
        break;

      case E_FM_OP_MOVE:
        edje_object_signal_emit(o, "e,action,icon,move", "e");
        break;

      case E_FM_OP_REMOVE:
        edje_object_signal_emit(o, "e,action,icon,delete", "e");
        break;

      case E_FM_OP_SECURE_REMOVE:
        edje_object_signal_emit(o, "e,action,icon,secure_delete", "e");
        break;

      default:
        edje_object_signal_emit(o, "e,action,icon,unknown", "e");
   }

   // Update information text
   switch (ere->status)
     {
      case E_FM2_OP_STATUS_ABORTED:
        switch (ere->op)
          {
           case E_FM_OP_COPY:
             snprintf(buf, sizeof(buf), _("Copying is aborted"));
             break;

           case E_FM_OP_MOVE:
             snprintf(buf, sizeof(buf), _("Moving is aborted"));
             break;

           case E_FM_OP_REMOVE:
             snprintf(buf, sizeof(buf), _("Deleting is aborted"));
             break;

           case E_FM_OP_SECURE_REMOVE:
             snprintf(buf, sizeof(buf), _("Secure deletion is aborted"));
             break;

           default:
             snprintf(buf, sizeof(buf), _("Unknown operation from slave is aborted"));
          }
        break;

      default:
        total = e_util_size_string_get(ere->total);
        switch (ere->op)
          {
           case E_FM_OP_COPY:
             if (ere->finished)
               snprintf(buf, sizeof(buf), _("Copy of %s done"), total);
             else
               snprintf(buf, sizeof(buf), _("Copying %s (eta: %s)"),
                             total, e_util_time_str_get(ere->eta));
             break;

           case E_FM_OP_MOVE:
             if (ere->finished)
               snprintf(buf, sizeof(buf), _("Move of %s done"), total);
             else
               snprintf(buf, sizeof(buf), _("Moving %s (eta: %s)"),
                             total, e_util_time_str_get(ere->eta));
             break;

           case E_FM_OP_REMOVE:
             if (ere->finished)
               snprintf(buf, sizeof(buf), _("Delete done"));
             else
               snprintf(buf, sizeof(buf), _("Deleting files..."));
             break;

           case E_FM_OP_SECURE_REMOVE:
             if (ere->finished)
               snprintf(buf, sizeof(buf), _("Secure delete done"));
             else
               snprintf(buf, sizeof(buf), _("Securely deleting files..."));
             break;

           default:
             snprintf(buf, sizeof(buf), _("Unknown operation from slave %d"), ere->id);
          }
        E_FREE(total);
     }
   edje_object_part_text_set(o, "e.text.info", buf);

   // Update spinner wheel
   if ((ere->finished) || (ere->status == E_FM2_OP_STATUS_ABORTED))
     {
        if (!evas_object_data_get(o, "stopped"))
          {
             evas_object_data_set(o, "stopped", o);
             edje_object_signal_emit(o, "e,state,busy,stop", "e");
          }
     }
   if (ere->percent > 0)
     {
        if (!evas_object_data_get(o, "started"))
          {
             evas_object_data_set(o, "started", o);
             edje_object_signal_emit(o, "e,state,busy,start", "e");
          }
     }

   // Show/hide the red attention led
   if (ere->needs_attention)
     {
        if (!evas_object_data_get(o, "attention_started"))
          {
             evas_object_data_set(o, "attention_started", o);
             edje_object_signal_emit(o, "e,state,attention,start", "e");
             if (evas_object_data_get(o, "attention_stopped"))
               evas_object_data_del(o, "attention_stopped");
          }
     }
   else
     {
        if (!evas_object_data_get(o, "attention_stopped"))
          {
             evas_object_data_set(o, "attention_stopped", o);
             edje_object_signal_emit(o, "e,state,attention,stop", "e");
             if (evas_object_data_get(o, "attention_started"))
               evas_object_data_del(o, "attention_started");
          }
     }

   // Update gauge
   edje_object_part_drag_size_set(o, "e.gauge.bar",
                                  ((double)(ere->percent)) / 100.0, 1.0);
   msg.val = ((double)(ere->percent)) / 100.0;
   edje_object_message_send(o, EDJE_MESSAGE_FLOAT, 1, &msg);

   // resize element to fit the box
   edje_object_size_min_calc(o, &mw, &mh);
   e_box_pack_options_set(o, 1, 0, 1, 0, 0.0, 0.0, mw, mh, 9999, mh);
   evas_object_show(o);
}

static void
_opinfo_op_registry_free_data(void *data)
{
   ecore_timer_add(5.0, _opinfo_op_registry_free_data_delayed, data);
}

static Eina_Bool
_opinfo_op_registry_free_data_delayed(void *data)
{
   Evas_Object *o = data;
   
   if (o)
     {
        e_box_unpack(o);
        evas_object_del(o);
     }
   
   return ECORE_CALLBACK_CANCEL;
}

static void 
_opinfo_op_registry_abort_cb(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   int id;
   
   id = (long)data;
   if (!id) return;
   
   e_fm2_operation_abort(id);
}

static void 
_opinfo_op_registry_window_jump_cb(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   int id = (long)data;
   E_Fm2_Op_Registry_Entry *ere;
   E_Win *win;
   
   if (!id) return;
   ere = e_fm2_op_registry_entry_get(id);
   if (!ere) return;

   // if attention dialog is present then raise it, otherwise raise the efm window
   win = (ere->needs_attention && ere->dialog) ? ere->dialog->win
                                               : e_win_evas_object_win_get(ere->e_fm);
   if (!win) return;
   
   if (win->border)
     {
        if (win->border->iconic)
           e_border_uniconify(win->border);
        if (win->border->shaded)
           e_border_unshade(win->border, win->border->shade.dir);
     }
   else
     e_win_show(win);
   e_win_raise(win);
   e_desk_show(win->border->desk);
   e_border_focus_set_with_pointer(win->border);
   
   if (ere->needs_attention && e_config->pointer_slide)
      e_border_pointer_warp_to_center(win->border);
}

static Eina_Bool
_opinfo_op_registry_entry_add_cb(void *data, __UNUSED__ int type, void *event)
{
   E_Fm2_Op_Registry_Entry *ere = event;
   Instance *inst = data;
   Evas_Object *o;
   
   if (!inst || !ere)
      return ECORE_CALLBACK_RENEW;

   _opinfo_op_registry_update_status(inst);

   if (!(ere->op == E_FM_OP_COPY || ere->op == E_FM_OP_MOVE || ere->op == E_FM_OP_REMOVE))
      return ECORE_CALLBACK_RENEW;
   
   o = edje_object_add(evas_object_evas_get(inst->o_box));
   e_theme_edje_object_set(o, "base/theme/fileman", "e/fileman/default/progress");
   edje_object_signal_callback_add(o, "e,fm,operation,abort", "",
                                   _opinfo_op_registry_abort_cb, (void*)(long)ere->id);
   edje_object_signal_callback_add(o, "e,fm,window,jump", "", 
                                   _opinfo_op_registry_window_jump_cb, (void*)(long)ere->id); 
   e_box_pack_end(inst->o_box, o);
   
   e_fm2_op_registry_entry_listener_add(ere, _opinfo_op_registry_listener,
                                        o, _opinfo_op_registry_free_data);
   
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_opinfo_op_registry_entry_del_cb(void *data, __UNUSED__ int type, __UNUSED__ void *event)
{
   Instance *inst = data;
   
   if (!inst)
      return ECORE_CALLBACK_RENEW;

   _opinfo_op_registry_update_status(inst);

   return ECORE_CALLBACK_RENEW;
}

static void
_opinfo_op_registry_update_all(Instance *inst)
{
   Eina_Iterator *itr;
   E_Fm2_Op_Registry_Entry *ere;

   itr = e_fm2_op_registry_iterator_new();
   EINA_ITERATOR_FOREACH(itr, ere)
      _opinfo_op_registry_entry_add_cb(inst, 0, ere);
   eina_iterator_free(itr);

   _opinfo_op_registry_update_status(inst);
}

static void 
_opinfo_op_registry_update_status(Instance *inst)
{
   int cnt;
   char buf[256];
   
   cnt = e_fm2_op_registry_count();
   if (cnt)
     {
        snprintf(buf, sizeof(buf), P_("Processing %d operation", "Processing %d operations", cnt), cnt);
        edje_object_part_text_set(inst->o_status, "e.text.info", buf);
     }
   else
     edje_object_part_text_set(inst->o_status, "e.text.info", _("Filemanager is idle"));
}

/********************   GADCON   *******************************************/

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   E_Gadcon_Client *gcc;
   Instance *inst;
   int mw, mh;
   int r;

   inst = E_NEW(Instance, 1);

   r = asprintf(&inst->theme_file, "%s/e-module-fileman_opinfo.edj",
                e_module_dir_get(opinfo_module));
   if (r < 0)
     {
        E_FREE(inst);
        return NULL;
     }

   // main object
   inst->o_box = e_box_add(gc->evas);
   e_box_homogenous_set(inst->o_box, 0);
   e_box_orientation_set(inst->o_box, 0);
   e_box_align_set(inst->o_box, 0, 0);

   // status line
   inst->o_status = edje_object_add(evas_object_evas_get(inst->o_box));
   if (!e_theme_edje_object_set(inst->o_status, "base/theme/modules/fileman_opinfo",
                                "modules/fileman_opinfo/status"))
      edje_object_file_set(inst->o_status, inst->theme_file, "modules/fileman_opinfo/status");
   e_box_pack_end(inst->o_box, inst->o_status);
   evas_object_show(inst->o_status);
   edje_object_size_min_get(inst->o_status, &mw, &mh);
   e_box_pack_options_set(inst->o_status, 1, 0, 1, 0, 0.0, 0.0, mw, mh, 9999, mh);

   _opinfo_op_registry_update_all(inst);

   gcc = e_gadcon_client_new(gc, name, id, style, inst->o_box);
   gcc->data = inst;
   inst->gcc = gcc;

   e_gadcon_client_util_menu_attach(gcc);

   inst->fm_op_entry_add_handler =
      ecore_event_handler_add(E_EVENT_FM_OP_REGISTRY_ADD,
                              _opinfo_op_registry_entry_add_cb, inst);
   inst->fm_op_entry_del_handler =
      ecore_event_handler_add(E_EVENT_FM_OP_REGISTRY_DEL,
                              _opinfo_op_registry_entry_del_cb, inst);

   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst = gcc->data;

   if (inst->fm_op_entry_add_handler)
     ecore_event_handler_del(inst->fm_op_entry_add_handler);
   if (inst->fm_op_entry_del_handler)
     ecore_event_handler_del(inst->fm_op_entry_del_handler);
   e_box_unpack(inst->o_status);
   evas_object_del(inst->o_status);
   evas_object_del(inst->o_box);
   free(inst->theme_file);
   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
   Instance *inst = gcc->data;
   Evas_Coord mw = 200, mh = 100;

   evas_object_size_hint_min_set(inst->o_box, mw, mh);
   e_gadcon_client_aspect_set(gcc, mw, mh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("EFM Operation Info");
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;
   char buf[PATH_MAX];

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-fileman_opinfo.edj",
	    e_module_dir_get(opinfo_module));
   edje_object_file_set(o, buf, "icon");
   
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class __UNUSED__)
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
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   opinfo_module = NULL;
   e_gadcon_provider_unregister(&_gadcon_class);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
