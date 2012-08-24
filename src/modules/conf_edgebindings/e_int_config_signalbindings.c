#include "e.h"

#define TEXT_NO_PARAMS           _("<None>")

struct _E_Config_Dialog_Data
{
   Evas *evas;
   struct
   {
      Eina_List *signal;
   } binding;
   struct
   {
      const char *binding, *action;
      char       *params;
      const char *cur;
      int         button;
      int         cur_act;
      const char *signal;
      const char *source;

      E_Dialog   *dia;
      char *dia_source;
      char *dia_signal;
   } locals;
   struct
   {
      Evas_Object *o_add, *o_del, *o_del_all;
      Evas_Object *o_binding_list, *o_action_list;
      Evas_Object *o_params, *o_selector;
   } gui;

   const char      *params;

   int              fullscreen_flip;
   int              multiscreen_flip;

   E_Config_Dialog *cfd;
};

static E_Config_Binding_Signal *
_signal_binding_new(const char *sig, const char *src)
{
   E_Config_Binding_Signal *bi;

   bi = E_NEW(E_Config_Binding_Signal, 1);
   bi->context = 2;
   bi->any_mod = 1;
   bi->signal = eina_stringshare_add(sig);
   bi->source = eina_stringshare_add(src);
   return bi;
}

static E_Config_Binding_Signal *
_signal_binding_copy(E_Config_Binding_Signal *bi)
{
   E_Config_Binding_Signal *bi2;
   if (!bi) return NULL;

   if (!bi->signal) return NULL;
   if (!bi->source) return NULL;
   bi2 = E_NEW(E_Config_Binding_Signal, 1);
   bi2->context = bi->context;
   bi2->modifiers = bi->modifiers;
   bi2->any_mod = bi->any_mod;
   bi2->action = (bi->action && bi->action[0]) ? eina_stringshare_ref(bi->action) : NULL;
   bi2->params = (bi->params && bi->params[0]) ? eina_stringshare_ref(bi->params) : NULL;
   bi2->signal = eina_stringshare_ref(bi->signal);
   bi2->source = eina_stringshare_ref(bi->source);
   return bi2;
}

static void
_signal_binding_free(E_Config_Binding_Signal *bi)
{
   if (!bi) return;
   eina_stringshare_del(bi->action);
   eina_stringshare_del(bi->params);
   eina_stringshare_del(bi->signal);
   eina_stringshare_del(bi->source);
   free(bi);
}


static void
_auto_apply_changes(E_Config_Dialog_Data *cfdata)
{
   int n, g, a, ok;
   E_Config_Binding_Signal *bi;
   E_Action_Group *actg;
   E_Action_Description *actd;

   if ((!cfdata->locals.cur) || (!cfdata->locals.cur[0]) ||
       (!cfdata->locals.action) || (!cfdata->locals.action[0])) return;

   if (sscanf(cfdata->locals.cur, "s%d", &n) != 1)
     return;
   if (sscanf(cfdata->locals.action, "%d %d", &g, &a) != 2)
     return;

   bi = eina_list_nth(cfdata->binding.signal, n);
   if (!bi) return;

   actg = eina_list_nth(e_action_groups_get(), g);
   if (!actg) return;
   actd = eina_list_nth(actg->acts, a);
   if (!actd) return;

   eina_stringshare_del(bi->action);
   bi->action = NULL;

   if (actd->act_cmd) bi->action = eina_stringshare_add(actd->act_cmd);

   eina_stringshare_del(bi->params);
   bi->params = NULL;

   if (actd->act_params)
     bi->params = eina_stringshare_add(actd->act_params);
   else
     {
        ok = 1;
        if (cfdata->locals.params)
          {
             if (!strcmp(cfdata->locals.params, TEXT_NO_PARAMS))
               ok = 0;

             if ((actd->param_example) && (!strcmp(cfdata->locals.params, actd->param_example)))
               ok = 0;
          }
        else
          ok = 0;

        if (ok)
          bi->params = eina_stringshare_add(cfdata->locals.params);
     }
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Signal *bi, *bi2;
   Eina_List *l;

   cfdata->locals.params = strdup("");
   cfdata->locals.action = eina_stringshare_add("");
   cfdata->locals.binding = eina_stringshare_add("");
   cfdata->locals.signal = eina_stringshare_add("");
   cfdata->locals.source = eina_stringshare_add("");
   cfdata->locals.cur = NULL;
   cfdata->locals.dia = NULL;
   cfdata->binding.signal = NULL;

   EINA_LIST_FOREACH(e_config->signal_bindings, l, bi)
     {
        if (!bi) continue;
        bi2 = _signal_binding_copy(bi);
        cfdata->binding.signal = eina_list_append(cfdata->binding.signal, bi2);
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
   E_FREE_LIST(cfdata->binding.signal, _signal_binding_free);

   eina_stringshare_del(cfdata->locals.cur);
   eina_stringshare_del(cfdata->params);
   eina_stringshare_del(cfdata->locals.binding);
   eina_stringshare_del(cfdata->locals.action);
   eina_stringshare_del(cfdata->locals.signal);
   eina_stringshare_del(cfdata->locals.source);

   if (cfdata->locals.dia) e_object_del(E_OBJECT(cfdata->locals.dia));

   free(cfdata->locals.params);
   E_FREE(cfdata);
}


static void
_update_action_params(E_Config_Dialog_Data *cfdata)
{
   int g, a, b;
   E_Action_Group *actg;
   E_Action_Description *actd;
   E_Config_Binding_Signal *bi;
   const char *action, *params;

#define SIGNAL_EXAMPLE_PARAMS                                         \
  if ((!actd->param_example) || (!actd->param_example[0]))          \
    e_widget_entry_text_set(cfdata->gui.o_params, TEXT_NO_PARAMS);  \
  else                                                              \
    e_widget_entry_text_set(cfdata->gui.o_params, actd->param_example)

   if ((!cfdata->locals.action) || (!cfdata->locals.action[0]))
     {
        e_widget_disabled_set(cfdata->gui.o_params, 1);
        e_widget_entry_clear(cfdata->gui.o_params);
        return;
     }
   if (sscanf(cfdata->locals.action, "%d %d", &g, &a) != 2)
     return;

   actg = eina_list_nth(e_action_groups_get(), g);
   if (!actg) return;
   actd = eina_list_nth(actg->acts, a);
   if (!actd) return;

   if (actd->act_params)
     {
        e_widget_disabled_set(cfdata->gui.o_params, 1);
        e_widget_entry_text_set(cfdata->gui.o_params, actd->act_params);
        return;
     }

   if ((!cfdata->locals.cur) || (!cfdata->locals.cur[0]))
     {
        e_widget_disabled_set(cfdata->gui.o_params, 1);
        SIGNAL_EXAMPLE_PARAMS;
        return;
     }

   if (!actd->editable)
     e_widget_disabled_set(cfdata->gui.o_params, 1);
   else
     e_widget_disabled_set(cfdata->gui.o_params, 0);

   if (cfdata->locals.cur[0] == 's')
     {
        if (sscanf(cfdata->locals.cur, "s%d", &b) != 1)
          {
             e_widget_disabled_set(cfdata->gui.o_params, 1);
             SIGNAL_EXAMPLE_PARAMS;
             return;
          }

        bi = eina_list_nth(cfdata->binding.signal, b);
        if (!bi)
          {
             e_widget_disabled_set(cfdata->gui.o_params, 1);
             SIGNAL_EXAMPLE_PARAMS;
             return;
          }
        action = bi->action;
        params = bi->params;
     }
   else
     {
        e_widget_disabled_set(cfdata->gui.o_params, 1);
        SIGNAL_EXAMPLE_PARAMS;
        return;
     }

   if (action)
     {
        if (!strcmp(action, actd->act_cmd))
          {
             if ((!params) || (!params[0]))
               SIGNAL_EXAMPLE_PARAMS;
             else
               e_widget_entry_text_set(cfdata->gui.o_params, params);
          }
        else
          SIGNAL_EXAMPLE_PARAMS;
     }
   else
     SIGNAL_EXAMPLE_PARAMS;
}

static void
_action_change_cb(void *data)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   _update_action_params(cfdata);
}


static int
_signal_binding_sort_cb(E_Config_Binding_Signal *a, E_Config_Binding_Signal *b)
{
   int ret;

   ret = strcmp(a->source, b->source);
   if (ret) return ret;
   return strcmp(a->signal, b->signal);
}


static void
_update_buttons(E_Config_Dialog_Data *cfdata)
{
   if (e_widget_ilist_count(cfdata->gui.o_binding_list))
     e_widget_disabled_set(cfdata->gui.o_del_all, 0);
   else
     e_widget_disabled_set(cfdata->gui.o_del_all, 1);

   if (!cfdata->locals.cur)
     {
        e_widget_disabled_set(cfdata->gui.o_del, 1);
        return;
     }
   e_widget_disabled_set(cfdata->gui.o_del, 0);
}


static void
_find_signal_binding_action(const char *action, const char *params, int *g, int *a, int *n)
{
   Eina_List *l, *l2;
   int gg = -1, aa = -1, nn = -1, found;
   E_Action_Group *actg;
   E_Action_Description *actd;

   if (g) *g = -1;
   if (a) *a = -1;
   if (n) *n = -1;

   found = 0;
   for (l = e_action_groups_get(), gg = 0, nn = 0; l; l = l->next, gg++)
     {
        actg = l->data;

        for (l2 = actg->acts, aa = 0; l2; l2 = l2->next, aa++)
          {
             actd = l2->data;
             if (!strcmp((!action ? "" : action), (!actd->act_cmd ? "" : actd->act_cmd)))
               {
                  if (!params || !params[0])
                    {
                       if ((!actd->act_params) || (!actd->act_params[0]))
                         {
                            if (g) *g = gg;
                            if (a) *a = aa;
                            if (n) *n = nn;
                            return;
                         }
                       else
                         continue;
                    }
                  else
                    {
                       if ((!actd->act_params) || (!actd->act_params[0]))
                         {
                            if (g) *g = gg;
                            if (a) *a = aa;
                            if (n) *n = nn;
                            found = 1;
                         }
                       else
                         {
                            if (!strcmp(params, actd->act_params))
                              {
                                 if (g) *g = gg;
                                 if (a) *a = aa;
                                 if (n) *n = nn;
                                 return;
                              }
                         }
                    }
               }
             nn++;
          }
        if (found) break;
     }

   if (!found)
     {
        if (g) *g = -1;
        if (a) *a = -1;
        if (n) *n = -1;
     }
}

static void
_update_action_list(E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Signal *bi;
   int j = -1, i, n;
   const char *action, *params;

   if (!cfdata->locals.cur) return;

   if (cfdata->locals.cur[0] == 's')
     {
        if (sscanf(cfdata->locals.cur, "s%d", &n) != 1)
          return;

        bi = eina_list_nth(cfdata->binding.signal, n);
        if (!bi)
          {
             e_widget_ilist_unselect(cfdata->gui.o_action_list);
             e_widget_entry_clear(cfdata->gui.o_params);
             e_widget_disabled_set(cfdata->gui.o_params, 1);
             return;
          }
        action = bi->action;
        params = bi->params;
     }
   else
     return;

   _find_signal_binding_action(action, params, NULL, NULL, &j);

   if (j >= 0)
     {
        for (i = 0; i < e_widget_ilist_count(cfdata->gui.o_action_list); i++)
          {
             if (i > j) break;
             if (e_widget_ilist_nth_is_header(cfdata->gui.o_action_list, i)) j++;
          }
     }

   if (j >= 0)
     {
        if (j == e_widget_ilist_selected_get(cfdata->gui.o_action_list))
          _update_action_params(cfdata);
        else
          e_widget_ilist_selected_set(cfdata->gui.o_action_list, j);
     }
   else
     {
        e_widget_ilist_unselect(cfdata->gui.o_action_list);
        eina_stringshare_del(cfdata->locals.action);
        cfdata->locals.action = eina_stringshare_add("");
        e_widget_entry_clear(cfdata->gui.o_params);
     }
}

static void
_binding_change_cb(void *data)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   _auto_apply_changes(cfdata);
   if (cfdata->locals.cur) eina_stringshare_del(cfdata->locals.cur);
   cfdata->locals.cur = NULL;

   if ((!cfdata->locals.binding) || (!cfdata->locals.binding[0])) return;

   cfdata->locals.cur = eina_stringshare_ref(cfdata->locals.binding);

   _update_buttons(cfdata);
   _update_action_list(cfdata);
}


static int
_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Eina_List *l;
   E_Config_Binding_Signal *bi, *bi2;

   _auto_apply_changes(cfdata);
   E_FREE_LIST(e_config->signal_bindings, _signal_binding_free);
   EINA_LIST_FOREACH(cfdata->binding.signal, l, bi2)
     {
        bi = _signal_binding_copy(bi2);
        e_config->signal_bindings = eina_list_append(e_config->signal_bindings, bi);
     }
   e_bindings_signal_reset();

   e_config_save_queue();

   return 1;
}

static void
_update_signal_binding_list(E_Config_Dialog_Data *cfdata)
{
   int i = 0;
   char b2[64];
   Eina_List *l;
   const char *source = NULL;
   E_Config_Binding_Signal *bi;

   evas_event_freeze(evas_object_evas_get(cfdata->gui.o_binding_list));
   edje_freeze();
   e_widget_ilist_freeze(cfdata->gui.o_binding_list);

   e_widget_ilist_clear(cfdata->gui.o_binding_list);
   e_widget_ilist_go(cfdata->gui.o_binding_list);

   if (cfdata->binding.signal)
     cfdata->binding.signal = eina_list_sort(cfdata->binding.signal, 0, (Eina_Compare_Cb)_signal_binding_sort_cb);

   EINA_LIST_FOREACH(cfdata->binding.signal, l, bi)
     {
        if (source != bi->source)
          e_widget_ilist_header_append(cfdata->gui.o_binding_list, NULL, bi->source);

        snprintf(b2, sizeof(b2), "s%d", i);
        e_widget_ilist_append(cfdata->gui.o_binding_list, NULL, bi->signal, _binding_change_cb, cfdata, b2);
        i++;
        source = bi->source;
     }
   e_widget_ilist_go(cfdata->gui.o_binding_list);

   e_widget_ilist_thaw(cfdata->gui.o_binding_list);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(cfdata->gui.o_binding_list));

   if (eina_list_count(cfdata->binding.signal))
     e_widget_disabled_set(cfdata->gui.o_del_all, 0);
   else
     e_widget_disabled_set(cfdata->gui.o_del_all, 1);
}

static void
_fill_actions_list(E_Config_Dialog_Data *cfdata)
{
   char buf[1024];
   Eina_List *l, *l2;
   E_Action_Group *actg;
   E_Action_Description *actd;
   int g, a;

   evas_event_freeze(evas_object_evas_get(cfdata->gui.o_action_list));
   edje_freeze();
   e_widget_ilist_freeze(cfdata->gui.o_action_list);

   e_widget_ilist_clear(cfdata->gui.o_action_list);
   for (l = e_action_groups_get(), g = 0; l; l = l->next, g++)
     {
        actg = l->data;

        if (!actg->acts) continue;

        e_widget_ilist_header_append(cfdata->gui.o_action_list, NULL, _(actg->act_grp));

        for (l2 = actg->acts, a = 0; l2; l2 = l2->next, a++)
          {
             actd = l2->data;

             snprintf(buf, sizeof(buf), "%d %d", g, a);
             e_widget_ilist_append(cfdata->gui.o_action_list, NULL, _(actd->act_name),
                                   _action_change_cb, cfdata, buf);
          }
     }
   e_widget_ilist_go(cfdata->gui.o_action_list);
   e_widget_ilist_thaw(cfdata->gui.o_action_list);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(cfdata->gui.o_action_list));
}

static void
_signal_add_cb_ok(void *data, E_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata = data;
   E_Config_Binding_Signal *bi;
   Eina_List *l;
   const char *sig, *src;


   sig = eina_stringshare_add(cfdata->locals.dia_signal);
   src = eina_stringshare_add(cfdata->locals.dia_source);
   if ((!sig) || (!src) || (!sig[0]) || (!src[0]))
     {
        e_util_dialog_show(_("Signal Binding Error"), _("Signal and Source must NOT be blank!"));
        e_object_del(E_OBJECT(dia));
        return;
     }
   EINA_LIST_FOREACH(cfdata->binding.signal, l, bi)
     {
        if ((sig == bi->signal) && (src == bi->source))
          {
             eina_stringshare_del(sig);
             eina_stringshare_del(src);
             e_util_dialog_show(_("Signal Binding Error"),
                                _("The signal and source that you entered are already used by<br>"
                                  "<hilight>%s</hilight> action.<br>"),
                                bi->action ?: _("Unknown"));
             e_object_del(E_OBJECT(dia));
             return;
          }
     }

   bi = E_NEW(E_Config_Binding_Signal, 1);
   bi->context = 2;
   bi->any_mod = 1;
   bi->signal = sig;
   bi->source = src;
   cfdata->binding.signal = eina_list_append(cfdata->binding.signal, bi);
   _update_signal_binding_list(cfdata);

   e_object_del(E_OBJECT(dia));
}

static void
_signal_add_cb_cancel(void *data __UNUSED__, E_Dialog *dia)
{
   e_object_del(E_OBJECT(dia));
}

static void
_signal_add_del(void *data)
{
   E_Dialog *dia = data;
   E_Config_Dialog_Data *cfdata;

   if (!dia->data) return;
   cfdata = dia->data;
   cfdata->locals.dia = NULL;
   E_FREE(cfdata->locals.dia_signal);
   E_FREE(cfdata->locals.dia_source);
}

static void
_signal_add_show(E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *obg, *ol, *entry;
   Evas *evas;
   int w, h;

   if (cfdata->locals.dia) return;

   cfdata->locals.dia = e_dialog_new(NULL, "E", "_signalbind_new_dialog");
   e_dialog_title_set(cfdata->locals.dia, _("Add Signal Binding"));
   e_dialog_icon_set(cfdata->locals.dia, "enlightenment/signals", 48);
   e_dialog_button_add(cfdata->locals.dia, _("OK"), NULL, _signal_add_cb_ok, cfdata);
   e_dialog_button_add(cfdata->locals.dia, _("Cancel"), NULL, _signal_add_cb_cancel, cfdata);
   e_object_del_attach_func_set(E_OBJECT(cfdata->locals.dia), _signal_add_del);
   cfdata->locals.dia->data = cfdata;
   e_win_centered_set(cfdata->locals.dia->win, 1);

   evas = e_win_evas_get(cfdata->locals.dia->win);
   obg = e_widget_list_add(evas, 1, 0);

   ol = e_widget_framelist_add(evas, "Source:", 0);
   entry = o = e_widget_entry_add(evas, &cfdata->locals.dia_source, NULL, NULL, NULL);
   e_widget_framelist_object_append(ol, o);
   e_widget_list_object_append(obg, ol, 1, 0, 0.5);
   
   ol = e_widget_framelist_add(evas, "Signal:", 0);
   o = e_widget_entry_add(evas, &cfdata->locals.dia_signal, NULL, NULL, NULL);
   e_widget_framelist_object_append(ol, o);
   e_widget_list_object_append(obg, ol, 1, 0, 0.5);

   e_widget_size_min_get(obg, &w, &h);
   e_dialog_content_set(cfdata->locals.dia, obg, MAX(w, 200), MAX(h, 100));

   e_dialog_show(cfdata->locals.dia);
   e_widget_focus_set(entry, 1);
   e_dialog_parent_set(cfdata->locals.dia, cfdata->cfd->dia->win);
}

static void
_add_signal_binding_cb(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   
   _auto_apply_changes(cfdata);
   _signal_add_show(cfdata);
}

static void
_delete_signal_binding_cb(void *data, void *data2 __UNUSED__)
{
   Eina_List *l = NULL;
   int sel, n;
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   sel = e_widget_ilist_selected_get(cfdata->gui.o_binding_list);
   if (cfdata->locals.binding[0] == 's')
     {
        if (sscanf(cfdata->locals.binding, "s%d", &n) != 1)
          return;

        l = eina_list_nth_list(cfdata->binding.signal, n);

        if (l)
          {
             _signal_binding_free(eina_list_data_get(l));
             cfdata->binding.signal = eina_list_remove_list(cfdata->binding.signal, l);
          }
     }

   _update_signal_binding_list(cfdata);

   if (sel >= e_widget_ilist_count(cfdata->gui.o_binding_list))
     sel = e_widget_ilist_count(cfdata->gui.o_binding_list) - 1;

   eina_stringshare_del(cfdata->locals.cur);
   cfdata->locals.cur = NULL;

   e_widget_ilist_selected_set(cfdata->gui.o_binding_list, sel);
   if (sel < 0)
     {
        e_widget_ilist_unselect(cfdata->gui.o_action_list);
        e_widget_entry_clear(cfdata->gui.o_params);
        e_widget_disabled_set(cfdata->gui.o_params, 1);
        _update_buttons(cfdata);
     }
}


static void
_delete_all_signal_binding_cb(void *data, void *data2 __UNUSED__)
{
   E_Config_Binding_Signal *bi;
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   EINA_LIST_FREE(cfdata->binding.signal, bi)
     _signal_binding_free(bi);

   eina_stringshare_del(cfdata->locals.cur);
   cfdata->locals.cur = NULL;

   e_widget_ilist_clear(cfdata->gui.o_binding_list);
   e_widget_ilist_go(cfdata->gui.o_binding_list);
   e_widget_ilist_unselect(cfdata->gui.o_action_list);
   e_widget_entry_clear(cfdata->gui.o_params);
   e_widget_disabled_set(cfdata->gui.o_params, 1);

   _update_buttons(cfdata);
}


static void
_restore_signal_binding_defaults_cb(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Signal *bi;

   cfdata = data;

   E_FREE_LIST(cfdata->binding.signal, _signal_binding_free);

#define CFG_SIGBIND_DFLT(_signal, _source, _action, _params) \
  bi = _signal_binding_new(_signal, _source); \
  bi->action = eina_stringshare_add(_action);                                             \
  bi->params = eina_stringshare_add(_params);                                             \
  cfdata->binding.signal = eina_list_append(cfdata->binding.signal, bi)

   CFG_SIGBIND_DFLT("mouse,down,1,double", "e.event.titlebar", "window_shaded_toggle", "up");
   CFG_SIGBIND_DFLT("mouse,down,2", "e.event.titlebar", "window_shaded_toggle", "up");
   CFG_SIGBIND_DFLT("mouse,wheel,?,1", "e.event.titlebar", "window_shaded", "0 up");
   CFG_SIGBIND_DFLT("mouse,wheel,?,-1", "e.event.titlebar", "window_shaded", "1 up");
   CFG_SIGBIND_DFLT("mouse,clicked,3", "e.event.titlebar", "window_menu", NULL);
   CFG_SIGBIND_DFLT("mouse,clicked,?", "e.event.icon", "window_menu", NULL);
   CFG_SIGBIND_DFLT("mouse,clicked,[12]", "e.event.close", "window_close", NULL);
   CFG_SIGBIND_DFLT("mouse,clicked,3", "e.event.close", "window_kill", NULL);
   CFG_SIGBIND_DFLT("mouse,clicked,1", "e.event.maximize", "window_maximized_toggle", NULL);
   CFG_SIGBIND_DFLT("mouse,clicked,2", "e.event.maximize", "window_maximized_toggle", "smart");
   CFG_SIGBIND_DFLT("mouse,clicked,3", "e.event.maximize", "window_maximized_toggle", "expand");
   CFG_SIGBIND_DFLT("mouse,clicked,?", "e.event.minimize", "window_iconic_toggle", NULL);
   CFG_SIGBIND_DFLT("mouse,clicked,?", "e.event.shade", "window_shaded_toggle", "up");
   CFG_SIGBIND_DFLT("mouse,clicked,?", "e.event.lower", "window_lower", NULL);
   CFG_SIGBIND_DFLT("mouse,down,1", "e.event.icon", "window_drag_icon", NULL);
   CFG_SIGBIND_DFLT("mouse,down,1", "e.event.titlebar", "window_move", NULL);
   CFG_SIGBIND_DFLT("mouse,up,1", "e.event.titlebar", "window_move", "end");
   CFG_SIGBIND_DFLT("mouse,down,1", "e.event.resize.tl", "window_resize", "tl");
   CFG_SIGBIND_DFLT("mouse,down,1", "e.event.resize.t", "window_resize", "t");
   CFG_SIGBIND_DFLT("mouse,down,1", "e.event.resize.tr", "window_resize", "tr");
   CFG_SIGBIND_DFLT("mouse,down,1", "e.event.resize.r", "window_resize", "r");
   CFG_SIGBIND_DFLT("mouse,down,1", "e.event.resize.br", "window_resize", "br");
   CFG_SIGBIND_DFLT("mouse,down,1", "e.event.resize.b", "window_resize", "b");
   CFG_SIGBIND_DFLT("mouse,down,1", "e.event.resize.bl", "window_resize", "bl");
   CFG_SIGBIND_DFLT("mouse,down,1", "e.event.resize.l", "window_resize", "l");
   CFG_SIGBIND_DFLT("mouse,up,1", "e.event.resize.*", "window_resize", "end");
   CFG_SIGBIND_DFLT("mouse,down,3", "e.event.resize.*", "window_move", NULL);
   CFG_SIGBIND_DFLT("mouse,up,3", "e.event.resize.*", "window_move", "end");

   eina_stringshare_del(cfdata->locals.cur);
   cfdata->locals.cur = NULL;

   _update_signal_binding_list(cfdata);
   _update_buttons(cfdata);

   e_widget_ilist_unselect(cfdata->gui.o_action_list);
   e_widget_entry_clear(cfdata->gui.o_params);
   e_widget_disabled_set(cfdata->gui.o_params, 1);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ol, *ot, *of, *ob;

   cfdata->evas = evas;
   o = e_widget_list_add(evas, 0, 0);
   ol = e_widget_list_add(evas, 0, 1);

   of = e_widget_frametable_add(evas, _("Signal Bindings"), 0);
   ob = e_widget_ilist_add(evas, 32, 32, &(cfdata->locals.binding));
   cfdata->gui.o_binding_list = ob;
   e_widget_size_min_set(ob, 200, 160);
   e_widget_frametable_object_append(of, ob, 0, 0, 2, 1, 1, 1, 1, 1);
   ob = e_widget_button_add(evas, _("Add"), "list-add", _add_signal_binding_cb, cfdata, NULL);
   cfdata->gui.o_add = ob;
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 2, 1, 0, 1, 0);
   ob = e_widget_button_add(evas, _("Delete"), "list-remove", _delete_signal_binding_cb, cfdata, NULL);
   cfdata->gui.o_del = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 1, 1, 1, 1, 1, 0, 1, 0);
   ob = e_widget_button_add(evas, _("Delete All"), "edit-clear", _delete_all_signal_binding_cb, cfdata, NULL);
   cfdata->gui.o_del_all = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 1, 2, 1, 1, 1, 0, 1, 0);
   ob = e_widget_button_add(evas, _("Restore Default Bindings"), "enlightenment", _restore_signal_binding_defaults_cb, cfdata, NULL);
   e_widget_frametable_object_append(of, ob, 0, 3, 2, 1, 1, 0, 1, 0);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);

   ot = e_widget_table_add(evas, 0);
   of = e_widget_framelist_add(evas, _("Action"), 0);
   ob = e_widget_ilist_add(evas, 24, 24, &(cfdata->locals.action));
   cfdata->gui.o_action_list = ob;
   e_widget_size_min_set(ob, 200, 240);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Action Params"), 0);
   ob = e_widget_entry_add(evas, &(cfdata->locals.params), NULL, NULL, NULL);
   cfdata->gui.o_params = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 3, 1, 1, 1, 1, 1, 0);
   e_widget_list_object_append(ol, ot, 1, 1, 0.5);

   e_widget_list_object_append(o, ol, 1, 1, 0.5);

   _update_signal_binding_list(cfdata);
   _fill_actions_list(cfdata);

   e_dialog_resizable_set(cfd->dia, 1);
   return o;
}


E_Config_Dialog *
e_int_config_signalbindings(E_Container *con, const char *params)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "keyboard_and_mouse/signal_bindings")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->override_auto_apply = 1;

   cfd = e_config_dialog_new(con, _("Signal Bindings Settings"), "E",
                             "keyboard_and_mouse/signal_bindings",
                             "enlightenment/signals", 0, v, NULL);
   if ((params) && (params[0]))
     {
        cfd->cfdata->params = eina_stringshare_add(params);
//        _add_signal_binding_cb(cfd->cfdata, NULL);
     }

   return cfd;
}
