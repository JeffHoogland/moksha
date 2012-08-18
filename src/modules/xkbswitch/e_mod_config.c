#include "e.h"
#include "e_mod_main.h"
#include "e_mod_parse.h"

struct _E_Config_Dialog_Data
{
   Evas *evas, *dlg_evas;
   Evas_Object *layout_list, *used_list;
   Evas_Object *dmodel_list, *model_list, *variant_list;
   Evas_Object *btn_add, *btn_del, *btn_up, *btn_down;
   Ecore_Timer *fill_delay;
   Ecore_Timer *dlg_fill_delay;

   Eina_List  *cfg_layouts;
   Eina_List  *cfg_options;
   const char *default_model;

   int only_label;

   E_Dialog *dlg_add_new;
};

typedef struct _E_XKB_Dialog_Option
{
   int enabled;
   const char *name;
} E_XKB_Dialog_Option;

/* Local prototypes */

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

static void _cb_add(void *data, void *data2 __UNUSED__);
static void _cb_del(void *data, void *data2 __UNUSED__);

static void _cb_up(void *data, void *data2 __UNUSED__);
static void _cb_dn(void *data, void *data2 __UNUSED__);

static void _dlg_add_cb_ok(void *data, E_Dialog *dlg);
static void _dlg_add_cb_cancel(void *data, E_Dialog *dlg);

static E_Dialog *_dlg_add_new(E_Config_Dialog_Data *cfdata);

static void _dlg_add_cb_del(void *obj);

static Eina_Bool _cb_dlg_fill_delay(void *data);

static void _cb_layout_select(void *data);
static void _cb_used_select  (void *data);

static Eina_Bool _cb_fill_delay(void *data);

/* Externals */

E_Config_Dialog *
_xkb_cfg_dialog(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "keyboard_and_mouse/xkbswitch"))
     return NULL;
   if (!(v = E_NEW(E_Config_Dialog_View, 1))) return NULL;

   v->create_cfdata        = _create_data;
   v->free_cfdata          = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata   = _basic_apply;

   cfd = e_config_dialog_new(con, _("Keyboard Settings"), "E",
                             "keyboard_and_mouse/xkbswitch",
                             "preferences-desktop-keyboard",
                             0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
   _xkb.cfd = cfd;
   return cfd;
}

/* Locals */

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *l, *ll, *lll;
   E_Config_XKB_Layout *cl, *nl;
   E_XKB_Dialog_Option *od;
   E_XKB_Option *op;
   E_XKB_Option_Group *gr;

   find_rules();
   parse_rules(); /* XXX: handle in case nothing was found? */

   cfdata = E_NEW(E_Config_Dialog_Data, 1);

   cfdata->cfg_layouts = NULL;
   EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, cl)
     {
        nl          = E_NEW(E_Config_XKB_Layout, 1);
        nl->name    = eina_stringshare_add(cl->name);
        nl->model   = eina_stringshare_add(cl->model);
        nl->variant = eina_stringshare_add(cl->variant);

        cfdata->cfg_layouts = eina_list_append(cfdata->cfg_layouts, nl);
     }

   /* Initialize options */

   cfdata->only_label  = e_config->xkb.only_label;
   cfdata->cfg_options = NULL;

   lll = e_config->xkb.used_options;
   EINA_LIST_FOREACH(optgroups, l, gr)
     {
        EINA_LIST_FOREACH(gr->options, ll, op)
          {
             od = E_NEW(E_XKB_Dialog_Option, 1);
             od->name = eina_stringshare_add(op->name);
             if (lll &&
                 (od->name == ((E_Config_XKB_Option *)
                               eina_list_data_get(lll))->name))
               {
                  od->enabled = 1;
                  lll = eina_list_next(lll);
               }
             else od->enabled = 0;
             cfdata->cfg_options = eina_list_append(cfdata->cfg_options, od);
          }
     }

   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_Config_XKB_Layout *cl;
   E_XKB_Dialog_Option *od;

   _xkb.cfd = NULL;

   EINA_LIST_FREE(cfdata->cfg_layouts, cl)
     {
        eina_stringshare_del(cl->name);
        eina_stringshare_del(cl->model);
        eina_stringshare_del(cl->variant);
        E_FREE(cl);
     }

   EINA_LIST_FREE(cfdata->cfg_options, od)
     {
        eina_stringshare_del(od->name);
        E_FREE(od);
     }

   eina_stringshare_del(cfdata->default_model);
   E_FREE(cfdata);
   clear_rules();
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Eina_List *l;
   E_Config_XKB_Layout *cl, *nl;
   E_Config_XKB_Option *oc;
   E_XKB_Dialog_Option *od;

   EINA_LIST_FREE(e_config->xkb.used_layouts, cl)
     {
        eina_stringshare_del(cl->name);
        eina_stringshare_del(cl->model);
        eina_stringshare_del(cl->variant);
        E_FREE(cl);
     }

   EINA_LIST_FOREACH(cfdata->cfg_layouts, l, cl)
     {
        nl          = E_NEW(E_Config_XKB_Layout, 1);
        nl->name    = eina_stringshare_add(cl->name);
        nl->model   = eina_stringshare_add(cl->model);
        nl->variant = eina_stringshare_add(cl->variant);

        e_config->xkb.used_layouts =
          eina_list_append(e_config->xkb.used_layouts, nl);
     }

   if (e_config->xkb.default_model)
     eina_stringshare_del(e_config->xkb.default_model);

   e_config->xkb.default_model = eina_stringshare_add(cfdata->default_model);

   /* Save options */
   e_config->xkb.only_label = cfdata->only_label;

   EINA_LIST_FREE(e_config->xkb.used_options, oc)
     {
        eina_stringshare_del(oc->name);
        E_FREE(oc);
     }

   EINA_LIST_FOREACH(cfdata->cfg_options, l, od)
     {
        if (!od->enabled) continue;

        oc = E_NEW(E_Config_XKB_Option, 1);
        oc->name = eina_stringshare_add(od->name);
        e_config->xkb.used_options = eina_list_append(e_config->xkb.used_options, oc);
    }

   e_xkb_update(-1);
   _xkb_update_icon(0);

   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* Holds the dialog contents, displays a toolbar on the top */
   Evas_Object *mainn = e_widget_toolbook_add(evas, 24, 24);
     {
        /* Holds the used layouts ilist and the button table */
        Evas_Object *layoutss;
        Evas_Object *modelss;
        Evas_Object *options;

        layoutss = e_widget_list_add(evas, 0, 0);
          {
             /* Holds the used layouts */
             Evas_Object *configs;
             Evas_Object *buttons;

             configs = e_widget_ilist_add(evas, 32, 32, NULL);

               {
                  e_widget_size_min_set(configs, 220, 160);
                  e_widget_ilist_go(configs);

                  e_widget_list_object_append(layoutss, configs, 1, 1, 0.5);
                  cfdata->used_list = configs;
               }

             /* Holds the buttons */
             buttons = e_widget_table_add(evas, 1);
               {
                  cfdata->btn_up = e_widget_button_add(evas, _("Up"), "go-up", _cb_up, cfdata, NULL);
                    {
                       e_widget_disabled_set(cfdata->btn_up, EINA_TRUE);
                       e_widget_table_object_append(buttons, cfdata->btn_up, 0, 0, 1, 1, 1, 1, 1, 0);
                    }

                  cfdata->btn_down = e_widget_button_add(evas, _("Down"), "go-down", _cb_dn, cfdata, NULL);
                    {
                       e_widget_disabled_set(cfdata->btn_down, EINA_TRUE);
                       e_widget_table_object_append(buttons, cfdata->btn_down, 1, 0, 1, 1, 1, 1, 1, 0);
                    }

                  cfdata->btn_add = e_widget_button_add(evas, _("Add"), "list-add", _cb_add, cfdata, NULL);
                    {
                       e_widget_table_object_append(buttons, cfdata->btn_add, 0, 1, 1, 1, 1, 1, 1, 0);
                    }

                  cfdata->btn_del = e_widget_button_add(evas, _("Remove"), "list-remove", _cb_del, cfdata, NULL);
                    {
                       e_widget_disabled_set(cfdata->btn_del,  EINA_TRUE);
                       e_widget_table_object_append(buttons, cfdata->btn_del, 1, 1, 1, 1, 1, 1, 1, 0);
                    }

                  e_widget_list_object_append(layoutss, buttons, 1, 0, 1);
               }

             e_widget_toolbook_page_append(mainn, NULL, _("Configurations"), layoutss, 1, 1, 1, 1, 0.5, 0.0);
          }

        /* Holds the default models */
        modelss = e_widget_ilist_add(evas, 32, 32, &cfdata->default_model);
          {
             e_widget_size_min_set(modelss, 220, 160);
             cfdata->dmodel_list = modelss;

             e_widget_toolbook_page_append(mainn, NULL, _("Models"), modelss, 1, 1, 1, 1, 0.5, 0.0);
          }

        /* Holds the options */
        options = e_widget_list_add(evas, 0, 0);
          {
             E_XKB_Option *option;
             E_XKB_Option_Group *group;
             Eina_List *l, *ll, *lll;
             Evas_Coord mw, mh;
             Evas_Object *general;
             Evas_Object *scroller;

             general =  e_widget_framelist_add(evas, _("Gadgets"), 0);
               {
                  Evas_Object *only_label = e_widget_check_add(evas, _("Label only"), &(cfdata->only_label));
                    {
                       e_widget_framelist_object_append(general, only_label);
                    }
                  e_widget_list_object_append(options, general, 1, 1, 0.0);
               }

             lll  = cfdata->cfg_options;

             EINA_LIST_FOREACH(optgroups, l, group)
               {
                  Evas_Object *grp = e_widget_framelist_add(evas, group->description, 0);

                  EINA_LIST_FOREACH(group->options, ll, option)
                    {
                       Evas_Object *chk = e_widget_check_add(evas, option->description,
                                                             &(((E_XKB_Dialog_Option *)
                                                                eina_list_data_get(lll))->enabled));
                       e_widget_framelist_object_append(grp, chk);
                       lll = eina_list_next(lll);
                    }
                  e_widget_list_object_append(options, grp, 1, 1, 0.0);
               }

             e_widget_size_min_get(options, &mw, &mh);

             if (mw < 220) mw = 220;
             if (mh < 160) mh = 160;

             evas_object_resize(options, mw, mh);

             scroller = e_widget_scrollframe_simple_add(evas, options);
             e_widget_size_min_set(scroller, 220, 160);

             e_widget_toolbook_page_append(mainn, NULL, _("Options"), scroller, 1, 1, 1, 1, 0.5, 0.0);
          }

        /* Display the first page by default */
        e_widget_toolbook_page_show(mainn, 0);
     }

   /* The main evas */
   cfdata->evas = evas;

   /* Clear up any previous timer */
   if (cfdata->fill_delay)
     ecore_timer_del(cfdata->fill_delay);

   /* Trigger the fill */
   cfdata->fill_delay = ecore_timer_add(0.2, _cb_fill_delay, cfdata);

   return mainn;
}

static void
_cb_add(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   if (!(cfdata = data)) return;

   if (cfdata->dlg_add_new) e_win_raise(cfdata->dlg_add_new->win);
   else cfdata->dlg_add_new = _dlg_add_new(cfdata);
}

static void
_cb_del(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   int n = 0;

   if (!(cfdata = data)) return;
   if ((n = e_widget_ilist_selected_get(cfdata->used_list)) < 0) return;

   cfdata->cfg_layouts = eina_list_remove_list(cfdata->cfg_layouts, eina_list_nth_list(cfdata->cfg_layouts, n));

   /* Update the list */
   evas_event_freeze(cfdata->evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->used_list);
   e_widget_ilist_remove_num(cfdata->used_list, n);
   e_widget_ilist_go(cfdata->used_list);
   e_widget_ilist_thaw(cfdata->used_list);
   edje_thaw();
   evas_event_thaw(cfdata->evas);
}

static void
_cb_up(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   void *nddata;
   Evas_Object *ic;
   Eina_List *l;
   const char *lbl;
   int n;

   if (!(cfdata = data)) return;
   if ((n = e_widget_ilist_selected_get(cfdata->used_list)) < 0) return;

   l = eina_list_nth_list(cfdata->cfg_layouts, n);

   nddata = eina_list_data_get(eina_list_prev(l));
   eina_list_data_set(eina_list_prev(l), eina_list_data_get(l));
   eina_list_data_set(l, nddata);

   /* Update the list */

   evas_event_freeze(cfdata->evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->used_list);

   ic = e_icon_add(cfdata->evas);
   e_icon_file_set(ic, e_icon_file_get(e_widget_ilist_nth_icon_get(cfdata->used_list, n)));
   lbl = e_widget_ilist_nth_label_get(cfdata->used_list, n);
   e_widget_ilist_prepend_relative_full(cfdata->used_list, ic, NULL, lbl, _cb_used_select, cfdata, NULL, (n - 1));
   e_widget_ilist_remove_num(cfdata->used_list, n);

   e_widget_ilist_go(cfdata->used_list);
   e_widget_ilist_thaw(cfdata->used_list);
   edje_thaw();
   evas_event_thaw(cfdata->evas);

   e_widget_ilist_selected_set(cfdata->used_list, (n - 1));
}

static void
_cb_dn(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   void *nddata;
   Evas_Object *ic;
   Eina_List *l;
   const char *lbl;
   int n;

   if (!(cfdata = data)) return;
   if ((n = e_widget_ilist_selected_get(cfdata->used_list)) < 0) return;

   l = eina_list_nth_list(cfdata->cfg_layouts, n);

   nddata = eina_list_data_get(eina_list_next(l));
   eina_list_data_set(eina_list_next(l), eina_list_data_get(l));
   eina_list_data_set(l, nddata);

   /* Update the list */

   evas_event_freeze(cfdata->evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->used_list);

   ic = e_icon_add(cfdata->evas);
   e_icon_file_set(ic, e_icon_file_get(e_widget_ilist_nth_icon_get(cfdata->used_list, n)));
   lbl = e_widget_ilist_nth_label_get(cfdata->used_list, n);
   e_widget_ilist_append_relative_full(cfdata->used_list, ic, NULL, lbl, _cb_used_select, cfdata, NULL, n);
   e_widget_ilist_remove_num(cfdata->used_list, n);

   e_widget_ilist_go(cfdata->used_list);
   e_widget_ilist_thaw(cfdata->used_list);
   edje_thaw();
   evas_event_thaw(cfdata->evas);

   e_widget_ilist_selected_set(cfdata->used_list, (n + 1));
}

static E_Dialog *
_dlg_add_new(E_Config_Dialog_Data *cfdata)
{
   E_Dialog *dlg;
   Evas *evas;
   Evas_Coord mw, mh;
   Evas_Object *mainn;

   if (!(dlg = e_dialog_new(_xkb.cfd->con, "E", "xkbswitch_config_add_dialog"))) return NULL;

   dlg->data = cfdata;

   e_object_del_attach_func_set(E_OBJECT(dlg), _dlg_add_cb_del);
   e_win_centered_set(dlg->win, 1);

   evas = e_win_evas_get(dlg->win);
   e_dialog_title_set(dlg, _("Add New Configuration"));

   /* The main toolbook, holds the lists and tabs */
   mainn = e_widget_toolbook_add(evas, 24, 24);
     {
        /* Holds the available layouts */
        Evas_Object *available = e_widget_ilist_add(evas, 32, 32, NULL);
        Evas_Object *modelss;
        Evas_Object *variants;

          {
             e_widget_size_min_set(available, 220, 160);
             e_widget_ilist_go(available);
             e_widget_toolbook_page_append(mainn, NULL, _("Available"), available, 1, 1, 1, 1, 0.5, 0.0);
             cfdata->layout_list = available;
          }

        /* Holds the available models */
        modelss = e_widget_ilist_add(evas, 32, 32, NULL);
          {
             e_widget_toolbook_page_append(mainn, NULL, _("Model"), modelss, 1, 1, 1, 1, 0.5, 0.0);
             cfdata->model_list = modelss;
        }

        /* Holds the available variants */
        variants = e_widget_ilist_add(evas, 32, 32, NULL);
          {
             e_widget_toolbook_page_append(mainn, NULL, _("Variant"), variants, 1, 1, 1, 1, 0.5, 0.0);
             cfdata->variant_list = variants;
          }
        e_widget_toolbook_page_show(mainn, 0);
     }

   e_widget_size_min_get(mainn, &mw, &mh);
   e_dialog_content_set(dlg, mainn, mw,  mh);

   cfdata->dlg_evas = evas;

   /* Clear up any previous timer */
   if (cfdata->dlg_fill_delay) ecore_timer_del(cfdata->dlg_fill_delay);

   /* Trigger the fill */
   cfdata->dlg_fill_delay = ecore_timer_add(0.2, _cb_dlg_fill_delay, cfdata);

   /* Some buttons */
   e_dialog_button_add(dlg, _("OK"), NULL, _dlg_add_cb_ok, cfdata);
   e_dialog_button_add(dlg, _("Cancel"), NULL, _dlg_add_cb_cancel, cfdata);

   e_dialog_button_disable_num_set(dlg, 0, 1);
   e_dialog_button_disable_num_set(dlg, 1, 0);

   e_dialog_resizable_set(dlg, 1);
   e_dialog_show(dlg);

   return dlg;
}

static void
_dlg_add_cb_ok(void *data __UNUSED__, E_Dialog *dlg)
{
   E_Config_Dialog_Data *cfdata = dlg->data;
   E_Config_XKB_Layout  *cl;
   char buf[PATH_MAX];
   /* Configuration information */
   const char *layout = e_widget_ilist_selected_value_get(cfdata->layout_list);
   const char *model = e_widget_ilist_selected_value_get(cfdata->model_list);
   const char *variant = e_widget_ilist_selected_value_get(cfdata->variant_list);

   /* The new configuration */
   cl          = E_NEW(E_Config_XKB_Layout, 1);
   cl->name    = eina_stringshare_add(layout);
   cl->model   = eina_stringshare_add(model);
   cl->variant = eina_stringshare_add(variant);

   cfdata->cfg_layouts = eina_list_append(cfdata->cfg_layouts, cl);

   /* Update the main list */
   evas_event_freeze(cfdata->evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->used_list);

     {
        Evas_Object *ic = e_icon_add(cfdata->evas);
        const char *name = cl->name;

        e_xkb_e_icon_flag_setup(ic, name);
        snprintf(buf, sizeof(buf), "%s (%s, %s)",
                 cl->name, cl->model, cl->variant);
        e_widget_ilist_append_full(cfdata->used_list, ic, NULL, buf,
                                   _cb_used_select, cfdata, NULL);
     }

   e_widget_ilist_go  (cfdata->used_list);
   e_widget_ilist_thaw(cfdata->used_list);
   edje_thaw();
   evas_event_thaw(cfdata->evas);

   cfdata->dlg_add_new = NULL;
   e_object_unref(E_OBJECT(dlg));
}

static void
_dlg_add_cb_cancel(void *data __UNUSED__, E_Dialog *dlg)
{
   E_Config_Dialog_Data *cfdata = dlg->data;
   cfdata->dlg_add_new = NULL;
   e_object_unref(E_OBJECT(dlg));
}

static void
_dlg_add_cb_del(void *obj)
{
   E_Dialog *dlg = obj;
   E_Config_Dialog_Data *cfdata = dlg->data;
   cfdata->dlg_add_new = NULL;
   e_object_unref(E_OBJECT(dlg));
}

static Eina_Bool
_cb_dlg_fill_delay(void *data)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *l;
   E_XKB_Layout *layout;
   char buf[PATH_MAX];

   if (!(cfdata = data)) return ECORE_CALLBACK_RENEW;

   /* Update the list of available layouts */
   evas_event_freeze(cfdata->dlg_evas);
   edje_freeze();

   e_widget_ilist_freeze(cfdata->layout_list);
   e_widget_ilist_clear (cfdata->layout_list);

   EINA_LIST_FOREACH(layouts, l, layout)
     {
        Evas_Object *ic = e_icon_add(cfdata->dlg_evas);
        const char *name = layout->name;

        e_xkb_e_icon_flag_setup(ic, name);
        snprintf(buf, sizeof(buf), "%s (%s)",
                 layout->description, layout->name);
        e_widget_ilist_append_full(cfdata->layout_list, ic, NULL, buf,
                                   _cb_layout_select, cfdata, layout->name);
     }

   e_widget_ilist_go  (cfdata->layout_list);
   e_widget_ilist_thaw(cfdata->layout_list);

   edje_thaw();
   evas_event_thaw(cfdata->dlg_evas);

   cfdata->dlg_fill_delay = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_cb_layout_select(void *data)
{
   E_Config_Dialog_Data *cfdata;
   E_XKB_Variant *variant;
   E_XKB_Layout *layout;
   E_XKB_Model *model;
   Eina_List *l;
   const char *label;
   int n;
   char buf[PATH_MAX];

   if (!(cfdata = data)) return;

   /* Find the right layout */

   if ((n = e_widget_ilist_selected_get(cfdata->layout_list)) < 0)
     return;
   if (!(label = e_widget_ilist_nth_label_get(cfdata->layout_list, n)))
     return;

   if (!(layout = eina_list_search_unsorted
         (layouts, layout_sort_by_name_cb,
             e_widget_ilist_nth_value_get(cfdata->layout_list, n)
         ))) return;

   /* Update the lists */
   evas_event_freeze(cfdata->dlg_evas);
   edje_freeze();

   /* Models */
   e_widget_ilist_freeze(cfdata->model_list);
   e_widget_ilist_clear(cfdata->model_list);

   EINA_LIST_FOREACH(models, l, model)
     {
        snprintf(buf, sizeof(buf), "%s (%s)", model->description, model->name);
        e_widget_ilist_append(cfdata->model_list, NULL, buf, NULL, cfdata, model->name);
     }

   e_widget_ilist_go(cfdata->model_list);
   e_widget_ilist_thaw(cfdata->model_list);

   /* Variants */
   e_widget_ilist_freeze(cfdata->variant_list);
   e_widget_ilist_clear(cfdata->variant_list);

   EINA_LIST_FOREACH(layout->variants, l, variant)
     {
        snprintf(buf, sizeof(buf),  "%s (%s)", variant->name, variant->description);
        e_widget_ilist_append(cfdata->variant_list, NULL, buf, NULL, cfdata, variant->name);
     }

   e_widget_ilist_go(cfdata->variant_list);
   e_widget_ilist_thaw(cfdata->variant_list);

   edje_thaw();
   evas_event_thaw(cfdata->dlg_evas);

   e_widget_ilist_selected_set(cfdata->model_list,   0);
   e_widget_ilist_selected_set(cfdata->variant_list, 0);

   e_dialog_button_disable_num_set(cfdata->dlg_add_new, 0, 0);
}

static Eina_Bool
_cb_fill_delay(void *data)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *l;
   E_Config_XKB_Layout *cl;
   E_XKB_Model *model;
   int n = 0;
   char buf[PATH_MAX];

   if (!(cfdata = data)) return ECORE_CALLBACK_RENEW;

   /* Update the list of used layouts */
   evas_event_freeze(cfdata->evas);
   edje_freeze();

   e_widget_ilist_freeze(cfdata->used_list);
   e_widget_ilist_clear(cfdata->used_list);

   EINA_LIST_FOREACH(cfdata->cfg_layouts, l, cl)
     {
        Evas_Object *ic = e_icon_add(cfdata->evas);
        const char *name = cl->name;

        e_xkb_e_icon_flag_setup(ic, name);
        snprintf(buf, sizeof(buf), "%s (%s, %s)",
                 cl->name, cl->model, cl->variant);
        e_widget_ilist_append_full(cfdata->used_list, ic, NULL, buf,
                                   _cb_used_select, cfdata, NULL);
    }

   e_widget_ilist_go(cfdata->used_list);
   e_widget_ilist_thaw(cfdata->used_list);

   e_widget_ilist_freeze(cfdata->dmodel_list);
   e_widget_ilist_clear(cfdata->dmodel_list);

   /* Update the global model list */
   EINA_LIST_FOREACH(models, l, model)
     {
        snprintf(buf, sizeof(buf), "%s (%s)", model->description, model->name);
        e_widget_ilist_append(cfdata->dmodel_list, NULL, buf, NULL,
                              cfdata, model->name);
        if (model->name == e_config->xkb.default_model)
          e_widget_ilist_selected_set(cfdata->dmodel_list, n);
        n++;
     }

   e_widget_ilist_go(cfdata->dmodel_list);
   e_widget_ilist_thaw(cfdata->dmodel_list);
   edje_thaw();
   evas_event_thaw(cfdata->evas);

   cfdata->fill_delay = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_cb_used_select(void *data)
{
   E_Config_Dialog_Data *cfdata;
   int n, c;

   if (!(cfdata = data)) return;
   if ((n = e_widget_ilist_selected_get(cfdata->used_list)) < 0) return;

   c = e_widget_ilist_count(cfdata->used_list);
   e_widget_disabled_set(cfdata->btn_del, EINA_FALSE);

   if (n == (c - 1))
     {
        e_widget_disabled_set(cfdata->btn_up,   EINA_FALSE);
        e_widget_disabled_set(cfdata->btn_down, EINA_TRUE );
     }
   else if (n == 0)
     {
        e_widget_disabled_set(cfdata->btn_up,   EINA_TRUE );
        e_widget_disabled_set(cfdata->btn_down, EINA_FALSE);
     }
   else
     {
        e_widget_disabled_set(cfdata->btn_up,   EINA_FALSE);
        e_widget_disabled_set(cfdata->btn_down, EINA_FALSE);
     }
}
