#include "e.h"

typedef struct _CFGadget CFGadget;
struct _CFGadget 
{
   const char *name, *id;
};

struct _E_Config_Dialog_Data 
{
   E_Gadcon *gc;
   E_Gadcon_Site site;

   Evas_Object *o_list, *o_add, *o_del;
   Ecore_Event_Handler *hdl;
   Ecore_Timer *load_timer;
   Eina_Hash *gadget_hash;
};

/* local function prototypes */
static void _create_dialog(E_Gadcon *gc, const char *title);
static void *_create_data(E_Config_Dialog *cfd);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Eina_Bool _free_gadgets(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata __UNUSED__);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static Eina_Bool _cb_load_timer(void *data);
static void _fill_list(E_Config_Dialog_Data *cfdata);
static void _cb_list_selected(void *data);
static void _cb_add(void *data, void *data2 __UNUSED__);
static void _cb_del(void *data, void *data2 __UNUSED__);
static CFGadget *_search_hash(E_Config_Dialog_Data *cfdata, const char *name);
static Eina_Bool _cb_mod_update(void *data, int type, void *event __UNUSED__);

EAPI void 
e_int_gadcon_config_shelf(E_Gadcon *gc) 
{
   _create_dialog(gc, _("Shelf Contents"));
}

EAPI void 
e_int_gadcon_config_toolbar(E_Gadcon *gc) 
{
   _create_dialog(gc, _("Toolbar Contents"));
}

/* local function prototypes */
static void 
_create_dialog(E_Gadcon *gc, const char *title) 
{
   E_Config_Dialog_View *v;
   E_Container *con;

   if (!(v = E_NEW(E_Config_Dialog_View, 1))) return;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;

   con = e_container_current_get(e_manager_current_get());
   gc->config_dialog = 
     e_config_dialog_new(con, title, "E", "_gadcon_config_dialog", 
                         "preferences-desktop-shelf", 0, v, gc);
   e_dialog_resizable_set(gc->config_dialog->dia, EINA_TRUE);
   e_win_centered_set(gc->config_dialog->dia->win, EINA_TRUE);
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->gadget_hash = eina_hash_string_superfast_new(NULL);
   cfdata->gc = cfd->data;
   if (cfdata->gc->shelf)
     cfdata->site = E_GADCON_SITE_SHELF;
   else if (cfdata->gc->toolbar)
     cfdata->site = E_GADCON_SITE_EFM_TOOLBAR;
   else
     cfdata->site = E_GADCON_SITE_UNKNOWN;
   _fill_data(cfdata);
   return cfdata;
}

static void 
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   Eina_List *l;
   E_Config_Gadcon_Client *cgc;

   EINA_LIST_FOREACH(cfdata->gc->cf->clients, l, cgc) 
     {
        CFGadget *gad;

        if (!cgc->id) continue;
        gad = E_NEW(CFGadget, 1);
        gad->name = eina_stringshare_add(cgc->name);
        gad->id = eina_stringshare_add(cgc->id);
        eina_hash_direct_add(cfdata->gadget_hash, gad->id, gad);
     }
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_Gadcon *gc;

   if (cfdata->hdl) ecore_event_handler_del(cfdata->hdl);
   cfdata->hdl = NULL;

   if (cfdata->load_timer) ecore_timer_del(cfdata->load_timer);
   cfdata->load_timer = NULL;

   if (cfdata->gadget_hash) 
     {
        eina_hash_foreach(cfdata->gadget_hash, _free_gadgets, NULL);
        eina_hash_free(cfdata->gadget_hash);
     }
   cfdata->gadget_hash = NULL;

   if (!(gc = cfd->data)) return;
   gc->config_dialog = NULL;
}

static Eina_Bool 
_free_gadgets(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata __UNUSED__) 
{
   CFGadget *gadget;

   if (!(gadget = data)) return EINA_FALSE;
   if (gadget->name) eina_stringshare_del(gadget->name);
   if (gadget->id) eina_stringshare_del(gadget->id);
   E_FREE(gadget);

   return EINA_TRUE;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *ot;
   int mw;

   ot = e_widget_table_add(evas, 0);

   cfdata->o_list = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_ilist_multi_select_set(cfdata->o_list, EINA_TRUE);
   e_widget_size_min_get(cfdata->o_list, &mw, NULL);
   if (mw < (200 * e_scale)) mw = (200 * e_scale);
   e_widget_size_min_set(cfdata->o_list, mw, (160 * e_scale));
   e_widget_table_object_append(ot, cfdata->o_list, 0, 0, 2, 1, 1, 1, 1, 1);

   cfdata->o_add = 
     e_widget_button_add(evas, _("Add Gadget"), NULL, _cb_add, cfdata, NULL);
   e_widget_disabled_set(cfdata->o_add, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->o_add, 0, 1, 1, 1, 1, 1, 1, 0);

   cfdata->o_del = 
     e_widget_button_add(evas, _("Remove Gadget"), NULL, _cb_del, cfdata, NULL);
   e_widget_disabled_set(cfdata->o_del, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->o_del, 1, 1, 1, 1, 1, 1, 1, 0);

   if (cfdata->hdl) ecore_event_handler_del(cfdata->hdl);
   cfdata->hdl = ecore_event_handler_add(E_EVENT_MODULE_UPDATE, 
                                         _cb_mod_update, cfdata);

   if (cfdata->load_timer) ecore_timer_del(cfdata->load_timer);
   cfdata->load_timer = ecore_timer_add(0.2, _cb_load_timer, cfdata);

   e_dialog_resizable_set(cfd->dia, EINA_TRUE);
   e_win_centered_set(cfd->dia->win, EINA_TRUE);

   return ot;
}

static Eina_Bool
_cb_load_timer(void *data)
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = data)) return ECORE_CALLBACK_RENEW;
   _fill_list(cfdata);
   cfdata->load_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void 
_fill_list(E_Config_Dialog_Data *cfdata) 
{
   Eina_List *l;
   E_Gadcon_Client_Class *gcc;
   Evas *evas;
   int mw;

   evas = evas_object_evas_get(cfdata->o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_list);
   e_widget_ilist_clear(cfdata->o_list);

   EINA_LIST_FOREACH(e_gadcon_provider_list(), l, gcc) 
     {
        Evas_Object *icon = NULL, *end;
        const char *lbl = NULL;

        if ((gcc->func.is_site) && (!gcc->func.is_site(cfdata->site)))
          continue;
        if (gcc->func.label) lbl = gcc->func.label(gcc);
        if (!lbl) lbl = gcc->name;
        if (gcc->func.icon) icon = gcc->func.icon(gcc, evas);

        end = edje_object_add(evas);
        if (!e_theme_edje_object_set(end, "base/theme/widgets", 
                                     "e/widgets/ilist/toggle_end")) 
          {
             evas_object_del(end);
             end = NULL;
          }

        if (_search_hash(cfdata, gcc->name))
          {
             if (end) edje_object_signal_emit(end, "e,state,checked", "e");
          }
        else 
          {
             if (end) edje_object_signal_emit(end, "e,state,unchecked", "e");
          }

        e_widget_ilist_append_full(cfdata->o_list, icon, end, lbl, 
                                   _cb_list_selected, cfdata, gcc->name);
     }

   e_widget_ilist_go(cfdata->o_list);
   e_widget_size_min_get(cfdata->o_list, &mw, NULL);
   if (mw < (200 * e_scale)) mw = (200 * e_scale);
   e_widget_size_min_set(cfdata->o_list, mw, (160 * e_scale));
   e_widget_ilist_thaw(cfdata->o_list);
   edje_thaw();
   evas_event_thaw(evas);
}

static void 
_cb_list_selected(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   const E_Ilist_Item *it;
   Eina_List *l;
   unsigned int loaded = 0;

   if (!(cfdata = data)) return;
   EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->o_list), l, it) 
     {
        const char *name;

        if ((!it->selected) || (it->header)) continue;
        if (!(name = e_widget_ilist_item_value_get(it))) continue;
        if (_search_hash(cfdata, name)) loaded++;
     }
   e_widget_disabled_set(cfdata->o_add, EINA_FALSE);
   e_widget_disabled_set(cfdata->o_del, !loaded);
}

static void 
_cb_add(void *data, void *data2 __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;
   const E_Ilist_Item *it;
   Eina_List *l;
   int update = 0;

   if (!(cfdata = data)) return;
   EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->o_list), l, it) 
     {
        E_Config_Gadcon_Client *cgc;
        CFGadget *gad;
        Evas_Object *end;
        const char *name;

        if ((!it->selected) || (it->header)) continue;
        if (!(name = e_widget_ilist_item_value_get(it))) continue;
        if (!(cgc = e_gadcon_client_config_new(cfdata->gc, name))) continue;
        if ((end = e_widget_ilist_item_end_get(it)))
          edje_object_signal_emit(end, "e,state,checked", "e");

        gad = E_NEW(CFGadget, 1);
        gad->name = eina_stringshare_add(cgc->name);
        gad->id = eina_stringshare_add(cgc->id);
        eina_hash_direct_add(cfdata->gadget_hash, gad->id, gad);

        update = 1;
     }
   if (update) 
     {
        e_gadcon_unpopulate(cfdata->gc);
        e_gadcon_populate(cfdata->gc);
        e_config_save_queue();
     }
   e_widget_ilist_unselect(cfdata->o_list);
   e_widget_disabled_set(cfdata->o_add, EINA_TRUE);
   e_widget_disabled_set(cfdata->o_del, EINA_TRUE);
}

static void 
_cb_del(void *data, void *data2 __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;
   const E_Ilist_Item *it;
   Eina_List *l;
   int update = 0;

   if (!(cfdata = data)) return;
   EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->o_list), l, it) 
     {
        E_Config_Gadcon_Client *cgc;
        Eina_List *cl;
        Evas_Object *end;
        const char *name;

        if ((!it->selected) || (it->header)) continue;
        if (!(name = e_widget_ilist_item_value_get(it))) continue;

        /* remove from actual gadget container */
        EINA_LIST_FOREACH(cfdata->gc->cf->clients, cl, cgc) 
          {
             CFGadget *gad;

             if (strcmp(name, cgc->name)) continue;

             /* remove from gadget hash if exists */
             if ((gad = eina_hash_find(cfdata->gadget_hash, cgc->id)))
               {
                  eina_hash_del(cfdata->gadget_hash, gad->id, gad);
                  if (gad->name) eina_stringshare_del(gad->name);
                  if (gad->id) eina_stringshare_del(gad->id);
                  E_FREE(gad);
               }

             /* set ilist end toggle if we don't have any more in the hash */
             if (!_search_hash(cfdata, name))
               {
                  if ((end = e_widget_ilist_item_end_get(it)))
                    edje_object_signal_emit(end, "e,state,unchecked", "e");
               }

             /* remove from gadget container */
             e_gadcon_client_config_del(cfdata->gc->cf, cgc);

             update = 1;
             break;
          }
     }
   if (update) 
     {
        e_gadcon_unpopulate(cfdata->gc);
        e_gadcon_populate(cfdata->gc);
        e_config_save_queue();
     }
   e_widget_ilist_unselect(cfdata->o_list);
   e_widget_disabled_set(cfdata->o_add, EINA_TRUE);
   e_widget_disabled_set(cfdata->o_del, EINA_TRUE);
}

static CFGadget * 
_search_hash(E_Config_Dialog_Data *cfdata, const char *name) 
{
   Eina_Iterator *it;
   CFGadget *gad, *ret = NULL;

   if (!name) return NULL;
   if (!(it = eina_hash_iterator_data_new(cfdata->gadget_hash))) 
     return NULL;
   EINA_ITERATOR_FOREACH(it, gad) 
     {
        if ((gad->name) && (!strcmp(gad->name, name)))
          {
             ret = gad;
             break;
          }
     }
   eina_iterator_free(it);
   return ret;
}

static Eina_Bool
_cb_mod_update(void *data, int type, void *event __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   if (type != E_EVENT_MODULE_UPDATE) return ECORE_CALLBACK_PASS_ON;
   if (!(cfdata = data)) return ECORE_CALLBACK_PASS_ON;
   _fill_list(cfdata);
   return ECORE_CALLBACK_PASS_ON;
}
