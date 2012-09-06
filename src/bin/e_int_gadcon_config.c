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

   const char *sel;

   struct
   {
      Evas_Object *o_list;
   } basic, advanced;
   Evas_Object *class_list;
   Evas_Object *o_add, *o_del;
   Ecore_Event_Handler *gcc_add;
   Ecore_Event_Handler *gcc_del;
   Ecore_Event_Handler *cc_del;
   Ecore_Event_Handler *cc_add;
   Ecore_Timer *load_timer;
   Eina_Hash *gadget_hash;
};

/* local function prototypes */
static void *_create_data(E_Config_Dialog *cfd);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Eina_Bool _free_gadgets(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata __UNUSED__);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static Eina_Bool _cb_load_timer(void *data);
static void _fill_list(E_Config_Dialog_Data *cfdata);
static void _cb_list_selected(void *data);
static void _cb_add(void *data, void *data2 __UNUSED__);
static void _cb_del(void *data, void *data2 __UNUSED__);
static CFGadget *_search_hash(E_Config_Dialog_Data *cfdata, const char *name);
static Eina_Bool _cb_cc_add(E_Config_Dialog_Data *cfdata, int type, E_Event_Gadcon_Client_Class_Add *ev);
static Eina_Bool _cb_cc_del(E_Config_Dialog_Data *cfdata, int type, E_Event_Gadcon_Client_Class_Add *ev);
static Eina_Bool _cb_gcc_add(E_Config_Dialog_Data *cfdata, int type, E_Event_Gadcon_Client_Add *ev);
static Eina_Bool _cb_gcc_del(E_Config_Dialog_Data *cfdata, int type, E_Event_Gadcon_Client_Add *ev);

/* local function prototypes */
static void
_create_dialog(E_Gadcon *gc, const char *title, E_Gadcon_Site site)
{
   E_Config_Dialog_View *v;
   E_Container *con;

   if (gc->config_dialog)
     {
        e_win_raise(gc->config_dialog->dia->win);
        e_border_focus_set(gc->config_dialog->dia->win->border, 1, 1);
        return;
     }
   if (!(v = E_NEW(E_Config_Dialog_View, 1))) return;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->advanced.create_widgets = _advanced_create;

   con = e_container_current_get(e_manager_current_get());
   gc->config_dialog =
     e_config_dialog_new(con, title, "E", "_gadcon_config_dialog",
                         "preferences-desktop-shelf", 0, v, gc);
   if (site) gc->config_dialog->cfdata->site = site;
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

   cfdata->gcc_add = ecore_event_handler_add(E_EVENT_GADCON_CLIENT_ADD,
                                         (Ecore_Event_Handler_Cb)_cb_gcc_add, cfdata);
   cfdata->gcc_del = ecore_event_handler_add(E_EVENT_GADCON_CLIENT_DEL,
                                         (Ecore_Event_Handler_Cb)_cb_gcc_del, cfdata);

   cfdata->cc_add = ecore_event_handler_add(E_EVENT_GADCON_CLIENT_CLASS_ADD,
                                         (Ecore_Event_Handler_Cb)_cb_cc_add, cfdata);
   cfdata->cc_del = ecore_event_handler_add(E_EVENT_GADCON_CLIENT_CLASS_DEL,
                                         (Ecore_Event_Handler_Cb)_cb_cc_del, cfdata);
   return cfdata;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   Eina_List *l;
   E_Config_Gadcon_Client *cf_gcc;

   EINA_LIST_FOREACH(cfdata->gc->cf->clients, l, cf_gcc)
     {
        CFGadget *gad;

        if (!cf_gcc->id) continue;
        gad = E_NEW(CFGadget, 1);
        gad->name = eina_stringshare_add(cf_gcc->name);
        gad->id = eina_stringshare_add(cf_gcc->id);
        eina_hash_direct_add(cfdata->gadget_hash, gad->id, gad);
     }
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->gcc_add) ecore_event_handler_del(cfdata->gcc_add);
   if (cfdata->gcc_del) ecore_event_handler_del(cfdata->gcc_del);
   if (cfdata->cc_add) ecore_event_handler_del(cfdata->cc_add);
   if (cfdata->cc_del) ecore_event_handler_del(cfdata->cc_del);

   if (cfdata->load_timer) ecore_timer_del(cfdata->load_timer);

   if (cfdata->gadget_hash)
     {
        eina_hash_foreach(cfdata->gadget_hash, _free_gadgets, NULL);
        eina_hash_free(cfdata->gadget_hash);
     }

   cfdata->gc->config_dialog = NULL;
   free(cfdata);
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

static void
_cb_list_selected(void *data)
{
   E_Config_Dialog_Data *cfdata;
   const E_Ilist_Item *it;
   Eina_List *l;
   unsigned int loaded = 0;

   if (!(cfdata = data)) return;
   EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->basic.o_list), l, it)
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
_list_item_del_advanced(E_Config_Dialog_Data *cfdata, E_Gadcon_Client *gcc)
{
   E_Ilist_Item *ili;
   Eina_List *l;
   int x = 0;

   EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->advanced.o_list), l, ili)
     {
        if (e_widget_ilist_item_value_get(ili) == gcc->cf->id)
          {
             e_widget_ilist_remove_num(cfdata->advanced.o_list, x);
             break;
          }
        x++;
     }
}

static void
_list_item_del(E_Config_Dialog_Data *cfdata __UNUSED__, Evas_Object *list, const E_Gadcon_Client_Class *cc)
{
   E_Ilist_Item *ili;
   Eina_List *l;
   int x = 0;

   EINA_LIST_FOREACH(e_widget_ilist_items_get(list), l, ili)
     {
        const char *name;

        name = e_widget_ilist_item_value_get(ili);
        if (!strcmp(name, cc->name))
          {
             e_widget_ilist_remove_num(list, x);
             break;
          }
        x++;
     }
}

static void
_list_item_add_advanced(E_Config_Dialog_Data *cfdata, E_Gadcon_Client *gcc, E_Config_Gadcon_Client *cf_gcc)
{
   Evas_Object *icon = NULL, *end;
   Evas *evas;

   if (gcc && (gcc->gadcon != cfdata->gc)) return;
   if (!gcc)
     {
        Eina_List *l;
        EINA_LIST_FOREACH(cfdata->gc->clients, l, gcc)
          if (gcc->cf == cf_gcc) break;
        if (gcc && (gcc->cf != cf_gcc)) gcc = NULL;
     }
   else if (!cf_gcc)
     cf_gcc = gcc->cf;
   evas = evas_object_evas_get(cfdata->advanced.o_list);
   if (gcc && gcc->client_class->func.icon) icon = gcc->client_class->func.icon(gcc->client_class, evas);

   end = edje_object_add(evas);
   if (!e_theme_edje_object_set(end, "base/theme/widgets",
                                "e/widgets/ilist/toggle_end"))
     {
        evas_object_del(end);
        end = NULL;
     }

   if (gcc)
     edje_object_signal_emit(end, "e,state,checked", "e");
   else
     edje_object_signal_emit(end, "e,state,unchecked", "e");

   e_widget_ilist_append_full(cfdata->advanced.o_list, icon, end, cf_gcc->id,
                              NULL, cfdata, cf_gcc->id);
}

static void
_list_item_class_add(E_Config_Dialog_Data *cfdata, const E_Gadcon_Client_Class *cc)
{
   Evas_Object *icon = NULL;
   const char *lbl = NULL;
   Evas *evas;

   evas = evas_object_evas_get(cfdata->class_list);
   if ((cc->func.is_site) && (!cc->func.is_site(cfdata->site)))
     return;
   if (cc->func.label) lbl = cc->func.label(cc);
   if (!lbl) lbl = cc->name;
   if (cc->func.icon) icon = cc->func.icon(cc, evas);

   e_widget_ilist_append_full(cfdata->class_list, icon, NULL, lbl,
                              NULL, cfdata, cc->name);
}

static void
_list_item_add(E_Config_Dialog_Data *cfdata, const E_Gadcon_Client_Class *cc)
{
   Evas_Object *icon = NULL, *end;
   const char *lbl = NULL;
   Evas *evas;

   evas = evas_object_evas_get(cfdata->basic.o_list);
   if ((cc->func.is_site) && (!cc->func.is_site(cfdata->site)))
     return;
   if (cc->func.label) lbl = cc->func.label(cc);
   if (!lbl) lbl = cc->name;
   if (cc->func.icon) icon = cc->func.icon(cc, evas);

   end = edje_object_add(evas);
   if (!e_theme_edje_object_set(end, "base/theme/widgets",
                                "e/widgets/ilist/toggle_end"))
     {
        evas_object_del(end);
        end = NULL;
     }

   if (_search_hash(cfdata, cc->name))
     {
        if (end) edje_object_signal_emit(end, "e,state,checked", "e");
     }
   else
     {
        if (end) edje_object_signal_emit(end, "e,state,unchecked", "e");
     }

   e_widget_ilist_append_full(cfdata->basic.o_list, icon, end, lbl,
                              _cb_list_selected, cfdata, cc->name);
}

static void
_fill_list_advanced(E_Config_Dialog_Data *cfdata)
{
   Eina_List *l;
   E_Config_Gadcon_Client *cf_gcc;
   const E_Gadcon_Client_Class *cc;
   Evas *evas;
   int mw;

   evas = evas_object_evas_get(cfdata->advanced.o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->advanced.o_list);
   e_widget_ilist_clear(cfdata->advanced.o_list);

   EINA_LIST_FOREACH(cfdata->gc->cf->clients, l, cf_gcc)
     _list_item_add_advanced(cfdata, NULL, cf_gcc);

   e_widget_ilist_go(cfdata->advanced.o_list);
   e_widget_size_min_get(cfdata->advanced.o_list, &mw, NULL);
   if (mw < (200 * e_scale)) mw = (200 * e_scale);
   e_widget_size_min_set(cfdata->advanced.o_list, mw, (160 * e_scale));
   e_widget_ilist_thaw(cfdata->advanced.o_list);
   edje_thaw();
   evas_event_thaw(evas);
   e_widget_ilist_selected_set(cfdata->advanced.o_list, 0);
   e_widget_disabled_set(cfdata->o_del, 0);
///////////////
   evas = evas_object_evas_get(cfdata->class_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->class_list);
   e_widget_ilist_clear(cfdata->class_list);

   EINA_LIST_FOREACH(e_gadcon_provider_list(), l, cc)
     _list_item_class_add(cfdata, cc);

   e_widget_ilist_go(cfdata->class_list);
   e_widget_size_min_get(cfdata->class_list, &mw, NULL);
   if (mw < (200 * e_scale)) mw = (200 * e_scale);
   e_widget_size_min_set(cfdata->class_list, mw, (160 * e_scale));
   e_widget_ilist_thaw(cfdata->class_list);
   edje_thaw();
   evas_event_thaw(evas);
   e_widget_ilist_selected_set(cfdata->class_list, 0);
   e_widget_disabled_set(cfdata->o_add, 0);
}

static void
_fill_list(E_Config_Dialog_Data *cfdata)
{
   Eina_List *l;
   E_Gadcon_Client_Class *cc;
   Evas *evas;
   int mw;

   evas = evas_object_evas_get(cfdata->basic.o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->basic.o_list);
   e_widget_ilist_clear(cfdata->basic.o_list);

   EINA_LIST_FOREACH(e_gadcon_provider_list(), l, cc)
     _list_item_add(cfdata, cc);

   e_widget_ilist_go(cfdata->basic.o_list);
   e_widget_size_min_get(cfdata->basic.o_list, &mw, NULL);
   if (mw < (200 * e_scale)) mw = (200 * e_scale);
   e_widget_size_min_set(cfdata->basic.o_list, mw, (160 * e_scale));
   e_widget_ilist_thaw(cfdata->basic.o_list);
   edje_thaw();
   evas_event_thaw(evas);
}

static void
_cb_add_advanced(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Gadcon_Client *cf_gcc;
   CFGadget *gad;

   if (!(cfdata = data)) return;
   cf_gcc = e_gadcon_client_config_new(cfdata->gc, e_widget_ilist_selected_value_get(cfdata->class_list));

   gad = E_NEW(CFGadget, 1);
   gad->name = eina_stringshare_add(cf_gcc->name);
   gad->id = eina_stringshare_add(cf_gcc->id);
   eina_hash_direct_add(cfdata->gadget_hash, gad->id, gad);

   e_gadcon_unpopulate(cfdata->gc);
   e_gadcon_populate(cfdata->gc);
   e_config_save_queue();
   e_widget_ilist_selected_set(cfdata->class_list, 0);
}

static void
_cb_add(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   const E_Ilist_Item *it;
   Eina_List *l;
   int update = 0;

   if (!(cfdata = data)) return;
   EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->basic.o_list), l, it)
     {
        E_Config_Gadcon_Client *cf_gcc;
        CFGadget *gad;
        Evas_Object *end;
        const char *name;

        if ((!it->selected) || (it->header)) continue;
        if (!(name = e_widget_ilist_item_value_get(it))) continue;
        if (!(cf_gcc = e_gadcon_client_config_new(cfdata->gc, name))) continue;
        if ((end = e_widget_ilist_item_end_get(it)))
          edje_object_signal_emit(end, "e,state,checked", "e");

        gad = E_NEW(CFGadget, 1);
        gad->name = eina_stringshare_add(cf_gcc->name);
        gad->id = eina_stringshare_add(cf_gcc->id);
        eina_hash_direct_add(cfdata->gadget_hash, gad->id, gad);

        update = 1;
     }
   if (update)
     {
        e_gadcon_unpopulate(cfdata->gc);
        if (!e_gadcon_populate(cfdata->gc))
          _cb_del(cfdata, NULL);
        e_config_save_queue();
     }
   e_widget_ilist_unselect(cfdata->basic.o_list);
   e_widget_disabled_set(cfdata->o_add, EINA_TRUE);
   e_widget_disabled_set(cfdata->o_del, EINA_TRUE);
}

static void
_cb_del_advanced(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   E_Config_Gadcon_Client *cf_gcc;
   Eina_List *l;
   int x = 0;
   
   EINA_LIST_FOREACH(cfdata->gc->cf->clients, l, cf_gcc)
     {
        if (cf_gcc->id == cfdata->sel)
          {
             CFGadget *gad;
             if ((gad = eina_hash_find(cfdata->gadget_hash, cf_gcc->id)))
               {
                  eina_hash_del(cfdata->gadget_hash, gad->id, gad);
                  if (gad->name) eina_stringshare_del(gad->name);
                  if (gad->id) eina_stringshare_del(gad->id);
                  E_FREE(gad);
               }
             e_gadcon_client_config_del(cfdata->gc->cf, cf_gcc);
             e_gadcon_unpopulate(cfdata->gc);
             e_gadcon_populate(cfdata->gc);
             e_config_save_queue();
             e_widget_ilist_remove_num(cfdata->advanced.o_list, x);
             e_widget_ilist_selected_set(cfdata->advanced.o_list, 0);
             break;
          }
        x++;
     }
}

static void
_cb_del(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   const E_Ilist_Item *it;
   Eina_List *l;
   int update = 0;

   if (!(cfdata = data)) return;
   EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->basic.o_list), l, it)
     {
        E_Config_Gadcon_Client *cf_gcc;
        Eina_List *cl;
        Evas_Object *end;
        const char *name;

        if ((!it->selected) || (it->header)) continue;
        name = e_widget_ilist_item_value_get(it);

        /* remove from actual gadget container */
        EINA_LIST_FOREACH(cfdata->gc->cf->clients, cl, cf_gcc)
          {
             CFGadget *gad;

             if (name != cf_gcc->name) continue;

             /* remove from gadget hash if exists */
             if ((gad = eina_hash_find(cfdata->gadget_hash, cf_gcc->id)))
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
             e_gadcon_client_config_del(cfdata->gc->cf, cf_gcc);

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
   e_widget_ilist_unselect(cfdata->basic.o_list);
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
_cb_gcc_del(E_Config_Dialog_Data *cfdata, int type __UNUSED__, E_Event_Gadcon_Client_Add *ev)
{
   if (cfdata->advanced.o_list)
     _list_item_del_advanced(cfdata, ev->gcc);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_gcc_add(E_Config_Dialog_Data *cfdata, int type __UNUSED__, E_Event_Gadcon_Client_Add *ev)
{
   if (cfdata->advanced.o_list)
     _list_item_add_advanced(cfdata, ev->gcc, ev->gcc->cf);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_cc_del(E_Config_Dialog_Data *cfdata, int type __UNUSED__, E_Event_Gadcon_Client_Class_Add *ev)
{
   if (cfdata->basic.o_list)
     _list_item_del(cfdata, cfdata->basic.o_list, ev->cc);
   else
     _list_item_del(cfdata, cfdata->class_list, ev->cc);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_cc_add(E_Config_Dialog_Data *cfdata, int type __UNUSED__, E_Event_Gadcon_Client_Class_Add *ev)
{
   if (cfdata->basic.o_list)
     _list_item_add(cfdata, ev->cc);
   else
     _list_item_class_add(cfdata, ev->cc);
   return ECORE_CALLBACK_PASS_ON;
}

static Evas_Object *
_advanced_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ot, *otb;
   int mw;

   cfdata->basic.o_list = NULL;
   otb = e_widget_toolbook_add(evas, 48 * e_scale, 48 * e_scale);
   ////////////////////////////////////////////////////////////
   ot = e_widget_list_add(evas, 0, 0);

   cfdata->advanced.o_list = e_widget_ilist_add(evas, 24, 24, &cfdata->sel);
   e_widget_ilist_multi_select_set(cfdata->advanced.o_list, EINA_TRUE);
   e_widget_size_min_get(cfdata->advanced.o_list, &mw, NULL);
   if (mw < (200 * e_scale)) mw = (200 * e_scale);
   e_widget_size_min_set(cfdata->advanced.o_list, mw, (160 * e_scale));
   e_widget_list_object_append(ot, cfdata->advanced.o_list, 1, 1, 0.5);

   cfdata->o_del =
     e_widget_button_add(evas, _("Remove Gadget"), NULL, _cb_del_advanced, cfdata, NULL);
   e_widget_disabled_set(cfdata->o_del, EINA_TRUE);
   e_widget_list_object_append(ot, cfdata->o_del, 1, 0, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("Gadgets"), ot, 0, 0, 0, 0, 0.5, 0.0);
   ////////////////////////////////////////////////////////////
   ot = e_widget_list_add(evas, 0, 0);

   cfdata->class_list = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_size_min_get(cfdata->class_list, &mw, NULL);
   if (mw < (200 * e_scale)) mw = (200 * e_scale);
   e_widget_size_min_set(cfdata->class_list, mw, (160 * e_scale));
   e_widget_list_object_append(ot, cfdata->class_list, 1, 1, 0.5);

   cfdata->o_add =
     e_widget_button_add(evas, _("Add Gadget"), NULL, _cb_add_advanced, cfdata, NULL);
   e_widget_disabled_set(cfdata->o_add, EINA_TRUE);
   e_widget_list_object_append(ot, cfdata->o_add, 1, 0, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("Classes"), ot, 0, 0, 0, 0, 0.5, 0.0);
   ////////////////////////////////////////////////////////////
   e_widget_toolbook_page_show(otb, 0);
   if (cfdata->load_timer) ecore_timer_del(cfdata->load_timer);
   cfdata->load_timer = ecore_timer_add(0.01, _cb_load_timer, cfdata);

   e_dialog_resizable_set(cfd->dia, EINA_TRUE);
   e_win_centered_set(cfd->dia->win, EINA_TRUE);

   return otb;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ot;
   int mw;

   cfdata->advanced.o_list = cfdata->class_list = NULL;
   ot = e_widget_table_add(evas, 0);

   cfdata->basic.o_list = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_ilist_multi_select_set(cfdata->basic.o_list, EINA_TRUE);
   e_widget_size_min_get(cfdata->basic.o_list, &mw, NULL);
   if (mw < (200 * e_scale)) mw = (200 * e_scale);
   e_widget_size_min_set(cfdata->basic.o_list, mw, (160 * e_scale));
   e_widget_table_object_append(ot, cfdata->basic.o_list, 0, 0, 2, 1, 1, 1, 1, 1);

   cfdata->o_add =
     e_widget_button_add(evas, _("Add Gadget"), NULL, _cb_add, cfdata, NULL);
   e_widget_disabled_set(cfdata->o_add, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->o_add, 0, 1, 1, 1, 1, 1, 1, 0);

   cfdata->o_del =
     e_widget_button_add(evas, _("Remove Gadget"), NULL, _cb_del, cfdata, NULL);
   e_widget_disabled_set(cfdata->o_del, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->o_del, 1, 1, 1, 1, 1, 1, 1, 0);

   if (cfdata->load_timer) ecore_timer_del(cfdata->load_timer);
   cfdata->load_timer = ecore_timer_add(0.01, _cb_load_timer, cfdata);

   e_dialog_resizable_set(cfd->dia, EINA_TRUE);
   e_win_centered_set(cfd->dia->win, EINA_TRUE);

   return ot;
}

static Eina_Bool
_cb_load_timer(void *data)
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = data)) return ECORE_CALLBACK_RENEW;
   if (cfdata->basic.o_list)
     _fill_list(cfdata);
   else
     _fill_list_advanced(cfdata);
   cfdata->load_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

EAPI void
e_int_gadcon_config_shelf(E_Gadcon *gc)
{
   _create_dialog(gc, _("Shelf Contents"), 0);
}

EAPI void
e_int_gadcon_config_toolbar(E_Gadcon *gc)
{
   _create_dialog(gc, _("Toolbar Contents"), 0);
}

EAPI void
e_int_gadcon_config_hook(E_Gadcon *gc, const char *name, E_Gadcon_Site site)
{
   _create_dialog(gc, name, site);
}
