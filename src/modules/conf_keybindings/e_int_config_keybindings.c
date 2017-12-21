#include "e.h"

#define TEXT_NONE_ACTION_KEY    _("<None>")
#define TEXT_PRESS_KEY_SEQUENCE _("Please press key sequence,<br><br>" \
                                  "or <hilight>Escape</hilight> to abort.")

#define TEXT_NO_PARAMS          _("<None>")
#define TEXT_NO_MODIFIER_HEADER _("Single key")

static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd,
                               E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data(E_Config_Dialog *cfd,
                                      E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd,
                                          Evas *evas,
                                          E_Config_Dialog_Data *cfdata);

/********* private functions ***************/
static void         _fill_actions_list(E_Config_Dialog_Data *cfdata);

/**************** Updates ***********/
static int          _update_key_binding_list(E_Config_Dialog_Data *cfdata,
                                             E_Config_Binding_Key *bi);
static void         _update_action_list(E_Config_Dialog_Data *cfdata);
static void         _update_action_params(E_Config_Dialog_Data *cfdata);
static void         _update_buttons(E_Config_Dialog_Data *cfdata);

/**************** Callbacks *********/
static void         _binding_change_cb(void *data);
static void         _action_change_cb(void *data);
static void         _delete_all_key_binding_cb(void *data,
                                               void *data2);
static void         _delete_key_binding_cb(void *data,
                                           void *data2);
static void         _restore_key_binding_defaults_cb(void *data,
                                                     void *data2);
static void         _add_key_binding_cb(void *data,
                                        void *data2);
static void         _modify_key_binding_cb(void *data,
                                           void *data2);

/********* Helper *************************/
static char        *_key_binding_header_get(int modifiers);
static char        *_key_binding_text_get(E_Config_Binding_Key *bi);
static void         _auto_apply_changes(E_Config_Dialog_Data *cfdata);
static void         _find_key_binding_action(const char *action,
                                             const char *params,
                                             int *g,
                                             int *a,
                                             int *n);

/********* Sorting ************************/
static int       _key_binding_sort_cb(const void *d1,
                                      const void *d2);

/**************** grab window *******/
static void      _grab_wnd_show(E_Config_Dialog_Data *cfdata);
static Eina_Bool _grab_key_down_cb(void *data,
                                   int type,
                                   void *event);

struct _E_Config_Dialog_Data
{
   Evas *evas;
   struct
   {
      Eina_List *key;
   } binding;
   struct
   {
      const char    *binding, *action, *cur;
      char          *params;
      int            cur_act, add;
      Eina_Bool     changed : 1;

      E_Grab_Dialog *eg;
   } locals;
   struct
   {
      Evas_Object *o_add, *o_mod, *o_del, *o_del_all;
      Evas_Object *o_binding_list, *o_action_list;
      Evas_Object *o_params;
   } gui;

   char            *params;
   E_Config_Dialog *cfd;
};

E_Config_Dialog *
e_int_config_keybindings(E_Container *con,
                         const char *params)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "keyboard_and_mouse/key_bindings")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->override_auto_apply = 1;

   cfd = e_config_dialog_new(con, _("Key Bindings Settings"), "E",
                             "keyboard_and_mouse/key_bindings",
                             "preferences-desktop-keyboard-shortcuts", 0, v, NULL);
   if ((params) && (params[0]))
     {
        cfd->cfdata->params = strdup(params);
        _add_key_binding_cb(cfd->cfdata, NULL);
     }

   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   Eina_List *l = NULL;
   E_Config_Binding_Key *bi, *bi2;

   cfdata->locals.binding = eina_stringshare_add("");
   cfdata->locals.action = eina_stringshare_add("");
   cfdata->locals.params = strdup("");
   cfdata->locals.cur = NULL;
   cfdata->binding.key = NULL;
   cfdata->locals.eg = NULL;

   EINA_LIST_FOREACH(e_config->key_bindings, l, bi)
     {
        if (!bi) continue;

        bi2 = E_NEW(E_Config_Binding_Key, 1);
        bi2->context = bi->context;
        bi2->key = eina_stringshare_add(bi->key);
        bi2->modifiers = bi->modifiers;
        bi2->any_mod = bi->any_mod;
        bi2->action = eina_stringshare_ref(bi->action);
        bi2->params = eina_stringshare_ref(bi->params);

        cfdata->binding.key = eina_list_append(cfdata->binding.key, bi2);
     }
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;
   cfdata->locals.cur_act = -1;
   _fill_data(cfdata);

   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd  __UNUSED__,
           E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Key *bi;

   EINA_LIST_FREE(cfdata->binding.key, bi)
     {
        eina_stringshare_del(bi->key);
        eina_stringshare_del(bi->action);
        eina_stringshare_del(bi->params);
        E_FREE(bi);
     }

   eina_stringshare_del(cfdata->locals.cur);
   eina_stringshare_del(cfdata->locals.binding);
   eina_stringshare_del(cfdata->locals.action);

   free(cfdata->locals.params);
   free(cfdata->params);
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd  __UNUSED__,
                  E_Config_Dialog_Data *cfdata)
{
   Eina_List *l = NULL;
   E_Config_Binding_Key *bi, *bi2;

   _auto_apply_changes(cfdata);

   e_managers_keys_ungrab();
   EINA_LIST_FREE(e_config->key_bindings, bi)
     {
        e_bindings_key_del(bi->context, bi->key, bi->modifiers, bi->any_mod,
                           bi->action, bi->params);
        if (bi->key) eina_stringshare_del(bi->key);
        if (bi->action) eina_stringshare_del(bi->action);
        if (bi->params) eina_stringshare_del(bi->params);
        E_FREE(bi);
     }

   EINA_LIST_FOREACH(cfdata->binding.key, l, bi2)
     {
        bi2 = l->data;

        if (!bi2->key || !bi2->key[0]) continue;

        bi = E_NEW(E_Config_Binding_Key, 1);
        bi->context = bi2->context;
        bi->key = eina_stringshare_add(bi2->key);
        bi->modifiers = bi2->modifiers;
        bi->any_mod = bi2->any_mod;
        bi->action =
          ((!bi2->action) || (!bi2->action[0])) ? NULL : eina_stringshare_ref(bi2->action);
        bi->params =
          ((!bi2->params) || (!bi2->params[0])) ? NULL : eina_stringshare_ref(bi2->params);

        e_config->key_bindings = eina_list_append(e_config->key_bindings, bi);
        e_bindings_key_add(bi->context, bi->key, bi->modifiers, bi->any_mod,
                           bi->action, bi->params);
     }
   e_managers_keys_grab();
   e_config_save_queue();

   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd,
                      Evas *evas,
                      E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ot, *of, *ob;

   cfdata->evas = evas;
   o = e_widget_list_add(evas, 0, 1);

   of = e_widget_frametable_add(evas, _("Key Bindings"), 0);
   ob = e_widget_ilist_add(evas, 32, 32, &(cfdata->locals.binding));
   cfdata->gui.o_binding_list = ob;
   e_widget_size_min_set(ob, 200, 200);
   e_widget_frametable_object_append(of, ob, 0, 0, 2, 1, 1, 1, 1, 1);
   ob = e_widget_button_add(evas, _("Add"), "list-add", _add_key_binding_cb, cfdata, NULL);
   cfdata->gui.o_add = ob;
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 0, 1, 0);
   ob = e_widget_button_add(evas, _("Delete"), "list-remove", _delete_key_binding_cb, cfdata, NULL);
   cfdata->gui.o_del = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 1, 1, 1, 1, 1, 0, 1, 0);
   ob = e_widget_button_add(evas, _("Modify"), NULL, _modify_key_binding_cb, cfdata, NULL);
   cfdata->gui.o_mod = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 1, 0, 1, 0);
   ob = e_widget_button_add(evas, _("Delete All"), "edit-clear", _delete_all_key_binding_cb, cfdata, NULL);
   cfdata->gui.o_del_all = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 1, 2, 1, 1, 1, 0, 1, 0);
   ob = e_widget_button_add(evas, _("Restore Default Bindings"), "enlightenment", _restore_key_binding_defaults_cb, cfdata, NULL);
   e_widget_frametable_object_append(of, ob, 0, 3, 2, 1, 1, 0, 1, 0);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   ot = e_widget_table_add(evas, 0);
   of = e_widget_framelist_add(evas, _("Action"), 0);
   ob = e_widget_ilist_add(evas, 24, 24, &(cfdata->locals.action));
   cfdata->gui.o_action_list = ob;
   e_widget_size_min_set(ob, 200, 280);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Action Params"), 0);
   ob = e_widget_entry_add(evas, &(cfdata->locals.params), NULL, NULL, NULL);
   cfdata->gui.o_params = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 1, 1, 1, 1, 1, 1, 0);
   e_widget_list_object_append(o, ot, 1, 1, 0.5);

   _update_key_binding_list(cfdata, NULL);
   _fill_actions_list(cfdata);

   e_dialog_resizable_set(cfd->dia, 1);
   return o;
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

/**************** Callbacks *********/

static void
_add_key_binding_cb(void *data,
                    void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   _auto_apply_changes(cfdata);

   cfdata->locals.add = 1;
   _grab_wnd_show(cfdata);
}

static void
_modify_key_binding_cb(void *data,
                       void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   _auto_apply_changes(cfdata);

   cfdata->locals.add = 0;
   _grab_wnd_show(cfdata);
}

static void
_binding_change_cb(void *data)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   if (cfdata->locals.changed) _auto_apply_changes(cfdata);
   eina_stringshare_del(cfdata->locals.cur);
   cfdata->locals.cur = NULL;
   cfdata->locals.cur_act = -1;

   cfdata->locals.changed = 0;
   if ((!cfdata->locals.binding) || (!cfdata->locals.binding[0])) return;

   cfdata->locals.cur = eina_stringshare_ref(cfdata->locals.binding);

   _update_buttons(cfdata);
   _update_action_list(cfdata);
}

static void
_action_change_cb(void *data)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   _update_action_params(cfdata);
   cfdata->locals.cur_act = e_widget_ilist_selected_get(cfdata->gui.o_action_list);
   cfdata->locals.changed = 1;
}

static void
_delete_all_key_binding_cb(void *data,
                           void *data2 __UNUSED__)
{
   E_Config_Binding_Key *bi;
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   /* FIXME: need confirmation dialog */
   EINA_LIST_FREE(cfdata->binding.key, bi)
     {
        eina_stringshare_del(bi->key);
        eina_stringshare_del(bi->action);
        eina_stringshare_del(bi->params);
        E_FREE(bi);
     }

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
_delete_key_binding_cb(void *data,
                       void *data2 __UNUSED__)
{
   Eina_List *l = NULL;
   const char *n;
   int sel;
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Key *bi;

   cfdata = data;

   sel = e_widget_ilist_selected_get(cfdata->gui.o_binding_list);
   if (cfdata->locals.binding[0] == 'k')
     {
        n = cfdata->locals.binding;
        l = eina_list_nth_list(cfdata->binding.key, atoi(++n));

        /* FIXME: need confirmation dialog */
        if (l)
          {
             bi = eina_list_data_get(l);
             eina_stringshare_del(bi->key);
             eina_stringshare_del(bi->action);
             eina_stringshare_del(bi->params);
             E_FREE(bi);

             cfdata->binding.key = eina_list_remove_list(cfdata->binding.key, l);
          }
     }

   _update_key_binding_list(cfdata, NULL);

   if (sel >= e_widget_ilist_count(cfdata->gui.o_binding_list))
     sel = e_widget_ilist_count(cfdata->gui.o_binding_list) - 1;

   eina_stringshare_del(cfdata->locals.cur);
   cfdata->locals.cur = NULL;

   if (sel < 0)
     {
        e_widget_ilist_unselect(cfdata->gui.o_action_list);
        e_widget_entry_clear(cfdata->gui.o_params);
        e_widget_disabled_set(cfdata->gui.o_params, 1);
        _update_buttons(cfdata);
     }
   else
     {
        e_widget_ilist_selected_set(cfdata->gui.o_binding_list, sel);
        e_widget_ilist_nth_show(cfdata->gui.o_binding_list, sel, 0);
     }
}

static void
_restore_key_binding_defaults_cb(void *data,
                                 void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Key *bi;

   cfdata = data;

   EINA_LIST_FREE(cfdata->binding.key, bi)
     {
        eina_stringshare_del(bi->key);
        eina_stringshare_del(bi->action);
        eina_stringshare_del(bi->params);
        E_FREE(bi);
     }

#define CFG_KEYBIND_DFLT(_context, _key, _modifiers, _anymod, _action, _params) \
  bi = E_NEW(E_Config_Binding_Key, 1);                                          \
  bi->context = _context;                                                       \
  bi->key = eina_stringshare_add(_key);                                         \
  bi->modifiers = _modifiers;                                                   \
  bi->any_mod = _anymod;                                                        \
  bi->action = eina_stringshare_add(_action);                                   \
  bi->params = eina_stringshare_add(_params);                                   \
  cfdata->binding.key = eina_list_append(cfdata->binding.key, bi)

   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Left",
                    E_BINDING_MODIFIER_SHIFT | E_BINDING_MODIFIER_ALT, 0,
                    "desk_flip_by", "-1 0");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Right",
                    E_BINDING_MODIFIER_SHIFT | E_BINDING_MODIFIER_ALT, 0,
                    "desk_flip_by", "1 0");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Up",
                    E_BINDING_MODIFIER_SHIFT | E_BINDING_MODIFIER_ALT, 0,
                    "desk_flip_by", "0 -1");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Down",
                    E_BINDING_MODIFIER_SHIFT | E_BINDING_MODIFIER_ALT, 0,
                    "desk_flip_by", "0 1");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Up",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "window_raise", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Down",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "window_lower", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "x",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "window_close", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "k",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "window_kill", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "w",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "window_menu", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "s",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "window_sticky_toggle", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "t",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "window_stack_top_toggle", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "f",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "window_fullscreen_toggle", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "i",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "window_iconic_toggle", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "n",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "window_maximized_toggle", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "F10",
                    E_BINDING_MODIFIER_SHIFT, 0,
                    "window_maximized_toggle", "default vertical");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "F10",
                    E_BINDING_MODIFIER_CTRL, 0,
                    "window_maximized_toggle", "default horizontal");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Left",
                    E_BINDING_MODIFIER_WIN, 0,
                    "window_maximized_toggle", "default left");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Right",
                    E_BINDING_MODIFIER_WIN, 0,
                    "window_maximized_toggle", "default right");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "r",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "window_shaded_toggle", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Left",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "desk_linear_flip_by", "-1");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Right",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "desk_linear_flip_by", "1");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "F1",
                    E_BINDING_MODIFIER_ALT, 0,
                    "desk_linear_flip_to", "0");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "F2",
                    E_BINDING_MODIFIER_ALT, 0,
                    "desk_linear_flip_to", "1");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "F3",
                    E_BINDING_MODIFIER_ALT, 0,
                    "desk_linear_flip_to", "2");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "F4",
                    E_BINDING_MODIFIER_ALT, 0,
                    "desk_linear_flip_to", "3");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "F5",
                    E_BINDING_MODIFIER_ALT, 0,
                    "desk_linear_flip_to", "4");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "F6",
                    E_BINDING_MODIFIER_ALT, 0,
                    "desk_linear_flip_to", "5");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "F7",
                    E_BINDING_MODIFIER_ALT, 0,
                    "desk_linear_flip_to", "6");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "F8",
                    E_BINDING_MODIFIER_ALT, 0,
                    "desk_linear_flip_to", "7");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "F9",
                    E_BINDING_MODIFIER_ALT, 0,
                    "desk_linear_flip_to", "8");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "F10",
                    E_BINDING_MODIFIER_ALT, 0,
                    "desk_linear_flip_to", "9");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "F11",
                    E_BINDING_MODIFIER_ALT, 0,
                    "desk_linear_flip_to", "10");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "F12",
                    E_BINDING_MODIFIER_ALT, 0,
                    "desk_linear_flip_to", "11");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "m",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "menu_show", "main");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "a",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "menu_show", "favorites");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Menu",
                    0, 0,
                    "menu_show", "main");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Menu",
                    E_BINDING_MODIFIER_CTRL, 0,
                    "menu_show", "clients");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Menu",
                    E_BINDING_MODIFIER_ALT, 0,
                    "menu_show", "favorites");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Insert",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "exec", "terminology");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Tab",
                    E_BINDING_MODIFIER_ALT, 0,
                    "winlist", "next");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Tab",
                    E_BINDING_MODIFIER_SHIFT | E_BINDING_MODIFIER_ALT, 0,
                    "winlist", "prev");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "End",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "restart", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Delete",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "syscon", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Escape",
                    E_BINDING_MODIFIER_ALT, 0,
                    "everything", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "l",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "desk_lock", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "d",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_ALT, 0,
                    "desk_deskshow_toggle", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "F1",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_SHIFT, 0,
                    "screen_send_to", "0");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "F2",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_SHIFT, 0,
                    "screen_send_to", "1");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "F3",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_SHIFT, 0,
                    "screen_send_to", "2");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "F4",
                    E_BINDING_MODIFIER_CTRL | E_BINDING_MODIFIER_SHIFT, 0,
                    "screen_send_to", "3");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "XF86AudioLowerVolume",
                    0, 0,
                    "volume_decrease", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "XF86AudioRaiseVolume",
                    0, 0,
                    "volume_increase", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "XF86AudioMute",
                    0, 0,
                    "volume_mute", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Print",
                    0, 0,
                    "shot", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "XF86Standby",
                    0, 0,
                    "suspend", "now");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "XF86Start",
                    0, 0,
                    "menu_show", "all");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "XF86PowerDown",
                    0, 0,
                    "hibernate", "now");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "XF86PowerOff",
                    0, 0,
                    "halt", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "XF86Sleep",
                    0, 0,
                    "suspend", "now");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "XF86Suspend",
                    0, 0,
                    "suspend", "now");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "XF86Hibernate",
                    0, 0,
                    "hibernate", "now");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "Execute",
                    0, 0,
                    "everything", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "XF86MonBrightnessUp",
                    0, 0,
                    "backlight_adjust", "0.1");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "XF86MonBrightnessDown",
                    0, 0,
                    "backlight_adjust", "-0.1");
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "XF86LightBulb",
                    0, 0,
                    "backlight", NULL);
   CFG_KEYBIND_DFLT(E_BINDING_CONTEXT_ANY, "XF86BrightnessAdjust",
                    0, 0,
                    "backlight", NULL);

   eina_stringshare_del(cfdata->locals.cur);
   cfdata->locals.cur = NULL;

   _update_key_binding_list(cfdata, NULL);
   _update_buttons(cfdata);

   e_widget_ilist_unselect(cfdata->gui.o_action_list);
   e_widget_entry_clear(cfdata->gui.o_params);
   e_widget_disabled_set(cfdata->gui.o_params, 1);
}

/**************** Updates ***********/
static void
_update_action_list(E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Key *bi;
   int j = -1, i, n, cnt;
   const char *action, *params;

   if (!cfdata->locals.cur) return;

   if (cfdata->locals.cur[0] == 'k')
     {
        sscanf(cfdata->locals.cur, "k%d", &n);
        bi = eina_list_nth(cfdata->binding.key, n);
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

   _find_key_binding_action(action, params, NULL, NULL, &j);

   if (j >= 0)
     {
        cnt = e_widget_ilist_count(cfdata->gui.o_action_list);
        for (i = 0; i < cnt; i++)
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

   /*if (cfdata->locals.cur[0] == 'k')
      {
        sscanf(cfdata->locals.cur, "k%d", &n);
        bi = eina_list_nth(cfdata->binding.key, n);
        if (!bi)
          {
             e_widget_ilist_unselect(cfdata->gui.o_action_list);
             e_widget_entry_clear(cfdata->gui.o_params);
             e_widget_disabled_set(cfdata->gui.o_params, 1);
             return;
          }

        _find_key_binding_action(bi, NULL, NULL, &j);
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
             free(cfdata->locals.action);
             cfdata->locals.action = strdup("");
             e_widget_entry_clear(cfdata->gui.o_params);
          }
      }*/
}

static void
_update_action_params(E_Config_Dialog_Data *cfdata)
{
   int g, a, b;
   E_Action_Group *actg;
   E_Action_Description *actd;
   E_Config_Binding_Key *bi;
   const char *action, *params;

#define KB_EXAMPLE_PARAMS                                           \
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
   sscanf(cfdata->locals.action, "%d %d", &g, &a);

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
        KB_EXAMPLE_PARAMS;
        return;
     }

   if (!actd->editable)
     e_widget_disabled_set(cfdata->gui.o_params, 1);
   else
     e_widget_disabled_set(cfdata->gui.o_params, 0);

   if (cfdata->locals.cur[0] == 'k')
     {
        sscanf(cfdata->locals.cur, "k%d", &b);
        bi = eina_list_nth(cfdata->binding.key, b);
        if (!bi)
          {
             e_widget_disabled_set(cfdata->gui.o_params, 1);
             KB_EXAMPLE_PARAMS;
             return;
          }
        action = bi->action;
        params = bi->params;
     }
   else
     {
        e_widget_disabled_set(cfdata->gui.o_params, 1);
        KB_EXAMPLE_PARAMS;
        return;
     }

   if (action)
     {
        if (!e_util_strcmp(action, actd->act_cmd))
          {
             if ((cfdata->locals.cur_act >= 0) && (cfdata->locals.cur_act != e_widget_ilist_selected_get(cfdata->gui.o_action_list)))
               KB_EXAMPLE_PARAMS;
             else
               e_widget_entry_text_set(cfdata->gui.o_params, params);
          }
        else
          KB_EXAMPLE_PARAMS;
     }
   else
     KB_EXAMPLE_PARAMS;
}

static int
_update_key_binding_list(E_Config_Dialog_Data *cfdata,
                         E_Config_Binding_Key *bi_new)
{
   int i;
   char *b, b2[64];
   Eina_List *l;
   E_Config_Binding_Key *bi;
   int modifiers = -1;
   int bi_pos = 0;
   int ret = -1;

   evas_event_freeze(evas_object_evas_get(cfdata->gui.o_binding_list));
   edje_freeze();
   e_widget_ilist_freeze(cfdata->gui.o_binding_list);

   e_widget_ilist_clear(cfdata->gui.o_binding_list);
   e_widget_ilist_go(cfdata->gui.o_binding_list);

   if (cfdata->binding.key)
     {
        cfdata->binding.key = eina_list_sort(cfdata->binding.key,
                                             eina_list_count(cfdata->binding.key), _key_binding_sort_cb);
     }

   for (l = cfdata->binding.key, i = 0; l; l = l->next, i++)
     {
        bi = l->data;
        if (bi == bi_new) ret = bi_pos;
        if (ret < 0) bi_pos++;

        if (modifiers != (int)bi->modifiers)
          {
             modifiers = bi->modifiers;
             b = _key_binding_header_get(modifiers);
             if (b)
               {
                  if (ret < 0) bi_pos++;
                  e_widget_ilist_header_append(cfdata->gui.o_binding_list, NULL, b);
                  free(b);
               }
          }

        b = _key_binding_text_get(bi);
        if (!b) continue;

        snprintf(b2, sizeof(b2), "k%d", i);
        e_widget_ilist_append(cfdata->gui.o_binding_list, NULL, b,
                              _binding_change_cb, cfdata, b2);
        free(b);
     }
   e_widget_ilist_go(cfdata->gui.o_binding_list);

   e_widget_ilist_thaw(cfdata->gui.o_binding_list);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(cfdata->gui.o_binding_list));

   if (eina_list_count(cfdata->binding.key))
     e_widget_disabled_set(cfdata->gui.o_del_all, 0);
   else
     e_widget_disabled_set(cfdata->gui.o_del_all, 1);

   return ret;
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
        e_widget_disabled_set(cfdata->gui.o_mod, 1);
        e_widget_disabled_set(cfdata->gui.o_del, 1);
        return;
     }
   e_widget_disabled_set(cfdata->gui.o_mod, 0);
   e_widget_disabled_set(cfdata->gui.o_del, 0);
}

/*************** Sorting *****************************/
static int
_key_binding_sort_cb(const void *d1,
                     const void *d2)
{
   int i, j;
   const E_Config_Binding_Key *bi, *bi2;

   bi = d1;
   bi2 = d2;

   i = 0; j = 0;
   if (bi->modifiers & E_BINDING_MODIFIER_CTRL) i++;
   if (bi->modifiers & E_BINDING_MODIFIER_ALT) i++;
   if (bi->modifiers & E_BINDING_MODIFIER_SHIFT) i++;
   if (bi->modifiers & E_BINDING_MODIFIER_WIN) i++;

   if (bi2->modifiers & E_BINDING_MODIFIER_CTRL) j++;
   if (bi2->modifiers & E_BINDING_MODIFIER_ALT) j++;
   if (bi2->modifiers & E_BINDING_MODIFIER_SHIFT) j++;
   if (bi2->modifiers & E_BINDING_MODIFIER_WIN) j++;

   if (i < j) return -1;
   else if (i > j)
     return 1;

   if (bi->modifiers < bi2->modifiers) return -1;
   else if (bi->modifiers > bi2->modifiers)
     return 1;

   i = strlen(bi->key ? bi->key : "");
   j = strlen(bi2->key ? bi2->key : "");

   if (i < j) return -1;
   else if (i > j)
     return 1;

   i = e_util_strcmp(bi->key, bi2->key);
   if (i < 0) return -1;
   else if (i > 0)
     return 1;

   return 0;
}

/**************** grab window *******/

static void
_grab_wnd_hide(void *data)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = e_object_data_get(data);
   cfdata->locals.eg = NULL;
}

static void
_grab_wnd_show(E_Config_Dialog_Data *cfdata)
{
   if (cfdata->locals.eg) return;
   cfdata->locals.eg = e_grab_dialog_show(cfdata->cfd->dia->win, EINA_FALSE, _grab_key_down_cb, NULL, NULL, cfdata);
   e_object_data_set(E_OBJECT(cfdata->locals.eg), cfdata);
   e_object_del_attach_func_set(E_OBJECT(cfdata->locals.eg), _grab_wnd_hide);
}

static Eina_Bool
_grab_key_down_cb(void *data,
                  __UNUSED__ int type,
                  void *event)
{
   E_Config_Dialog_Data *cfdata;
   Ecore_Event_Key *ev;

   ev = event;
   cfdata = data;

   if ((ev->key) && (ev->key) && (ev->compose))
     printf("'%s' '%s' '%s'\n", ev->key, ev->key, ev->compose);
   else if ((ev->key) && (ev->key))
     printf("'%s' '%s'\n", ev->key, ev->key);
   else
     printf("unknown key!!!!\n");
   if (!e_util_strcmp(ev->key, "Control_L") || !e_util_strcmp(ev->key, "Control_R") ||
       !e_util_strcmp(ev->key, "Shift_L") || !e_util_strcmp(ev->key, "Shift_R") ||
       !e_util_strcmp(ev->key, "Alt_L") || !e_util_strcmp(ev->key, "Alt_R") ||
       !e_util_strcmp(ev->key, "Super_L") || !e_util_strcmp(ev->key, "Super_R"))
     {
        /* Do nothing */
     }
   else
     {
        E_Config_Binding_Key *bi = NULL;
        const Eina_List *l = NULL;
        unsigned int mod = E_BINDING_MODIFIER_NONE;
        unsigned int n, found = 0;

        if (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)
          mod |= E_BINDING_MODIFIER_SHIFT;
        if (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)
          mod |= E_BINDING_MODIFIER_CTRL;
        if (ev->modifiers & ECORE_EVENT_MODIFIER_ALT)
          mod |= E_BINDING_MODIFIER_ALT;
        if (ev->modifiers & ECORE_EVENT_MODIFIER_WIN)
          mod |= E_BINDING_MODIFIER_WIN;

        if (cfdata->locals.add)
          found = !!e_util_binding_match(cfdata->binding.key, ev, &n, NULL);
        else if (cfdata->locals.cur && cfdata->locals.cur[0])
          {
             sscanf(cfdata->locals.cur, "k%d", &n);
             bi = eina_list_nth(cfdata->binding.key, n);
             found = !!e_util_binding_match(cfdata->binding.key, ev, &n, bi);
          }

        if (!found)
          {
             if (cfdata->locals.add)
               {
                  bi = E_NEW(E_Config_Binding_Key, 1);

                  bi->context = E_BINDING_CONTEXT_ANY;
                  bi->modifiers = mod;
                  bi->key = eina_stringshare_add(ev->key);
                  bi->action = NULL;
                  bi->params = NULL;
                  bi->any_mod = 0;

                  cfdata->binding.key = eina_list_append(cfdata->binding.key, bi);

                  n = _update_key_binding_list(cfdata, bi);

                  e_widget_ilist_selected_set(cfdata->gui.o_binding_list, n);
                  e_widget_ilist_nth_show(cfdata->gui.o_binding_list, n, 0);
                  e_widget_ilist_unselect(cfdata->gui.o_action_list);
                  eina_stringshare_del(cfdata->locals.action);
                  cfdata->locals.action = eina_stringshare_add("");
                  if ((cfdata->params) && (cfdata->params[0]))
                    {
                       int j, g = -1;
                       _find_key_binding_action("exec", NULL, &g, NULL, &j);
                       if (j >= 0)
                         {
                            e_widget_ilist_unselect(cfdata->gui.o_action_list);
                            e_widget_ilist_selected_set(cfdata->gui.o_action_list, (j + g + 1));
                            e_widget_entry_clear(cfdata->gui.o_params);
                            e_widget_entry_text_set(cfdata->gui.o_params, cfdata->params);
                         }
                    }
                  else
                    {
                       e_widget_entry_clear(cfdata->gui.o_params);
                       e_widget_disabled_set(cfdata->gui.o_params, 1);
                    }
               }
             else if (cfdata->locals.cur && cfdata->locals.cur[0])
               {
                  char *label;
                  E_Ilist_Item *it;
                  unsigned int i = 0;

                  sscanf(cfdata->locals.cur, "k%d", &n);
                  bi = eina_list_nth(cfdata->binding.key, n);

                  bi->modifiers = mod;
                  if (bi->key) eina_stringshare_del(bi->key);
                  bi->key = eina_stringshare_add(ev->key);
                  printf("blub\n");

                  label = _key_binding_text_get(bi);

                  EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->gui.o_binding_list), l, it)
                    {
                       if (it->header) n++;
                       if (i++ >= n) break;
                    }

                  e_widget_ilist_nth_label_set(cfdata->gui.o_binding_list, n, label);
                  free(label);
               }
          }
        else
          {
             unsigned int i = 0;
             E_Ilist_Item *it;
#if 0
             /* this advice is rather irritating as one sees that the
                key is bound to an action. if you want to set a
                keybinding you dont care about whether there is
                sth else set to it. */
             int g, a, j;
             const char *label = NULL;
             E_Action_Group *actg = NULL;
             E_Action_Description *actd = NULL;

             if (cfdata->locals.add)
               _find_key_binding_action(bi->action, bi->params, &g, &a, &j);
             else
               _find_key_binding_action(bi2->action, bi2->params, &g, &a, &j);

             actg = eina_list_nth(e_action_groups_get(), g);
             if (actg) actd = eina_list_nth(actg->acts, a);

             if (actd) label = _(actd->act_name);

             e_util_dialog_show(_("Binding Key Error"),
                                _("The binding key sequence, that you choose,"
                                  " is already used by <br>"
                                  "<hilight>%s</hilight> action.<br>"
                                  "Please choose another binding key sequence."),
                                label ? label : _("Unknown"));
#endif
             EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->gui.o_binding_list), l, it)
               {
                  if (it->header) n++;
                  if (i++ >= n) break;
               }

             e_widget_ilist_nth_show(cfdata->gui.o_binding_list, n, 1);
             e_widget_ilist_selected_set(cfdata->gui.o_binding_list, n);
          }
        e_object_del(E_OBJECT(cfdata->locals.eg));
     }
   return ECORE_CALLBACK_PASS_ON;
}


/********** Helper *********************************/
static void
_auto_apply_changes(E_Config_Dialog_Data *cfdata)
{
   int n, g, a, ok = 0;
   E_Config_Binding_Key *bi;
   E_Action_Group *actg;
   E_Action_Description *actd;

   if ((!cfdata->locals.cur) || (!cfdata->locals.cur[0]) ||
       (!cfdata->locals.action) || (!cfdata->locals.action[0])) return;

   sscanf(cfdata->locals.cur, "k%d", &n);
   sscanf(cfdata->locals.action, "%d %d", &g, &a);

   bi = eina_list_nth(cfdata->binding.key, n);
   if (!bi) return;

   actg = eina_list_nth(e_action_groups_get(), g);
   if (!actg) return;
   actd = eina_list_nth(actg->acts, a);
   if (!actd) return;

   eina_stringshare_replace(&bi->action, actd->act_cmd);
   eina_stringshare_replace(&bi->params, actd->act_params);
   if (bi->params) return;
   if (cfdata->locals.params)
     {
        ok = 1;
        if (!e_util_strcmp(cfdata->locals.params, TEXT_NO_PARAMS))
          ok = 0;

        else if ((actd->param_example) && (!e_util_strcmp(cfdata->locals.params, actd->param_example)))
          ok = 0;
     }

   if (ok)
     bi->params = eina_stringshare_add(cfdata->locals.params);
}

static void
_find_key_binding_action(const char *action,
                         const char *params,
                         int *g,
                         int *a,
                         int *n)
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
             if (!e_util_strcmp((!action ? "" : action), (!actd->act_cmd ? "" : actd->act_cmd)))
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
                            if (!e_util_strcmp(params, actd->act_params))
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

static char *
_key_binding_header_get(int modifiers)
{
   char b[256] = "";

   if (modifiers & E_BINDING_MODIFIER_CTRL)
     strcat(b, _("CTRL"));

   if (modifiers & E_BINDING_MODIFIER_ALT)
     {
        if (b[0]) strcat(b, " + ");
        strcat(b, _("ALT"));
     }

   if (modifiers & E_BINDING_MODIFIER_SHIFT)
     {
        if (b[0]) strcat(b, " + ");
        strcat(b, _("SHIFT"));
     }

   if (modifiers & E_BINDING_MODIFIER_WIN)
     {
        if (b[0]) strcat(b, " + ");
        strcat(b, _("WIN"));
     }

   if (!b[0]) return strdup(TEXT_NO_MODIFIER_HEADER);
   return strdup(b);
}

static char *
_key_binding_text_get(E_Config_Binding_Key *bi)
{
   char b[256] = "";

   if (!bi) return NULL;

   if (bi->modifiers & E_BINDING_MODIFIER_CTRL)
     strcat(b, _("CTRL"));

   if (bi->modifiers & E_BINDING_MODIFIER_ALT)
     {
        if (b[0]) strcat(b, " + ");
        strcat(b, _("ALT"));
     }

   if (bi->modifiers & E_BINDING_MODIFIER_SHIFT)
     {
        if (b[0]) strcat(b, " + ");
        strcat(b, _("SHIFT"));
     }

   if (bi->modifiers & E_BINDING_MODIFIER_WIN)
     {
        if (b[0]) strcat(b, " + ");
        strcat(b, _("WIN"));
     }

   if (bi->key && bi->key[0])
     {
        char *l;
        if (b[0]) strcat(b, " + ");

        l = strdup(bi->key);
        l[0] = (char)toupper(bi->key[0]);
        strcat(b, l);
        free(l);
     }

   /* see comment in e_bindings on numlock
      if (bi->modifiers & ECORE_X_LOCK_NUM)
      {
        if (b[0]) strcat(b, " ");
        strcat(b, _("OFF"));
      }
    */

   if (!b[0]) return strdup(TEXT_NONE_ACTION_KEY);
   return strdup(b);
}

