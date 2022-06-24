#include "e_mod_main.h"

struct _E_Config_Dialog_Data
{
   /* general page variables */
   int    show_low;
   int    show_normal;
   int    show_critical;
   int    force_timeout;
   int    ignore_replacement;
   int    dual_screen;
   double timeout;
   int    corner;
   /* menu page variables */
   int time_stamp;
   int show_app;
   int show_count;
   int reverse;
   double item_length;
   double menu_items;
   double jump_delay;
   char *blacklist;
};

/* local function protos */
static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog      *cfd,
                               E_Config_Dialog_Data *cfdata);
static void         _fill_data(E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog      *cfd,
                                  Evas                 *evas,
                                  E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog      *cfd,
                        E_Config_Dialog_Data *cfdata);
                        
E_Config_Dialog *
e_int_config_notification_module(E_Container *con,
                                 const char  *params __UNUSED__)
{
   E_Config_Dialog *cfd = NULL;
   E_Config_Dialog_View *v = NULL;
   //~ char buf[4096];

   if (e_config_dialog_find("Notification", "extensions/notification")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   //~ snprintf(buf, sizeof(buf), "%s/e-module-notification.edj", notification_mod->dir);
   cfd = e_config_dialog_new(con, _("Notification Settings"), "Notification",
                             "extensions/notification", "preferences-system-notifications", 0, v, NULL);
   notification_cfg->cfd = cfd;
   return cfd;
}

/* local functions */
static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = NULL;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog      *cfd __UNUSED__,
           E_Config_Dialog_Data *cfdata)
{
   notification_cfg->cfd = NULL;
   E_FREE(cfdata);
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->show_low = notification_cfg->show_low;
   cfdata->show_normal = notification_cfg->show_normal;
   cfdata->show_critical = notification_cfg->show_critical;
   cfdata->timeout = notification_cfg->timeout;
   cfdata->corner = notification_cfg->corner;
   cfdata->force_timeout = notification_cfg->force_timeout;
   cfdata->ignore_replacement = notification_cfg->ignore_replacement;
   cfdata->dual_screen = notification_cfg->dual_screen;
   cfdata->time_stamp = notification_cfg->time_stamp;
   cfdata->show_app = notification_cfg->show_app;
   cfdata->show_count = notification_cfg->show_count;
   cfdata->reverse = notification_cfg->reverse;
   cfdata->menu_items = notification_cfg->menu_items;
   cfdata->item_length = notification_cfg->item_length;
   cfdata->jump_delay = notification_cfg->jump_delay;
   if (notification_cfg->blacklist)
     cfdata->blacklist = strdup(notification_cfg->blacklist);
}

static Evas_Object *
_basic_create(E_Config_Dialog      *cfd __UNUSED__,
              Evas                 *evas,
              E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o = NULL, *of = NULL, *ow = NULL, *otb = NULL;
   otb = e_widget_toolbook_add(evas, (48 * e_scale), (48 * e_scale));
   E_Radio_Group *rg;
//   E_Manager *man;

   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Urgency"), 0);
   ow = e_widget_label_add(evas, _("Levels of urgency to display:"));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Low"), &(cfdata->show_low));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Normal"), &(cfdata->show_normal));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Critical"), &(cfdata->show_critical));
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Default Timeout"), 0);
   ow = e_widget_check_add(evas, _("Force timeout for all notifications"), &(cfdata->force_timeout));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_slider_add(evas, 1, 0, _("%.1f s"), 0.0, 15.0, 0.1, 0,
                            &(cfdata->timeout), NULL, 200);
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   /* man = e_manager_current_get();
    * of = e_widget_framelist_add(evas, _("Placement"), 0);
    * ow = e_widget_slider_add(evas, 1, 0, _("%2.0f x"), 0.0, man->w, 1.0, 0,
    *                          NULL, &(cfdata->placement.x), 200);
    * e_widget_framelist_object_append(of, ow);
    * ow = e_widget_slider_add(evas, 1, 0, _("%2.0f y"), 0.0, man->h, 1.0, 0,
    *                          NULL, &(cfdata->placement.y), 200);
    * e_widget_framelist_object_append(of, ow);
    * e_widget_list_object_append(o, of, 1, 1, 0.5); */

   of = e_widget_frametable_add(evas, _("Popup Orientation"),1);
   rg = e_widget_radio_group_new(&(cfdata->corner));
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-left",
                                24, 24, CORNER_L, rg);
   e_widget_frametable_object_append(of, ow, 0, 2, 1, 1, 1, 1, 1, 1);
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-right",
                                24, 24, CORNER_R, rg);
   e_widget_frametable_object_append(of, ow, 2, 2, 1, 1, 1, 1, 1, 1);
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-top",
                                24, 24, CORNER_T, rg);
   e_widget_frametable_object_append(of, ow, 1, 0, 1, 1, 1, 1, 1, 1);
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-bottom",
                                24, 24, CORNER_B, rg);
   e_widget_frametable_object_append(of, ow, 1, 4, 1, 1, 1, 1, 1, 1);
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-top-left",
                                24, 24, CORNER_TL, rg);
   e_widget_frametable_object_append(of, ow, 0, 0, 1, 1, 1, 1, 1, 1);
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-top-right",
                                24, 24, CORNER_TR, rg);
   e_widget_frametable_object_append(of, ow, 2, 0, 1, 1, 1, 1, 1, 1);
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-bottom-left",
                                24, 24, CORNER_BL, rg);
   e_widget_frametable_object_append(of, ow, 0, 4, 1, 1, 1, 1, 1, 1);
   ow = e_widget_radio_icon_add(evas, NULL, "preferences-position-bottom-right",
                                24, 24, CORNER_BR, rg);
   e_widget_frametable_object_append(of, ow, 2, 4, 1, 1, 1, 1, 1, 1);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   /* of = e_widget_framelist_add(evas, _("Gap"), 0);
   * ow = e_widget_label_add(evas, _("Size of the gap between two popups : "));
   * e_widget_framelist_object_append(of, ow);
   * ow = e_widget_slider_add(evas, 1, 0, _("%2.0f pixels"), 0.0, 50, 1.0, 0,
   *                          NULL, &(cfdata->gap), 200);
   * e_widget_framelist_object_append(of, ow);
   * e_widget_list_object_append(o, of, 1, 1, 0.5); */
   of = e_widget_framelist_add(evas, _("Miscellaneous"), 0);
   ow = e_widget_check_add(evas, _("Ignore replace ID"), &(cfdata->ignore_replacement));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Use multiple monitor geometry"), &(cfdata->dual_screen));
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("General"), o, 1, 0, 1, 0,
                                 0.5, 0.0);

    o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("History Settings"), 0);
   ow = e_widget_check_add(evas, _("Time stamp"),  &(cfdata->time_stamp));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Show application name"),  &(cfdata->show_app));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Show notification counter"),  &(cfdata->show_count));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Reverse order"),  &(cfdata->reverse));
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Labels"), 0);
   ow = e_widget_label_add(evas, _("Labels length"));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_slider_add(evas, 1, 0, _("%2.0f"), 20, 80, 1, 0,
                             &(cfdata->item_length), NULL, 100);
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Items"), 0);
   ow = e_widget_label_add(evas, _("Items in history"));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_slider_add(evas, 1, 0, _("%2.0f"), HIST_MIN, HIST_MAX, 1, 0,
                             &(cfdata->menu_items), NULL, 100);
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Miscellaneous"), 0);
   ow = e_widget_label_add(evas, _("Gadget animation delay [s]"));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_slider_add(evas, 1, 0, _("%2.0f"), 0, 300, 1, 0,
                             &(cfdata->jump_delay), NULL, 100);
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Applications"), 0);
   ow = e_widget_label_add(evas, _("Apps blacklist (app1,app2,app3...)"));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_entry_add(evas, &cfdata->blacklist, NULL, NULL, NULL);
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   e_widget_toolbook_page_append(otb, NULL, _("History"), o, 1, 0, 1, 0,
                                 0.5, 0.0);
   e_widget_toolbook_page_show(otb, 0);
   return otb;
}

Eet_Error
truncate_menu(const unsigned int n)
{
  Eet_Error err = EET_ERROR_NONE;
  Eina_Bool ret;
  Popup_Items *items;
  
  EINA_SAFETY_ON_NULL_RETURN_VAL(notification_cfg, EET_ERROR_BAD_OBJECT);
  
  if (notification_cfg->popup_items) {
    if (eina_list_count(notification_cfg->popup_items) > n) {
      Eina_List *last, *discard, *l;
      last = eina_list_nth_list(notification_cfg->popup_items, n-1);
      notification_cfg->popup_items = eina_list_split_list(notification_cfg->popup_items, last, &discard);
      if (discard){
         EINA_LIST_FOREACH(discard, l, items) {
           ret = ecore_file_remove(items->item_icon_img);    
           if (!ret) 
              printf("Notif: Error during files removing!\n");
         }      
        E_FREE_LIST(discard, free_menu_data);
      }
      write_history(notification_cfg->popup_items);
    }
  }
  else
    err = EET_ERROR_EMPTY;
  return err;
}



static int
_basic_apply(E_Config_Dialog      *cfd __UNUSED__,
             E_Config_Dialog_Data *cfdata)
{
   if (!EINA_DBL_EQ(notification_cfg->menu_items, cfdata->menu_items))
      truncate_menu(cfdata->menu_items); 
   
   if (cfdata->jump_delay > 0)
      ecore_timer_interval_set(notification_cfg->jump_timer, cfdata->jump_delay);
   else
      ecore_timer_interval_set(notification_cfg->jump_timer, 0.1);
    
   ecore_timer_reset(notification_cfg->jump_timer);
      
   notification_cfg->show_low = cfdata->show_low;
   notification_cfg->show_normal = cfdata->show_normal;
   notification_cfg->show_critical = cfdata->show_critical;
   notification_cfg->timeout = cfdata->timeout;
   notification_cfg->corner = cfdata->corner;
   notification_cfg->force_timeout = cfdata->force_timeout;
   notification_cfg->ignore_replacement = cfdata->ignore_replacement;
   notification_cfg->dual_screen = cfdata->dual_screen;
   notification_cfg->time_stamp = cfdata->time_stamp;
   notification_cfg->show_app = cfdata->show_app;
   notification_cfg->show_count = cfdata->show_count;
   notification_cfg->reverse = cfdata->reverse;
   notification_cfg->menu_items = cfdata->menu_items;
   notification_cfg->item_length = cfdata->item_length;
   notification_cfg->jump_delay = cfdata->jump_delay;
   
   if (notification_cfg->blacklist)
     eina_stringshare_del(notification_cfg->blacklist);
   
   char *t;
   t = strdup(cfdata->blacklist);
   *t = toupper(*t);
   notification_cfg->blacklist = eina_stringshare_add(t);

   e_modapi_save(notification_mod);
   return 1;
}

