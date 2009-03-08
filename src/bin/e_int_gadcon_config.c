#include "e.h"

/* local function protos */
static void _e_int_gadcon_config(E_Gadcon *gc, const char *title);
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _cb_mod_update(void *data, int type, void *event);
static void _avail_list_cb_change(void *data, Evas_Object *obj);
static void _sel_list_cb_change(void *data, Evas_Object *obj);
static void _load_avail_gadgets(void *data);
static void _load_sel_gadgets(void *data);
static void _cb_add(void *data, void *data2);
static void _cb_del(void *data, void *data2);
static void _set_description(void *data, const char *name);
static int _gad_list_sort(void *data1, void *data2);

struct _E_Config_Dialog_Data 
{
   Evas_Object *o_avail, *o_sel;
   Evas_Object *o_add, *o_del;
   Evas_Object *o_desc;

   E_Gadcon *gc;
   Ecore_Event_Handler *hdl;
};

/* externals */
EAPI void 
e_int_gadcon_config_shelf(E_Gadcon *gc) 
{
   _e_int_gadcon_config(gc, _("Shelf Contents"));
}

EAPI void 
e_int_gadcon_config_toolbar(E_Gadcon *gc) 
{
   _e_int_gadcon_config(gc, _("Toolbar Contents"));
}

/* local functions */
static void 
_e_int_gadcon_config(E_Gadcon *gc, const char *title) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   E_Container *con;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return;

   con = e_container_current_get(e_manager_current_get());

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;

   cfd = e_config_dialog_new(con, title, "E", "_gadcon_config_dialog", 
                             "preferences-desktop-shelf", 0, v, gc);
   gc->config_dialog = cfd;
   e_dialog_resizable_set(cfd->dia, 1);
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->gc = cfd->data;
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_Gadcon *gc = NULL;

   if (cfdata->hdl) ecore_event_handler_del(cfdata->hdl);
   E_FREE(cfdata);

   if (!(gc = cfd->data)) return;
   gc->config_dialog = NULL;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o = NULL, *of = NULL;
   Evas_Object *ow = NULL;

   o = e_widget_table_add(evas, 0);
   of = e_widget_frametable_add(evas, _("Available Gadgets"), 0);
   ow = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_ilist_multi_select_set(ow, 1);
   e_widget_on_change_hook_set(ow, _avail_list_cb_change, cfdata);
   cfdata->o_avail = ow;
   _load_avail_gadgets(cfdata);
   e_widget_frametable_object_append(of, ow, 0, 0, 1, 1, 1, 1, 1, 1);
   ow = e_widget_button_add(evas, _("Add Gadget"), NULL, _cb_add, cfdata, NULL);
   e_widget_disabled_set(ow, 1);
   cfdata->o_add = ow;
   e_widget_frametable_object_append(of, ow, 0, 1, 1, 1, 1, 1, 1, 0);
   e_widget_table_object_append(o, of, 0, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_frametable_add(evas, _("Selected Gadgets"), 0);
   ow = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_ilist_multi_select_set(ow, 1);
   e_widget_on_change_hook_set(ow, _sel_list_cb_change, cfdata);
   cfdata->o_sel = ow;
   _load_sel_gadgets(cfdata);
   e_widget_frametable_object_append(of, ow, 0, 0, 1, 1, 1, 1, 1, 1);
   ow = e_widget_button_add(evas, _("Remove Gadget"), NULL, _cb_del, cfdata, NULL);
   e_widget_disabled_set(ow, 1);
   cfdata->o_del = ow;
   e_widget_frametable_object_append(of, ow, 0, 1, 1, 1, 1, 1, 1, 0);
   e_widget_table_object_append(o, of, 1, 0, 1, 1, 1, 1, 1, 1);

   ow = e_widget_textblock_add(evas);
   e_widget_min_size_set(ow, 200, 70);
   e_widget_textblock_markup_set(ow, _("Description: Unavailable"));
   cfdata->o_desc = ow;
   e_widget_table_object_append(o, ow, 0, 1, 2, 1, 1, 1, 1, 0);

   if (cfdata->hdl) ecore_event_handler_del(cfdata->hdl);
   cfdata->hdl = ecore_event_handler_add(E_EVENT_MODULE_UPDATE, 
                                         _cb_mod_update, cfdata);
   return o;
}

static int 
_cb_mod_update(void *data, int type, void *event) 
{
   E_Config_Dialog_Data *cfdata = NULL;

   if (type != E_EVENT_MODULE_UPDATE) return 1;
   if (!(cfdata = data)) return 1;
   _load_avail_gadgets(cfdata);
   _load_sel_gadgets(cfdata);
   return 1;
}

static void 
_avail_list_cb_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata = NULL;
   const char *name;
   int sel, count;

   if (!(cfdata = data)) return;
   e_widget_ilist_unselect(cfdata->o_sel);
   e_widget_disabled_set(cfdata->o_del, 1);
   e_widget_disabled_set(cfdata->o_add, 0);
   count = e_widget_ilist_selected_count_get(cfdata->o_avail);
   if ((count > 1) || (count == 0))
     e_widget_textblock_markup_set(cfdata->o_desc, _("Description: Unavailable"));
   else 
     {
        sel = e_widget_ilist_selected_get(cfdata->o_avail);
        name = (char *)e_widget_ilist_nth_data_get(cfdata->o_avail, sel);
        _set_description(cfdata, name);
     }
}

static void 
_sel_list_cb_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata = NULL;
   const char *name;
   int sel, count;

   if (!(cfdata = data)) return;
   e_widget_ilist_unselect(cfdata->o_avail);
   e_widget_disabled_set(cfdata->o_add, 1);
   e_widget_disabled_set(cfdata->o_del, 0);
   count = e_widget_ilist_selected_count_get(cfdata->o_sel);
   if ((count > 1) || (count == 0))
     e_widget_textblock_markup_set(cfdata->o_desc, _("Description: Unavailable"));
   else 
     {
        sel = e_widget_ilist_selected_get(cfdata->o_sel);
        name = (char *)e_widget_ilist_nth_data_get(cfdata->o_sel, sel);
        _set_description(cfdata, name);
     }
}

static void 
_load_avail_gadgets(void *data) 
{
   E_Config_Dialog_Data *cfdata = NULL;
   Eina_List *l = NULL;
   Evas *evas;
   int w;

   if (!(cfdata = data)) return;
   evas = evas_object_evas_get(cfdata->o_avail);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_avail);
   e_widget_ilist_clear(cfdata->o_avail);
//   l = e_gadcon_provider_list();
//   if (l) l = eina_list_sort(l, -1, _gad_list_sort);
   for (l = e_gadcon_provider_list(); l; l = l->next) 
     {
        E_Gadcon_Client_Class *cc;
        Evas_Object *icon = NULL;
        const char *lbl = NULL;

        if (!(cc = l->data)) continue;
        if (cc->func.label) lbl = cc->func.label(cc);
        if (!lbl) lbl = cc->name;
        if (cc->func.icon) icon = cc->func.icon(cc, evas);
        e_widget_ilist_append(cfdata->o_avail, icon, lbl, NULL, 
                              (void *)cc->name, NULL);
     }
   e_widget_ilist_go(cfdata->o_avail);
   e_widget_min_size_get(cfdata->o_avail, &w, NULL);
   if (w < 200) w = 200;
   e_widget_min_size_set(cfdata->o_avail, w, 250);
   e_widget_ilist_thaw(cfdata->o_avail);
   edje_thaw();
   evas_event_thaw(evas);
}

static void 
_load_sel_gadgets(void *data) 
{
   E_Config_Dialog_Data *cfdata = NULL;
   Eina_List *l = NULL, *l2 = NULL;
   Evas *evas;
   int w;

   if (!(cfdata = data)) return;
   evas = evas_object_evas_get(cfdata->o_sel);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_sel);
   e_widget_ilist_clear(cfdata->o_sel);
   for (l = cfdata->gc->cf->clients; l; l = l->next) 
     {
        E_Config_Gadcon_Client *cgc;

        if (!(cgc = l->data)) continue;
        for (l2 = e_gadcon_provider_list(); l2; l2 = l2->next) 
          {
             E_Gadcon_Client_Class *gcc;
             Evas_Object *icon = NULL;
             const char *lbl = NULL;

             if (!(gcc = l2->data)) continue;
             if ((cgc->name) && (gcc->name) && 
                 (!strcmp(cgc->name, gcc->name))) 
               {
                  if (gcc->func.label) lbl = gcc->func.label(gcc);
                  if (!lbl) lbl = gcc->name;
                  if (gcc->func.icon) icon = gcc->func.icon(gcc, evas);
                  e_widget_ilist_append(cfdata->o_sel, icon, lbl, NULL, 
                                        (void *)gcc->name, NULL);
               }
          }
     }
   e_widget_ilist_go(cfdata->o_sel);
   e_widget_min_size_get(cfdata->o_sel, &w, NULL);
   if (w < 200) w = 200;
   e_widget_min_size_set(cfdata->o_sel, w, 250);
   e_widget_ilist_thaw(cfdata->o_sel);
   edje_thaw();
   evas_event_thaw(evas);
}

static void 
_cb_add(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata = NULL;
   Eina_List *l = NULL;
   int i = 0, update = 0;

   if (!(cfdata = data)) return;
   for (i = 0, l = e_widget_ilist_items_get(cfdata->o_avail); l; l = l->next, i++) 
     {
        E_Ilist_Item *item = NULL;
        const char *name = NULL;

        if (!(item = l->data)) continue;
        if (!item->selected) continue;
        name = (char *)e_widget_ilist_nth_data_get(cfdata->o_avail, i);
        if (!name) continue;
        if (!e_gadcon_client_config_new(cfdata->gc, name)) continue;
        update = 1;
     }
   if (update) 
     {
        e_gadcon_unpopulate(cfdata->gc);
        e_gadcon_populate(cfdata->gc);
        e_config_save_queue();

        _load_sel_gadgets(cfdata);
        e_widget_ilist_selected_set(cfdata->o_sel, i);
     }
}

static void 
_cb_del(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata = NULL;
   Eina_List *l = NULL, *g = NULL;
   int i = 0, update = 0;

   if (!(cfdata = data)) return;
   for (i = 0, l = e_widget_ilist_items_get(cfdata->o_sel); l; l = l->next, i++) 
     {
        E_Ilist_Item *item = NULL;
        const char *name = NULL;

        if (!(item = l->data)) continue;
        if (!item->selected) continue;
        name = (char *)e_widget_ilist_nth_data_get(cfdata->o_sel, i);
        if (!name) continue;
        for (g = cfdata->gc->cf->clients; g; g = g->next)
          {
             E_Config_Gadcon_Client *cgc;

             if (!(cgc = g->data)) continue;
             if (strcmp(name, cgc->name)) continue;
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

        _load_sel_gadgets(cfdata);

        /* we just default to selecting first one here as the user may have had
         * more than one selected */
        e_widget_ilist_selected_set(cfdata->o_sel, 0);
     }
}

static void 
_set_description(void *data, const char *name) 
{
   E_Config_Dialog_Data *cfdata = NULL;
   E_Module *mod = NULL;
   Efreet_Desktop *desk = NULL;
   char buf[4096];

   if (!(cfdata = data)) return;
   if (!name) return;
   if (!(mod = e_module_find(name))) return;

   snprintf(buf, sizeof(buf), "%s/module.desktop", e_module_dir_get(mod));
   if (!ecore_file_exists(buf)) return;
   if (!(desk = efreet_desktop_get(buf))) return;
   if (desk->comment) 
     e_widget_textblock_markup_set(cfdata->o_desc, desk->comment);
   efreet_desktop_free(desk);
}

static int 
_gad_list_sort(void *data1, void *data2) 
{
   E_Gadcon_Client_Class *cc, *cc2;
   const char *lbl1 = NULL, *lbl2 = NULL;

   if (!(cc = data1)) return 1;
   if (!(cc2 = data2)) return -1;

   if (cc->func.label) lbl1 = cc->func.label(cc);
   if (!lbl1) lbl1 = cc->name;
   
   if (cc2->func.label) lbl2 = cc2->func.label(cc2);
   if (!lbl2) lbl2 = cc2->name;

   return (strcmp(lbl1, lbl2));
}
