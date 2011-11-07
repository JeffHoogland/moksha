#include "e.h"

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;

   Eina_List *env_vars;
   
   char *var_str, *val_str;
   int unset;
   
   struct {
      Evas_Object *var_en, *val_en, *unset, *list;
   } gui;
};

E_Config_Dialog *
e_int_config_env(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "advanced/environment_variables")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->basic.create_widgets    = _basic_create_widgets;
   v->basic.apply_cfdata      = _basic_apply_data;
   
   cfd = e_config_dialog_new(con, _("Environment-variables"),
			     "E", "advanced/environment_variables",
			     "preferences-system", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   Eina_List *l;
   E_Config_Env_Var *evr, *evr2;
   
   EINA_LIST_FOREACH(e_config->env_vars, l, evr)
     {
        evr2 = E_NEW(E_Config_Env_Var, 1);
        evr2->var = eina_stringshare_add(evr->var);
        if (evr->val) evr2->val = eina_stringshare_add(evr->val);
        evr2->unset = evr->unset;
        cfdata->env_vars = eina_list_append(cfdata->env_vars, evr2);
     }
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_Config_Env_Var *evr;
   
   EINA_LIST_FREE(cfdata->env_vars, evr)
     {
        eina_stringshare_del(evr->var);
        if (evr->val) eina_stringshare_del(evr->val);
        E_FREE(evr);
     }
   if (cfdata->var_str) free(cfdata->var_str);
   if (cfdata->val_str) free(cfdata->val_str);
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Eina_List *l, *l2;
   E_Config_Env_Var *evr, *evr2;
   int same;
   
   // old env vars removed from new set - unset
   EINA_LIST_FOREACH(e_config->env_vars, l, evr)
     {
        same = 0;
        EINA_LIST_FOREACH(cfdata->env_vars, l2, evr2)
          {
             if (!strcmp(evr->var, evr2->var))
               {
                  same = 1;
                  break;
               }
          }
        if (!same) e_env_unset(evr->var);
     }
   EINA_LIST_FREE(e_config->env_vars, evr)
     {
        eina_stringshare_del(evr->var);
        if (evr->val) eina_stringshare_del(evr->val);
        E_FREE(evr);
     }
   EINA_LIST_FOREACH(cfdata->env_vars, l, evr)
     {
        evr2 = E_NEW(E_Config_Env_Var, 1);
        
        evr2->var = eina_stringshare_add(evr->var);
        if (evr->val) evr2->val = eina_stringshare_add(evr->val);
        evr2->unset = evr->unset;
        e_config->env_vars = eina_list_append(e_config->env_vars, evr2);
     }
   // set all env vars (or unset as needed)
   EINA_LIST_FOREACH(e_config->env_vars, l, evr)
     {
        if (evr->unset)
           e_env_unset(evr->var);
        else
           e_env_set(evr->var, evr->val);
     }
   
   e_config_save_queue();
   return 1;
}

static void
_sel_cb(void *data)
{
   E_Config_Dialog_Data *cfdata = data;
   int sel_n = e_widget_ilist_selected_get(cfdata->gui.list);
   E_Config_Env_Var *evr = eina_list_nth(cfdata->env_vars, sel_n);
   if (!evr) return;
   e_widget_check_checked_set(cfdata->gui.unset, evr->unset);
   e_widget_disabled_set(cfdata->gui.val_en, evr->unset);
   e_widget_entry_text_set(cfdata->gui.var_en, evr->var);
   if ((evr->val) && (!evr->unset))
      e_widget_entry_text_set(cfdata->gui.val_en, evr->val);
   else
      e_widget_entry_text_set(cfdata->gui.val_en, "");
}

static void
_add_cb(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   Eina_List *l;
   E_Config_Env_Var *evr = NULL;
   int i, sel = -1;
   
   if (!cfdata->var_str) return;
   
   i = 0;
   EINA_LIST_FOREACH(cfdata->env_vars, l, evr)
     {
        if (!strcmp(cfdata->var_str, evr->var))
          {
             sel = i;
             break;
          }
        evr = NULL;
        i++;
     }
   if (!evr) // new
     {
        evr = E_NEW(E_Config_Env_Var, 1);
        if (evr)
          {
             evr->var = eina_stringshare_add(cfdata->var_str);
             if (cfdata->unset)
                cfdata->unset = 1;
             else
               {
                  if (cfdata->val_str)
                     evr->val = eina_stringshare_add(cfdata->val_str);
                  cfdata->unset = 0;
               }
             cfdata->env_vars = eina_list_append(cfdata->env_vars, evr);
             e_widget_ilist_append(cfdata->gui.list, NULL, 
                                   evr->var, _sel_cb, cfdata, NULL);
             e_widget_ilist_go(cfdata->gui.list);
             sel = e_widget_ilist_count(cfdata->gui.list) - 1;
             e_widget_ilist_selected_set(cfdata->gui.list, sel);
             e_widget_ilist_nth_show(cfdata->gui.list, sel, 0);
          }
     }
   else // modify
     {
        if (evr->val) eina_stringshare_del(evr->val);
        evr->val = NULL;
        if (cfdata->unset)
           evr->unset = 1;
        else
          {
             if (cfdata->val_str)
                evr->val = eina_stringshare_add(cfdata->val_str);
             evr->unset = 0;
          }
        if (sel >= 0)
          {
             e_widget_ilist_selected_set(cfdata->gui.list, sel);
             e_widget_ilist_nth_show(cfdata->gui.list, sel, 0);
          }
     }
}

static void
_del_cb(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   Eina_List *l;
   E_Config_Env_Var *evr = NULL;
/*
   int i, sel = -1;
   
   EINA_LIST_FOREACH(cfdata->env_vars, l, evr)
     {
        if (!strcmp(cfdata->var_str, evr->var))
          {
             sel = i;
             break;
          }
        evr = NULL;
        i++;
     }
*/
   evr = eina_list_data_get(cfdata->env_vars);
   if (evr && strcmp(cfdata->var_str, evr->var)) evr = NULL;
   if (evr)
     {
        eina_stringshare_del(evr->var);
        if (evr->val) eina_stringshare_del(evr->val);
        E_FREE(evr);
//        cfdata->env_vars = eina_list_remove_list(cfdata->env_vars, l);
        e_widget_ilist_clear(cfdata->gui.list);
        e_widget_ilist_freeze(cfdata->gui.list);
        EINA_LIST_FOREACH(cfdata->env_vars, l, evr)
          {
             e_widget_ilist_append(cfdata->gui.list, NULL, evr->var, 
                                   _sel_cb, cfdata, NULL);
          }
        e_widget_ilist_go(cfdata->gui.list);
        e_widget_ilist_thaw(cfdata->gui.list);
     }
}

static void
_mod_cb(void *data, void *data2 __UNUSED__)
{
   _add_cb(data, data2);
}

static void
_unset_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   if (cfdata->unset) e_widget_entry_text_set(cfdata->gui.val_en, "");
   e_widget_disabled_set(cfdata->gui.val_en, cfdata->unset);
}


static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ol, *oe, *ob, *oc;
   Eina_List *l;
   E_Config_Env_Var *evr;
   
   o = e_widget_table_add(evas, 0);
   
   ol = e_widget_ilist_add(evas, 0, 0, NULL);
   cfdata->gui.list = ol;
   e_widget_ilist_freeze(ol);
   EINA_LIST_FOREACH(cfdata->env_vars, l, evr)
     {
        e_widget_ilist_append(ol, NULL, evr->var, _sel_cb, cfdata, NULL);
     }
   e_widget_ilist_go(ol);
   e_widget_ilist_thaw(ol);
   e_widget_size_min_set(ol, 200, 160);
   e_widget_table_object_append(o, ol, 0, 0, 3, 1, 1, 1, 1, 1);
   
   oe = e_widget_entry_add(evas, &(cfdata->var_str), NULL, NULL, NULL);
   cfdata->gui.var_en = oe;
   e_widget_table_object_append(o, oe, 0, 1, 1, 1, 1, 1, 1, 0);
   
   oe = e_widget_entry_add(evas, &(cfdata->val_str), NULL, NULL, NULL);
   cfdata->gui.val_en = oe;
   e_widget_table_object_append(o, oe, 1, 1, 1, 1, 1, 1, 1, 0);
   
   oc = e_widget_check_add(evas, _("Unset"), &(cfdata->unset));
   cfdata->gui.unset = oc;
   e_widget_table_object_append(o, oc, 2, 1, 1, 1, 1, 1, 1, 0);
   evas_object_smart_callback_add(oc, "changed", _unset_cb, cfdata);
   
   ob = e_widget_button_add(evas, _("Add"), "list-add", _add_cb, cfdata, NULL);
   e_widget_table_object_append(o, ob, 0, 2, 1, 1, 1, 1, 0, 0);

   ob = e_widget_button_add(evas, _("Modify"), NULL, _mod_cb, cfdata, NULL);
   e_widget_table_object_append(o, ob, 1, 2, 1, 1, 1, 1, 0, 0);

   ob = e_widget_button_add(evas, _("Del"), "list-remove", _del_cb, cfdata, NULL);
   e_widget_table_object_append(o, ob, 2, 2, 1, 1, 1, 1, 0, 0);
   
   e_dialog_resizable_set(cfd->dia, 1);
   return o;
}
