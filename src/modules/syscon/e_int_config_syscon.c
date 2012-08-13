#include "e.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data
{
   struct
   {
      int icon_size;
   } main, secondary, extra;
   double     timeout;
   int        do_input;
   Eina_List *actions;

   struct
   {
      Eina_List   *disable_iconified;
      Eina_List   *disable_scroll_animate;
      Eina_List   *disable_warp;
      Evas_Object *min_w, *min_h;
   } gui;
};

E_Config_Dialog *
e_int_config_syscon(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "windows/conf_syscon")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;
   v->basic.check_changed = _basic_check_changed;

   cfd = e_config_dialog_new(con, _("Syscon Settings"),
                             "E", "windows/conf_syscon",
                             "system-shutdown", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   Eina_List *l;
   E_Config_Syscon_Action *sa, *sa2;

   cfdata->main.icon_size = e_config->syscon.main.icon_size;
   cfdata->secondary.icon_size = e_config->syscon.secondary.icon_size;
   cfdata->extra.icon_size = e_config->syscon.extra.icon_size;
   cfdata->timeout = e_config->syscon.timeout;
   cfdata->do_input = e_config->syscon.do_input;
   EINA_LIST_FOREACH(e_config->syscon.actions, l, sa)
     {
        sa2 = E_NEW(E_Config_Syscon_Action, 1);
        if (sa->action) sa2->action = strdup(sa->action);
        if (sa->params) sa2->params = strdup(sa->params);
        if (sa->button) sa2->button = strdup(sa->button);
        if (sa->icon) sa2->icon = strdup(sa->icon);
        sa2->is_main = sa->is_main;
        cfdata->actions = eina_list_append(cfdata->actions, sa2);
     }
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_Config_Syscon_Action *sa;

   EINA_LIST_FREE(cfdata->actions, sa)
     {
        if (sa->action) free((char *)sa->action);
        if (sa->params) free((char *)sa->params);
        if (sa->button) free((char *)sa->button);
        if (sa->icon) free((char *)sa->icon);
        E_FREE(sa);
     }
   E_FREE(cfdata);
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Eina_List *l;
   E_Config_Syscon_Action *sa, *sa2;

   e_config->syscon.main.icon_size = cfdata->main.icon_size;
   e_config->syscon.secondary.icon_size = cfdata->secondary.icon_size;
   e_config->syscon.extra.icon_size = cfdata->extra.icon_size;
   e_config->syscon.timeout = cfdata->timeout;
   e_config->syscon.do_input = cfdata->do_input;
   EINA_LIST_FREE(e_config->syscon.actions, sa)
     {
        if (sa->action) eina_stringshare_del(sa->action);
        if (sa->params) eina_stringshare_del(sa->params);
        if (sa->button) eina_stringshare_del(sa->button);
        if (sa->icon) eina_stringshare_del(sa->icon);
        E_FREE(sa);
     }
   EINA_LIST_FOREACH(cfdata->actions, l, sa)
     {
        sa2 = E_NEW(E_Config_Syscon_Action, 1);
        if (sa->action) sa2->action = eina_stringshare_add(sa->action);
        if (sa->params) sa2->params = eina_stringshare_add(sa->params);
        if (sa->button) sa2->button = eina_stringshare_add(sa->button);
        if (sa->icon) sa2->icon = eina_stringshare_add(sa->icon);
        sa2->is_main = sa->is_main;
        e_config->syscon.actions = eina_list_append(e_config->syscon.actions, sa2);
     }
   e_config_save_queue();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata __UNUSED__)
{
   return 1;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *otb, *ol, *ob;

   otb = e_widget_toolbook_add(evas, (48 * e_scale), (48 * e_scale));

   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_label_add(evas, _("Main"));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 16.0, 256.0, 1.0, 0,
                            NULL, &(cfdata->main.icon_size), 100);
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_label_add(evas, _("Secondary"));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 16.0, 256.0, 1.0, 0,
                            NULL, &(cfdata->secondary.icon_size), 100);
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_label_add(evas, _("Extra"));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 16.0, 256.0, 1.0, 0,
                            NULL, &(cfdata->extra.icon_size), 100);
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   e_widget_toolbook_page_append(otb, NULL, _("Icon Sizes"), ol,
                                 0, 0, 1, 0, 0.5, 0.0);

   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_check_add(evas, _("Do default action after timeout"), &(cfdata->do_input));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_label_add(evas, _("Timeout"));
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.1f"), 0.0, 60.0, 0.1, 0,
                            &(cfdata->timeout), NULL, 100);
   e_widget_list_object_append(ol, ob, 1, 0, 0.0);
   e_widget_toolbook_page_append(otb, NULL, _("Default Action"), ol,
                                 0, 0, 1, 0, 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);

   return otb;
}

