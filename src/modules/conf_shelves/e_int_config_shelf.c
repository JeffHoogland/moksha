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
static void         _new_shelf_dialog(E_Config_Dialog_Data *cfdata);
static void         _new_shelf_cb_close(void *data);
static void         _new_shelf_cb_ok(void *data, char *text);
static void         _new_shelf_cb_dia_del(void *obj);
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
   Ecore_Event_Handler *shelf_handler;
   Eina_List           *shelves;
   E_Config_Dialog     *cfd;
   E_Entry_Dialog    *dia_new_shelf;
   char                *new_shelf;
   Eina_Bool           header;
};

static int orientations[] =
{
   [E_GADCON_ORIENT_FLOAT] = 2,
   [E_GADCON_ORIENT_HORIZ] = 2,
   [E_GADCON_ORIENT_VERT] = 2,
   [E_GADCON_ORIENT_LEFT] = 37,
   [E_GADCON_ORIENT_RIGHT] = 31,
   [E_GADCON_ORIENT_TOP] = 29,
   [E_GADCON_ORIENT_BOTTOM] = 23,
   [E_GADCON_ORIENT_CORNER_TL] = 19,
   [E_GADCON_ORIENT_CORNER_TR] = 17,
   [E_GADCON_ORIENT_CORNER_BL] = 13,
   [E_GADCON_ORIENT_CORNER_BR] = 11,
   [E_GADCON_ORIENT_CORNER_LT] = 7,
   [E_GADCON_ORIENT_CORNER_RT] = 5,
   [E_GADCON_ORIENT_CORNER_LB] = 3,
   [E_GADCON_ORIENT_CORNER_RB] = 2
};

E_Config_Dialog *
e_int_config_shelf(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

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
_shelf_handler_cb(E_Config_Dialog_Data *cfdata, int type __UNUSED__, E_Event_Shelf_Add *ev __UNUSED__)
{
   _ilist_empty(cfdata);
   _ilist_fill(cfdata);
   return ECORE_CALLBACK_RENEW;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->shelf_handler = ecore_event_handler_add(E_EVENT_SHELF_ADD, (Ecore_Event_Handler_Cb)_shelf_handler_cb, cfdata);
   cfdata->cfd = cfd;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_Shelf *es;
   EINA_LIST_FREE(cfdata->shelves, es)
     {
        if (e_object_is_del(E_OBJECT(es))) continue;
        evas_object_data_del(es->o_base, "cfdata");
        e_object_del_func_set(E_OBJECT(es), NULL);
        if (es->config_dialog) e_object_del_attach_func_set(E_OBJECT(es->config_dialog), NULL);
     }
   ecore_event_handler_del(cfdata->shelf_handler);
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ol, *ow, *ot, *of;
   char buf[64];
   E_Zone *zone;

   ol = e_widget_list_add(evas, 0, 0);
   zone = cfdata->cfd->dia->win->border ? cfdata->cfd->dia->win->border->zone : e_zone_current_get(cfdata->cfd->con);
   snprintf(buf, sizeof(buf), "%s %d", _("Configured Shelves: Display"), zone->num);
   of = e_widget_framelist_add(evas, buf, 0);
   cfdata->o_list = e_widget_ilist_add(evas, 24, 24, &(cfdata->cur_shelf));
   evas_object_data_set(cfdata->o_list, "cfdata", cfdata);
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
   cfdata->o_config = e_widget_button_add(evas, _("Setup"), "configure",
                                          _cb_config, cfdata, NULL);
   e_widget_table_object_align_append(ot, cfdata->o_config,
                                      4, 0, 1, 1, 0, 1, 1, 1, 1.0, 0.5);
   e_widget_list_object_append(ol, ot, 1, 0, 0.0);

   _ilist_fill(cfdata);
   e_widget_disabled_set(cfdata->o_add, 0);

   e_dialog_resizable_set(cfd->dia, 1);

   return ol;
}

/* private functions */
static void
_ilist_refresh(E_Shelf *es)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = evas_object_data_get(es->o_base, "cfdata");
   if (!cfdata) return;
   _ilist_empty(cfdata);
   _ilist_fill(cfdata);
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
     snprintf(buf, sizeof(buf), "%s %s", _("Shelf"), e_shelf_orient_string_get(es));
   e_object_del_func_set(E_OBJECT(es), (E_Object_Cleanup_Func)_ilist_refresh);
   evas_object_data_set(es->o_base, "cfdata", cfdata);

   ob = e_icon_add(evas_object_evas_get(cfdata->o_list));
   switch (es->cfg->orient)
     {
      case E_GADCON_ORIENT_LEFT:
        e_util_icon_theme_set(ob, "preferences-position-left");
        break;

      case E_GADCON_ORIENT_RIGHT:
        e_util_icon_theme_set(ob, "preferences-position-right");
        break;

      case E_GADCON_ORIENT_TOP:
        e_util_icon_theme_set(ob, "preferences-position-top");
        break;

      case E_GADCON_ORIENT_BOTTOM:
        e_util_icon_theme_set(ob, "preferences-position-bottom");
        break;

      case E_GADCON_ORIENT_CORNER_TL:
        e_util_icon_theme_set(ob, "preferences-position-top-left");
        break;

      case E_GADCON_ORIENT_CORNER_TR:
        e_util_icon_theme_set(ob, "preferences-position-top-right");
        break;

      case E_GADCON_ORIENT_CORNER_BL:
        e_util_icon_theme_set(ob, "preferences-position-bottom-left");
        break;

      case E_GADCON_ORIENT_CORNER_BR:
        e_util_icon_theme_set(ob, "preferences-position-bottom-right");
        break;

      case E_GADCON_ORIENT_CORNER_LT:
        e_util_icon_theme_set(ob, "preferences-position-left-top");
        break;

      case E_GADCON_ORIENT_CORNER_RT:
        e_util_icon_theme_set(ob, "preferences-position-right-top");
        break;

      case E_GADCON_ORIENT_CORNER_LB:
        e_util_icon_theme_set(ob, "preferences-position-left-bottom");
        break;

      case E_GADCON_ORIENT_CORNER_RB:
        e_util_icon_theme_set(ob, "preferences-position-right-bottom");
        break;

      default:
        e_util_icon_theme_set(ob, "enlightenment");
        break;
     }
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
                       evas_object_data_del(es->o_base, "cfdata");
                       break;
                    }
               }
          }
        else
          {
             e_object_del_func_set(E_OBJECT(es), NULL);
             evas_object_data_del(es->o_base, "cfdata");
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
_ilist_cb_selected(void *data)
{
   E_Config_Dialog_Data *cfdata;
   E_Shelf *es = data;

   if (!(cfdata = evas_object_data_get(es->o_base, "cfdata"))) return;
   _widgets_disable(cfdata, 0, EINA_TRUE);
}

static void
_new_shelf_dialog(E_Config_Dialog_Data *cfdata)
{
   char buf[256];
   int id;
   id = e_widget_ilist_count(cfdata->o_list);
   snprintf(buf, sizeof(buf), "%s #%d", _("Shelf"), id);
   cfdata->dia_new_shelf = e_entry_dialog_show(_("Add New Shelf"), "preferences-desktop-shelf",
                             _("Name:"), buf, NULL, NULL,
                             _new_shelf_cb_ok, _new_shelf_cb_close,
                             cfdata);
   _widgets_disable(cfdata, 1, EINA_TRUE);
}

static void
_new_shelf_cb_close(void *data)
{
   E_Config_Dialog_Data *cfdata;
   cfdata = data;
   if (!cfdata) return;

   _widgets_disable(cfdata, 0, EINA_TRUE);
   cfdata->dia_new_shelf = NULL;
}

static void
_cb_add(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (!cfdata) return;

   _new_shelf_dialog(cfdata);
}

static void
_new_shelf_cb_ok(void *data, char *text)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Shelf *cfg, *c;
   E_Zone *zone;
   Eina_List *l;
   unsigned int x;
   unsigned long orient = 1;

   cfdata = data;
   if (!cfdata) return;
   if ((!text) || (!text[0]))
     {
        _new_shelf_cb_dia_del(cfdata->dia_new_shelf);
        return;
     }

   if (cfdata->cfd && cfdata->cfd->dia && cfdata->cfd->dia->win && cfdata->cfd->dia->win->border && cfdata->cfd->dia->win->border->zone)
     zone = cfdata->cfd->dia->win->border->zone;
   else
     zone = e_util_zone_current_get(e_manager_current_get());

   cfg = E_NEW(E_Config_Shelf, 1);
   cfg->name = eina_stringshare_add(text);
   cfg->container = zone->container->num;
   cfg->zone = zone->num;
   cfg->popup = 1;
   cfg->layer = 200;
   EINA_LIST_FOREACH(e_config->shelves, l, c)
     orient *= orientations[c->orient];
   for (x = 3; x < (sizeof(orientations) / sizeof(orientations[0])); x++)
     if (orient % orientations[x])
       {
          cfg->orient = x;
          break;
       }
   cfg->fit_along = 1;
   cfg->fit_size = 0;
   cfg->style = eina_stringshare_add("default");
   cfg->size = 40;
   cfg->overlap = 0;
   cfg->autohide = 0;
   e_config->shelves = eina_list_append(e_config->shelves, cfg);
   e_config_save_queue();

   c = eina_list_data_get(eina_list_last(e_config->shelves));
   cfg->id = c->id + 1;
   e_shelf_config_new(zone, cfg);

   cfdata->dia_new_shelf = NULL;
   _ilist_fill(cfdata);
}

static void
_new_shelf_cb_dia_del(void *obj)
{
   E_Entry_Dialog *dia = obj;
   E_Config_Dialog_Data *cfdata = dia->ok.data;

   cfdata->dia_new_shelf = NULL;
   e_widget_disabled_set(cfdata->o_list, 0);
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
   snprintf(buf, sizeof(buf), _("You requested to delete \"%s\".<br><br>"
                                "Are you sure you want to delete this shelf?"),
            cfdata->cur_shelf);

   evas_object_data_set(es->o_base, "cfdata", cfdata);
   e_confirm_dialog_show(_("Are you sure you want to delete this shelf?"),
                         "application-exit", buf, _("Delete"), _("Keep"),
                         _cb_dialog_yes, NULL, es, NULL, _cb_dialog_destroy, es);
}

static void
_cb_dialog_yes(void *data)
{
   E_Shelf *es;
   E_Config_Dialog_Data *cfdata;

   es = data;;
   if (e_object_is_del(E_OBJECT(es))) return;
   cfdata = evas_object_data_get(es->o_base, "cfdata");
   if (!cfdata) return;
   evas_object_data_del(es->o_base, "cfdata");
   e_shelf_unsave(es);
   e_object_unref(E_OBJECT(es));
   e_object_del(E_OBJECT(es));
   e_config_save_queue();
   _ilist_empty(cfdata);
   _ilist_fill(cfdata);
}

static void
_cb_dialog_destroy(void *data)
{
   E_Shelf *es;
   E_Config_Dialog_Data *cfdata;

   es = data;;
   if (e_object_is_del(E_OBJECT(es))) return;
   cfdata = evas_object_data_get(es->o_base, "cfdata");
   if (!cfdata) return;
   evas_object_data_del(es->o_base, "cfdata");
   e_object_unref(E_OBJECT(es));
   _widgets_disable(cfdata, 0, EINA_TRUE);
}

static void
_cb_config_end(void *data)
{
   E_Config_Dialog *cfd = data;
   E_Shelf *es;
   E_Config_Dialog_Data *cfdata;

   es = cfd->data;
   cfdata = evas_object_data_get(es->o_base, "cfdata");
   if (!cfdata) return;
   evas_object_data_del(es->o_base, "cfdata");
   e_widget_disabled_set(cfdata->o_list, 0);
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
        evas_object_data_set(es->o_base, "cfdata", cfdata);
        e_object_del_attach_func_set(E_OBJECT(es->config_dialog), _cb_config_end);
     }
}

static void
_cb_contents_end(void *data)
{
   E_Config_Dialog *cfd = data;
   E_Shelf *es;
   E_Gadcon *gc;
   E_Config_Dialog_Data *cfdata;

   gc = cfd->data;
   es = gc->shelf;
   cfdata = evas_object_data_get(es->o_base, "cfdata");
   if (!cfdata) return;
   evas_object_data_del(es->o_base, "cfdata");
   e_widget_disabled_set(cfdata->o_list, 0);
}

static void
_cb_contents(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   E_Shelf *es;

   if (!(cfdata = data)) return;
   es = e_widget_ilist_selected_data_get(cfdata->o_list);
   if (!es) return;
   if (!es->config_dialog)
     {
        e_int_gadcon_config_shelf(es->gadcon);
        evas_object_data_set(es->o_base, "cfdata", cfdata);
        e_object_del_attach_func_set(E_OBJECT(es->gadcon->config_dialog), _cb_contents_end);
     }
}

