#include "e_mod_main.h"

struct _E_Config_Dialog_Data
{
   int show_label;
   int show_popup;
   int focus_window;
   int store_low;
   int store_normal;
   int store_critical;
};

static void         _ci_fill_data(Config_Item          *ci,
                                  E_Config_Dialog_Data *cfdata);
static void        *_ci_create_data(E_Config_Dialog *cfd);
static void         _ci_free_data(E_Config_Dialog      *cfd,
                                  E_Config_Dialog_Data *cfdata);
static Evas_Object *_ci_basic_create_widgets(E_Config_Dialog      *cfd,
                                             Evas                 *evas,
                                             E_Config_Dialog_Data *cfdata);
static int _ci_basic_apply_data(E_Config_Dialog      *cfd,
                                E_Config_Dialog_Data *cfdata);

void
config_notification_box_module(Config_Item *ci)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   char buf[4096];

   v = E_NEW(E_Config_Dialog_View, 1);

   /* Dialog Methods */
   v->create_cfdata = _ci_create_data;
   v->free_cfdata = _ci_free_data;
   v->basic.apply_cfdata = _ci_basic_apply_data;
   v->basic.create_widgets = _ci_basic_create_widgets;

   /* Create The Dialog */
   snprintf(buf, sizeof(buf), "%s/e-module-notification.edj", e_module_dir_get(notification_mod));
   cfd = e_config_dialog_new(e_container_current_get(e_manager_current_get()),
                             D_("Notification Box Configuration"),
                             "E", "_e_mod_notification_box_config_dialog",
                             buf, 0, v, ci);
   notification_cfg->config_dialog = eina_list_append(notification_cfg->config_dialog, cfd);
}

static void
_ci_fill_data(Config_Item          *ci,
              E_Config_Dialog_Data *cfdata)
{
   cfdata->show_label = ci->show_label;
   cfdata->show_popup = ci->show_popup;
   cfdata->focus_window = ci->focus_window;
   cfdata->store_low = ci->store_low;
   cfdata->store_normal = ci->store_normal;
   cfdata->store_critical = ci->store_critical;
}

static void *
_ci_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;
   Config_Item *ci;

   ci = cfd->data;
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _ci_fill_data(ci, cfdata);
   return cfdata;
}

static void
_ci_free_data(E_Config_Dialog      *cfd,
              E_Config_Dialog_Data *cfdata)
{
   notification_cfg->config_dialog = eina_list_remove(notification_cfg->config_dialog, cfd);
   free(cfdata);
}

static Evas_Object *
_ci_basic_create_widgets(E_Config_Dialog      *cfd __UNUSED__,
                         Evas                 *evas,
                         E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, D_("General Settings"), 0);
   ob = e_widget_check_add(evas, D_("Show Icon Label"), &(cfdata->show_label));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, D_("Show the popup on mouse over"), &(cfdata->show_popup));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, D_("Focus the source window when clicking"), &(cfdata->focus_window));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, D_("Urgency"), 0);
   ob = e_widget_label_add(evas, D_("Levels of urgency to store:"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, D_("Low"), &(cfdata->store_low));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, D_("Normal"), &(cfdata->store_normal));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, D_("Critical"), &(cfdata->store_critical));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static int
_ci_basic_apply_data(E_Config_Dialog      *cfd,
                     E_Config_Dialog_Data *cfdata)
{
   Config_Item *ci;

   ci = cfd->data;
   ci->show_label = cfdata->show_label;
   ci->show_popup = cfdata->show_popup;
   ci->focus_window = cfdata->focus_window;
   ci->store_low = cfdata->store_low;
   ci->store_normal = cfdata->store_normal;
   ci->store_critical = cfdata->store_critical;

   e_config_save_queue();
   return 1;
}

