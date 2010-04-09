#include "e.h"

struct _E_Config_Dialog_Data 
{
   E_Gadcon_Site site;
   E_Gadcon *gc;
   Ecore_Event_Handler *hdl;

   Evas_Object *o_list, *o_add, *o_del, *o_desc;
};

/* local function prototypes */
static void _create_dialog(E_Gadcon *gc, const char *title);
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _fill_gadget_list(E_Config_Dialog_Data *cfdata);
static void _cb_list_selected(void *data);
static const char *_get_comment(const char *name);
static void _cb_add(void *data, void *data2);
static void _cb_del(void *data, void *data2);
static int _cb_mod_update(void *data, int type, void *event);

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

/* local functions */
static void 
_create_dialog(E_Gadcon *gc, const char *title)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   E_Container *con;

   if (!(v = E_NEW(E_Config_Dialog_View, 1))) return;
   con = e_container_current_get(e_manager_current_get());

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;

   cfd = e_config_dialog_new(con, title, "E", "_gadcon_config_dialog", 
                             "preferences-desktop-shelf", 0, v, gc);
   gc->config_dialog = cfd;
   e_dialog_resizable_set(cfd->dia, EINA_TRUE);
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->gc = cfd->data;
   if (cfdata->gc->shelf)
     cfdata->site = E_GADCON_SITE_SHELF;
   else if (cfdata->gc->toolbar)
     cfdata->site = E_GADCON_SITE_EFM_TOOLBAR;
   else
     cfdata->site = E_GADCON_SITE_UNKNOWN;
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_Gadcon *gc;

   if (cfdata->hdl) ecore_event_handler_del(cfdata->hdl);
   E_FREE(cfdata);

   if (!(gc = cfd->data)) return;
   gc->config_dialog = NULL;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *ot;
   int mw;

   ot = e_widget_table_add(evas, 0);
   cfdata->o_list = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_ilist_multi_select_set(cfdata->o_list, EINA_TRUE);
   _fill_gadget_list(cfdata);
   e_widget_size_min_get(cfdata->o_list, &mw, NULL);
   if (mw < (200 * e_scale)) mw = (200 * e_scale);
   e_widget_size_min_set(cfdata->o_list, mw, (100 * e_scale));
   e_widget_table_object_append(ot, cfdata->o_list, 0, 0, 2, 1, 1, 1, 1, 1);

   cfdata->o_add = 
     e_widget_button_add(evas, _("Add Gadget"), NULL, _cb_add, cfdata, NULL);
   e_widget_disabled_set(cfdata->o_add, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->o_add, 0, 1, 1, 1, 1, 1, 1, 0);

   cfdata->o_del = 
     e_widget_button_add(evas, _("Remove Gadget"), NULL, _cb_del, cfdata, NULL);
   e_widget_disabled_set(cfdata->o_del, EINA_TRUE);
   e_widget_table_object_append(ot, cfdata->o_del, 1, 1, 1, 1, 1, 1, 1, 0);

   cfdata->o_desc = e_widget_textblock_add(evas);
   e_widget_textblock_markup_set(cfdata->o_desc, 
                                 _("Description: Unavailable"));
   e_widget_size_min_set(cfdata->o_desc, mw, (70 * e_scale));
   e_widget_table_object_append(ot, cfdata->o_desc, 0, 2, 2, 1, 1, 1, 1, 0);

   if (cfdata->hdl) ecore_event_handler_del(cfdata->hdl);
   cfdata->hdl = ecore_event_handler_add(E_EVENT_MODULE_UPDATE, 
                                         _cb_mod_update, cfdata);

   e_win_centered_set(cfd->dia->win, EINA_TRUE);
   return ot;
}

static void 
_fill_gadget_list(E_Config_Dialog_Data *cfdata) 
{
   Evas *evas;
   Eina_List *l;
   E_Gadcon_Site site;
   E_Gadcon_Client_Class *gcc;

   evas = evas_object_evas_get(cfdata->o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_list);
   e_widget_ilist_clear(cfdata->o_list);

   EINA_LIST_FOREACH(e_gadcon_provider_list(), l, gcc) 
     {
        Eina_List *cl;
        E_Config_Gadcon_Client *cgc;
        Evas_Object *end = NULL, *icon = NULL;
        const char *lbl;
        int found = 0;

        if (!gcc) continue;
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

        EINA_LIST_FOREACH(cfdata->gc->cf->clients, cl, cgc) 
          {
             if ((cgc->name) && (gcc->name) && 
                 (!strcmp(cgc->name, gcc->name))) 
               {
                  found = 1;
                  break;
               }
          }
        if (found) 
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
   e_widget_ilist_thaw(cfdata->o_list);
   edje_thaw();
   evas_event_thaw(evas);
}

static void 
_cb_list_selected(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *l;
   const E_Ilist_Item *it;
   const char *comment;
   unsigned int loaded = 0, unloaded = 0;

   if (!(cfdata = data)) return;
   EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->o_list), l, it) 
     {
        E_Config_Gadcon_Client *cgc;
        Eina_List *cl;
        const char *name;
        int found = 0;

        if ((!it->selected) || (it->header)) continue;
        name = e_widget_ilist_item_value_get(it);
        EINA_LIST_FOREACH(cfdata->gc->cf->clients, cl, cgc) 
          {
             if ((name) && (cgc->name) && (!strcmp(name, cgc->name))) 
               {
                  found = 1;
                  break;
               }
          }
        if (found) loaded++;
        else unloaded++;
     }
   e_widget_disabled_set(cfdata->o_add, !unloaded);
   e_widget_disabled_set(cfdata->o_del, !loaded);

   if (loaded + unloaded == 1) 
     {
        const char *name;

        name = e_widget_ilist_selected_value_get(cfdata->o_list);
        if (name) comment = _get_comment(name);
        else comment = _("Description: Unavailable");
     }
   else if (loaded + unloaded > 1)
     comment = _("More than one gadget selected.");
   else
     comment = _("No gadget selected.");

   e_widget_textblock_markup_set(cfdata->o_desc, comment);
}

static const char *
_get_comment(const char *name) 
{
   E_Module *mod;
   Efreet_Desktop *desk;
   const char *ret;
   char buff[PATH_MAX];

   if (!name) return _("Description: Unavailable");
   if (!(mod = e_module_find(name))) return _("Description: Unavailable");
   snprintf(buff, sizeof(buff), "%s/module.desktop", mod->dir);
   if (!ecore_file_exists(buff)) return _("Description: Unavailable");
   if (!(desk = efreet_desktop_new(buff))) 
     return _("Description: Unavailable");
   if ((desk->comment) && (desk->comment[0] != '\0'))
     ret = strdup(desk->comment);
   efreet_desktop_free(desk);
   return ret;
}

static void 
_cb_add(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   const E_Ilist_Item *it;
   Eina_List *l;
   int update = 0;

   if (!(cfdata = data)) return;
   EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->o_list), l, it) 
     {
        Evas_Object *end;
        const char *name;

        if ((!it->selected) || (it->header)) continue;
        if (!(name = e_widget_ilist_item_value_get(it))) continue;
        if (!e_gadcon_client_config_new(cfdata->gc, name)) continue;
        update = 1;
        if (!(end = e_widget_ilist_item_end_get(it))) continue;
        edje_object_signal_emit(end, "e,state,checked", "e");
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
   e_widget_textblock_markup_set(cfdata->o_desc, _("No gadget selected."));
}

static void 
_cb_del(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   const E_Ilist_Item *it;
   Eina_List *l;
   int update = 0;

   if (!(cfdata = data)) return;
   EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->o_list), l, it) 
     {
        Evas_Object *end;
        const char *name;
        Eina_List *cl;
        E_Config_Gadcon_Client *cgc;

        if ((!it->selected) || (it->header)) continue;
        if (!(name = e_widget_ilist_item_value_get(it))) continue;
        EINA_LIST_FOREACH(cfdata->gc->cf->clients, cl, cgc) 
          {
             if (strcmp(name, cgc->name)) continue;
             e_gadcon_client_config_del(cfdata->gc->cf, cgc);
             update = 1;
             if (end = e_widget_ilist_item_end_get(it))
               edje_object_signal_emit(end, "e,state,unchecked", "e");
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
   e_widget_textblock_markup_set(cfdata->o_desc, _("No gadget selected."));
}

static int 
_cb_mod_update(void *data, int type, void *event) 
{
   E_Config_Dialog_Data *cfdata;

   if (type != E_EVENT_MODULE_UPDATE) return 1;
   if (!(cfdata = data)) return 1;
   _fill_gadget_list(cfdata);
   return 1;
}
