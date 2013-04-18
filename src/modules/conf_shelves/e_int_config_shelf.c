#include "e.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void         _ilist_fill(E_Config_Dialog_Data *cfdata);
static void         _ilist_empty(E_Config_Dialog_Data *cfdata);
static void         _ilist_cb_selected(void *data);
static void         _cb_add(void *data, void *data2);
static void         _cb_delete(void *data, void *data2);
static void         _cb_rename(void *data, void *data2);
static void         _cb_dialog_yes(void *data);
static void         _cb_dialog_destroy(void *data);
static void         _cb_config(void *data, void *data2);
static void         _cb_contents(void *data, void *data2);
static void         _ilist_refresh(E_Shelf *es);
static void         _new_shelf_cb_close(void *data);
static void         _ilist_item_new(E_Config_Dialog_Data *cfdata, Eina_Bool append, E_Shelf *es);

struct _E_Config_Dialog_Data
{
   Evas_Object         *o_list;
   Evas_Object         *o_add;
   Evas_Object         *o_delete;
   Evas_Object         *o_rename;
   Evas_Object         *o_config;
   Evas_Object         *o_contents;

   const char          *cur_shelf;
   Eina_List           *handlers;
   Eina_List           *shelves;
   E_Config_Dialog     *cfd;
   E_Entry_Dialog    *dia_new_shelf;
   char                *new_shelf;
   Eina_Bool           header;
   unsigned int num_shelves;
};

static E_Config_Dialog_Data *_cfdata;

E_Config_Dialog *
e_int_config_shelf(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "extensions/shelves")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;

   cfd = e_config_dialog_new(con, _("Shelf Settings"), "E",
                             "extensions/shelves",
                             "preferences-desktop-shelf", 0, v, NULL);
   return cfd;
}

static Eina_Bool
_shelf_handler_rename_cb(E_Config_Dialog_Data *cfdata, int type __UNUSED__, E_Event_Shelf *ev)
{
   const Eina_List *l;
   E_Ilist_Item *ili;

   EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->o_list), l, ili)
     {
        if (e_widget_ilist_item_data_get(ili) != ev->shelf) continue;
        e_ilist_item_label_set(ili, ev->shelf->name);
        break;
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_shelf_handler_cb(E_Config_Dialog_Data *cfdata, int type __UNUSED__, E_Event_Shelf_Add *ev)
{
   E_Zone *zone;

   if (!cfdata->cfd->dia->win->border) return ECORE_CALLBACK_RENEW;
   zone = cfdata->cfd->dia->win->border->zone;
   if (ev->shelf->zone == zone)
   _ilist_item_new(cfdata, 1, ev->shelf);
   return ECORE_CALLBACK_RENEW;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   if (_cfdata) return NULL;
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   E_LIST_HANDLER_APPEND(cfdata->handlers, E_EVENT_SHELF_ADD, _shelf_handler_cb, cfdata);
   E_LIST_HANDLER_APPEND(cfdata->handlers, E_EVENT_SHELF_RENAME, _shelf_handler_rename_cb, cfdata);
   cfdata->cfd = cfd;
   cfd->dia->win->state.no_reopen = EINA_TRUE;
   _cfdata = cfdata;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_Shelf *es;
   EINA_LIST_FREE(cfdata->shelves, es)
     {
        if (e_object_is_del(E_OBJECT(es))) continue;
        e_object_del_func_set(E_OBJECT(es), NULL);
        if (es->config_dialog) e_object_del_attach_func_set(E_OBJECT(es->config_dialog), NULL);
     }
   E_FREE_LIST(cfdata->handlers, ecore_event_handler_del);
   E_FREE(cfdata);
   _cfdata = NULL;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ol, *ow, *ot, *of;
   char buf[64];
   E_Zone *zone;

   e_dialog_resizable_set(cfd->dia, 1);
   
   ol = e_widget_list_add(evas, 0, 0);
   zone = cfdata->cfd->dia->win->border ? cfdata->cfd->dia->win->border->zone : e_zone_current_get(cfdata->cfd->con);
   snprintf(buf, sizeof(buf), _("Configured Shelves: Display %d"), zone->num);
   of = e_widget_framelist_add(evas, buf, 0);
   cfdata->o_list = e_widget_ilist_add(evas, 24, 24, &(cfdata->cur_shelf));
   e_widget_size_min_set(cfdata->o_list, (140 * e_scale), (80 * e_scale));
   e_widget_framelist_object_append(of, cfdata->o_list);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);

   ot = e_widget_table_add(evas, 0);
   cfdata->o_add = ow = e_widget_button_add(evas, _("Add"), "list-add", _cb_add, cfdata, NULL);
   e_widget_table_object_append(ot, ow, 0, 0, 1, 1, 1, 1, 0, 0);
   cfdata->o_delete = e_widget_button_add(evas, _("Delete"), "list-remove",
                                          _cb_delete, cfdata, NULL);
   e_widget_table_object_append(ot, cfdata->o_delete, 1, 0, 1, 1, 1, 1, 0, 0);
   cfdata->o_rename = e_widget_button_add(evas, _("Rename"), "edit-rename",
                                          _cb_rename, cfdata, NULL);
   e_widget_table_object_append(ot, cfdata->o_rename, 2, 0, 1, 1, 1, 1, 0, 0);
   cfdata->o_contents = e_widget_button_add(evas, _("Contents"), "preferences-desktop-shelf",
                                            _cb_contents, cfdata, NULL);
   e_widget_table_object_align_append(ot, cfdata->o_contents,
                                      3, 0, 1, 1, 0, 1, 1, 1, 1.0, 0.5);
   cfdata->o_config = e_widget_button_add(evas, _("Settings"), "configure",
                                          _cb_config, cfdata, NULL);
   e_widget_table_object_align_append(ot, cfdata->o_config,
                                      4, 0, 1, 1, 0, 1, 1, 1, 1.0, 0.5);
   e_widget_list_object_append(ol, ot, 1, 0, 0.0);

   _ilist_fill(cfdata);
   e_widget_disabled_set(cfdata->o_add, 0);


   return ol;
}

/* private functions */
static void
_ilist_refresh(E_Shelf *es __UNUSED__)
{
   _ilist_empty(_cfdata);
   _ilist_fill(_cfdata);
}

static void
_widgets_disable(E_Config_Dialog_Data *cfdata, Eina_Bool disable, Eina_Bool list_too)
{
   e_widget_disabled_set(cfdata->o_add, disable);
   if (disable || e_widget_ilist_selected_count_get(cfdata->o_list))
     {
        e_widget_disabled_set(cfdata->o_delete, disable);
        e_widget_disabled_set(cfdata->o_config, disable);
        e_widget_disabled_set(cfdata->o_contents, disable);
        e_widget_disabled_set(cfdata->o_rename, disable);
     }
   if (list_too) e_widget_disabled_set(cfdata->o_list, disable);
}

static void
_ilist_item_new(E_Config_Dialog_Data *cfdata, Eina_Bool append, E_Shelf *es)
{
   char buf[256];
   Evas_Object *ob;

   if (es->name)
     snprintf(buf, sizeof(buf), "%s", es->name);
   else
     snprintf(buf, sizeof(buf), _("Shelf %s"), e_shelf_orient_string_get(es));
   e_object_del_func_set(E_OBJECT(es), (E_Object_Cleanup_Func)_ilist_refresh);

   ob = e_icon_add(evas_object_evas_get(cfdata->o_list));
   e_util_gadcon_orient_icon_set(es->cfg->orient, ob);
   if (append)
     e_widget_ilist_append(cfdata->o_list, ob, buf,
                           _ilist_cb_selected, es, buf);
   else
     e_widget_ilist_prepend(cfdata->o_list, ob, buf,
                            _ilist_cb_selected, es, buf);
   cfdata->shelves = eina_list_append(cfdata->shelves, es);
}

static void
_ilist_empty(E_Config_Dialog_Data *cfdata)
{
   Eina_List *l;
   E_Shelf *es;
   E_Desk *desk;
   E_Zone *zone;

   if ((!cfdata) || (!cfdata->cfd) || (!cfdata->cfd->con) || (!cfdata->cfd->con->manager)) return;
   zone = cfdata->cfd->dia->win->border ? cfdata->cfd->dia->win->border->zone : e_zone_current_get(cfdata->cfd->con);
   if (!zone) return;
   desk = cfdata->cfd->dia->win->border ? cfdata->cfd->dia->win->border->desk : e_desk_current_get(zone);
   EINA_LIST_FOREACH(e_shelf_list(), l, es)
     {
        if (es->zone != zone) continue;
        if (es->cfg->desk_show_mode)
          {
             Eina_List *ll;
             E_Config_Shelf_Desk *sd;

             EINA_LIST_FOREACH(es->cfg->desk_list, ll, sd)
               {
                  if ((desk->x == sd->x) && (desk->y == sd->y))
                    {
                       e_object_del_func_set(E_OBJECT(es), NULL);
                       break;
                    }
               }
          }
        else
          {
             e_object_del_func_set(E_OBJECT(es), NULL);
          }
     }
   e_widget_ilist_clear(cfdata->o_list);
   cfdata->shelves = eina_list_free(cfdata->shelves);
}

static void
_ilist_fill(E_Config_Dialog_Data *cfdata)
{
   Evas *evas;
   Eina_List *l;
   E_Shelf *es;
   E_Desk *desk;
   E_Zone *zone;
   int n = -1;

   if (!cfdata) return;
   if (!cfdata->o_list) return;

   evas = evas_object_evas_get(cfdata->o_list);

   if (e_widget_ilist_count(cfdata->o_list) > 0)
     n = e_widget_ilist_selected_get(cfdata->o_list);

   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_list);
   e_widget_ilist_clear(cfdata->o_list);
   e_widget_ilist_go(cfdata->o_list);
   zone = cfdata->cfd->dia->win->border ? cfdata->cfd->dia->win->border->zone : e_zone_current_get(cfdata->cfd->con);
   desk = e_desk_current_get(zone);

   EINA_LIST_FOREACH(e_shelf_list(), l, es)
     {
        if (es->zone != zone) continue;
        if (es->cfg->desk_show_mode)
          {
             Eina_List *ll;
             E_Config_Shelf_Desk *sd;

             EINA_LIST_FOREACH(es->cfg->desk_list, ll, sd)
               {
                  if ((desk->x == sd->x) && (desk->y == sd->y))
                    {
                       if (!cfdata->header)
                         {
                            char buf[32];
                            cfdata->header = EINA_TRUE;
                            snprintf(buf, sizeof(buf), "Desk %d,%d", desk->x, desk->y);
                            e_widget_ilist_header_append(cfdata->o_list, NULL, buf);
                         }
                       _ilist_item_new(cfdata, EINA_TRUE, es);
                       break;
                    }
               }
          }
        else
          _ilist_item_new(cfdata, !cfdata->header, es);
     }

   e_widget_size_min_set(cfdata->o_list, 155, 250);
   e_widget_ilist_go(cfdata->o_list);
   e_widget_ilist_thaw(cfdata->o_list);
   edje_thaw();
   evas_event_thaw(evas);

   if (n > -1)
     {
        _widgets_disable(cfdata, 0, EINA_TRUE);
        e_widget_ilist_selected_set(cfdata->o_list, n);
     }
   else
     {
        _widgets_disable(cfdata, 1, EINA_FALSE);
        e_widget_disabled_set(cfdata->o_list, 0);
        e_widget_disabled_set(cfdata->o_add, 0);
     }
}

static void
_ilist_cb_selected(void *data __UNUSED__)
{
   _widgets_disable(_cfdata, 0, EINA_TRUE);
}

static void
_new_shelf_cb_close(void *data)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = e_object_data_get(data);
   cfdata->dia_new_shelf = NULL;
   if (e_widget_ilist_selected_get(cfdata->o_list) >= 0)
     _widgets_disable(cfdata, 0, EINA_TRUE);
   else
     {
        e_widget_disabled_set(cfdata->o_list, 0);
        e_widget_disabled_set(cfdata->o_add, 0);
     }
}

static void
_cb_add(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   E_Zone *zone;

   cfdata = data;
   if (!cfdata) return;

   zone = cfdata->cfd->dia->win->border ? cfdata->cfd->dia->win->border->zone : e_zone_current_get(cfdata->cfd->con);
   cfdata->dia_new_shelf = e_shelf_new_dialog(zone);
   e_object_data_set(E_OBJECT(cfdata->dia_new_shelf), cfdata);
   e_object_del_attach_func_set(E_OBJECT(cfdata->dia_new_shelf), _new_shelf_cb_close);
   _widgets_disable(cfdata, 1, EINA_TRUE);
   cfdata->num_shelves = eina_list_count(e_config->shelves);
}

static void
_cb_rename(void *data, void *d __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   E_Shelf *es;
   es = e_widget_ilist_selected_data_get(cfdata->o_list);
   e_shelf_rename_dialog(es);
}

static void
_cb_delete(void *data, void *data2 __UNUSED__)
{
   char buf[1024];
   E_Config_Dialog_Data *cfdata = data;
   E_Shelf *es;

   es = e_widget_ilist_selected_data_get(cfdata->o_list);
   if (!es) return;

   e_object_ref(E_OBJECT(es));

   if (e_config->cnfmdlg_disabled)
     {
        if (e_object_is_del(E_OBJECT(es))) return;
        e_shelf_unsave(es);
        e_object_del(E_OBJECT(es));
        e_config_save_queue();

        e_object_unref(E_OBJECT(es));
        _ilist_fill(cfdata);
        return;
     }
   _widgets_disable(data, 1, EINA_TRUE);
   snprintf(buf, sizeof(buf), _("Are you sure you want to delete \"%s\"?"),
            cfdata->cur_shelf);

   e_confirm_dialog_show(_("Confirm Shelf Deletion"),
                         "application-exit", buf, _("Delete"), _("Keep"),
                         _cb_dialog_yes, NULL, es, NULL, _cb_dialog_destroy, es);
}

static void
_cb_dialog_yes(void *data)
{
   E_Shelf *es;
   E_Config_Dialog_Data *cfdata = _cfdata;

   es = data;;
   if (e_object_is_del(E_OBJECT(es))) return;
   e_shelf_unsave(es);
   e_object_del(E_OBJECT(es));
   e_object_unref(E_OBJECT(es));
   e_config_save_queue();
   _ilist_empty(cfdata);
   _ilist_fill(cfdata);
}

static void
_cb_dialog_destroy(void *data)
{
   E_Shelf *es;
   E_Config_Dialog_Data *cfdata = _cfdata;

   es = data;;
   if (e_object_is_del(E_OBJECT(es))) return;
   e_object_unref(E_OBJECT(es));
   _widgets_disable(cfdata, 0, EINA_TRUE);
}

static void
_cb_config_end(void *data __UNUSED__)
{
   e_widget_disabled_set(_cfdata->o_list, 0);
}

static void
_cb_config(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   E_Shelf *es;

   if (!(cfdata = data)) return;
   es = e_widget_ilist_selected_data_get(cfdata->o_list);
   if (!es) return;
   if (!es->config_dialog)
     {
        e_int_shelf_config(es);
        e_object_del_attach_func_set(E_OBJECT(es->config_dialog), _cb_config_end);
     }
}

static void
_cb_contents_end(void *data __UNUSED__)
{
   e_widget_disabled_set(_cfdata->o_list, 0);
}

static void
_cb_contents(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   E_Shelf *es;

   if (!(cfdata = data)) return;
   es = e_widget_ilist_selected_data_get(cfdata->o_list);
   if (!es) return;
   if (!es->gadcon->config_dialog)
     {
        e_int_gadcon_config_shelf(es->gadcon);
        e_object_del_attach_func_set(E_OBJECT(es->gadcon->config_dialog), _cb_contents_end);
     }
}

