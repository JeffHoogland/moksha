#include "e_mod_main.h"

typedef struct Config_Entry
{
   EINA_INLIST;
   const char *id;
   E_Quick_Access_Entry *entry;
} Config_Entry;

struct _E_Config_Dialog_Data
{
   const char *entry;
   Evas_Object *o_list_entry;
   Evas_Object *o_list_transient;
   E_Entry_Dialog *ed;

   Eina_Inlist *entries;
   Eina_Inlist *transient_entries;

   int autohide;
   int hide_when_behind;
   int skip_taskbar;
   int skip_pager;
   int dont_bug_me;
};

static E_Config_DD *conf_edd, *entry_edd;

/**
 * in priority order:
 *
 * @todo configure match name (to be used by actions)
 *
 * @todo configure match icccm match class and name.
 *
 * @todo configure show/hide effects:
 *        - fullscreen
 *        - centered
 *        - slide from top, bottom, left or right
 *
 * @todo match more than one, doing tabs (my idea is to do another
 *       tabbing module first, experiment with that, maybe use/reuse
 *       it here)
 */

static Config_Entry *
_config_entry_new(E_Quick_Access_Entry *entry)
{
   Config_Entry *ce;

   ce = E_NEW(Config_Entry, 1);
   ce->entry = entry;
   entry->cfg_entry = ce;
   return ce;
}

static void
_config_entry_free(Config_Entry *ce)
{
   if (!ce) return;
   ce->entry->cfg_entry = NULL;
   eina_stringshare_del(ce->id);
   if (ce->entry->transient)
     qa_mod->cfd->cfdata->transient_entries = eina_inlist_remove(qa_mod->cfd->cfdata->transient_entries, EINA_INLIST_GET(ce));
   else
     qa_mod->cfd->cfdata->entries = eina_inlist_remove(qa_mod->cfd->cfdata->entries, EINA_INLIST_GET(ce));
   free(ce);
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   Eina_List *l;
   E_Quick_Access_Entry *entry;

   cfdata->autohide = qa_config->autohide;
   cfdata->hide_when_behind = qa_config->hide_when_behind;
   cfdata->skip_taskbar = qa_config->skip_taskbar;
   cfdata->skip_pager = qa_config->skip_pager;

   EINA_LIST_FOREACH(qa_config->entries, l, entry)
     cfdata->entries = eina_inlist_append(cfdata->entries, EINA_INLIST_GET(_config_entry_new(entry)));

   EINA_LIST_FOREACH(qa_config->transient_entries, l, entry)
     cfdata->transient_entries = eina_inlist_append(cfdata->transient_entries, EINA_INLIST_GET(_config_entry_new(entry)));
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   qa_mod->cfd = cfd;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd  __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Eina_Inlist *l;
   Config_Entry *ce;
   EINA_INLIST_FOREACH_SAFE(cfdata->entries, l, ce)
     _config_entry_free(ce);
   EINA_INLIST_FOREACH_SAFE(cfdata->transient_entries, l, ce)
     _config_entry_free(ce);
   free(cfdata);
   qa_mod->cfd = NULL;
}

static int
_advanced_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Config_Entry *ce;
#define CHECK(X) \
   if (cfdata->X != qa_config->X) return 1

   CHECK(dont_bug_me);

   EINA_INLIST_FOREACH(cfdata->entries, ce)
     if (ce->id) return 1;
   EINA_INLIST_FOREACH(cfdata->transient_entries, ce)
     if (ce->id) return 1;

#undef CHECK
   return 0;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
#define CHECK(X) \
   if (cfdata->X != qa_config->X) return 1

   CHECK(autohide);
   CHECK(skip_pager);
   CHECK(skip_taskbar);
   CHECK(hide_when_behind);

#undef CHECK
   return 0;
}

static void
_list_select(void *data)
{
   (void) data;
}

static void
_list_fill(E_Config_Dialog_Data *cfdata, Evas_Object *list, Eina_Bool transient)
{
   Config_Entry *ce;
   EINA_INLIST_FOREACH(transient ? cfdata->transient_entries : cfdata->entries, ce)
     {
        e_widget_ilist_append(list, NULL, ce->id ?: ce->entry->id, _list_select, ce, ce->entry->id);
     }
   e_widget_ilist_selected_set(list, 0);
}

static void
_rename_del(void *data)
{
   E_Config_Dialog_Data *cfdata = e_object_data_get(data);
   if (!cfdata) return;
   cfdata->ed = NULL;
}

static void
_rename_ok(char *text, void *data)
{
   Config_Entry *ce = data;
   const char *name;
   Evas_Object *list;

   name = eina_stringshare_add(text);
   if (name == ce->id)
     {
        eina_stringshare_del(name);
        return;
     }
   if (name == ce->entry->id)
     {
        eina_stringshare_del(name);
        if (!ce->id) return;
        eina_stringshare_replace(&ce->id, NULL);
     }
   else
     eina_stringshare_replace(&ce->id, text);
   list = ce->entry->transient ? qa_mod->cfd->cfdata->o_list_transient : qa_mod->cfd->cfdata->o_list_entry;
   e_widget_ilist_clear(list);
   _list_fill(qa_mod->cfd->cfdata, list, ce->entry->transient);
}

static void
_list_delete(void *data __UNUSED__, void *list)
{
   Config_Entry *ce;

   ce = e_widget_ilist_selected_data_get(list);
   if (!ce) return;
   e_qa_entry_free(ce->entry);
}

static void
_list_rename(void *data, void *list)
{
   E_Config_Dialog_Data *cfdata = data;
   Config_Entry *ce;

   if (cfdata->ed)
     {
        e_win_raise(cfdata->ed->dia->win);
        return;
     }

   ce = e_widget_ilist_selected_data_get(list);
   if (!ce) return;
   cfdata->ed = e_entry_dialog_show(_("Rename"), "edit-rename", _("Enter a unique name for this entry"),
                                    ce->id ?: ce->entry->id, NULL, NULL,
                                    _rename_ok, NULL, ce);
   e_object_data_set(E_OBJECT(cfdata->ed), cfdata);
   e_object_del_attach_func_set(E_OBJECT(cfdata->ed), _rename_del);
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ob, *ol, *otb, *tab;
   int w, h;

   tab = e_widget_table_add(evas, 0);
   evas_object_name_set(tab, "dia_table");

   otb = e_widget_toolbook_add(evas, 48 * e_scale, 48 * e_scale);

/////////////////////////////////////////////////////////////////
   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_check_add(evas, _("Disable Warning Dialogs"), (int*)&(cfdata->dont_bug_me));
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("Behavior"), ol, 1, 1, 1, 1, 0.5, 0.5);

/////////////////////////////////////////////////////////////////
   ol = e_widget_table_add(evas, 0);
   e_widget_table_freeze(ol);

   cfdata->o_list_entry = ob = e_widget_ilist_add(evas, 0, 0, &cfdata->entry);
   evas_event_freeze(evas_object_evas_get(ob));
   edje_freeze();
   e_widget_ilist_freeze(ob);

   _list_fill(cfdata, ob, EINA_FALSE);

   e_widget_size_min_get(ob, &w, &h);
   e_widget_size_min_set(ob, MIN(w, 200), MIN(h, 100));
   e_widget_ilist_go(ob);
   e_widget_ilist_thaw(ob);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(ol));

   e_widget_table_object_append(ol, ob, 0, 0, 2, 1, 1, 1, 1, 1);

   ob = e_widget_button_add(evas, _("Rename"), "edit-rename", _list_rename, cfdata, cfdata->o_list_entry);
   e_widget_table_object_append(ol, ob, 0, 1, 1, 1, 1, 1, 0, 0);

   ob = e_widget_button_add(evas, _("Delete"), "edit-delete", _list_delete, cfdata, cfdata->o_list_entry);
   e_widget_table_object_append(ol, ob, 1, 1, 1, 1, 1, 1, 0, 0);


   e_widget_table_thaw(ol);

   e_widget_toolbook_page_append(otb, NULL, _("Entries"), ol, 1, 1, 1, 1, 0.5, 0.5);
/////////////////////////////////////////////////////////////////
   ol = e_widget_table_add(evas, 0);
   e_widget_table_freeze(ol);

   cfdata->o_list_transient = ob = e_widget_ilist_add(evas, 0, 0, &cfdata->entry);
   evas_event_freeze(evas_object_evas_get(ob));
   edje_freeze();
   e_widget_ilist_freeze(ob);

   _list_fill(cfdata, ob, EINA_TRUE);

   e_widget_size_min_get(ob, &w, &h);
   e_widget_size_min_set(ob, MIN(w, 200), MIN(h, 100));
   e_widget_ilist_go(ob);
   e_widget_ilist_thaw(ob);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(ol));

   e_widget_table_object_append(ol, ob, 0, 0, 2, 1, 1, 1, 1, 1);

   ob = e_widget_button_add(evas, _("Rename"), "edit-rename", _list_rename, cfdata, cfdata->o_list_transient);
   e_widget_table_object_append(ol, ob, 0, 1, 1, 1, 1, 1, 0, 0);

   ob = e_widget_button_add(evas, _("Delete"), "edit-delete", _list_delete, cfdata, cfdata->o_list_transient);
   e_widget_table_object_append(ol, ob, 1, 1, 1, 1, 1, 1, 0, 0);

   e_widget_table_thaw(ol);

   e_widget_toolbook_page_append(otb, NULL, _("Transients"), ol, 1, 1, 1, 1, 0.5, 0.5);
/////////////////////////////////////////////////////////////////
   e_widget_toolbook_page_show(otb, 0);

   e_dialog_resizable_set(cfd->dia, 1);

   e_widget_table_object_append(tab, otb, 0, 0, 1, 1, 1, 1, 1, 1);
   return tab;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ob, *ol, *otb, *tab;

   cfdata->o_list_entry = cfdata->o_list_transient = NULL;

   tab = e_widget_table_add(evas, 0);
   evas_object_name_set(tab, "dia_table");

   otb = e_widget_toolbook_add(evas, 48 * e_scale, 48 * e_scale);

   ol = e_widget_list_add(evas, 0, 0);

   ob = e_widget_check_add(evas, _("Hide Instead Of Raising"), &cfdata->hide_when_behind);
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);

   ob = e_widget_check_add(evas, _("Hide If Focus Lost"), &cfdata->autohide);
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);

   ob = e_widget_check_add(evas, _("Skip Taskbar"), &cfdata->skip_taskbar);
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);

   ob = e_widget_check_add(evas, _("Skip Pager"), &cfdata->skip_pager);
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("Behavior"), ol, 1, 1, 1, 1, 0.5, 0.5);

   e_widget_toolbook_page_show(otb, 0);

   e_dialog_resizable_set(cfd->dia, 1);

   e_widget_table_object_append(tab, otb, 0, 0, 1, 1, 1, 1, 1, 1);
   return tab;
}


static int
_advanced_apply_data(E_Config_Dialog *cfd  __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Config_Entry *ce;
   Eina_Bool entry_changed = EINA_FALSE, transient_changed = EINA_FALSE;
#define SET(X) qa_config->X = cfdata->X

   SET(dont_bug_me);

   EINA_INLIST_FOREACH(cfdata->entries, ce)
     {
        if (!ce->id) continue;
        if (!e_qa_entry_rename(ce->entry, ce->id))
          entry_changed = EINA_TRUE;
        eina_stringshare_replace(&ce->id, NULL);
     }
   EINA_INLIST_FOREACH(cfdata->transient_entries, ce)
     {
        if (!ce->id) continue;
        if (!e_qa_entry_rename(ce->entry, ce->id))
          transient_changed = EINA_TRUE;
        eina_stringshare_replace(&ce->id, NULL);
     }
   if (entry_changed)
     {
        e_widget_ilist_clear(cfdata->o_list_entry);
        _list_fill(qa_mod->cfd->cfdata, cfdata->o_list_entry, EINA_FALSE);
     }
   if (transient_changed)
     {
        e_widget_ilist_clear(cfdata->o_list_transient);
        _list_fill(qa_mod->cfd->cfdata, cfdata->o_list_transient, EINA_TRUE);
     }
   e_config_save_queue();
   return 1;
}

static int
_basic_apply_data(E_Config_Dialog *cfd  __UNUSED__, E_Config_Dialog_Data *cfdata)
{
#define SET(X) qa_config->X = cfdata->X
   SET(autohide);
   SET(hide_when_behind);
   SET(skip_taskbar);
   SET(skip_pager);
   e_qa_entries_update();
   e_config_save_queue();
   return 1;
}

static void
_list_item_add(E_Quick_Access_Entry *entry)
{
   Evas_Object *list;
   Config_Entry *ce = entry->cfg_entry;

   list = ce->entry->transient ? qa_mod->cfd->cfdata->o_list_transient : qa_mod->cfd->cfdata->o_list_entry;
   if (!list) return;
   e_widget_ilist_append(list, NULL, ce->id ?: ce->entry->id, _list_select, ce, ce->entry->id);
   if (e_widget_ilist_selected_get(list) == -1) e_widget_ilist_selected_set(list, 0);
}

static void
_list_item_delete(E_Quick_Access_Entry *entry)
{
   Eina_List *l, *ll;
   E_Ilist_Item *ili;
   Evas_Object *list;
   unsigned int x = 0;

   list = entry->transient ? qa_mod->cfd->cfdata->o_list_transient : qa_mod->cfd->cfdata->o_list_entry;
   if (!list) return;
   l = e_widget_ilist_items_get(list);
   EINA_LIST_FOREACH(l, ll, ili)
     {
        if (e_widget_ilist_item_data_get(ili) == entry->cfg_entry)
          {
             e_widget_ilist_remove_num(list, x);
             break;
          }
        x++;
     }
   if (e_widget_ilist_selected_get(list) == -1) e_widget_ilist_selected_set(list, 0);
}
//////////////////////////////////////////////////////////////////////////////
E_Config_DD *
e_qa_config_dd_new(void)
{
   conf_edd = E_CONFIG_DD_NEW("Quickaccess_Config", Config);
   entry_edd = E_CONFIG_DD_NEW("E_Quick_Access_Entry", E_Quick_Access_Entry);

#undef T
#undef D
#define T E_Quick_Access_Entry
#define D entry_edd
   E_CONFIG_VAL(D, T, id, STR);
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, class, STR);
   E_CONFIG_VAL(D, T, cmd, STR);
   E_CONFIG_VAL(D, T, win, UINT);
   E_CONFIG_VAL(D, T, config.autohide, UCHAR);
   E_CONFIG_VAL(D, T, config.relaunch, UCHAR);
   E_CONFIG_VAL(D, T, config.hidden, UCHAR);
   E_CONFIG_VAL(D, T, config.jump, UCHAR);
   E_CONFIG_VAL(D, T, transient, UCHAR);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, config_version, UINT);
   E_CONFIG_LIST(D, T, entries, entry_edd);
   E_CONFIG_LIST(D, T, transient_entries, entry_edd);
   E_CONFIG_VAL(D, T, autohide, UCHAR);
   E_CONFIG_VAL(D, T, hide_when_behind, UCHAR);
   E_CONFIG_VAL(D, T, skip_taskbar, UCHAR);
   E_CONFIG_VAL(D, T, skip_pager, UCHAR);
   E_CONFIG_VAL(D, T, dont_bug_me, UCHAR);
   return conf_edd;
}

void *
e_qa_config_dd_free(void)
{
   E_FREE(entry_edd);
   E_FREE(conf_edd);
   return NULL;
}

void
e_qa_config_free(Config *conf)
{
   if (!conf) return;
   E_FREE_LIST(conf->entries, e_qa_entry_free);
   E_FREE_LIST(conf->transient_entries, e_qa_entry_free);
   free(conf);
}

Config *
e_qa_config_new(void)
{
   Config *conf;

   conf = E_NEW(Config, 1);
   conf->skip_taskbar = 1;
   conf->skip_pager = 1;
   return conf;
}

void
e_qa_config_entry_transient_convert(E_Quick_Access_Entry *entry)
{
   if (!entry) return;
   if (!entry->cfg_entry) return;
   _list_item_delete(entry);
   entry->transient = !entry->transient;
   _list_item_add(entry);
   entry->transient = !entry->transient;
}

void
e_qa_config_entry_free(E_Quick_Access_Entry *entry)
{
   if (!entry) return;
   if (!entry->cfg_entry) return;
   _list_item_delete(entry);
   _config_entry_free(entry->cfg_entry);
   entry->cfg_entry = NULL;
}

void
e_qa_config_entry_add(E_Quick_Access_Entry *entry)
{
   Config_Entry *ce;

   if (!entry) return;
   if (!qa_mod->cfd) return;
   ce = _config_entry_new(entry);
   if (entry->transient)
     qa_mod->cfd->cfdata->transient_entries = eina_inlist_append(qa_mod->cfd->cfdata->transient_entries, EINA_INLIST_GET(ce));
   else
     qa_mod->cfd->cfdata->entries = eina_inlist_append(qa_mod->cfd->cfdata->entries, EINA_INLIST_GET(ce));
   _list_item_add(entry);
}

E_Config_Dialog *
e_int_config_qa_module(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   char buf[PATH_MAX];

   if (qa_mod->cfd) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->basic.check_changed = _basic_check_changed;
   v->advanced.apply_cfdata = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   v->advanced.check_changed = _advanced_check_changed;

   snprintf(buf, sizeof(buf), "%s/e-module-quickaccess.edj", e_module_dir_get(qa_mod->module));
   cfd = e_config_dialog_new(con, _("Quickaccess Settings"),
                             "E", "launcher/quickaccess", buf, 32, v, NULL);
   return cfd;
}
