#include "e.h"

struct _E_Config_Dialog_Data 
{
   E_Shelf *es;
   E_Config_Shelf *escfg;

   Evas_Object *o_autohide, *o_desk_list;
   Eina_List *autohide_list, *desk_list;

   int layer, overlap;
   int orient, fit_along;
   int size;
   const char *style;
   int autohide, autohide_action;
   double hide_timeout, hide_duration;
   int desk_show_mode;
};

/* local function prototypes */
static void *_create_data(E_Config_Dialog *cfd);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static void _free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _fill_styles(E_Config_Dialog_Data *cfdata, Evas_Object *obj);
static void _cb_autohide_change(void *data, Evas_Object *obj __UNUSED__);
static void _fill_desks(E_Config_Dialog_Data *cfdata);

EAPI void 
e_int_shelf_config(E_Shelf *es) 
{
   E_Config_Dialog_View *v;

   if (!(v = E_NEW(E_Config_Dialog_View, 1))) return;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   es->config_dialog = 
     e_config_dialog_new(es->zone->container, _("Shelf Settings"), 
                         "E", "_shelf_config_dialog", 
                         "preferences-desktop-shelf", 0, v, es);
   e_win_centered_set(es->config_dialog->dia->win, EINA_TRUE);
}

/* local functions */
static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->es = cfd->data;
   cfdata->escfg = cfdata->es->cfg;
   _fill_data(cfdata);
   return cfdata;
}

static void 
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   /* stacking */
   if ((!cfdata->escfg->popup) && (cfdata->escfg->layer == 1))
     cfdata->layer = 0;
   else if ((cfdata->escfg->popup) && (cfdata->escfg->layer == 0))
     cfdata->layer = 1;
   else if ((cfdata->escfg->popup) && (cfdata->escfg->layer == 200))
     cfdata->layer = 2;
   else
     cfdata->layer = 2;
   cfdata->overlap = cfdata->escfg->overlap;

   /* position */
   cfdata->orient = cfdata->escfg->orient;
   cfdata->fit_along = cfdata->escfg->fit_along;

   /* size */
   cfdata->size = cfdata->escfg->size;

   /* style */
   if (cfdata->escfg->style) 
     cfdata->style = eina_stringshare_ref(cfdata->escfg->style);
   else
     cfdata->style = eina_stringshare_add("");

   /* autohide */
   cfdata->autohide = cfdata->escfg->autohide;
   cfdata->autohide_action = cfdata->escfg->autohide_show_action;
   cfdata->hide_timeout = cfdata->escfg->hide_timeout;
   cfdata->hide_duration = cfdata->escfg->hide_duration;

   /* desktop */
   cfdata->desk_show_mode = cfdata->escfg->desk_show_mode;
   cfdata->desk_list = cfdata->escfg->desk_list;
}

static void 
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata) 
{
   eina_list_free(cfdata->autohide_list);

   if (cfdata->style) eina_stringshare_del(cfdata->style);
   cfdata->style = NULL;

   cfdata->es->config_dialog = NULL;
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *otb, *ol, *ow;
   E_Radio_Group *rg;

   otb = e_widget_toolbook_add(evas, 24, 24);

   /* Stacking */
   ol = e_widget_list_add(evas, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->layer));
   ow = e_widget_radio_add(evas, _("Above Everything"), 2, rg);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_radio_add(evas, _("Below Windows"), 1, rg);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_radio_add(evas, _("Below Everything"), 0, rg);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_check_add(evas, _("Allow windows to overlap the shelf"), 
                           &(cfdata->overlap));
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Stacking"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   /* position */
   ol = e_widget_table_add(evas, 1);
   rg = e_widget_radio_group_new(&(cfdata->orient));
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-left", 
                                24, 24, E_GADCON_ORIENT_LEFT, rg);
   e_widget_table_object_append(ol, ow, 0, 2, 1, 1, 1, 1, 1, 1);
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-right", 
                                24, 24, E_GADCON_ORIENT_RIGHT, rg);
   e_widget_table_object_append(ol, ow, 2, 2, 1, 1, 1, 1, 1, 1);
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-top", 
                                24, 24, E_GADCON_ORIENT_TOP, rg);
   e_widget_table_object_append(ol, ow, 1, 0, 1, 1, 1, 1, 1, 1);
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-bottom", 
                                24, 24, E_GADCON_ORIENT_BOTTOM, rg);
   e_widget_table_object_append(ol, ow, 1, 4, 1, 1, 1, 1, 1, 1);
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-top-left", 
                                24, 24, E_GADCON_ORIENT_CORNER_TL, rg);
   e_widget_table_object_append(ol, ow, 0, 0, 1, 1, 1, 1, 1, 1);
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-top-right", 
                                24, 24, E_GADCON_ORIENT_CORNER_TR, rg);
   e_widget_table_object_append(ol, ow, 2, 0, 1, 1, 1, 1, 1, 1);
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-bottom-left", 
                                24, 24, E_GADCON_ORIENT_CORNER_BL, rg);
   e_widget_table_object_append(ol, ow, 0, 4, 1, 1, 1, 1, 1, 1);
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-bottom-right", 
                                24, 24, E_GADCON_ORIENT_CORNER_BR, rg);
   e_widget_table_object_append(ol, ow, 2, 4, 1, 1, 1, 1, 1, 1);
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-left-top", 
                                24, 24, E_GADCON_ORIENT_CORNER_LT, rg);
   e_widget_table_object_append(ol, ow, 0, 1, 1, 1, 1, 1, 1, 1);
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-right-top", 
                                24, 24, E_GADCON_ORIENT_CORNER_RT, rg);
   e_widget_table_object_append(ol, ow, 2, 1, 1, 1, 1, 1, 1, 1);
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-left-bottom", 
                                24, 24, E_GADCON_ORIENT_CORNER_LB, rg);
   e_widget_table_object_append(ol, ow, 0, 3, 1, 1, 1, 1, 1, 1);
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-right-bottom", 
                                24, 24, E_GADCON_ORIENT_CORNER_RB, rg);
   e_widget_table_object_append(ol, ow, 2, 3, 1, 1, 1, 1, 1, 1);
   e_widget_toolbook_page_append(otb, NULL, _("Position"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   /* size */
   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_slider_add(evas, 1, 0, _("Height (%3.0f pixels)"), 4, 256, 4, 0, 
                            NULL, &(cfdata->size), 100);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_check_add(evas, _("Shrink to Content Width"), 
                           &(cfdata->fit_along));
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Size"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   /* style */
   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_ilist_add(evas, 60, 20, &(cfdata->style));
   _fill_styles(cfdata, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Style"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   /* autohide */
   ol = e_widget_list_add(evas, 0, 0);
   cfdata->o_autohide = 
     e_widget_check_add(evas, _("Auto-hide the shelf"), &(cfdata->autohide));
   e_widget_on_change_hook_set(cfdata->o_autohide, _cb_autohide_change, cfdata);
   e_widget_list_object_append(ol, cfdata->o_autohide, 1, 1, 0.5);

   rg = e_widget_radio_group_new(&(cfdata->autohide_action));
   ow = e_widget_radio_add(evas, _("Show on mouse in"), 0, rg);
   e_widget_disabled_set(ow, !cfdata->autohide);
   cfdata->autohide_list = eina_list_append(cfdata->autohide_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_radio_add(evas, _("Show on mouse click"), 1, rg);
   cfdata->autohide_list = eina_list_append(cfdata->autohide_list, ow);
   e_widget_disabled_set(ow, !cfdata->autohide);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);

   ow = e_widget_label_add(evas, _("Hide timeout"));
   cfdata->autohide_list = eina_list_append(cfdata->autohide_list, ow);
   e_widget_disabled_set(ow, !cfdata->autohide);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%.1f seconds"), 0.2, 6.0, 0.2, 0, 
                            &(cfdata->hide_timeout), NULL, 100);
   cfdata->autohide_list = eina_list_append(cfdata->autohide_list, ow);
   e_widget_disabled_set(ow, !cfdata->autohide);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);

   ow = e_widget_label_add(evas, _("Hide duration"));
   cfdata->autohide_list = eina_list_append(cfdata->autohide_list, ow);
   e_widget_disabled_set(ow, !cfdata->autohide);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%.2f seconds"), 0.05, 6.0, 0.05, 0, 
                            &(cfdata->hide_duration), NULL, 100);
   cfdata->autohide_list = eina_list_append(cfdata->autohide_list, ow);
   e_widget_disabled_set(ow, !cfdata->autohide);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Auto Hide"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   /* Desktop */
   ol = e_widget_list_add(evas, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->desk_show_mode));
   ow = e_widget_radio_add(evas, _("Show on all Desktops"), 0, rg);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_radio_add(evas, _("Show on specified Desktops"), 1, rg);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);

   cfdata->o_desk_list = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_ilist_multi_select_set(cfdata->o_desk_list, EINA_TRUE);
   _fill_desks(cfdata);
   e_widget_list_object_append(ol, cfdata->o_desk_list, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Desktop"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);
   return otb;
}

static int 
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_Config_Shelf_Desk *sd;
   int restart = 0;

   if (!cfdata->escfg->style) 
     {
        cfdata->escfg->style = eina_stringshare_ref(cfdata->style);
        e_shelf_style_set(cfdata->es, cfdata->style);
     }
   else if ((cfdata->escfg->style) && 
            (strcmp(cfdata->escfg->style, cfdata->style)))
     {
        if (cfdata->escfg->style) eina_stringshare_del(cfdata->escfg->style);
        cfdata->escfg->style = eina_stringshare_ref(cfdata->style);
        e_shelf_style_set(cfdata->es, cfdata->style);
     }

   if (cfdata->escfg->orient != cfdata->orient) 
     {
        cfdata->escfg->orient = cfdata->orient;
        e_shelf_orient(cfdata->es, cfdata->orient);
        e_shelf_position_calc(cfdata->es);
        restart = 1;
     }

   if (cfdata->escfg->fit_along != cfdata->fit_along) 
     {
        cfdata->escfg->fit_along = cfdata->fit_along;
        cfdata->es->fit_along = cfdata->fit_along;
        restart = 1;
     }

   if (cfdata->escfg->size != cfdata->size) 
     {
        cfdata->escfg->size = cfdata->size;
        cfdata->es->size = cfdata->size;
        restart = 1;
     }

   if (cfdata->layer == 0) 
     {
        if ((cfdata->escfg->popup != 0) || (cfdata->escfg->layer != 1)) 
          {
             cfdata->escfg->popup = 0;
             cfdata->escfg->layer = 1;
             restart = 1;
          }
     }
   else if (cfdata->layer == 1) 
     {
        if ((cfdata->escfg->popup != 1) || (cfdata->escfg->layer != 0)) 
          {
             cfdata->escfg->popup = 1;
             cfdata->escfg->layer = 0;
             restart = 1;
          }
     }
   else if (cfdata->layer == 2) 
     {
        if ((cfdata->escfg->popup != 1) || (cfdata->escfg->layer != 200)) 
          {
             cfdata->escfg->popup = 1;
             cfdata->escfg->layer = 200;
             restart = 1;
          }
     }

   cfdata->escfg->overlap = cfdata->overlap;
   cfdata->escfg->autohide = cfdata->autohide;
   cfdata->escfg->autohide_show_action = cfdata->autohide_action;
   cfdata->escfg->hide_timeout = cfdata->hide_timeout;
   cfdata->escfg->hide_duration = cfdata->hide_duration;
   cfdata->escfg->desk_show_mode = cfdata->desk_show_mode;

   EINA_LIST_FREE(cfdata->escfg->desk_list, sd)
     E_FREE(sd);
   cfdata->escfg->desk_list = NULL;

   if (cfdata->desk_show_mode) 
     {
        Eina_List *l;
        const E_Ilist_Item *it;
        int idx = -1;

        EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->o_desk_list), l, it) 
          {
             E_Desk *desk;

             idx++;
             if ((!it) || (!it->selected)) continue;
             if (!(desk = e_desk_at_pos_get(cfdata->es->zone, idx))) continue;
             sd = E_NEW(E_Config_Shelf_Desk, 1);
             sd->x = desk->x;
             sd->y = desk->y;
             cfdata->escfg->desk_list = 
               eina_list_append(cfdata->escfg->desk_list, sd);
          }
     }

   if (restart) 
     {
        E_Zone *zone;

        zone = cfdata->es->zone;
        cfdata->es->config_dialog = NULL;
        e_object_del(E_OBJECT(cfdata->es));

        cfdata->es = 
          e_shelf_zone_new(zone, cfdata->escfg->name, 
                           cfdata->escfg->style, cfdata->escfg->popup, 
                           cfdata->escfg->layer, cfdata->escfg->id);
        cfdata->es->cfg = cfdata->escfg;
        cfdata->es->fit_along = cfdata->escfg->fit_along;
        e_shelf_orient(cfdata->es, cfdata->escfg->orient);
        e_shelf_position_calc(cfdata->es);
        e_shelf_populate(cfdata->es);
     }

   if (cfdata->escfg->desk_show_mode) 
     {
        E_Desk *desk;
        Eina_List *l;
        E_Config_Shelf_Desk *sd;
        int show = 0;

        desk = e_desk_current_get(cfdata->es->zone);
        EINA_LIST_FOREACH(cfdata->escfg->desk_list, l, sd) 
          {
             if ((desk->x == sd->x) && (desk->y == sd->y)) 
               {
                  show = 1;
                  break;
               }
          }
        if (show) e_shelf_show(cfdata->es);
        else e_shelf_hide(cfdata->es);
     }
   else
     e_shelf_show(cfdata->es);

   if ((cfdata->escfg->autohide) && (!cfdata->es->hidden))
     e_shelf_toggle(cfdata->es, 0);
   else if ((!cfdata->escfg->autohide) && (cfdata->es->hidden))
     e_shelf_toggle(cfdata->es, 1);

   e_zone_useful_geometry_dirty(cfdata->es->zone);
   e_config_save_queue();
   cfdata->es->config_dialog = cfd;
   return 1;
}

static void 
_fill_styles(E_Config_Dialog_Data *cfdata, Evas_Object *obj) 
{
   Evas *evas;
   Eina_List *l, *styles;
   char *style;
   const char *str;
   int mw, n = 0;

   evas = evas_object_evas_get(obj);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(obj);
   e_widget_ilist_clear(obj);

   styles = e_theme_shelf_list();
   EINA_LIST_FOREACH(styles, l, style) 
     {
        Evas_Object *thumb, *ow;
        char buff[PATH_MAX];

        thumb = e_livethumb_add(evas);
        e_livethumb_vsize_set(thumb, 120, 40);
        ow = edje_object_add(e_livethumb_evas_get(thumb));
        snprintf(buff, sizeof(buff), "e/shelf/%s/base", style);
        e_theme_edje_object_set(ow, "base/theme/shelf", buff);
        e_livethumb_thumb_set(thumb, ow);
        e_widget_ilist_append(obj, thumb, style, NULL, NULL, style);
        if (!strcmp(cfdata->style, style))
	  e_widget_ilist_selected_set(obj, n);
        n++;
     }

   e_widget_size_min_get(obj, &mw, NULL);
   e_widget_size_min_set(obj, mw, 180);

   e_widget_ilist_go(obj);
   e_widget_ilist_thaw(obj);
   edje_thaw();
   evas_event_thaw(evas);

   EINA_LIST_FREE(styles, str)
     eina_stringshare_del(str);
}

static void 
_cb_autohide_change(void *data, Evas_Object *obj __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *l;
   Evas_Object *ow;

   if (!(cfdata = data)) return;
   EINA_LIST_FOREACH(cfdata->autohide_list, l, ow)
     e_widget_disabled_set(ow, !cfdata->autohide);
}

static void 
_fill_desks(E_Config_Dialog_Data *cfdata) 
{
   Evas *evas;
   int mw, x, y;
   int i = 0;

   evas = evas_object_evas_get(cfdata->o_desk_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_desk_list);
   e_widget_ilist_clear(cfdata->o_desk_list);

   for (y = 0; y < e_config->zone_desks_y_count; y++)
     for (x = 0; x < e_config->zone_desks_x_count; x++) 
       {
          E_Desk *desk;
          Eina_List *l;
          E_Config_Shelf_Desk *sd;

          desk = e_desk_at_xy_get(cfdata->es->zone, x, y);
          e_widget_ilist_append(cfdata->o_desk_list, NULL, desk->name, 
                                NULL, NULL, NULL);
          i++;

          EINA_LIST_FOREACH(cfdata->desk_list, l, sd) 
            {
               if ((sd->x != x) || (sd->y != y)) continue;
               e_widget_ilist_multi_select(cfdata->o_desk_list, i);
               break;
            }
       }

   e_widget_size_min_get(cfdata->o_desk_list, &mw, NULL);
   e_widget_size_min_set(cfdata->o_desk_list, mw, 180);

   e_widget_ilist_go(cfdata->o_desk_list);
   e_widget_ilist_thaw(cfdata->o_desk_list);
   edje_thaw();
   evas_event_thaw(evas);
}
